LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := twrp_launcher
LOCAL_SRC_FILES := twrp_launcher.c
include $(BUILD_EXECUTABLE)
