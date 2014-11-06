LOCAL_PATH := $(abspath $(call my-dir))
include $(CLEAR_VARS)

LOCAL_MODULE := boost

BOOST_PATH := $(LOCAL_PATH)/boost_1_57_0

LOCAL_SRC_FILES := \
	$(BOOST_PATH)/libs/system/src/error_code.cpp \

LOCAL_C_INCLUDES = \
	$(BOOST_PATH) \

LOCAL_EXPORT_C_INCLUDES = \
	$(BOOST_PATH) \

include $(BUILD_STATIC_LIBRARY)
