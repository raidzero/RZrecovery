#!/sbin/sh
source /ui_commands.sh

BACKUP=$1
cd $BACKUP
echo "Start compressing $BACKUP"
tars=`find . -maxdepth 1 -name "*.tar" | xargs | sed 's/\.\///g'`
number=`echo $tars | awk '{print NF}'`
if [ $number -ge 1 ]; then
  C_START=`date +%s`
  echo "* print Begin compressing backup $BACKUP." 
  for tar in $tars; do
    name=`echo $tar | sed 's/\.tar//g'`
    echo "* print Compressing $name..."
    echo "* show_indeterminate_progress"
    gzip $tar 
  done
  C_END=`date +%s`
  ELAPSED=$(( $C_END - $C_START ))
  echo "* print Backup $BACKUP has been compressed!"
  echo "* print Compress operation took $ELAPSED seconds."
  exit 0
else 
  echo "*print Backup is already compressed or is incomplete!"
  exit 100
fi

