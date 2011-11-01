#!/sbin/sh

source /ui_commands.sh

list=`find . | sed -e 's/\.\///g' | grep -v nandroid.md5 | grep -v recovery.img`

bootp=`echo $list | grep boot | wc -l`
sysp=`echo $list | grep system | wc -l`
asp=`echo $list | grep secure | wc -l`
datap=`echo $list | grep data | wc -l`
cachp=`echo $list | grep cache | wc -l`
datadp=`echo $list | grep datadata | wc -l`

data_mounted=`mount | grep "/data" | wc -l`
datadata_mounted=`mount | grep "/data/data" | wc -l`
system_mounted=`mount | grep "/system" | wc -l`

if [ "$bootp" == "1" ]; then
	echo "* print Restoring boot..."
	flash_image boot $1/boot.img
	echo "* print boot partition restored."
fi

if [ "$sysp" == "1" ]; then
	if [ "$system_mounted" == "0" ]; then
		mount /system
	fi
	echo "* print Erasing system..."
	rm -rf /system/*
	cd /system
	echo "* print Restoring system..."
	unyaffs-arm-uclibc $1/system.img
	echo "* print system partition restored."
	echo "* print Fixing su permissions..."
	busybox chmod 6755 /system/bin/su
	busybox chmod 6755 /system/xbin/su
fi

if [ "$asp" == "1" ]; then
	echo "* print Erasing .android_secure..."
	rm -rf /sdcard/.android_secure/*
	cd /sdcard/.android_secure
	echo "* print Restoring .android_secure..."
	unyaffs-arm-uclibc $1/.android_secure.img
	echo "* print .android_secure restored."
fi

if [ "$datap" == "1" ]; then
	echo "* print Erasing data..."
	if [ "$data_mounted" == "0" ]; then
		mount /data
	fi
	rm -rf /data/*
	cd /data	
	echo "* print Restoring data..."
	unyaffs-arm-uclibc $1/data.img
	if [ "$datadp" =="1" ]; then
		if [ "$datadata_mounted" == "0" ]; then
			mount /data/data
		fi
		rm -rf /data/data/*
		cd /data/data
		echo "* print Restoring datadata..."
		unyaffs-arm-uclibc $1/datadata.img
	fi
	echo "* print data partition restored."
fi
	
if [ "$cachp" == "1" ]; then
	echo "* print Erasing cache..."
	cp /cache/rgb /sdcard/RZR/rgb
	cd /cache
	rm -rf /cache/*
	echo "* print Restoring cache..."
	unyaffs-arm-uclibc $1/cache.img
	cp /sdcard/RZR/rgb /cache/rgb
	echo "* print cache partition restored."
fi

if [ "$system_mounted" != "0" ]; then
	umount /system
fi
if [ "$data_mounted" != "0" ]; then
	umount /data
fi

sync

echo "* print Clockwork nandroid restore complete!"
echo "* print Thanks for using RZRecovery."

