ifneq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mmcutils.c

LOCAL_MODULE := libmmcutils
LOCAL_MODULE_TAGS := eng

LOCAL_STATIC_LIBRARIES := libe2fsck libtune2fs libmke2fs libext2fs libext2_blkid libext2_uuid libext2_profile libext2_com_err libext2_e2p

include $(BUILD_STATIC_LIBRARY)

endif	# TARGET_ARCH == arm
endif	# !TARGET_SIMULATOR
