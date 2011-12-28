#!/sbin/sh
source /ui_commands.sh
export PATH=/sbin:$PATH
SUBNAME=""
NOBOOT=0
NODATA=0
NOSYSTEM=0
NOSECURE=0
NOCACHE=0
NOSDEXT=0
RESTORE=0
BACKUP=0
COMPRESS=0
INSTALL_ROM=0
PLUGIN=0

#free up some ram first, cant hurt...
sync
echo 3 > /proc/sys/vm/drop_caches

# Boot, Data, System, Android-secure
# Enables the user to figure at a glance what is in the backup
BACKUPLEGEND=""

# Normally we want tar to be verbose for confidence building.
TARFLAGS="v"

ASSUMEDEFAULTUSERINPUT=0

data_mounted=`mount | grep "/data" | wc -l`
system_mounted=`mount | grep "/system" | wc -l`
cache_mounted=`mount | grep "/cache" | wc -l`

quit()
{
	sync
	echo 3 > /proc/sys/vm/drop_caches
	exit $1
}
	
echo2log()
{
    if [ -e /cache/recovery/log ]; then
	echo $1 >>/cache/recovery/log
    else
	echo $1 >/cache/recovery/log
    fi
}

batteryAtLeast()
{
    REQUIREDLEVEL=$1
    ENERGY=`cat /sys/class/power_supply/battery/capacity`
    if [ "`cat /sys/class/power_supply/battery/status`" == "Charging" ]; then
	ENERGY=100
    fi
    if [ ! $ENERGY -ge $REQUIREDLEVEL ]; then
	menu -h "Insufficient battery power" -h "Continue anyway at your own risk?" -h "" \
	  -i "No" \
	  -i "Yes"
	read BATT_CHOICE
	[ $BATT_CHOICE == 0 ] && action="abort"
	[ $BATT_CHOICE == 1 ] && action="continue"
	
	
	if [ "$action" != "continue" ]; then
	  echo "* print Error: not enough battery power, need at least $REQUIREDLEVEL%."
	  echo "* print Connect charger or USB power and try again."
	  quit 101
	fi
    fi
}

if [ "`echo $0 | grep /sbin/nandroid-mobile.sh`" == "" ]; then
    cp $0 /sbin
    chmod 755 /sbin/`basename $0`
    exec /sbin/`basename $0` $@
fi


DD_DEVICE=`cat /etc/fstab | grep "/datadata" | awk '{print$1}'`
EXT_DEVICE=`cat /etc/fstab | grep "/sd-ext" | awk '{print$1}'`
if [ `cat /etc/recovery.fstab | grep boot | egrep -v "("fota"|"bootloader")" | awk '{print$2}'` == "mtd" ]; then
  BOOT_DEVICE="/dev/mtd/"`cat /proc/mtd | grep boot | egrep -v "("fota"|"bootloader")" | awk '{print$1}' | sed 's/://g'`
else
  BOOT_DEVICE=`cat /etc/recovery.fstab | grep boot | egrep -v "("bootloader"|"fota_boot")" | awk '{print$3}'`
fi


# Hm, have to handle old options for the current UI
case $1 in
    restore)
        shift
        RESTORE=1
        ;;
    backup)
        shift
        BACKUP=1
        ;;
    --)
        ;;
esac

ECHO=echo
OUTPUT=""

for option in $(getopt --name="nandroid-mobile v2.2.1" -l backuppath -l progress -l install-rom: -l plugin -l noboot -l nodata -l nocache -l nosdext -l nosystem -l nosecure -l subname: -l backup -l compress -l restore -l defaultinput -- "cbruds:p:eqli:" "$@"); do
    case $option in
    --backuppath)
	  shift
	  BACKUPPATH="$1"
	  shift
	  ;;
	--verbose)
	    VERBOSE=1
	    shift
	    ;;
        --noboot)
            NOBOOT=1
            #$ECHO "No boot"
            shift
            ;;
        --nodata)
            NODATA=1
            #$ECHO "No data"
            shift
            ;;
		--nocache)
			NOCACHE=1
			shift
			;;
		--nosdext)
			NOSDEXT=1
			shift
			;;
        --nosystem)
            NOSYSTEM=1
            #$ECHO "No system"
            shift
            ;;
        --nosecure)
            NOSECURE=1
            #$ECHO "No secure"
            shift
            ;;
        --backup)
            BACKUP=1
            #$ECHO "backup"
            shift
            ;;
        -b)
            BACKUP=1
            #$ECHO "backup"
            shift
            ;;
	--compress)
	    COMPRESS=1
	    shift
	    ;;
			
        --restore)
            RESTORE=1
            #$ECHO "restore"
            shift
            ;;
			
        --defaultinput)
            ASSUMEDEFAULTUSERINPUT=1
            shift
            ;;
        -r)
            RESTORE=1
            #$ECHO "restore"
            shift
            ;;
        --subname)
            if [ "$2" == "$option" ]; then
                shift
            fi
            #$ECHO $2
            SUBNAME="$2"
            shift
            ;;
        --defaultinput)
            ASSUMEDEFAULTUSERINPUT=1
            shift
            ;;
        -s)
            if [ "$2" == "$option" ]; then
                shift
            fi
            #$ECHO $2
            SUBNAME="$2"
            shift
            ;;
	--install-rom)
	    if [ "$2" == "$option" ]; then
		shift
	    fi
	    INSTALL_ROM=1
	    ROM_FILE=$2
	    ;;
	--plugin)
	    PLUGIN=1
	    ;;
	--progress)
	    PROGRESS=1
	    ;;
        --)
            shift
            break
            ;;
    esac
done

ROOT_BACKUPDEVICE=`echo $BACKUPPATH | cut -d '/' -f2 | sed 's#^#/#g'`

[ "$PROGRESS" == "1" ] && echo "* show_indeterminate_progress"

pipeline() {
    if [ "$PROGRESS" == "1" ]; then
	[ "$PROGRESS" == "1" ] && echo "* show_indeterminate_progress"
	awk "NR==1 {print \"* ptotal $1\"} {print \"* pcur \" NR}"
	[ "$PROGRESS" == "1" ] && echo "* reset_progress"
    else
	cat
    fi
}

# Make sure it exists
touch /cache/recovery/log

if [ "$BACKUP" == 1 -o "$RESTORE" == 1 ]; then
    if [ "$ASSUMEDEFAULTUSERINPUT" == 0 ]; then
        read SUBNAME
    fi
fi	

if [ "$INSTALL_ROM" == 1 ]; then
    
    batteryAtLeast 20

    [ "$PROGRESS" == "1" ] && echo "* show_indeterminate_progress"

    [ "$PLUGIN" == "0" ] && echo "* print Installing ROM from "$ROM_FILE"..."
    mount $ROOT_BACKUPDEVICE

    if [ -z "$(mount | grep $ROOT_BACKUPDEVICE)" ]; then
	[ "$PLUGIN" == "0" ] && echo "* print error: unable to mount $ROOT_BACKUPDEVICE, aborting"
	quit 13
    fi
    
    if [ ! -f "$ROM_FILE" ]; then
        OLD_FILE=$ROM_FILE
        ROM_FILE=`echo $OLD_FILE | sed 's#$#.tgz#'`
	if [ ! -f $ROM_FILE ]; then
		ROM_FILE=`echo $OLD_FILE | sed 's#$#.tar#'`
		if [ ! -f $ROM_FILE ]; then
			[ "$PLUGIN" == "0" ] && echo "* print error: specified ROM file does not exist"
			quit 14
		fi
	fi
    fi

    if echo "$ROM_FILE" | grep ".*gz" >/dev/null;
    then
		[ "$PLUGIN" == "0" ] && echo "* print Decompressing $ROM_FILE..."
		busybox gzip -d "$ROM_FILE"
		ROM_FILE=$(echo "$ROM_FILE" | sed -e 's/.tar.gz$/.tar/' -e 's/.tgz/.tar/')
		[ "$PLUGIN" == "0" ] && echo "* print Decompressed to $ROM_FILE"
    fi

    cd /tmp
    TAR_OPTS="x"
    [ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
    TAR_OPTS="${TAR_OPTS}f"
    
    [ "$PLUGIN" == "0" ] && [ "$PROGRESS" == "1" ] && $ECHO "* print Unpacking $ROM_FILE..."
    PTOTAL=$(($(tar tf "$ROM_FILE" | wc -l)-2))
	[ "$PLUGIN" == "0" ] && echo "* print $PTOTAL files in archive!"
	[ "$PLUGIN" == "0" ] && echo "* print "
    
    tar $TAR_OPTS "$ROM_FILE" --exclude ./system.tar --exclude ./data.tar | pipeline $PTOTAL

    if [ -z $(tar tf "$ROM_FILE" | grep \./data\.tar) ]; then
	NODATA=1
    fi

    if [ -z $(tar tf "$ROM_FILE" | grep \./system\.tar) ]; then
	NOSYSTEM=1
    fi

    GETVAL=sed\ -r\ 's/.*=(.*)/\1/'

    [ "$PLUGIN" == "0" ] && if [ -d metadata ]; then
	HGREV=$(cat /recovery.version | awk '{print $2}')
	MIN_REV=$(cat metadata/* | grep "min_rev=" | head -n1 | $GETVAL)
	
	echo "* print "
	echo "* print --==AUTHORS==--"
	echo "* print  $(cat metadata/* | grep "author=" | $GETVAL)"
	echo "* print --==NAME==--"
	echo "* print  $(cat metadata/* | grep "name=" | $GETVAL)"
	echo "* print --==DESCRIPTION==--"
	echo "* print  $(cat metadata/* | grep "description=" | $GETVAL)"
	echo "* print --==HELP==--"
	echo "* print  $(cat metadata/* | grep "help=" | $GETVAL)"
	echo "* print --==URL==--"
	echo "* print  $(cat metadata/* | grep "url=" | $GETVAL)"
	echo "* print --==EMAIL==--"
	echo "* print $(cat metadata/* | grep "email=" | $GETVAL)"
	echo "* print "
    fi

    if [ -d pre.d ]; then
		[ "$PLUGIN" == "0" ] && echo "* print Executing pre-install scripts..."
		for sh in pre.d/[0-9]*.sh; do
			if [ -r "$sh" ]; then
			. "$sh"
			fi
		done
    fi

    for image in system data; do
	if [ "$image" == "data" -a "$NODATA" == "1" ];
	then
	    [ "$PLUGIN" == "0" ] && echo "* print Not flashing /data"
	    continue
	fi

	if [ "$image" == "system" -a "$NOSYSTEM" == "1" ];
	then
	    [ "$PLUGIN" == "0" ] && echo "* print Not flashing /system"
	    continue
	fi
	
	[ "$PLUGIN" == "0" ] && echo "* print Flashing /$image from $image.tar"	
	mount /$image 2>/dev/null

	
	if [ "$(mount | grep $image)" == "" ]; then
	    [ "$PLUGIN" == "0" ] && echo "* print error: unable to mount /$image"
	    quit 16
	fi

	if cat metadata/* | grep "^clobber_${image}=" | grep "true" > /dev/null; then
	    [ "$PLUGIN" == "0" ] && echo "* print wiping /${image}..."
	    rm -r /$image/* 2>/dev/null  
	fi

	cd /$image

	TAR_OPTS="x"
	[ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
	PTOTAL=$(tar xOf "$ROM_FILE" ./${image}.tar | tar t | wc -l)
	[ "$PLUGIN" == "0" ] && [ "$PROGRESS" == "1" ] && $ECHO "* print Extracting ${image}.tar"

	tar xOf "$ROM_FILE" ./${image}.tar | tar $TAR_OPTS | pipeline $PTOTAL
    done

    cd /tmp

    if [ -d post.d ]; then
	[ "$PLUGIN" == "0" ] && echo "* print Executing post-install scripts..."
	for sh in post.d/[0-9]*.sh; do
	    if [ -r "$sh" ]; then
		. "$sh"
	    fi
	done
    fi

    [ "$PLUGIN" == "0" ] && echo "* print Installed ROM from $ROM_FILE"
    umount /system 2>/dev/null
    umount /data 2>/dev/null
    quit 0
fi

if [ "$RESTORE" == 1 ]; then
    batteryAtLeast 20
    
    R_START=`date +%s`
    umount $ROOT_BACKUPDEVICE 2>/dev/null
    mount $ROOT_BACKUPDEVICE 2>/dev/null
	
	if [ "$(mount | grep $ROOT_BACKUPDEVICE)" == "" ]; then
	    echo "* print error: unable to mount $ROOT_BACKUPDEVICE"
	    quit 16
	fi


    RESTOREPATH=`ls -trd $BACKUPPATH/*$SUBNAME* 2>/dev/null | tail -1`

    if [ "$RESTOREPATH" = "" ]; then
		echo "* print Error: no backups found"
		quit 21
    else
        RECENT=`ls -lt $ROOT_BACKUPDEVICE/nandroid | head -n 1`
	BACKUPNAME=`basename $RESTOREPATH`
	CURRENT=`echo $RECENT | grep $BACKUPNAME | wc -l`
	if [ $CURRENT -gt 0 ]; then
	  LATEST=1
	else 
	  LATEST=0
	fi
	ls -trd $BACKUPPATH/*$SUBNAME* 2>/dev/null | grep -v $RESTOREPATH $OUTPUT

        if [ "$ASSUMEDEFAULTUSERINPUT" == 0 ]; then
            read SUBSTRING
        else
            SUBSTRING=""
        fi
	COMPRESSED=`ls -l $RESTOREPATH | grep ".gz" | wc -l`
	
	echo "* print "

        if [ ! "$SUBSTRING" == "" ]; then
            RESTOREPATH=`ls -trd $BACKUPPATH/*$SUBNAME* 2>/dev/null | grep $SUBSTRING | tail -1`
        else
            RESTOREPATH=`ls -trd $BACKUPPATH/*$SUBNAME* 2>/dev/null | tail -1`
        fi
        if [ "$RESTOREPATH" = "" ]; then
            echo "* print Error: no matching backups found, aborting"
            quit 22
        fi
    fi
    
    echo "* print Restoring $BACKUPNAME."

    if [ $LATEST == 1 ]; then
    	echo "* print (This backup is the latest.)"
    fi
    if [ $COMPRESSED -gt 0 ]; then
      echo "* print Backup is compressed. Please be patient."
    fi
    echo "* print "

    mount /system 2>/dev/null
    mount /data 2>/dev/null
    mount /cache 2>/dev/null
    [ ! -e /data/data ] && mkdir /data/data
    mount $DD_DEVICE /data/data 2>/dev/null
	[ ! -e /sd-ext ] && mkdir /sd-ext
	mount $EXT_DEVICE /sd-ext 2> /dev/null
    if [ -z "$(mount | grep data | grep -v "/data/data")" ]; then
		echo "* print error: unable to mount /data, aborting"	
		quit 23
    fi
    if [ -z "$(mount | grep system)" ]; then
		echo "* print error: unable to mount /system, aborting"	
		quit 24
    fi
    if [ -z "$(mount | grep cache)" ]; then
    		echo "*print error: unable to mount /cache, aborting"
		quit 25
    fi	
    
    CWD=$PWD
    cd $RESTOREPATH

    echo "* print Processing backup images:"

    if [ `ls boot* 2>/dev/null | wc -l` == 0 ]; then
        NOBOOT=1
    fi
    if [ `ls data* 2>/dev/null | wc -l` == 0 ]; then
        NODATA=1
    fi
    if [ `ls cache* 2>/dev/null | wc -l` == 0 ]; then
    	NOCACHE=1
    fi
    if [ `ls sdext* 2>/dev/null | wc -l` == 0 ]; then
    	NOSDEXT=1
    fi
    if [ `ls system* 2>/dev/null | wc -l` == 0 ]; then
        NOSYSTEM=1
    fi
	if [ `ls secure* 2>/dev/null | wc -l` == 0 ]; then
        NOSECURE=1
    fi

    for image in boot; do
        if [ "$NOBOOT" == "1" -a "$image" == "boot" ]; then
            echo "* print "
            echo "* print Not flashing boot image!"
            echo "* print "
            continue
        fi
        echo "* print Flashing $image..."
	[ "$PROGRESS" == "1" ] && echo "* show_indeterminate_progress"
	flash_image $image $image.img $OUTPUT
	if [ $? -ne 0 ]; then
	  dd if=$RESTOREPATH/$image.img of=$BOOT_DEVICE
	fi
	echo " done."
    done

    for image in system data cache secure sdext; do
        if [ "$NODATA" == 1 -a "$image" == "data" ]; then
            echo "* print "
            echo "* print Not restoring data image!"
            echo "* print "
            continue
        elif [ "$NODATA" == 0 -a "$image" == "data" ]; then
		image="data"	
		tarfile="data"
	fi
        if [ "$NOSYSTEM" == 1 -a "$image" == "system" ]; then
            echo "* print "
            echo "* print Not restoring system image!"
            echo "* print "
            continue
        elif [ "$NOSYSTEM" == 0 ] && [ "$image" == "system" ]; then
		image="system"	
		tarfile="system"
	fi
	if [ "$NOCACHE" == 1 -a "$image" == "cache" ]; then
	    echo "*print "
	    echo "* print Not restoring cache image!"
	    echo "* print "
	    continue
	elif [ "$NOCACHE" == 0 -a "$image" == "cache" ]; then
	    image="cache"
	    tarfile="cache"
	fi
        if [ "$NOSECURE" == 1 ] && [ "$image" == "secure" ]; then
            echo "* print "
            echo "* print Not restoring .android_secure!"
            echo "* print "
            continue
        elif [ "$NOSECURE" == 0 ] && [ "$image" == "secure" ]; then
			image="$ROOT_BACKUPDEVICE/.android_secure"	
			tarfile="secure"
	fi
        if [ "$NOSDEXT" == 1 ] && [ "$image" == "sdext" ]; then
            echo "* print "
            echo "* print Not restoring sd-ext!"
            echo "* print "
            continue
        elif [ "$NOSDEXT" == 0 ] && [ "$image" == "sdext" ]; then
			image="sd-ext"	
			tarfile="sdext"
	fi
	echo "* print Erasing /$image..."
	[ "$PROGRESS" == "1" ] && echo "* show_indeterminate_progress"
	if [ -d /data/media ] && [ "$image" == "data" ]; then
	  format /$image
	else 
	  rm -rf /$image/*
	fi
	
	TAR_OPTS="x"
	[ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
          COMPRESSED=`ls -l $RESTOREPATH | grep $image | grep "\.gz" | wc -l`
	if [ $COMPRESSED != 0 ]; then
	  TAR_OPTS="${TAR_OPTS}z"
	fi
	TAR_OPTS="${TAR_OPTS}f"
	if [ "$COMPRESSED" == "1" ]; then
	  PTOTAL=$(tar tzf $RESTOREPATH/$tarfile.tar.gz | wc -l)
	else
	  PTOTAL=$(tar tf $RESTOREPATH/$tarfile.tar | wc -l)
	fi  
	echo "* print Unpacking $image..."

	if [ "$COMPRESSED" == "1" ]; then
	  tar $TAR_OPTS $RESTOREPATH/$tarfile.tar.gz -C /$image | pipeline $PTOTAL
	else 
	  tar $TAR_OPTS $RESTOREPATH/$tarfile.tar -C /$image | pipeline $PTOTAL
	fi
	[ "$PROGRESS" == "1" ] && echo "* show_indeterminate_progress"
	if [ "$image" == "data" ]; then
	  if [ -e /data/misc/ril/pppd-notifier.fifo ]; then
	    rm /data/misc/ril/pppd-notifier.fifo
	  fi
	fi

	cd /
	sync
	if [ "$image" != "cache" ]; then
	  umount /$image 2> /dev/null
	fi
    done
    R_END=`date +%s`
    ELAPSED_SECS=$(( $R_END - $R_START))
    echo "* print Restore operation took $ELAPSED_SECS seconds."
    echo "* print Thanks for using RZRecovery."
    quit 0
fi


# 2.
if [ "$BACKUP" == 1 ]; then

    batteryAtLeast 20

    B_START=`date +%s`
    
    TAR_OPTS="c"
    if [ "$COMPRESS" == "1" ]; then
     TAR_OPTS="${TAR_OPTS}z"
     echo "* print Compression activated. Go make a sandwich."
     echo "* print This will take forever, but your"
     echo "* print SD Card will thank you :)"
     echo "* print "
    fi 
    [ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
    TAR_OPTS="${TAR_OPTS}f"
    
    echo "* print Mounting..."
	
   umount /system 2>/dev/null 
   umount /data 2>/dev/null
   umount $ROOT_BACKUPDEVICE 2>/dev/null
   mount /system 
   mount /data 
   mount $ROOT_BACKUPDEVICE 2> /dev/null 
   mkdir /data/data
   mount $DD_DEVICE /data/data 2>/dev/null
   mount $EXT_DEVICE /sd-ext 2>/dev/null
   if [ ! "$SUBNAME" == "" ]; then
     SUBNAME=$SUBNAME-
   fi

# Identify the backup with what partitions have been backed up
    if [ "$NOBOOT" == 0 ]; then
	BACKUPLEGEND=$BACKUPLEGEND"B"
    fi
    if [ "$NODATA" == 0 ]; then
	BACKUPLEGEND=$BACKUPLEGEND"D"
    fi
    if [ "$NOCACHE" == 0 ]; then
        BACKUPLEGEND=$BACKUPLEGEND"C"
    fi
    if [ "$NOSYSTEM" == 0 ]; then
	BACKUPLEGEND=$BACKUPLEGEND"S"
    fi
    if [ "$NOSECURE" == 0 ]; then
	BACKUPLEGEND=$BACKUPLEGEND"A"
    fi
    if [ "$NOSDEXT" == 0 ]; then
	BACKUPLEGEND=$BACKUPLEGEND"E"
    fi
    if [ ! "$BACKUPLEGEND" == "" ]; then
	BACKUPLEGEND=$BACKUPLEGEND-
    fi


    TIMESTAMP="`date +%Y%m%d-%H%M`"
    
    if [ -e /system/build.prop ]; then
      VERSION=`cat /system/build.prop | grep ro.build.display.id | cut -d '=' -f2 | sed 's/ /_/g;s/(/-/g;s/)/-/g'`
      if [ ! -z "$VERSION" ]; then
        SUBNAME="$VERSION"-"$SUBNAME"
      fi 
    fi
    
    DESTDIR="$BACKUPPATH/$SUBNAME$BACKUPLEGEND$TIMESTAMP"
    DESTDIRFM=`echo $DESTDIR | sed 's/\/\//\//g'`
    echo "* print Destination:"
    echo "* print $DESTDIRFM"
    echo "* print "
    if [ ! -d $DESTDIR ]; then 
	mkdir -p $DESTDIR
	if [ ! -d $DESTDIR ]; then 
	    echo "* print error: cannot create $DESTDIR"
		umount /system 2>/dev/null
		umount /data 2>/dev/null
		umount /data/data
		umount /sd-ext
		umount $ROOT_BACKUPDEVICE 2>/dev/null
	    quit 32
    else
		touch $DESTDIR/.nandroidwritable
		if [ ! -e $DESTDIR/.nandroidwritable ]; then
			echo "* print error: cannot write to $DESTDIR"
				umount /system 2>/dev/null
				umount /data 2>/dev/null
				umount $ROOT_BACKUPDEVICE 2>/dev/null
			quit 33
		fi
		rm $DESTDIR/.nandroidwritable
        fi
    fi

# 3.
    mount $ROOT_BACKUPDEVICE
    FREEBLOCKS=`df -m $ROOT_BACKUPDEVICE | grep $ROOT_BACKUPDEVICE | awk '{print$4}'`
    if [ ! -z `echo $FREEBLOCKS | grep %` ]; then #fs name must be too long
    	FREEBLOCKS=`df -m $ROOT_BACKUPDEVICE | grep $ROOT_BACKUPDEVICE | awk '{print$3}'`
    fi
    REQBLOCKS=0
    if [ $NODATA != 1 ]; then
	  mount data
      DATABLOCKS=`du -sm /data | awk '{print$1}'`
      MEDBLOCKS=`du -sm /data/media | awk '{print$1}'`
      [ -z "$MEDBLOCKS" ] && MEDBLOCKS=0
      let REQBLOCKS=$REQBLOCKS+$DATABLOCKS-$MEDBLOCKS
    fi  
    if [ $NOSYSTEM != 1 ]; then
	  mount system
      SYSBLOCKS=`du -sm /system | awk '{print$1}'`
      let REQBLOCKS=$REQBLOCKS+$SYSBLOCKS
    fi  
    if [ $NOSECURE != 1 ]; then
      SECBLOCKS=`du -sm $ROOT_BACKUPDEVICE/.android_secure | awk '{print$1}'`
      let REQBLOCKS=$REQBLOCKS+$SECBLOCKS
    fi
    if [ $NOCACHE != 1 ]; then
      CACHEBLOCKS=`du -sm /cache | awk '{print$1}'`
      let REQBLOCKS=$REQBLOCKS+$CACHEBLOCKS
    fi
    if [ $NOSDEXT != 1 ]; then
	  mount /sd-ext
      EXTBLOCKS=`du -sm /sd-ext | awk '{print$1}'`
      let REQBLOCKS=$REQBLOCKS+$EXTBLOCKS
    fi
    REQBLOCKSSTRING=`echo $REQBLOCKS | sed -e :a -e 's/\(.*[0-9]\)\([0-9]\{3\}\)/\1,\2/;ta'`
    FREEBLOCKSSTRING=`echo $FREEBLOCKS | sed -e :a -e 's/\(.*[0-9]\)\([0-9]\{3\}\)/\1,\2/;ta'`
    echo "* print $REQBLOCKSSTRING MB Required!"
    echo "* print $FREEBLOCKSSTRING MB Available!"
    if [ "$FREEBLOCKS" -le "$REQBLOCKS" ]; then
	echo "* print Error: not enough free space available on $ROOT_BACKUPDEVICE, aborting."
	umount /system 2>/dev/null
	umount /data 2>/dev/null
	rm -rf $DESTDIR
	umount $ROOT_BACKUPDEVICE 2>/dev/null
	quit 34
    fi
	


# 5.
    for image in boot; do
	case $image in
            boot)
		if [ "$NOBOOT" == 1 ]; then
                    echo "* print Dump of the boot partition suppressed."
                    continue
		fi
		;;
	esac
	echo "* print Dumping boot..."
	sleep 1s
	MD5RESULT=0
	ATTEMPT=0
	DEVICEMD5=`md5sum $BOOT_DEVICE | awk '{print$1}'`
	echo "DEVICEMD5: $DEVICEMD5"
	while [ $MD5RESULT -eq 0 ]; do
	    	let ATTEMPT=$ATTEMPT+1
		dump_image $image $DESTDIR/$image.img
		[ $? -ne 0 ] && dd if=$BOOT_DEVICE of=$DESTDIR/$image.img
	    	IMGMD5=`md5sum $DESTDIR/$image.img | awk '{print$1}'`
		echo "IMGMD5: $IMGMD5"
	    	echo -n "* print Verifying $image dump..."
	    	if [ "$IMGMD5" == "$DEVICEMD5" ]; then
			MD5RESULT=1
		else
			MD5RESULT=0
	    	fi
	   	if [ $ATTEMPT -eq 5 ]; then
			echo "* print Fatal error while trying to dump $image, aborting."
			umount /system 2>/dev/null
			umount /data 2>/dev/null
			umount $ROOT_BACKUPDEVICE 2>/dev/null
			quit 35
	    	fi
	done
	echo " complete!"
    done

# 6
    for image in system data cache secure sdext; do
	case $image in
            system)
		if [ "$NOSYSTEM" == 1 ]; then
                    echo "* print Dump of the system partition suppressed."
                    continue
		elif [ "$NOSYSTEM" == 0 ]; then
					dest="system"
					image="system"
		fi
		;;
            data)
		if [ "$NODATA" == 1 ]; then
                    echo "* print Dump of the data partition suppressed."
                    continue
		elif [ "$NODATA" != 1 ]; then
					dest="data"
					image="data"
		fi
		;;
	    cache)
	    	if [ "$NOCACHE" == 1 ]; then
		    echo "* print Dump of the cache partition suppressed."
		    continue
		elif [ "$NOCACHE" != 1 ]; then
		    dest="cache"
		    image="cache"
		fi
		;;
	    sdext)
	    	if [ "$NOSDEXT" == 1 ]; then
		    echo "* print Dump of the sd-ext partition suppressed."
		    continue
		elif [ "$NOSDEXT" != 1 ]; then
		    dest="sdext"
		    image="sd-ext"
		fi
		;;
            secure)
		if [ "$NOSECURE" == 1 ]; then
                    echo "* print Dump of .android_secure suppressed."
                    continue
		elif [ "$NOSECURE" != 1 ]; then
					image="$ROOT_BACKUPDEVICE/.android_secure"
					dest="secure"
		fi
		;;
	esac

	echo "* print Dumping $image... "

	cd /$image

	[ "$PROGRESS" == "1" ] && PTOTAL=$(find . | wc -l)
	[ "$image" == "data" ] && TAR_END="--exclude "./media""
	[ "$COMPRESS" == 0 ] && tar $TAR_OPTS $DESTDIR/$dest.tar . $TAR_END | pipeline $PTOTAL
	[ "$COMPRESS" == 1 ] && tar $TAR_OPTS $DESTDIR/$dest.tar.gz . $TAR_END| pipeline $PTOTAL
	sync
    done

    echo "* print Unmounting..."
			umount /system 2>/dev/null
			umount /data 2>/dev/null
			umount /data/data
			TOTALSIZE=`du -sm $DESTDIR | awk '{print$1}'`
			umount $ROOT_BACKUPDEVICE 2>/dev/null
			umount /boot 2>/dev/null
    echo "* print Backup successful."
    B_END=`date +%s`
    ELAPSED_SECS=$(( $B_END - $B_START ))
    [ "$PROGRESS" == "1" ] && echo "* reset_progress"
    echo "* print Backup operation took $ELAPSED_SECS seconds."
    echo "* print Total size of backup: $TOTALSIZE MB."
    echo "* print Thanks for using RZRecovery."
    quit 0
fi
