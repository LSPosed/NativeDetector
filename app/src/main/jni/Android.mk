
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := vvb2060
LOCAL_SRC_FILES := elf_util.cpp bitmap.cpp activity.cpp solist.cpp
LOCAL_LDLIBS    := -llog -landroid -ljnigraphics
LOCAL_STATIC_LIBRARIES := cxx
include $(BUILD_SHARED_LIBRARY)

$(call import-module,prefab/cxx)
