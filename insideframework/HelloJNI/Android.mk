# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= hellojni.cpp

LOCAL_MODULE:= hellojni

include $(BUILD_SHARED_LIBRARY)



#LOCAL_SRC_FILES:= HelloJNI.java

#LOCAL_MODULE:= hello

#include $(BUILD_EXECUTABLE)
