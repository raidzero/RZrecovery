#!/sbin/sh
source /ui_commands.sh

BACKUP=$1
cd $BACKUP
START=`date +%s`
tars=`find . -maxdepth 1 -name "*.tar" | xargs | sed 's/\.\///g'`
number=`echo $tars | awk '{print NF}'`
if [ $number -ge 1 ]; then
  echo "* print Begin compressing backup $BACKUP." 
  for tar in $tars; do
    name=`echo $tar | sed 's/\.tar//g'`
    echo "* print Compressing $name..."
    echo "* show_indeterminate_progress"
    gzip $tar 
  done
  END=`date +%s`
  ELAPSED=$(( $END - $START ))
else 
  echo "*print Backup is already compressed or is incomplete!"
  exit 100
fi

echo "* print Backup $BACKUP has been compressed!"
echo "* print Compress operation took $ELAPSES seconds."
exit 0
