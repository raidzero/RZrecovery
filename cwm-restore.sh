#!/sbin/sh
#cwm nandroid restore tool, should work on tar and img nandroids
. /ui_commands.sh

NAND_NAME=$1
STORAGE_ROOT=$2
NAND_DIR="$STORAGE_ROOT/clockworkmod/backup/$NAND_NAME"

pipeline() {
  echo "* show_indeterminate_progress"
  awk "NR==1 {print \"* ptotal $1\"} {print \"* pcur \" NR}"
  echo "* reset_progress" 
}

cd $NAND_DIR

system=`find . -name "*system*"`
data=`find . -name "*data*"`
boot=`find . -name "*boot*"`
secure=`find . -name "*android_secure*"`
cache=`find . -name "*cache*"`

for image in system data boot secure cache; do
image_dir=$image
dest_dir=$image
image=`find . -name "*$image*"`

if [ "$image_dir" == "secure" ]; then
  dest_dir=.android_secure
fi

if [ ! -z "$image" ]; then
  IMAGE_IS_TAR=`echo $image | grep "\.tar" | wc -l`
  ui_print "Processing $image..."
  if [ $IMAGE_IS_TAR -ne 0 ]; then
    ui_print "Unpacking..."
    PTOTAL=$(($(tar tf $image | wc -l)-2))
	if [ "$image_dir" == "secure" ]; then
	  dest_dir=$STORAGE_ROOT/.android_secure
	else
	  dest_dir="/$dest_dir"
    fi
	rm -rf $dest_dir/*
	tar xvf $NAND_DIR/$image -C $dest_dir | pipeline $PTOTAL
  else	  
    dest_dir="/$dest_dir"
	format $dest_dir
	cd $dest_dir
    unyaffs $NAND_DIR/$image
    RTN=`find . | grep -v "$image" | wc -l`
    if [ $RTN -le 1 ]; then
      flash_img $dest_dir $NAND_DIR/$image
    fi
  fi
fi
cd $NAND_DIR
#reclaim any cached ram and write all changes to disk
sync
echo 3 > /proc/sys/vm/drop_caches
done
ui_print "Restore complete!"

