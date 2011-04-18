ifneq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

commands_recovery_local_path := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
    recovery.c \
    recovery_lib.c \
    recovery_menu.c \
    nandroid_menu.c \
    install_menu.c \
    mount_menu.c \
    wipe_menu.c \
    bootloader.c \
    cw_nandroid_menu.c \
    install.c \
    options_menu.c \
    roots.c \
    mounts.c \
    colors_menu.c \
    ui.c \

LOCAL_SRC_FILES += test_roots.c

LOCAL_MODULE := recovery

LOCAL_FORCE_STATIC_EXECUTABLE := true

RECOVERY_API_VERSION := 3
LOCAL_CFLAGS += -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)

BOARD_RECOVERY_DEFINES := \
	BOARD_SDCARD_DEVICE_PRIMARY \
	BOARD_SDCARD_DEVICE_SECONDARY \
	BOARD_DATA_DEVICE \
	BOARD_DATA_FILESYSTEM \
	BOARD_DATA_FILESYSTEM_OPTIONS \
	BOARD_DATADATA_DEVICE \
	BOARD_DATADATA_FILESYSTEM \
	BOARD_DATADATA_FILESYSTEM_OPTIONS \
	BOARD_CACHE_DEVICE \
	BOARD_CACHE_FILESYSTEM \
	BOARD_CACHE_FILESYSTEM_OPTIONS \
	BOARD_SYSTEM_DEVICE \
	BOARD_SYSTEM_FILESYSTEM \
	BOARD_SYSTEM_FILESYSTEM_OPTIONS \
	BOARD_HAS_DATADATA \
	BOARD_SDCARD_DEVICE_INTERNAL

$(foreach board_define,$(BOARD_RECOVERY_DEFINES), \
  $(if $($(board_define)), \
    $(eval LOCAL_CFLAGS += -D$(board_define)=\"$($(board_define))\") \
  ) \
  )

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
LOCAL_STATIC_LIBRARIES += libminzip libunz libflashutils libmtdutils libmmcutils libbmlutils
LOCAL_STATIC_LIBRARIES += libminui libpixelflinger_static libpng libcutils
LOCAL_STATIC_LIBRARIES += libstdc++ libc

include $(BUILD_EXECUTABLE)


include $(commands_recovery_local_path)/flashutils/Android.mk
include $(commands_recovery_local_path)/minui/Android.mk
include $(commands_recovery_local_path)/minzip/Android.mk
include $(commands_recovery_local_path)/bmlutils/Android.mk
include $(commands_recovery_local_path)/mmcutils/Android.mk
include $(commands_recovery_local_path)/mtdutils/Android.mk
include $(commands_recovery_local_path)/tools/Android.mk
include $(commands_recovery_local_path)/edify/Android.mk
include $(commands_recovery_local_path)/updater/Android.mk
include $(commands_recovery_local_path)/applypatch/Android.mk
commands_recovery_local_path :=

endif   # TARGET_ARCH == arm
endif    # !TARGET_SIMULATOR

