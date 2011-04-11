#!/sbin/sh

source /ui_commands.sh
mount /sdcard

list=`find . | sed -e 's/\.\///g' | grep -v nandroid.md5 | grep -v recovery.img`

bootp=`echo $list | grep boot | wc -l`
sysp=`echo $list | grep system | wc -l`
asp=`echo $list | grep secure | wc -l`
datap=`echo $list | grep data | wc -l`
cachp=`echo $list | grep cache | wc -l`


if [ "$bootp" == "1" ]; then
	echo "* print Restoring boot..."
	format BOOT:
	echo "* print "
	flash_image boot $1/boot.img
	echo "* print boot partition restored."
fi

if [ "$sysp" == "1" ]; then
	echo "* print Restoring system..."
	umount /system
	format SYSTEM:
	mount /system
	cd /system
	echo "* "
	unyaffs-arm-uclibc $1/system.img
	echo "* print system partition restored."
	echo "* print Fixing su permissions..."
	busybox chmod 6755 /system/bin/su
	busybox chmod 6755 /system/xbin/su
fi

if [ "$asp" == "1" ]; then
	echo "* print Restoring .android_secure..."
	rm -rf /sdcard/.android_secure/*
	cd /sdcard/.android_secure
	echo "* "
	unyaffs-arm-uclibc $1/.android_secure.img
	echo "* print .android_secure restored."
fi

if [ "$datap" == "1" ]; then
	echo "* print Restoring data..."
	umount /data
	format DATA:
	mount /data
	cd /data
	echo "* "
	unyaffs-arm-uclibc $1/data.img
	echo "* print data partition restored."
fi
	
if [ "$cachp" == "1" ]; then
	echo "* print Restoring cache..."
	cp /cache/oc /sdcard/RZR/oc
	cp /cache/rgb /sdcard/RZR/rgb
	umount /cache
	format CACHE:
	cd /cache
	echo "* "
	unyaffs-arm-uclibc $1/cache.img
	mount /cache
	cp /sdcard/RZR/oc /cache/oc
	cp /sdcard/RZR/rgb /cache/rgb
	echo "* print cache partition restored."
fi
echo "* print Clockwork Nandroid Restore complete!"
echo "* print Thanks for using RZRecovery."
umount /data
umount /system
sync
