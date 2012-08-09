LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

commands_recovery_local_path := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
    recovery.c \
    bootloader.c \
    install.c \
    roots.c \
    ui.c \
    rz_funcs.c \
    power_menu.c \
    verifier.c

LOCAL_MODULE := recovery

LOCAL_FORCE_STATIC_EXECUTABLE := true

DEVICE_NAME := $(shell echo $$TARGET_PRODUCT | cut -d '_' -f2)
RECOVERY_VERSION = 3.0.0
RECOVERY_TITLE := "Welcome to RZRrecovery [$(RECOVERY_VERSION)-$(DEVICE_NAME)]"

#the world isnt ready for API level 3 yet
RECOVERY_API_VERSION := 2

LOCAL_CFLAGS += -DRECOVERY_TITLE=$(RECOVERY_TITLE)
LOCAL_CFLAGS += -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)

LOCAL_STATIC_LIBRARIES :=

ifeq ($(TARGET_USERIMAGES_USE_EXT4), true)
LOCAL_CFLAGS += -DUSE_EXT4
LOCAL_C_INCLUDES += system/extras/ext4_utils
LOCAL_STATIC_LIBRARIES += libext4_utils libz
endif

# This binary is in the recovery ramdisk, which is otherwise a copy of root.
# It gets copied there in config/Makefile.  LOCAL_MODULE_TAGS suppresses
# a (redundant) copy of the binary in /system/bin for user builds.
# TODO: Build the ramdisk image in a more principled way.

LOCAL_MODULE_TAGS := eng

ifeq ($(TARGET_RECOVERY_UI_LIB),)
  LOCAL_SRC_FILES += default_recovery_ui.c
else
  LOCAL_STATIC_LIBRARIES += $(TARGET_RECOVERY_UI_LIB)
endif
LOCAL_STATIC_LIBRARIES += libext4_utils libz
LOCAL_STATIC_LIBRARIES += libminzip libunz libmtdutils libmincrypt
LOCAL_STATIC_LIBRARIES += libminui libpixelflinger_static libpng libcutils
LOCAL_STATIC_LIBRARIES += libstdc++ libc
LOCAL_STATIC_LIBRARIES += libbusybox

LOCAL_C_INCLUDES += system/extras/ext4_utils

include $(BUILD_EXECUTABLE)

##recovery symlinks
RECOVERY_LINKS := busybox
RECOVERY_SYMLINKS := $(addprefix $(TARGET_RECOVERY_ROOT_OUT)/sbin/,$(RECOVERY_LINKS))
$(RECOVERY_SYMLINKS): RECOVERY_BINARY := $(LOCAL_MODULE)
$(RECOVERY_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Symlink: $@ -> $(RECOVERY_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(RECOVERY_BINARY) $@
ALL_DEFAULT_INSTALLED_MODULES += $(RECOVERY_SYMLINKS)

##busybox symlinks
BUSYBOX_LINKS := [ [[ arp ash awk base64 basename bbconfig blockdev brctl bunzip2 bzcat bzip2 cal cat catv chattr chgrp chmod chown chroot clear cmp comm cp cpio crond crontab cut date dc dd depmod devmem df diff dirname dmesg dnsd dos2unix du echo ed egrep env expand expr false fdisk fgrep find flashcp flash_unlock flash_lock flock fold free freeramdisk fsck fsync ftpget ftpput fuser getopt grep groups gunzip gzip halt head hexdump id ifconfig insmod iostat install ip kill killall killall5 length less ln losetup ls lsattr lsmod lsusb lzcat lzma lzop lzopcat man md5sum mesg mkdir mkfifo mke2fs mknod mkswap mktemp modinfo modprobe more mount mountpoint mpstat mv nanddump nandwrite nc netstat nice nohup nslookup ntpd od patch pgrep pidof ping pkill printenv printf ps pstree pmap poweroff pwd pwdx rdev readlink realpath renice reset resize rev rm rmdir rmmod route run-parts rx sed seq setconsole setserial setsid sh sha1sum sha256sum sha512sum sleep sort split stat strings stty sum swapoff swapon sync sysctl tac tail tar tee telnet telnetd test tftp tftpd time timeout top touch tr traceroute true tune2fs ttysize umount uname uncompress unexpand uniq unix2dos unxz unlzma unlzop unzip uptime usleep uudecode uuencode vi watch wc wget which whoami xargs xzcat xz yes zcat
BUSYBOX_SYMLINKS := $(addprefix $(TARGET_RECOVERY_ROOT_OUT)/sbin/,$(BUSYBOX_LINKS))
$(BUSYBOX_SYMLINKS): BUSYBOX_BINARY := busybox
$(BUSYBOX_SYMLINKS): $(LOCAL_INSTALLED_MODULE)	
	echo "Symlink: $@ -> $(BUSYBOX_BINARY)"
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


include $(commands_recovery_local_path)/minui/Android.mk
include $(commands_recovery_local_path)/minelf/Android.mk
include $(commands_recovery_local_path)/minzip/Android.mk
include $(commands_recovery_local_path)/mtdutils/Android.mk
include $(commands_recovery_local_path)/tools/Android.mk
include $(commands_recovery_local_path)/edify/Android.mk
include $(commands_recovery_local_path)/updater/Android.mk
include $(commands_recovery_local_path)/applypatch/Android.mk
commands_recovery_local_path :=
