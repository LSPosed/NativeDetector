#include <android/native_activity.h>
#include <android/bitmap.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <numeric>
#include <vector>
#include "bitmap.h"
#include "solist.h"
#include "logging.h"
#include "enc_str.h"

static void
onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window, std::string_view label) {
    ANativeWindow_Buffer buffer = {};
    buffer.height = ANativeWindow_getHeight(window);
    buffer.width = ANativeWindow_getWidth(window);
    buffer.format = ANativeWindow_getFormat(window);

    if (buffer.format != AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM) {
        ANativeWindow_setBuffersGeometry(window, buffer.width, buffer.height,
                                         AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);
    }
    ANativeWindow_lock(window, &buffer, nullptr);
    auto *bits = static_cast<uint32_t *>(buffer.bits);
    for (int i = 0; i < buffer.width; ++i) {
        bits[i] = 0xFF303030;
    }
    for (int i = 1; i < buffer.height; ++i) {
        bits += buffer.stride;
        memcpy(bits, buffer.bits, buffer.width * sizeof(uint32_t));
    }

    JNIEnv *env = activity->env;
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

static std::string getRiruLabel() {
    static auto libriru_enc = "libriru"_senc;
    static auto so_enc = ".so"_senc;
    const auto libriru = libriru_enc.obtain();
    const auto so = so_enc.obtain();
    auto paths = Solist::FindPathsFromSolist(libriru);
    if (paths.empty()) return "Riru not found"_ienc.c_str();
    return "Found:"_ienc.c_str() +
           std::accumulate(paths.begin(), paths.end(), std::string{}, [&](auto &p, auto &i) {
               if (auto s = i.find(libriru), e = i.find(so);
                       s != std::string::npos && e != std::string::npos) {
                   if (auto n = s + libriru.size(); n != e) s = n; else s += 3;
                   if (i[s] == '_') ++s;
                   auto ii = std::string(i.substr(s, e - s));
                   return p + "\n\t" + ii;
               }
               return p;
           });
}

static std::string getZygiskLabel() {
    static auto modules_enc = "/data/adb/modules/"_senc;
    static auto zygisk_enc = "zygisk/"_senc;
    const auto zygisk = zygisk_enc.obtain();
    const auto modules = modules_enc.obtain();
    auto paths = Solist::FindPathsFromSolist(modules);
    std::string zygisk_path = Solist::FindZygiskFromPreloads();
    if (paths.empty()) return zygisk_path;
    return std::accumulate(paths.begin(), paths.end(), zygisk_path + "\n\nModules:"_ienc.c_str(),
                           [&](auto &p, auto &i) {
                               if (auto e = i.find(zygisk), s = i.rfind('/', e - 2) + 1;
                                       s != std::string::npos && e != std::string::npos) {
                                   auto ii = std::string(i.substr(s, e - s - 1));
                                   return p + "\n\t" + ii;
                               }
                               return p;
                           });
}

static void riruWindow(ANativeActivity *activity, ANativeWindow *window) {
    onNativeWindowCreated(activity, window, getRiruLabel());
}

static void zygiskWindow(ANativeActivity *activity, ANativeWindow *window) {
    onNativeWindowCreated(activity, window, getZygiskLabel());
}

extern "C" {

JNIEXPORT void __unused riru(ANativeActivity *activity, void *, size_t) {
    activity->callbacks->onInputQueueCreated = onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
    activity->callbacks->onNativeWindowCreated = riruWindow;
    activity->instance = ALooper_prepare(0);
}

JNIEXPORT void __unused zygisk(ANativeActivity *activity, void *, size_t) {
    activity->callbacks->onInputQueueCreated = onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
    activity->callbacks->onNativeWindowCreated = zygiskWindow;
    activity->instance = ALooper_prepare(0);
}

}
