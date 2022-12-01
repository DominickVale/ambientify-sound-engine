package com.ambientifysoundengine

import android.app.*
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Binder
import android.os.Build
import android.os.IBinder
import android.os.PowerManager
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import android.support.v4.media.session.MediaSessionCompat
import androidx.media.app.NotificationCompat.MediaStyle
import androidx.lifecycle.MutableLiveData
import com.ambientifysoundengine.EngineModule.Companion.STATUS_NOTIF_CONTENT
import com.ambientifysoundengine.EngineModule.Companion.STATUS_NOTIF_ISPLAYING
import com.ambientifysoundengine.EngineModule.Companion.STATUS_NOTIF_TIMER_VAL
import com.ambientifysoundengine.RemoteControlReceiver.Companion.createBroadcastIntent
import com.facebook.react.bridge.RuntimeExecutor
import com.facebook.react.modules.core.DeviceEventManagerModule
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.onCompletion
import org.fmod.FMOD

private const val LOG_TAG = "ASoundEngineService"
private var isServiceRunning = false

/* TODO:
  - use res strings instead of hardcoded strings
*/
class EngineService : Service() {

  companion object {
    const val ACTION_START = "com.ambientify.action.START_SERVICE"
    const val ACTION_PLAYPAUSE = "com.ambientify.action.PLAYPAUSE"
    const val ACTION_STOP = "com.ambientify.action.STOP"
    const val ACTION_START_TIMER = "com.ambientify.action.START_TIMER"
    const val ACTION_STOP_TIMER = "com.ambientify.action.STOP_TIMER"
    const val ACTION_UPDATE_NOTIFICATION = "com.ambientify.action.UPDATE_NOTIFICATION"

    fun isRunning() = isServiceRunning

    @JvmStatic
    fun bindNotifyJS() {
      StateSingleton.reactContext.getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter::class.java)
        .emit("ambientify.engine.jsiLoaded", null)
    }

    @JvmStatic
    fun notificationTogglePlayNotifyJS(isNowPlaying: Boolean) {
      StateSingleton.reactContext.getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter::class.java)
        .emit("ambientify.engine.notifications.togglePlayback", isNowPlaying)
    }
  }

  private var wakeLock: PowerManager.WakeLock? = null
  private lateinit var countdownTimer: CountdownTimer
  private lateinit var countdownJob: Job
  private lateinit var notificationJob: Job

  private val mNotificationBuilder by lazy {
    NotificationCompat.Builder(
      this,
      EngineModule.NOTIF_CH_ID
    )
  }
  private val mNotificationManager by lazy { NotificationManagerCompat.from(this) }
  private val playPauseBroadcast by lazy { createBroadcastIntent(this@EngineService, ACTION_PLAYPAUSE, null) }
  private val stopBroadcast by lazy { createBroadcastIntent(this@EngineService, ACTION_STOP, null) }

  private val mediaStyle: MediaStyle by lazy {
    val session = MediaSessionCompat(this, "tag").sessionToken
    MediaStyle().setMediaSession(session)
  }
  private val mainActivityIntent by lazy {
    val i = StateSingleton.reactContext.getPackageManager()
      .getLaunchIntentForPackage(StateSingleton.reactContext.getPackageName())
    PendingIntent.getActivity(this, 1, i, PendingIntent.FLAG_IMMUTABLE)
  }

  private val _timerElapsedTime: MutableLiveData<Long> = MutableLiveData(0L)
  private val _isTimerRunning: MutableLiveData<Boolean> = MutableLiveData(true)

  private var isEnginePlaying = false
  private var currentNotificationText = "No sound loaded..."

  private val serviceScope = CoroutineScope(Dispatchers.IO)

  private external fun nativeInstall(
    jsiPtr: Long,
    runtimeExecutor: RuntimeExecutor,
    callInvoker: CallInvokerHolderImpl
  )

  private external fun toggleMaster(): Boolean

  inner class LocalBinder : Binder() {
    fun getService(): EngineService = this@EngineService
  }

  override fun onBind(intent: Intent): IBinder = LocalBinder()

  override fun onCreate() {
    super.onCreate()
    isServiceRunning = true
  }

  override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
    super.onStartCommand(intent, flags, startId)
    when (intent?.action) {
      null -> {
        Log.e(LOG_TAG, "onStartCommand: action is null")
        return START_NOT_STICKY
      }
      ACTION_STOP -> stopService()
      ACTION_PLAYPAUSE -> {
        isEnginePlaying = toggleMaster()
        notificationTogglePlayNotifyJS(isEnginePlaying)
        updateNotificationActions()
      }
      ACTION_START_TIMER -> {
        if (!intent.hasExtra(STATUS_NOTIF_TIMER_VAL)) {
          Log.e(LOG_TAG, "onStartCommand: $STATUS_NOTIF_TIMER_VAL not set for $ACTION_START_TIMER")
          return START_NOT_STICKY
        }
        if (this::countdownTimer.isInitialized) {
          countdownJob.cancel()
        }
        val timerValue = intent.getDoubleExtra(STATUS_NOTIF_TIMER_VAL, 0.0).toLong()
        StateSingleton.timerValHuman = timerValue.to24HourFormat()
        countdownTimer = CountdownTimer(timerValue) { isTimerRunning ->
          serviceScope.launch(Dispatchers.Main) {
            _isTimerRunning.value = isTimerRunning
          }
        }
        countdownJob = serviceScope.launch { observeCountdownTimer(countdownTimer) }
      }
      ACTION_STOP_TIMER -> {
        if (this::countdownTimer.isInitialized) {
          countdownJob.cancel()
          StateSingleton.timerValHuman = ""
          updateNotificationText("")
        }
      }
      ACTION_START -> {
        notificationJob = serviceScope.launch {
          val bitMap: Bitmap = BitmapFactory.decodeResource(resources, R.drawable.image)

          withContext(Dispatchers.Main) {
            val notification = buildNotification(currentNotificationText, bitMap)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
              if (mNotificationManager.areNotificationsEnabled())
                startForeground(EngineModule.NOTIF_ID, notification)
              else
                startService(Intent(this@EngineService, EngineService::class.java))
            } else {
              startForeground(EngineModule.NOTIF_ID, notification)
            }
          }
        }

        startEngine()
      }
      ACTION_UPDATE_NOTIFICATION -> {
        if (intent.hasExtra(STATUS_NOTIF_ISPLAYING)) {
          isEnginePlaying = intent.getBooleanExtra(STATUS_NOTIF_ISPLAYING, false)
          updateNotificationActions()
        }
        if (intent.hasExtra(STATUS_NOTIF_CONTENT)) {
          val contentText = intent.getStringExtra(STATUS_NOTIF_CONTENT)
          if(contentText != null){
            currentNotificationText = if(contentText == "") "No sound loaded..."
              else contentText
          }
          updateNotificationText()
        }
      }
    }
    return START_NOT_STICKY
  }

  fun stopService() {
    isServiceRunning = false
    try {
      wakeLock?.let {
        if (it.isHeld) {
          it.release()
        }
      }
      FMOD.close()
      StateSingleton.reactContext.currentActivity?.finish()
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
        stopForeground(STOP_FOREGROUND_REMOVE)
      else stopForeground(true)
      super.onDestroy()
      stopSelf()
      System.runFinalization()
      System.exit(0)
    } catch (e: Exception) {
      Log.e(LOG_TAG, "Service wasn't stopped correctly ${e.message}")
    }
  }

  override fun onDestroy() {
    Log.d(LOG_TAG, "onDestroy()")
    stopService()
    super.onDestroy()
  }

  private fun loadLibs() {
    try {
      for (lib in BuildConfig.FMOD_LIBS) {
        System.loadLibrary(lib)
      }
      System.loadLibrary("ambientifySoundEngine")
      FMOD.init(this)
      Log.d(LOG_TAG, "Installing JSI bindings...")
      val jsContext = StateSingleton.reactContext.javaScriptContextHolder
      if (jsContext.get() != 0L) {
        val runtimeExecutor = StateSingleton.reactContext.catalystInstance.runtimeExecutor
        val jsCallInvokerHolder =
          StateSingleton.reactContext.catalystInstance.jsCallInvokerHolder as CallInvokerHolderImpl
        nativeInstall(jsContext.get(), runtimeExecutor, jsCallInvokerHolder)
        Log.d(LOG_TAG, "JSI bindings installed")
      } else {
        Log.e(LOG_TAG, "JSI Runtime is not available in debug mode")
      }
    } catch (exception: Exception) {
      Log.e(LOG_TAG, "Exception trying to load native libs: ", exception)
    }
  }

  private fun startEngine() {
    isServiceRunning = true
    // we need this lock so our service gets not affected by Doze Mode
    wakeLock =
      (getSystemService(Context.POWER_SERVICE) as PowerManager).run {
        newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "EngineService::lock").apply {
          acquire(8 * 60 * 60 * 1000L /*8 hours*/)
        }
      }
    serviceScope.launch {
      loadLibs()
    }
  }

  private fun buildNotification(contentText: String, bitMap: Bitmap): Notification {
    return mNotificationBuilder.apply {
      setSmallIcon(R.mipmap.ic_launcher)
      setLargeIcon(bitMap)
      setSilent(true)
      setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
      priority = NotificationCompat.PRIORITY_HIGH
      addAction(R.drawable.play, "Play", playPauseBroadcast)
      addAction(R.drawable.reset, "Stop", stopBroadcast)
      setStyle(mediaStyle.setShowActionsInCompactView(0, 1))
      setContentTitle("Ambientify")
      setContentText(contentText)
      setContentIntent(mainActivityIntent)
    }.build()
  }

  private fun updateNotificationText(timerValueHuman: String = "") {
    val stringBuilder = StringBuilder().apply {
      append(currentNotificationText)
      if (timerValueHuman.isNotEmpty()) {
        append(" - $timerValueHuman")
      }
    }
    mNotificationBuilder.setContentText(stringBuilder).build()
    mNotificationManager.notify(EngineModule.NOTIF_ID, mNotificationBuilder.build())
  }

  private fun updateNotificationActions() {
    mNotificationBuilder.apply {
      clearActions()
      if (isEnginePlaying)
        addAction(R.drawable.pause, "Pause", playPauseBroadcast)
      else
        addAction(R.drawable.play, "Play", playPauseBroadcast)
      addAction(R.drawable.reset, "Stop", stopBroadcast)

      mNotificationBuilder.setStyle(mediaStyle)
      mNotificationManager.notify(EngineModule.NOTIF_ID, mNotificationBuilder.build())
    }
  }

  private suspend fun observeCountdownTimer(timer: CountdownTimer) = timer.get
    .onCompletion {
      when (it) {
        // only stop if completed successfully (no exceptions)
        null -> {
          stopService()
        }
      }

    }
    .collect { millis ->
      withContext(Dispatchers.Main) {
        _timerElapsedTime.value = millis

        if (notificationJob.isCompleted) {
          val millisFormat = millis.to24HourFormat()
          StateSingleton.timerValHuman = millisFormat
          updateNotificationText(millisFormat)
        }
      }
    }
}
