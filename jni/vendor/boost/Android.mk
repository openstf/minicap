LOCAL_PATH := $(abspath $(call my-dir))
include $(CLEAR_VARS)

LOCAL_MODULE := boost

SOURCE_PATH := boost_1_57_0

LOCAL_SRC_FILES := \
	$(SOURCE_PATH)/libs/system/src/error_code.cpp \
	$(SOURCE_PATH)/libs/timer/src/cpu_timer.cpp \
	$(SOURCE_PATH)/libs/timer/src/auto_timers_construction.cpp \
	$(SOURCE_PATH)/libs/chrono/src/chrono.cpp \

LOCAL_C_INCLUDES = \
	$(LOCAL_PATH)/$(SOURCE_PATH) \

LOCAL_EXPORT_C_INCLUDES = \
	$(LOCAL_PATH)/$(SOURCE_PATH) \

include $(BUILD_STATIC_LIBRARY)
