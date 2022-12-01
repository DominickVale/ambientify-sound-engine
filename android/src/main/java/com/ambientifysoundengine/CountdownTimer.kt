package com.ambientifysoundengine

import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.flow


class CountdownTimer(private var valueMs: Long, private val timerIsRunning: (Boolean) -> Unit = {}) {
  private var isRunning = true
  private var isEnded = false

  val get = flow<Long> {
    while (!isEnded) {
      if (isRunning) {
        //timer is not canceled or paused
        valueMs -= 1000
        emit(valueMs)
      }

      if (valueMs <= 0L) reset()
      delay(1000)
    }
  }

  fun resume() {
    isRunning = true
    timerIsRunning(true)
  }

  fun pause() {
    isRunning = false
    timerIsRunning(false)
  }

  fun reset() {
    isEnded = true
    isRunning = false
    timerIsRunning(false)
  }
}
