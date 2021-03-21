#include <android/native_activity.h>
#include <android/bitmap.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <numeric>
#include "bitmap.h"
#include "solist.h"
#include "logging.h"

static jstring getLabel(JNIEnv *env) {
    static std::string_view libriru{"libriru"};
    static std::string_view so{".so"};
    const auto paths = Solist::FindPathsFromSolist(libriru);
    if (paths.empty()) return env->NewStringUTF("Riru not found");
    return env->NewStringUTF(
            std::accumulate(paths.begin(), paths.end(), std::string{}, [](auto &p, auto &i) {
                if (auto s = i.find(libriru), e = i.find(so);
                        s != std::string::npos && e != std::string::npos) {
                    s += libriru.size();
                    if (i[s] == '_') ++s;
                    auto ii = std::string(i.substr(s, e-s));
                    return p.empty() ? ii : p + ", " + ii;
                }
                return p;
            }).c_str());
}

static void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window) {
    ANativeWindow_Buffer buffer = {};
    ANativeWindow_lock(window, &buffer, nullptr);

    if (buffer.format != AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM) {
        ANativeWindow_setBuffersGeometry(window, buffer.width, buffer.height,
                                         AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);
    }
    auto *bits = static_cast<uint32_t *>(buffer.bits);
    for (int i = 0; i < buffer.width; ++i) {
        bits[i] = 0xFF121212;
    }
    for (int i = 1; i < buffer.height; ++i) {
        bits += buffer.stride;
        memcpy(bits, buffer.bits, buffer.width * sizeof(uint32_t));
    }

    JNIEnv *env = activity->env;
    jstring label = getLabel(env);
    jobject bitmap = asBitmap(env, (float) (buffer.width * 0.618), label);

    void *pixels;
    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);
    AndroidBitmap_lockPixels(env, bitmap, &pixels);

    auto *dst = static_cast<uint32_t *>(buffer.bits);
    auto *src = static_cast<uint32_t *>(pixels);
    size_t top = (buffer.height - info.height) / 2;
    size_t left = (buffer.width - info.width) / 2;
    dst += buffer.stride * top;
    for (size_t i = 0; i < info.height; ++i) {
        dst += left;
        memcpy(dst, src, info.width * sizeof(uint32_t));
        src += info.width;
        dst += (buffer.stride - left);
    }

    AndroidBitmap_unlockPixels(env, bitmap);
    ANativeWindow_unlockAndPost(window);
}

static void onStart(ANativeActivity *) {

}

static void onStop(ANativeActivity *) {
    kill(getpid(), SIGTERM);
    _exit(0);
}

static void onResume(ANativeActivity *) {

}

static void onDestroy(ANativeActivity *) {

}

static int repeat;

static int handleEvent(AInputEvent *event) {
    switch (AInputEvent_getType(event)) {
        case AINPUT_EVENT_TYPE_KEY:
            if (AKeyEvent_getKeyCode(event) == AKEYCODE_BACK) {
                if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP) {
                    ++repeat;
                }
                return repeat < 1;
            }
            break;
        default:
            break;
    }
    return 0;
}

static int onInputEvent(int, int events, void *data) {
    AInputEvent *event;
    auto queue = static_cast<AInputQueue *>(data);
    if (events == ALOOPER_EVENT_INPUT) {
        while (AInputQueue_getEvent(queue, &event) >= 0) {
            if (AInputQueue_preDispatchEvent(queue, event) == 0) {
                AInputQueue_finishEvent(queue, event, handleEvent(event));
            }
        }
    }
    return 1;
}

static void onInputQueueCreated(ANativeActivity *activity, AInputQueue *queue) {
    AInputQueue_attachLooper(queue, (ALooper *) activity->instance, ALOOPER_POLL_CALLBACK,
                             onInputEvent, queue);
}

static void onInputQueueDestroyed(ANativeActivity *, AInputQueue *queue) {
    AInputQueue_detachLooper(queue);
}

JNIEXPORT void __unused ANativeActivity_onCreate(ANativeActivity *activity, void *, size_t) {
    activity->callbacks->onStart = onStart;
    activity->callbacks->onStop = onStop;
    activity->callbacks->onResume = onResume;
    activity->callbacks->onDestroy = onDestroy;
    activity->callbacks->onInputQueueCreated = onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;

    activity->instance = ALooper_prepare(0);
}
