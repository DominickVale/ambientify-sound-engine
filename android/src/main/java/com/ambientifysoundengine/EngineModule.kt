package com.ambientifysoundengine

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.util.Log
import androidx.core.app.NotificationManagerCompat
import com.ambientifysoundengine.RemoteControlReceiver.Companion.createBroadcastIntent
import com.facebook.react.bridge.*
import com.facebook.react.module.annotations.ReactModule
import org.fmod.FMOD


@ReactModule(name = EngineModule.NAME)
class EngineModule(private val reactContext: ReactApplicationContext) :
  ReactContextBaseJavaModule(reactContext) {

  private var mNotificationManager: NotificationManager

  companion object {
    const val NAME = "ASoundEngine"

    const val NOTIF_ID = 1
    const val NOTIF_CH_ID = "ambientify_channel"
    const val NOTIF_CH_NAME = "Ambientify Notification"

    const val STATUS_NOTIF_ISPLAYING = "isPlaying"
    const val STATUS_NOTIF_CONTENT = "contentText"

    const val STATUS_NOTIF_TIMER_VAL = "timerValue"
  }

  private fun hasNotifPermission(): Boolean {
    return NotificationManagerCompat.from(reactContext).areNotificationsEnabled()
  }

  init {
    StateSingleton.reactContext = this.reactContext
    StateSingleton.hasNotifPermission = hasNotifPermission()
    mNotificationManager =
      reactContext.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
    createNotificationChannel()
  }

  override fun getName() = NAME

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun install() {
    if (!EngineService.isRunning()) {
      Log.d(NAME, "Launching service intent...")
      val intent = Intent(reactContext, EngineService::class.java)
      intent.action = EngineService.ACTION_START
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        reactContext.startForegroundService(intent)
      } else {
        reactContext.startService(intent)
      }
    }
    Log.w(NAME, "Service already running, not launching")
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun setTimerOptions(timerValue: Double) {
    val intent = Intent(reactContext, EngineService::class.java)
    if(timerValue > 0) {
      intent.action = EngineService.ACTION_START_TIMER
      intent.putExtra(STATUS_NOTIF_TIMER_VAL, timerValue)
    } else {
      intent.action = EngineService.ACTION_STOP_TIMER
      createBroadcastIntent(reactContext, EngineService.ACTION_STOP_TIMER, null)
    }
    reactContext.startService(intent)
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun getCurrentTimerValue(): String {
    return StateSingleton.timerValHuman
  }

  @ReactMethod()
  fun updateNotification(state: ReadableMap){
    val intent = Intent(reactContext, EngineService::class.java)
    intent.action = EngineService.ACTION_UPDATE_NOTIFICATION
    intent.putExtra(STATUS_NOTIF_ISPLAYING, state.getBoolean(STATUS_NOTIF_ISPLAYING))
    intent.putExtra(STATUS_NOTIF_CONTENT, state.getString(STATUS_NOTIF_CONTENT))
    reactContext.startService(intent)
  }

  private fun createNotificationChannel() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val channel = NotificationChannel(
        NOTIF_CH_ID,
        NOTIF_CH_NAME,
        NotificationManager.IMPORTANCE_HIGH
      ).let {
        it.description = "Ambientify Notification Channel"
        it.enableLights(false)
        it.setShowBadge(false)
        it.lockscreenVisibility = Notification.VISIBILITY_PUBLIC
        it
      }
      mNotificationManager.createNotificationChannel(channel)
    }
  }
}
