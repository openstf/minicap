LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := minicap

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	src/minicap.cpp \

LOCAL_PRELINK_MODULE := false

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libui \

ifeq ($(PLATFORM_SDK_VERSION),10)
LOCAL_SHARED_LIBRARIES += libsurfaceflinger_client
else
LOCAL_SHARED_LIBRARIES += libgui
endif

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \

LOCAL_CFLAGS += -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

include $(BUILD_SHARED_LIBRARY)
