#include "maze_jni_bridge.h"

#include <condition_variable>
#include <mutex>
#include <new>
#include <string>

#include "api/maze_api.h"

namespace {

struct AndroidMazeSessionBridge {
  MazeSession* session = nullptr;
  JavaVM* jvm = nullptr;
  jobject listener_global_ref = nullptr;
  jmethodID on_event_method = nullptr;
  std::mutex mutex;
  std::condition_variable run_completed_cv;
  bool running = false;
};

auto ToJString(JNIEnv* env, const char* text) -> jstring {
  if (env == nullptr) {
    return nullptr;
  }
  if (text == nullptr) {
    return env->NewStringUTF("");
  }
  return env->NewStringUTF(text);
}

auto GetBridge(jlong session_ptr) -> AndroidMazeSessionBridge* {
  return reinterpret_cast<AndroidMazeSessionBridge*>(session_ptr);
}

auto GetUtfChars(JNIEnv* env, jstring text) -> const char* {
  if (env == nullptr || text == nullptr) {
    return nullptr;
  }
  return env->GetStringUTFChars(text, nullptr);
}

void ReleaseUtfChars(JNIEnv* env, jstring text, const char* chars) {
  if (env == nullptr || text == nullptr || chars == nullptr) {
    return;
  }
  env->ReleaseStringUTFChars(text, chars);
}

void ClearListener(JNIEnv* env, AndroidMazeSessionBridge* bridge) {
  if (env == nullptr || bridge == nullptr) {
    return;
  }
  if (bridge->listener_global_ref != nullptr) {
    env->DeleteGlobalRef(bridge->listener_global_ref);
    bridge->listener_global_ref = nullptr;
  }
  bridge->on_event_method = nullptr;
}

auto AttachCurrentThread(JavaVM* jvm, JNIEnv** out_env) -> bool {
  if (jvm == nullptr || out_env == nullptr) {
    return false;
  }
  *out_env = nullptr;
  const jint status =
      jvm->GetEnv(reinterpret_cast<void**>(out_env), JNI_VERSION_1_6);
  if (status == JNI_OK) {
    return false;
  }
  if (status != JNI_EDETACHED) {
    return false;
  }
#if defined(__ANDROID__) || defined(ANDROID)
  if (jvm->AttachCurrentThread(out_env, nullptr) != JNI_OK) {
    *out_env = nullptr;
    return false;
  }
#else
  if (jvm->AttachCurrentThread(reinterpret_cast<void**>(out_env), nullptr) !=
      JNI_OK) {
    *out_env = nullptr;
    return false;
  }
#endif
  return true;
}

void OnMazeEvent(const MazeEvent* event, void* user_data) {
  auto* bridge = static_cast<AndroidMazeSessionBridge*>(user_data);
  if (bridge == nullptr || bridge->jvm == nullptr || event == nullptr) {
    return;
  }

  JNIEnv* env = nullptr;
  const bool detach_after_callback = AttachCurrentThread(bridge->jvm, &env);
  if (env == nullptr) {
    return;
  }

  jobject listener_local_ref = nullptr;
  jmethodID on_event_method = nullptr;
  {
    std::lock_guard<std::mutex> lock(bridge->mutex);
    if (bridge->listener_global_ref != nullptr) {
      listener_local_ref = env->NewLocalRef(bridge->listener_global_ref);
      on_event_method = bridge->on_event_method;
    }
  }

  if (listener_local_ref != nullptr && on_event_method != nullptr) {
    std::string payload_text;
    if (event->payload != nullptr && event->payload_size > 0) {
      payload_text.assign(static_cast<const char*>(event->payload),
                          event->payload_size);
    }
    jstring payload = env->NewStringUTF(payload_text.c_str());
    env->CallVoidMethod(listener_local_ref, on_event_method,
                        static_cast<jlong>(event->seq),
                        static_cast<jint>(event->type), payload);
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
    }
    env->DeleteLocalRef(payload);
    env->DeleteLocalRef(listener_local_ref);
  }

  if (detach_after_callback) {
    bridge->jvm->DetachCurrentThread();
  }
}

}  // namespace

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeCreateSession(
    JNIEnv* env, jclass /*clazz*/) {
  if (env == nullptr) {
    return 0;
  }
  MazeSession* session = nullptr;
  if (MazeSessionCreate(&session) != MAZE_STATUS_OK || session == nullptr) {
    return 0;
  }

  auto* bridge = new (std::nothrow) AndroidMazeSessionBridge();
  if (bridge == nullptr) {
    MazeSessionDestroy(session);
    return 0;
  }
  bridge->session = session;
  if (env->GetJavaVM(&bridge->jvm) != JNI_OK || bridge->jvm == nullptr) {
    MazeSessionDestroy(session);
    delete bridge;
    return 0;
  }
  return reinterpret_cast<jlong>(bridge);
}

JNIEXPORT void JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeDestroySession(
    JNIEnv* env, jclass /*clazz*/, jlong session_ptr) {
  auto* bridge = GetBridge(session_ptr);
  if (bridge == nullptr) {
    return;
  }

  MazeSession* session = nullptr;
  {
    std::unique_lock<std::mutex> lock(bridge->mutex);
    session = bridge->session;
    ClearListener(env, bridge);
  }
  if (session != nullptr) {
    MazeSessionCancel(session);
  }
  {
    std::unique_lock<std::mutex> lock(bridge->mutex);
    bridge->run_completed_cv.wait(lock, [&]() { return !bridge->running; });
    session = bridge->session;
    bridge->session = nullptr;
  }
  if (session != nullptr) {
    MazeSessionDestroy(session);
  }
  delete bridge;
}

JNIEXPORT jint JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeConfigureSession(
    JNIEnv* env, jclass /*clazz*/, jlong session_ptr, jstring request_json) {
  auto* bridge = GetBridge(session_ptr);
  MazeSession* session = (bridge != nullptr) ? bridge->session : nullptr;
  if (session == nullptr || request_json == nullptr || env == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }

  const char* request_utf8 = GetUtfChars(env, request_json);
  if (request_utf8 == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }

  const int configure_status = MazeSessionConfigure(session, request_utf8);
  ReleaseUtfChars(env, request_json, request_utf8);
  return configure_status;
}

JNIEXPORT jint JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeSetEventListener(
    JNIEnv* env, jclass /*clazz*/, jlong session_ptr, jobject listener) {
  auto* bridge = GetBridge(session_ptr);
  MazeSession* session = (bridge != nullptr) ? bridge->session : nullptr;
  if (bridge == nullptr || session == nullptr || env == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }

  std::lock_guard<std::mutex> lock(bridge->mutex);
  ClearListener(env, bridge);

  if (listener == nullptr) {
    return MazeSessionSetEventCallback(session, nullptr, nullptr);
  }

  jclass listener_class = env->GetObjectClass(listener);
  if (listener_class == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  jmethodID on_event_method = env->GetMethodID(
      listener_class, "onMazeEvent", "(JILjava/lang/String;)V");
  env->DeleteLocalRef(listener_class);
  if (on_event_method == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }

  bridge->listener_global_ref = env->NewGlobalRef(listener);
  bridge->on_event_method = on_event_method;
  if (bridge->listener_global_ref == nullptr) {
    bridge->on_event_method = nullptr;
    return MAZE_STATUS_INTERNAL_ERROR;
  }

  return MazeSessionSetEventCallback(session, &OnMazeEvent, bridge);
}

JNIEXPORT jint JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeRunSession(
    JNIEnv* env, jclass /*clazz*/, jlong session_ptr) {
  (void)env;
  auto* bridge = GetBridge(session_ptr);
  MazeSession* session = (bridge != nullptr) ? bridge->session : nullptr;
  if (bridge == nullptr || session == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }

  {
    std::lock_guard<std::mutex> lock(bridge->mutex);
    if (bridge->running) {
      return MAZE_STATUS_INVALID_ARGUMENT;
    }
    bridge->running = true;
  }

  const int run_code = MazeSessionRun(session);

  {
    std::lock_guard<std::mutex> lock(bridge->mutex);
    bridge->running = false;
  }
  bridge->run_completed_cv.notify_all();
  return run_code;
}

JNIEXPORT jstring JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeGetSummaryJson(
    JNIEnv* env, jclass /*clazz*/, jlong session_ptr) {
  auto* bridge = GetBridge(session_ptr);
  MazeSession* session = (bridge != nullptr) ? bridge->session : nullptr;
  if (session == nullptr) {
    return ToJString(env, "{}");
  }
  const char* summary = nullptr;
  if (MazeSessionGetSummaryJson(session, &summary) != MAZE_STATUS_OK) {
    return ToJString(env, "{}");
  }
  return ToJString(env, summary);
}

JNIEXPORT jstring JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeGetLastError(
    JNIEnv* env, jclass /*clazz*/, jlong session_ptr) {
  auto* bridge = GetBridge(session_ptr);
  MazeSession* session = (bridge != nullptr) ? bridge->session : nullptr;
  if (session == nullptr) {
    return ToJString(env, "MazeSession is null.");
  }
  const char* error = nullptr;
  if (MazeSessionGetLastError(session, &error) != MAZE_STATUS_OK) {
    return ToJString(env, "");
  }
  return ToJString(env, error);
}

JNIEXPORT jint JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeCancel(
    JNIEnv* env, jclass /*clazz*/, jlong session_ptr) {
  (void)env;
  auto* bridge = GetBridge(session_ptr);
  MazeSession* session = (bridge != nullptr) ? bridge->session : nullptr;
  if (session == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  return MazeSessionCancel(session);
}

JNIEXPORT jstring JNICALL
Java_com_maze_nativebridge_MazeNativeBridge_nativeGetApiVersion(
    JNIEnv* env, jclass /*clazz*/) {
  return ToJString(env, MazeApiVersion());
}

}  // extern "C"
