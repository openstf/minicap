LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Enable PIE manually. Will get reset on $(CLEAR_VARS).
LOCAL_CFLAGS += -fPIE
LOCAL_LDFLAGS += -fPIE -pie

LOCAL_MODULE := minicap

LOCAL_SRC_FILES := \
	minicap.cpp \

LOCAL_STATIC_LIBRARIES := \
	libjpeg-turbo \
	libwebsockets \

LOCAL_SHARED_LIBRARIES := \
	minicap-shared \

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE := minicap-nopie

LOCAL_SRC_FILES := \
	minicap.cpp \

LOCAL_STATIC_LIBRARIES := \
	libjpeg-turbo \
	libwebsockets \

LOCAL_SHARED_LIBRARIES := \
	minicap-shared \

include $(BUILD_EXECUTABLE)
