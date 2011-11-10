#/sbin/sh
source /ui_commands.sh

MOUNTPOINT=$1
DESIRED_FS=$2
mkdir /tmp/bootimg
mkdir /tmp/recimg

echo "* print Dumping recovery..."
dump_image recovery /tmp/recimg/recovery.img

echo "* print unpacking recovery image..."
cd /tmp/recimg
unpack_bootimg -i recovery.img
REC_BASE_ADDR=`cat recovery.img-base`
REC_CMD_LINE=`cat recovery.img-cmdline`
REC_PAGE_SIZE=`cat recovery.img-pagesize`
echo "* print REC_BASE_ADDR: $REC_BASE_ADDR"
echo "* print REC_CMD_LINE: $REC_CMD_LINE"
echo "* print REC_PAGE_SIZE: $REC_PAGE_SIZE"
mkdir ramdisk
cd ramdisk
gzip -dc ../recovery.img-ramdisk.gz | cpio -i
echo "* print Unpacked recovery ramdisk"
CURRENT_FS=`cat etc/recovery.fstab | grep /data | awk '{print$2}'`
#escape the slashes so sed doesnt freak
MOUNTPOINT=$(echo $MOUNTPOINT | sed 's/\//\\\//g')
sed -i '/'"$MOUNTPOINT"'/s/'"$CURRENT_FS"'/'"$DESIRED_FS"'/g' etc/recovery.fstab
sed -i '/'"$MOUNTPOINT"'/s/'"$CURRENT_FS"'/'"$DESIRED_FS"'/g' init*.rc
#create the bootfs
cd /tmp/recimg
mkbootfs ./ramdisk | gzip > rec-ramdisk-new.gz
#create the new img
mkbootimg --kernel recovery.img-zImage --ramdisk rec-ramdisk-new.gz --cmdline "$REC_CMD_LINE" --base "$REC_BASE_ADDR" --pagesize "$REC_PAGE_SIZE" -o recovery-new.img
#flash it
echo "* print Flashing modified img..."
flash_image recovery /tmp/recimg/recovery-new.img

echo "* print Dumping boot..."
dump_image boot /tmp/bootimg/boot.img

echo "* print Unpacking boot image..."
cd /tmp/bootimg
unpack_bootimg -i boot.img
BOOT_BASE_ADDR=`cat boot.img-base`
BOOT_CMD_LINE=`cat boot.img-cmdline`
BOOT_PAGE_SIZE=`cat boot.img-pagesize`
echo "* print BOOT_BASE_ADDR: $BOOT_BASE_ADDR"
echo "* print BOOT_CMD_LINE: $BOOT_CMD_LINE"
echo "* print BOOT_PAGE_SIZE: $BOOT_PAGE_SIZE"
mkdir ramdisk
cd ramdisk
gzip -dc ../boot.img-ramdisk.gz | cpio -i
echo "* print Unpacked boot image"
sed -i '/'"$MOUNTPOINT"'/s/'"$CURRENT_FS"'/'"$DESIRED_FS"'/g' init*.rc
#create bootfs
cd /tmp/bootimg
mkbootfs ./ramdisk | gzip > boot-ramdisk-new.gz
#create the new img
mkbootimg --kernel boot.img-zImage --ramdisk boot-ramdisk-new.gz --cmdline "$BOOT_CMD_LINE" --base "$BOOT_BASE_ADDR" --pagesize "$BOOT_PAGE_SIZE" -o boot-new.img
#flash it
flash_image boot boot-new.img

echo "* print Done! mounts modified in recovery & boot images"
