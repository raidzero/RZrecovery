ifneq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

commands_recovery_local_path := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
    recovery.c \
    bootloader.c \
    install.c \
    roots.c \
    ui.c \
    default_recovery_ui.c \
    encryptedfs_provisioning.c \
    verifier.c \
    colors_menu.c \
    extras_menu.c \
    romTweaks_menu.c \
    mount_menu.c \
    wipe_menu.c \
    install_menu.c \
    nandroid_menu.c \
    cw_nandroid_menu.c \
    overclock_menu.c \
    mounts.c

LOCAL_MODULE := recovery

LOCAL_FORCE_STATIC_EXECUTABLE := true

#the world just isnt ready for API level 3 yet
RECOVERY_API_VERSION := 2 

LOCAL_CFLAGS += -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)

TARGET_DEVICE := $(shell echo $$TARGET_PRODUCT | cut -d '_' -f2)
RECOVERY_VERSION := 2.1.0
VERS_STRING := "$(RECOVERY_VERSION)-$(TARGET_DEVICE) finally"

# This binary is in the recovery ramdisk, which is otherwise a copy of root.
# It gets copied there in config/Makefile.  LOCAL_MODULE_TAGS suppresses
# a (redundant) copy of the binary in /system/bin for user builds.
# TODO: Build the ramdisk image in a more principled way.

LOCAL_MODULE_TAGS := eng

LOCAL_STATIC_LIBRARIES :=
LOCAL_STATIC_LIBRARIES += libext4_utils libz
LOCAL_STATIC_LIBRARIES += libminzip libunz libmincrypt
LOCAL_STATIC_LIBRARIES += libminui libpixelflinger_static libpng libcutils
LOCAL_STATIC_LIBRARIES += libflashutils libmtdutils libmmcutils libbmlutils liberase_image libdump_image libflash_image
LOCAL_STATIC_LIBRARIES += libstdc++ libc

LOCAL_C_INCLUDES += system/extras/ext4_utils

include $(BUILD_EXECUTABLE)

#symlinks
RECOVERY_LINKS := flash_image dump_image erase_image format
RECOVERY_SYMLINKS := $(addprefix $(TARGET_RECOVERY_ROOT_OUT)/sbin/,$(RECOVERY_LINKS))
$(RECOVERY_SYMLINKS): RECOVERY_BINARY := $(LOCAL_MODULE)
$(RECOVERY_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Symlink: $@ -> $(RECOVERY_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	@echo "$(VERS_STRING)" > $(TARGET_RECOVERY_ROOT_OUT)/recovery.version
	$(hide) ln -sf $(RECOVERY_BINARY) $@
ALL_DEFAULT_INSTALLED_MODULES += $(RECOVERY_SYMLINKS)


# copy prebuilt items
include $(CLEAR_VARS)
LOCAL_MODULE := initlogo.rle
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)
LOCAL_SRC_FILES := initlogo.rle
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := mke2fs
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := mke2fs
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := tune2fs
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := tune2fs
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := e2fsck
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := e2fsck
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
LOCAL_MODULE := RZR-noverify.sh
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := RZR-noverify.sh
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := unyaffs-arm-uclibc
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := unyaffs-arm-uclibc
include $(BUILD_PREBUILT)

#busybox links
include $(CLEAR_VARS)
BUSYBOX_LINKS := [ [[ arp ash awk base64 basename bbconfig blockdev brctl bunzip2 bzcat bzip2 cal cat catv chattr chgrp chmod chown chroot clear cmp comm cp cpio crond crontab cut date dc dd depmod devmem df diff dirname dmesg dnsd dos2unix du echo ed egrep env expand expr false fdisk fgrep find flashcp flash_unlock flash_lock flock fold free freeramdisk fsync ftpget ftpput fuser getopt grep groups gunzip gzip halt head hexdump id ifconfig insmod iostat install ip kill killall killall5 length less ln losetup ls lsattr lsmod lsusb lzcat lzma lzop lzopcat man md5sum mesg mkdir mkfifo mknod mkswap mktemp modinfo modprobe more mount mountpoint mpstat mv nanddump nandwrite nc netstat nice nohup nslookup ntpd od patch pgrep pidof ping pkill printenv printf ps pstree pmap poweroff pwd pwdx rdev readlink realpath renice reset resize rev rm rmdir rmmod route run-parts rx sed seq setconsole setserial setsid sh sha1sum sha256sum sha512sum sleep sort split stat strings stty sum swapoff swapon sync sysctl tac tail tar tee telnet telnetd test tftp tftpd time timeout top touch tr traceroute true ttysize umount uname uncompress unexpand uniq unix2dos unxz unlzma unlzop unzip uptime usleep uudecode uuencode vi watch wc wget which whoami xargs xzcat xz yes zcat
BUSYBOX_SYMLINKS := $(addprefix $(TARGET_RECOVERY_ROOT_OUT)/sbin/,$(BUSYBOX_LINKS))
$(BUSYBOX_SYMLINKS): BUSYBOX_BINARY := busybox
$(BUSYBOX_SYMLINKS): $(LOCAL_INSTALLED_MODULE)	
	echo "Symlink: $@ $(BUSYBOX_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(BUSYBOX_BINARY) $@
ALL_DEFAULT_INSTALLED_MODULES += $(BUSYBOX_SYMLINKS)

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

