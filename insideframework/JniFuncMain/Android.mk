# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= jnifunc.cpp

LOCAL_MODULE:= jnifunc

include $(BUILD_SHARED_LIBRARY)

