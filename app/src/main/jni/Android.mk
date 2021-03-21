LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := vvb2060
LOCAL_SRC_FILES := tinynew.cpp elf_util.cpp solist.cpp bitmap.cpp activity.cpp
LOCAL_LDFLAGS   := -nostdlib
LOCAL_LDLIBS    := -llog -landroid -ljnigraphics
include $(BUILD_SHARED_LIBRARY)
