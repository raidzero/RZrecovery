ifneq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

commands_recovery_local_path := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
    default_recovery_ui.c \
    recovery.c \
    bootloader.c \
    install.c \
    roots.c \
    ui.c \
    encryptedfs_provisioning.c \
    verifier.c \
    colors_menu.c \
    extras_menu.c \
    mount_menu.c \
    wipe_menu.c \
    install_menu.c \
    nandroid_menu.c \
    cw_nandroid_menu.c \
    overclock_menu.c \
    mounts.c
    
	
    

LOCAL_MODULE := recovery

LOCAL_FORCE_STATIC_EXECUTABLE := true

RECOVERY_API_VERSION := 3
LOCAL_CFLAGS += -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)

# This binary is in the recovery ramdisk, which is otherwise a copy of root.
# It gets copied there in config/Makefile.  LOCAL_MODULE_TAGS suppresses
# a (redundant) copy of the binary in /system/bin for user builds.
# TODO: Build the ramdisk image in a more principled way.

LOCAL_MODULE_TAGS := eng

LOCAL_STATIC_LIBRARIES :=
ifeq ($(TARGET_RECOVERY_UI_LIB),)
  LOCAL_SRC_FILES += default_recovery_ui.c
else
  LOCAL_STATIC_LIBRARIES += $(TARGET_RECOVERY_UI_LIB)
endif
LOCAL_STATIC_LIBRARIES += libext4_utils libz
LOCAL_STATIC_LIBRARIES += libminzip libunz libmincrypt
LOCAL_STATIC_LIBRARIES += libminui libpixelflinger_static libpng libcutils
LOCAL_STATIC_LIBRARIES += libflashutils libmtdutils libmmcutils libbmlutils liberase_image libdump_image libflash_image
LOCAL_STATIC_LIBRARIES += libstdc++ libc

LOCAL_C_INCLUDES += system/extras/ext4_utils

include $(BUILD_EXECUTABLE)

# copy prebuilt items

include $(CLEAR_VARS)
LOCAL_MODULE := recovery.version
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)
LOCAL_SRC_FILES := recovery.version
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := initlogo.rle
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)
LOCAL_SRC_FILES := initlogo.rle
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := busybox
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := busybox
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := cw_restore.sh
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := cw_restore.sh
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := nandroid-mobile.sh
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := nandroid-mobile.sh
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := dump_image-arm-uclibc
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := dump_image-arm-uclibc
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)

LOCAL_MODULE := mkyaffs2image-arm-uclibc
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := mkyaffs2image-arm-uclibc
include $(BUILD_PREBUILT)
include $(CLEAR_VARS)

LOCAL_MODULE := unyaffs-arm-uclibc
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := unyaffs-arm-uclibc
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := verifier_test.c verifier.c

LOCAL_MODULE := verifier_test

LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_MODULE_TAGS := tests

LOCAL_STATIC_LIBRARIES := libmincrypt libcutils libstdc++ libc

include $(BUILD_EXECUTABLE)

include $(commands_recovery_local_path)/flashutils/Android.mk
include $(commands_recovery_local_path)/mtdutils/Android.mk
include $(commands_recovery_local_path)/mmcutils/Android.mk
include $(commands_recovery_local_path)/bmlutils/Android.mk
include $(commands_recovery_local_path)/minui/Android.mk
include $(commands_recovery_local_path)/minzip/Android.mk
include $(commands_recovery_local_path)/tools/Android.mk
include $(commands_recovery_local_path)/edify/Android.mk
include $(commands_recovery_local_path)/updater/Android.mk
include $(commands_recovery_local_path)/applypatch/Android.mk

commands_recovery_local_path :=

endif   # TARGET_ARCH == arm
endif    # !TARGET_SIMULATOR

