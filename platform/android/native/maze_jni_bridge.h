#ifndef MAZE_ANDROID_NATIVE_MAZE_JNI_BRIDGE_H
#define MAZE_ANDROID_NATIVE_MAZE_JNI_BRIDGE_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeCreateSession(
    JNIEnv* env, jclass clazz);

JNIEXPORT void JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeDestroySession(
    JNIEnv* env, jclass clazz, jlong session_ptr);

JNIEXPORT jint JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeConfigureSession(
    JNIEnv* env, jclass clazz, jlong session_ptr, jstring request_json);

JNIEXPORT jint JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeSetEventListener(
    JNIEnv* env, jclass clazz, jlong session_ptr, jobject listener);

JNIEXPORT jint JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeRunSession(
    JNIEnv* env, jclass clazz, jlong session_ptr);

JNIEXPORT jstring JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeGetSummaryJson(
    JNIEnv* env, jclass clazz, jlong session_ptr);

JNIEXPORT jstring JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeGetLastError(
    JNIEnv* env, jclass clazz, jlong session_ptr);

JNIEXPORT jint JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeCancel(
    JNIEnv* env, jclass clazz, jlong session_ptr);

JNIEXPORT jstring JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeGetApiVersion(
    JNIEnv* env, jclass clazz);

#ifdef __cplusplus
}
#endif

#endif  // MAZE_ANDROID_NATIVE_MAZE_JNI_BRIDGE_H
