#RZRecovery version 2.1.4
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
    plugins_menu.c \
    mount_menu.c \
    wipe_menu.c \
    install_menu.c \
    dirsize.c \
    nandroid.c \
    nandroid_menu.c \
    overclock_menu.c \
    mkbootimg.c \
    unpackbootimg.c \
    mkbootfs.c \
    unyaffs.c \
    mounts.c \
    popen.c 

##the world just isnt ready for API level 3 yet
RECOVERY_API_VERSION := 2 

##generate the recovery version file
TARGET_DEVICE := $(shell echo $$TARGET_PRODUCT | cut -d '_' -f2)
RZR_VERSION := "2.1.4"
VERS_STRING := "$(RZR_VERSION)-$(TARGET_DEVICE) finally"

SOURCE_HOME := "/home/raidzero/android/system/2.3.7/bootable/recovery"
DEVICE_HOME := ../../device/raidzero/$(TARGET_DEVICE)

##build the main recovery module
LOCAL_MODULE := recovery
LOCAL_MODULE_TAGS := eng
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_CFLAGS += -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
LOCAL_STATIC_LIBRARIES :=
LOCAL_STATIC_LIBRARIES += libext4_utils libz
LOCAL_STATIC_LIBRARIES += libminzip libunz libmincrypt
LOCAL_STATIC_LIBRARIES += libminui libpixelflinger_static libpng libcutils
LOCAL_STATIC_LIBRARIES += libflashutils libmtdutils libmmcutils libbmlutils liberase_image libdump_image libflash_image

BOARD_RECOVERY_DEFINES := BOARD_HAS_NO_SELECT_BUTTON BOARD_HAS_INVERTED_VOLUME BOARD_UMS_LUNFILE

$(foreach board_define,$(BOARD_RECOVERY_DEFINES), \
  $(if $($(board_define)), \
      $(eval LOCAL_CFLAGS += -D$(board_define)=\"$($(board_define))\") \
   ) \
)

LOCAL_STATIC_LIBRARIES += libstdc++ libc

LOCAL_C_INCLUDES += system/extras/ext4_utils
include $(BUILD_EXECUTABLE)

##recovery symlinks
RECOVERY_LINKS := flash_image dump_image erase_image format mkfs.ext4 mkbootimg unpack_bootimg mkbootfs reboot_android unyaffs keytest compute_size compute_files list_files freespace printfile
RECOVERY_SYMLINKS := $(addprefix $(TARGET_RECOVERY_ROOT_OUT)/sbin/,$(RECOVERY_LINKS))
$(RECOVERY_SYMLINKS): RECOVERY_BINARY := $(LOCAL_MODULE)
$(RECOVERY_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Symlink: $@ -> $(RECOVERY_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	@echo "$(VERS_STRING)" > $(TARGET_RECOVERY_ROOT_OUT)/recovery.version
	$(hide) ln -sf $(RECOVERY_BINARY) $@
ALL_DEFAULT_INSTALLED_MODULES += $(RECOVERY_SYMLINKS)

include $(CLEAR_VARS)
RECOVERY_VERSION: VERS_FILE := $(LOCAL_MODULE)
$(RECOVERY_VERSION) : $(LOCAL_INSTALLED_MODULE)
	@echo "$(VERS_STRING)" > $(TARGET_RECOVERY_ROOT_OUT)/$(VERS_FILE)
ALL_DEFAULT_INSTALLED_MODULES += $(RECOVERY_VERSION)

include $(CLEAR_VARS)
LOCAL_MODULE := e2fsck
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := flash_img
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := dump_img
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := nandroid-mobile.sh
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := compress_nandroid.sh
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := su
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := symlink_sbin
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := ui_commands.sh
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := block_update
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

POSTRECOVERYBOOT := $(wildcard $(TARGET_DEVICE_DIR)/recovery/postrecoveryboot.sh)
ifneq ($(strip $(POSTRECOVERYBOOT)),)
rule: POSTRECOVERYBOOT
include $(CLEAR_VARS)
LOCAL_MODULE := postrecoveryboot.sh
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_SRC_FILES := $(DEVICE_HOME)/recovery/postrecoveryboot.sh
include $(BUILD_PREBUILT)
endif


CUSTOM_BUSYBOX := $(wildcard $(TARGET_DEVICE_DIR)/recovery/busybox)
include $(CLEAR_VARS)
LOCAL_MODULE := busybox
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := RECOVERY_EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
ifeq ($(strip $(CUSTOM_BUSYBOX)),)
rule:
LOCAL_SRC_FILES := busybox
else
rule: CUSTOM_BUSYBOX
LOCAL_SRC_FILES := $(DEVICE_HOME)/recovery/busybox
endif
include $(BUILD_PREBUILT)

##busybox symlinks
BUSYBOX_LINKS := [ [[ arp ash awk base64 basename bbconfig blockdev brctl bunzip2 bzcat bzip2 cal cat catv chattr chgrp chmod chown chroot clear cmp comm cp cpio crond crontab cut date dc dd depmod devmem df diff dirname dmesg dnsd dos2unix du echo ed egrep env expand expr false fdisk fgrep find flashcp flash_unlock flash_lock flock fold free freeramdisk fsck fsync ftpget ftpput fuser getopt grep groups gunzip gzip halt head hexdump id ifconfig insmod iostat install ip kill killall killall5 length less ln losetup ls lsattr lsmod lsusb lzcat lzma lzop lzopcat man md5sum mesg mkdir mkfifo mke2fs mknod mkswap mktemp modinfo modprobe more mount mountpoint mpstat mv nanddump nandwrite nc netstat nice nohup nslookup ntpd od patch pgrep pidof ping pkill printenv printf ps pstree pmap poweroff pwd pwdx rdev readlink realpath renice reset resize rev rm rmdir rmmod route run-parts rx sed seq setconsole setserial setsid sh sha1sum sha256sum sha512sum sleep sort split stat strings stty sum swapoff swapon sync sysctl tac tail tar tee telnet telnetd test tftp tftpd time timeout top touch tr traceroute true tune2fs ttysize umount uname uncompress unexpand uniq unix2dos unxz unlzma unlzop unzip uptime usleep uudecode uuencode vi watch wc wget which whoami xargs xzcat xz yes zcat
BUSYBOX_SYMLINKS := $(addprefix $(TARGET_RECOVERY_ROOT_OUT)/sbin/,$(BUSYBOX_LINKS))
$(BUSYBOX_SYMLINKS): BUSYBOX_BINARY := busybox
$(BUSYBOX_SYMLINKS): $(LOCAL_INSTALLED_MODULE)	
	echo "Symlink: $@ $(BUSYBOX_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(BUSYBOX_BINARY) $@
ALL_DEFAULT_INSTALLED_MODULES += $(BUSYBOX_SYMLINKS)

include $(CLEAR_VARS)
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
