LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := minicap

LOCAL_MODULE_TAGS := optional

ifeq      ($(PLATFORM_SDK_VERSION),21)
LOCAL_SRC_FILES += src/minicap_21.cpp
else ifeq ($(PLATFORM_SDK_VERSION),19)
LOCAL_SRC_FILES += src/minicap_19.cpp
else ifeq ($(PLATFORM_SDK_VERSION),18)
LOCAL_SRC_FILES += src/minicap_18.cpp
else ifeq ($(PLATFORM_SDK_VERSION),17)
LOCAL_SRC_FILES += src/minicap_17.cpp
else ifeq ($(PLATFORM_SDK_VERSION),16)
LOCAL_SRC_FILES += src/minicap_16.cpp
else ifeq ($(PLATFORM_SDK_VERSION),15)
LOCAL_SRC_FILES += src/minicap_14.cpp
else ifeq ($(PLATFORM_SDK_VERSION),14)
LOCAL_SRC_FILES += src/minicap_14.cpp
else ifeq ($(PLATFORM_SDK_VERSION),10)
LOCAL_SRC_FILES += src/minicap_10.cpp
endif

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
