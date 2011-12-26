#!/sbin/sh
#cwm nandroid conversion tool, should work on tar and img nandroids
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

RZR_NAME="$STORAGE_ROOT/nandroid/CWM-`date +%Y%m%d-%H%M`"
[ ! -d $RZR_NAME ] && mkdir -p $RZR_NAME
RZRTMP=$STORAGE_ROOT/RZR/tmp
[ ! -d $RZRTMP ] && mkdir $RZRTMP 

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
    tar xvf $NAND_DIR/$image -C $RZRTMP | pipeline $PTOTAL
    cd $RZRTMP/$dest_dir
    ui_print "Converting..."
    PTOTAL=`find . | wc -l`
    tar cvf $RZR_NAME/$dest_dir.tar . | pipeline $PTOTAL
  else
    cd $RZRTMP
    imgtmp="${image}tmp"
    mkdir $imgtmp
    cd $imgtmp
    unyaffs $NAND_DIR/$image
    RTN=`find . | grep -v "$image" | wc -l`
    if [ $RTN -le 1 ]; then
      cp $NAND_DIR/$image $RZR_NAME/$dest_dir.img
    else
      PTOTAL=`find . | wc -l`
      tar cvf $RZR_NAME/$dest_dir.tar . | pipeline $PTOTAL
    fi
  fi
fi
cd $NAND_DIR
done
rm -rf $RZRTMP
ui_print "All done! RZR Nandroid dir:"
ui_print $RZR_NAME
