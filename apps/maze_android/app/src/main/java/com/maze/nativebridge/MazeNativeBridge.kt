package com.maze.nativebridge

object MazeNativeBridge {
    init {
        System.loadLibrary("maze_android_jni")
    }

    const val STATUS_OK = 0
    const val STATUS_INVALID_ARGUMENT = 1
    const val STATUS_NOT_IMPLEMENTED = 2
    const val STATUS_INTERNAL_ERROR = 3
    const val STATUS_CANCELLED = 4

    const val EVENT_RUN_STARTED = 0
    const val EVENT_CELL_STATE_CHANGED = 1
    const val EVENT_PATH_UPDATED = 2
    const val EVENT_PROGRESS = 3
    const val EVENT_RUN_FINISHED = 4
    const val EVENT_RUN_CANCELLED = 5
    const val EVENT_RUN_FAILED = 6
    const val EVENT_TOPOLOGY_SNAPSHOT = 7

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
