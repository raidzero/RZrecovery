#!/sbin/sh
export PATH=/sbin:$PATH
source /ui_commands.sh

BACKUP=$1
cd $BACKUP
echo "Start compressing $BACKUP"
tars=`find . -maxdepth 1 -name "*.tar" | xargs | sed 's/\.\///g'`

if [ -z "$tars" ]; then 
  echo "* print Backup is already compressed or incomplete."
  exit 100
fi

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

