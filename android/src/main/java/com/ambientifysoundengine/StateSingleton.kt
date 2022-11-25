package com.ambientifysoundengine

import android.content.Context
import com.facebook.react.bridge.ReactApplicationContext

object StateSingleton {
  lateinit var reactContext: ReactApplicationContext
  var hasNotifPermission = false
  var timerValHuman = ""
}
