LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := events.c resources.c

ifneq ($(BOARD_CUSTOM_GRAPHICS),)
  LOCAL_SRC_FILES += $(BOARD_CUSTOM_GRAPHICS)
else
  LOCAL_SRC_FILES += graphics.c
endif

LOCAL_MODULE := libminui

LOCAL_C_INCLUDES +=\
    external/libpng\
    external/zlib


include $(BUILD_STATIC_LIBRARY)
