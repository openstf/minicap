LOCAL_PATH := $(abspath $(call my-dir))
include $(CLEAR_VARS)

LOCAL_MODULE := libjpeg-turbo
LOCAL_SRC_FILES := $(LOCAL_PATH)/build/$(TARGET_ARCH_ABI)/lib/libturbojpeg.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/build/$(TARGET_ARCH_ABI)/include

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libjpeg
LOCAL_SRC_FILES := $(LOCAL_PATH)/build/$(TARGET_ARCH_ABI)/lib/libjpeg.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/build/$(TARGET_ARCH_ABI)/include

include $(PREBUILT_STATIC_LIBRARY)
