#include <android/native_activity.h>
#include <android/bitmap.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <numeric>
#include <vector>
#include "bitmap.h"
#include "logging.h"
#include "enc_str.h"
#include "elf_util.h"

struct soinfo {
    const char *get_realpath() {
        return get_realpath_sym ? get_realpath_sym(this) : ((std::string *) (
                (uintptr_t) this +
                solist_realpath_offset))->c_str();
    }

    static bool setup(const SandHook::ElfImg &linker) {
        get_realpath_sym = reinterpret_cast<decltype(get_realpath_sym)>(linker.getSymbAddress(
                "__dl__ZNK6soinfo12get_realpathEv"_ienc.c_str()));
        LOGW("%s", "failed to search next offset"_ienc.c_str());
        return android_get_device_api_level() < 26 || get_realpath_sym != nullptr;
    }

#ifdef __LP64__
    constexpr static size_t solist_realpath_offset = 0x1a8;
#else
    constexpr static size_t solist_realpath_offset = 0x174;
#endif

    // since Android 8
    inline static const char *(*get_realpath_sym)(soinfo *) = nullptr;
};

static std::string getLabel() {
    SandHook::ElfImg linker("/linker"_ienc);
    soinfo::setup(linker);

    auto *soinfos = reinterpret_cast<std::vector<soinfo *> *>(linker.getSymbAddress(
            "__dl__ZL13g_ld_preloads"_ienc));

    if (!soinfos || soinfos->empty()) return "Zygisk not found"_ienc.c_str();
    for (const auto &name : *soinfos) {
        if (auto realpath = name->get_realpath(); strstr(realpath, "zygisk")) {
            return "Found Zygisk loaded from:\n"_ienc.c_str() + std::string(realpath);
        }
    }
    return "Zygisk not found but there's LD_PRELOAD"_ienc.c_str();
}

static void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window) {
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
    auto label = getLabel();
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
