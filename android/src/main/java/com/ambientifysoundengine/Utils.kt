package com.ambientifysoundengine

import java.util.*

fun Long.to24HourFormat(): String {
  val seconds = (this / 1000 % 60).toInt()
  val minutes = (this / (1000 * 60) % 60).toInt()
  val hours = (this / (1000 * 60 * 60) % 24).toInt()

  return if (hours < 1) {
    String.format(Locale.getDefault(), "%02d:%02d", minutes, seconds)
  } else {
    String.format(Locale.getDefault(), "%02d:%02d:%02d", hours, minutes, seconds)
  }
}
