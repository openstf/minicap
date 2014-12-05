LOCAL_PATH := $(abspath $(call my-dir))
include $(CLEAR_VARS)

LOCAL_MODULE := json_spirit

SOURCE_PATH = json_spirit_v4.08

LOCAL_SRC_FILES := \
	$(SOURCE_PATH)/json_spirit/json_spirit_reader.cpp \
	$(SOURCE_PATH)/json_spirit/json_spirit_value.cpp \
	$(SOURCE_PATH)/json_spirit/json_spirit_writer.cpp \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/$(SOURCE_PATH)/json_spirit \

LOCAL_EXPORT_C_INCLUDES = \
	$(LOCAL_PATH)/$(SOURCE_PATH)/json_spirit \

LOCAL_STATIC_LIBRARIES := \
	boost \

include $(BUILD_STATIC_LIBRARY)
