# Maze Android App (Phase 4)

This Android app is a minimal Compose presentation layer that calls the JNI session
bridge defined in `platform/android/native/maze_jni_bridge.cpp` and renders search
state on a Compose `Canvas`.

## Current Scope

- Compose parameter panel (width/height/unitPixels/generation/search)
- `MazeSession` lifecycle from Kotlin:
  - create/destroy session
  - configure session
  - set `MazeEventCallback` listener bridge
  - run session on background coroutine
  - cancel
  - get summary/error/api version
- Compose consumes event stream with throttled updates to reduce recomposition pressure
- UI shows topology, incremental cell/path updates, live event counters, latest
  payload, and final summary while the native search is running
- PNG export is no longer part of the Android run path

## Android Studio Setup

1. Open folder: `apps/maze_android`
2. Sync Gradle
3. Build and run on emulator/device (`arm64-v8a` or `x86_64`)

## Build Versions

- Android Gradle Plugin: `9.0.1`
- Gradle Wrapper: `9.1.0`
- Compose plugin: `org.jetbrains.kotlin.plugin.compose` `2.2.10`
- Built-in Kotlin migration is enabled for AGP 9, so this app module no longer
  applies `org.jetbrains.kotlin.android`

## Native Build Notes

- `externalNativeBuild.cmake.path` points to repo root `CMakeLists.txt`
- CMake args:
  - `-DMAZE_ENABLE_ANDROID_JNI=ON`
  - `-DMAZE_ENABLE_WIN_GUI=OFF`
- Native build target: `maze_android_jni`
