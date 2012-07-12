LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := buffThread
LOCAL_SRC_FILES := com_buff_bThread.c
LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)