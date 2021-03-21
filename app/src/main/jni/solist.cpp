#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <dlfcn.h>
#include <link.h>
#include "logging.h"
#include "elf_util.h"
#include "solist.h"

namespace Solist {
    namespace {
        const char *GetLinkerPath() {
#if __LP64__
            if (android_get_device_api_level() >= 29) {
                return "/apex/com.android.runtime/bin/linker64";
            } else {
                return "/system/bin/linker64";
            }
#else
            if (android_get_device_api_level() >= 29) {
                return "/apex/com.android.runtime/bin/linker";
            } else {
                return "/system/bin/linker";
            }
#endif
        }

        struct soinfo;

        soinfo *solist = nullptr;
        soinfo *somain = nullptr;

        struct soinfo {
            soinfo *next() {
                return *(soinfo **) ((uintptr_t) this + solist_next_offset);
            }

            const char *get_realpath() {
                return get_realpath_sym ? get_realpath_sym(this) : ((std::string *) (
                        (uintptr_t) this +
                        solist_realpath_offset))->c_str();
            }

            static bool setup(const SandHook::ElfImg &linker) {
                get_realpath_sym = reinterpret_cast<decltype(get_realpath_sym)>(linker.getSymbAddress(
                        "__dl__ZNK6soinfo12get_realpathEv"));
                for (size_t i = 0; i < 1024 / sizeof(void *); i++) {
                    if (*(void **) ((uintptr_t) solist + i * sizeof(void *)) == somain) {
                        solist_next_offset = i * sizeof(void *);
                        return android_get_device_api_level() < 26 || get_realpath_sym != nullptr;
                    }
                }
                LOGW("failed to search next offset");
                // shortcut
                return android_get_device_api_level() < 26 || get_realpath_sym != nullptr;
            }

            static size_t solist_next_offset;
#ifdef __LP64__
            constexpr static size_t solist_realpath_offset = 0x1a8;
#else
            constexpr static size_t solist_realpath_offset = 0x174;
#endif

            // since Android 8
            static const char *(*get_realpath_sym)(soinfo *);
        };

#ifdef __LP64__
        size_t soinfo::solist_next_offset = 0x30;
#else
        size_t soinfo::solist_next_offset = 0xa4;
#endif

        // since Android 8
        const char *(*soinfo::get_realpath_sym)(soinfo *) = nullptr;

        const auto initialized = []() {
            SandHook::ElfImg linker(GetLinkerPath());
            return (solist = *reinterpret_cast<soinfo **>(linker.getSymbAddress(
                    "__dl__ZL6solist"))) != nullptr &&
                   (somain = *reinterpret_cast<soinfo **>(linker.getSymbAddress(
                           "__dl__ZL6somain"))) != nullptr &&
                   soinfo::setup(linker);
        }();

        std::vector<soinfo *> linker_get_solist() {
            std::vector<soinfo *> linker_solist{};
            for (auto *iter = solist; iter; iter = iter->next()) {
                linker_solist.push_back(iter);
            }
            return linker_solist;
        }
    }

    std::set<std::string_view> FindPathsFromSolist(std::string_view keyword) {
        std::set<std::string_view> paths{};
        if (!initialized) {
            LOGW("not initialized");
            return paths;
        }
        auto list = linker_get_solist();
        for (const auto &soinfo : linker_get_solist()) {
            const auto &real_path = soinfo->get_realpath();
            LOGD("%s", real_path);
            if (real_path && std::string_view(real_path).find(keyword) != std::string::npos) {
                paths.emplace(real_path);
                LOGE("Found Riru: %s", real_path);
            }
        }
        return paths;
    }
}
