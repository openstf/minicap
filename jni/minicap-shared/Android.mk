LOCAL_PATH := $(abspath $(call my-dir))
include $(CLEAR_VARS)

LOCAL_MODULE := minicap-shared
LOCAL_SRC_FILES := $(LOCAL_PATH)/aosp/libs/android-21/$(TARGET_ARCH_ABI)/minicap.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/aosp/include

include $(PREBUILT_SHARED_LIBRARY)
