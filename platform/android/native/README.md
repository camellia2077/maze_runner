# Android JNI Bridge (Step 6.1)

This module provides a JNI bridge from Android/Kotlin to `maze_api_c`.

## Package / Class Mapping

Expected Kotlin class:

```kotlin
package com.maze.nativebridge

object MazeNativeBridge {
    interface EventListener {
        fun onMazeEvent(seq: Long, type: Int, payload: String)
    }

    external fun nativeCreateSession(): Long
    external fun nativeDestroySession(sessionPtr: Long)
    external fun nativeConfigureSession(sessionPtr: Long, requestJson: String): Int
    external fun nativeSetEventListener(sessionPtr: Long, listener: EventListener?): Int
    external fun nativeRunSession(sessionPtr: Long): Int
    external fun nativeGetSummaryJson(sessionPtr: Long): String
    external fun nativeGetLastError(sessionPtr: Long): String
    external fun nativeCancel(sessionPtr: Long): Int
    external fun nativeGetApiVersion(): String
}
```

The native bridge stores a `MazeSession*`, forwards native search events to the
Kotlin listener, and lets Kotlin decide how to throttle UI updates.

## Build Gate

- Enabled only when:
  - `ANDROID=ON`
  - `MAZE_ENABLE_ANDROID_JNI=ON`

Root CMake adds this module via `add_subdirectory(platform/android/native)`.
