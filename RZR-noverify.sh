#!/sbin/sh
#disable/enable dalvik bytecode verification

CHOICE=$1

mount system
if [ "$CHOICE" == "disable" ]; then
	echo "* print Disabling dalvik-bytecode verification..."
	sed -i '/dalvik\.vm\.dexopt\-flags/d' /system/build.prop
	sed -i '/dalvik\.vm\.verify\-bytecode/d' /system/build.prop
	sed -i '/dalvik\.vm\.dexopt\-flags\=v\=n,o\=v/d' /system/build.prop
	sed -i '/dexopt RZR noverify/d' /system/build.prop
	sed -i '/dexopt noverify/d' /system/build.prop

	echo "#dexopt RZR noverify#" >> /system/build.prop
	echo "dalvik.vm.verify-bytecode=false" >> /system/build.prop 
	echo "dalvik.vm.dexopt-flags=v=n,o=v" >> /system/build.prop
	echo "#dexopt RZR noverify#" >> /system/build.prop
	

fi

if [ "$CHOICE" == "enable" ]; then
	echo "* print Enabling dalvik bytecode verification..."
	sed -i '/dalvik\.vm\.dexopt\-flags/d' /system/build.prop
	sed -i '/dalvik\.vm\.verify\-bytecode/d' /system/build.prop
	sed -i '/dalvik\.vm\.dexopt\-flags\=v\=n,o\=v/d' /system/build.prop
	sed -i '/dexopt RZR noverify/d' /system/build.prop
	sed -i '/dexopt noverify/d' /system/build.prop

	echo "#dexopt RZR noverify#" >> /system/build.prop
	echo "dalvik.vm.verify-bytecode=true" >> /system/build.prop 
	echo "dalvik.vm.dexopt-flags=v=n,o=vd" >> /system/build.prop
	echo "#dexopt RZR noverify#" >> /system/build.prop	
fi


echo "* print Wiping dalvik-cache...."
mount data
rm -rf /cache/dalvik-cache/*
rm -rf /data/dalvik-cache/*
umount data
umount system
