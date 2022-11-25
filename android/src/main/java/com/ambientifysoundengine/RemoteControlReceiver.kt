package com.ambientifysoundengine

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.util.Log

class RemoteControlReceiver : BroadcastReceiver() {
  companion object{
    fun createBroadcastIntent(context: Context, action: String, extras: Bundle?): PendingIntent {
      val intent = Intent(context, RemoteControlReceiver::class.java).apply {
        this.action = action
        if(extras != null) {
          this.putExtras(extras)
        }
      }
      return PendingIntent.getBroadcast(
        context,
        0,
        intent,
        PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
      )
    }
  }
  override fun onReceive(context: Context, intent: Intent?) {
    if (intent?.action == null) {
      Log.e("RemoteControlReceiver", "Intent action is null")
      return
    }
    Intent(context, EngineService::class.java).apply {
      action = intent.action
      context.startService(this)
    }
  }
}
