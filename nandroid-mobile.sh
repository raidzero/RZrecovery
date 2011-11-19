#!/sbin/sh
source /ui_commands.sh
export PATH=/sbin:$PATH
SUBNAME=""
NOBOOT=0
NODATA=0
NOSYSTEM=0
NOSECURE=0
NOCACHE=0
RESTORE=0
BACKUP=0
COMPRESS=0
INSTALL_ROM=0

BACKUPPATH="/sdcard/nandroid"


# Boot, Data, System, Android-secure
# Enables the user to figure at a glance what is in the backup
BACKUPLEGEND=""

# Normally we want tar to be verbose for confidence building.
TARFLAGS="v"

ASSUMEDEFAULTUSERINPUT=0

sd_mounted=`mount | grep "/sdcard" | wc -l`
data_mounted=`mount | grep "/data" | wc -l`
system_mounted=`mount | grep "/system" | wc -l`
cache_mounted=`mount | grep /cache" | wc -l`
	
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
	echo "* print Error: not enough battery power, need at least $REQUIREDLEVEL%."
	echo "* print Connect charger or USB power and try again"
	exit 1
    fi
}

if [ "`echo $0 | grep /sbin/nandroid-mobile.sh`" == "" ]; then
    cp $0 /sbin
    chmod 755 /sbin/`basename $0`
    exec /sbin/`basename $0` $@
fi


DD_DEVICE=`cat /etc/fstab | grep "/datadata" | awk '{print$1}'`

if [ -e /proc/mtd ]; then
	flashfile="/proc/mtd"
fi

if [ -e /proc/emmc ]; then
	flashfile="/proc/emmc"	
fi

if [ ! -z "$flashfile" ]; then
	bootP=`cat $flashfile | grep \"boot\" | awk '{print $1}' | sed 's/://g'`

	if [ ! -z `echo $bootP | grep mtd` ]; then
		blkDevice="/dev/mtd/$bootP"
	fi
	
	if [ ! -z `echo $bootP | grep mmc` ]; then
		blkDevice="/dev/block/$bootP"
	fi
fi	

bootP=`cat /etc/recovery.fstab | grep boot  | grep -v bootloader | awk '{print$2}'`
if [ "$bootP" -eq "vfat" ]; then
	bootIsMountable="true" #boot is not raw
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

for option in $(getopt --name="nandroid-mobile v2.2.1" -l progress -l install-rom: -l noboot -l nodata -l nocache -l nosystem -l nosecure -l subname: -l backup -l compress -l restore -l defaultinput -- "cbruds:p:eqli:" "$@"); do
    case $option in
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
	--progress)
	    PROGRESS=1
	    ;;
        --)
            shift
            break
            ;;
    esac
done

echo "* print "
echo "* print nandroid-mobile v2.2.1 (RZ Hack)"
echo "* print "
##

[ "$PROGRESS" == "1" ] && echo "* show_indeterminate_progress"

pipeline() {
    if [ "$PROGRESS" == "1" ]; then
	echo "* show_indeterminate_progress"
	awk "NR==1 {print \"* ptotal $1\"} {print \"* pcur \" NR}"
    else
	cat
    fi
}

# Make sure it exists
touch /cache/recovery/log

if [ ! "$SUBNAME" == "" ]; then
   if [ "$RESTORE" == 1 ]; then
       echo "* print "
   fi
else
    if [ "$BACKUP" == 1 ]; then
        if [ "$ASSUMEDEFAULTUSERINPUT" == 0 ]; then
            read SUBNAME
        fi
        echo "* print "
    else
        if [ "$RESTORE" == 1 -o "$DELETE" == 1 ]; then
            if [ "$ASSUMEDEFAULTUSERINPUT" == 0 ]; then
                read SUBNAME
            fi

        fi
    fi
fi

if [ "$INSTALL_ROM" == 1 ]; then
	
    batteryAtLeast 20

    $ECHO "* show_indeterminate_progress"

    echo "* print Installing ROM from $ROM_FILE..."
    mount /sdcard

    if [ -z "$(mount | grep sdcard)" ]; then
	echo "* print error: unable to mount /sdcard, aborting"
	exit 13
    fi
    
    if [ ! -f $ROM_FILE ]; then
	echo "* print error: specified ROM file does not exist"
	exit 14
    fi

    if echo "$ROM_FILE" | grep ".*gz" >/dev/null;
    then
		echo "* print Decompressing $ROM_FILE..."
		busybox gzip -d "$ROM_FILE"
		ROM_FILE=$(echo "$ROM_FILE" | sed -e 's/.tar.gz$/.tar/' -e 's/.tgz/.tar/')
		echo "* print Decompressed to $ROM_FILE"
    fi

    cd /tmp
    TAR_OPTS="x"
    [ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
    TAR_OPTS="${TAR_OPTS}f"
    
    [ "$PROGRESS" == "1" ] && $ECHO "* print Unpacking $ROM_FILE..."
    PTOTAL=$(($(tar tf "$ROM_FILE" | wc -l)-2))
	echo "* print $PTOTAL files in archive!"
	echo "* print "
    
    tar $TAR_OPTS "$ROM_FILE" --exclude ./system.tar --exclude ./data.tar | pipeline $PTOTAL

    if [ -z $(tar tf "$ROM_FILE" | grep \./data\.tar) ]; then
	NODATA=1
    fi

    if [ -z $(tar tf "$ROM_FILE" | grep \./system\.tar) ]; then
	NOSYSTEM=1
    fi

    GETVAL=sed\ -r\ 's/.*=(.*)/\1/'

    if [ -d metadata ]; then
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
		echo "* print Executing pre-install scripts..."
		for sh in pre.d/[0-9]*.sh; do
			if [ -r "$sh" ]; then
			. "$sh"
			fi
		done
    fi

    for image in system data; do
	if [ "$image" == "data" -a "$NODATA" == "1" ];
	then
	    echo "* print Not flashing /data"
	    continue
	fi

	if [ "$image" == "system" -a "$NOSYSTEM" == "1" ];
	then
	    echo "* print Not flashing /system"
	    continue
	fi
	
	echo "* print Flashing /$image from $image.tar"	
	mount /$image 2>/dev/null

	
	if [ "$(mount | grep $image)" == "" ]; then
	    echo "* print error: unable to mount /$image"
	    exit 16
	fi

	if cat metadata/* | grep "^clobber_${image}=" | grep "true" > /dev/null; then
	    echo "* print wiping /${image}..."
	    rm -r /$image/* 2>/dev/null  
	fi

	cd /$image

	TAR_OPTS="x"
	[ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
	PTOTAL=$(tar xOf "$ROM_FILE" ./${image}.tar | tar t | wc -l)
	[ "$PROGRESS" == "1" ] && $ECHO "* print Extracting ${image}.tar"

	tar xOf "$ROM_FILE" ./${image}.tar | tar $TAR_OPTS | pipeline $PTOTAL
    done

    cd /tmp

    if [ -d post.d ]; then
	echo "* print Executing post-install scripts..."
	for sh in post.d/[0-9]*.sh; do
	    if [ -r "$sh" ]; then
		. "$sh"
	    fi
	done
    fi

    echo "* print Installed ROM from $ROM_FILE"
    umount /system 2>/dev/null
    umount /data 2>/dev/null
    exit 0
fi

if [ "$RESTORE" == 1 ]; then
    R_START=`date +%s`
    batteryAtLeast 30
    umount /sdcard 2>/dev/null
    mount /sdcard 2>/dev/null
	
	if [ "$(mount | grep sdcard)" == "" ]; then
	    echo "* print error: unable to mount /sdcard"
	    exit 16
	fi



    RESTOREPATH=`ls -trd $BACKUPPATH/*$SUBNAME* 2>/dev/null | tail -1`

    if [ "$RESTOREPATH" = "" ]; then
		echo "* print Error: no backups found"
		exit 21
    else
        RECENT=`ls -lt /sdcard/nandroid | head -n 1`
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
            exit 22
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
    mkdir /data/data
    mount $DD_DEVICE /data/data 2>/dev/null
    if [ ! -z "$bootIsMountable" ]; then
    	mount /boot 2>/dev/null
    fi
    if [ -z "$(mount | grep data | grep -v "/data/data")" ]; then
		echo "* print error: unable to mount /data, aborting"	
		exit 23
    fi
    if [ -z "$(mount | grep system)" ]; then
		echo "* print error: unable to mount /system, aborting"	
		exit 24
    fi
    if [ -z "$(mount | grep cache)" ]; then
    		echo "*print error: unable to mount /cache, aborting"
		exit 25
    fi	
    
    CWD=$PWD
    cd $RESTOREPATH

    echo "* print Processing backup images..."

    if [ `ls boot* 2>/dev/null | wc -l` == 0 ]; then
        NOBOOT=1
    fi
    if [ `ls data* 2>/dev/null | wc -l` == 0 ]; then
        NODATA=1
    fi
    if [ `ls cache* 2>/dev/null | wc -l` == 0 ]; then
    	NOCACHE=1
    fi
    if [ `ls system* 2>/dev/null | wc -l` == 0 ]; then
        NOSYSTEM=1
    fi
	if [ `ls secure* 2>/dev/null | wc -l` == 0 ]; then
        NOSECURE=1
    fi

    if [ -z "$bootIsMountable" ]; then 
    	for image in boot; do
        	if [ "$NOBOOT" == "1" -a "$image" == "boot" ]; then
            	echo "* print "
            	echo "* print Not flashing boot image!"
            	echo "* print "
            	continue
        	fi
        	echo "* print Flashing $image..."
			echo "* show_indeterminate_progress"
			flash_image $image $image.img $OUTPUT
			echo " done."
    	done
    else 
	echo "* print Erasing /boot..."
	cd /boot
	rm -rf * 2> /dev/null
        TAR_OPTS="x"
	[ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
	TAR_OPTS="${TAR_OPTS}f"
	PTOTAL=$(tar tf $RESTOREPATH/boot.tar | wc -l)
        [ "$PROGRESS" == "1" ] && $ECHO "* print Unpacking boot..."
        tar $TAR_OPTS $RESTOREPATH/boot.tar -C /boot | pipeline $PTOTAL
	cd /
	sync
	umount boot
    fi

    for image in system data cache secure; do
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
			image="sdcard/.android_secure"	
			tarfile="secure"
	fi
	echo "* print Erasing /$image..."
	cd /$image
	echo "* show_indeterminate_progress"
	rm -rf *
	cd /
	
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
	[ "$PROGRESS" == "1" ] && $ECHO "* print Unpacking $image..."

	if [ "$COMPRESSED" == "1" ]; then
	  tar $TAR_OPTS $RESTOREPATH/$tarfile.tar.gz -C /$image | pipeline $PTOTAL
	else 
	  tar $TAR_OPTS $RESTOREPATH/$tarfile.tar -C /$image | pipeline $PTOTAL
	fi
	echo "* show_indeterminate_progress"
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
    exit 0
fi


# 2.
if [ "$BACKUP" == 1 ]; then
    B_START=`date +%s`
    
    TAR_OPTS="c"
    if [ "$COMPRESS" == "1" ]; then
     TAR_OPTS="${TAR_OPTS}z"
     echo "* print Compression activated. Go make a sandwich."
     echo "* print This will take forever, but your"
     echo "* print SD Card will thank you :)"
     echo "* print "
    fi 
    if [ "$PROGRESS" == "1" ]; then 
      TAR_OPTS="${TAR_OPTS}v"
    fi
    
    TAR_OPTS="${TAR_OPTS}f"
    
    echo "* print Mounting..."
	
    if [ ! -z "$bootIsMountable" ]; then 
      umount /boot 2>/dev/null
      mount /boot
   fi	
   umount /system 2>/dev/null 
   umount /data 2>/dev/null
   umount /sdcard 2>/dev/null
   mount /system 
   mount /data 
   mount /sdcard 2> /dev/null 
   mkdir /data/data
   mount $DD_DEVICE /data/data 2>/dev/null
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
    if [ ! "$BACKUPLEGEND" == "" ]; then
	BACKUPLEGEND=$BACKUPLEGEND-
    fi


    TIMESTAMP="`date +%Y%m%d-%H%M`"
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
		umount /sdcard 2>/dev/null
	    exit 32
    else
		touch $DESTDIR/.nandroidwritable
		if [ ! -e $DESTDIR/.nandroidwritable ]; then
			echo "* print error: cannot write to $DESTDIR"
				umount /system 2>/dev/null
				umount /data 2>/dev/null
				umount /sdcard 2>/dev/null
			exit 33
		fi
		rm $DESTDIR/.nandroidwritable
        fi
    fi

# 3.
    mount sdcard; mount data; mount system
    FREEBLOCKS=`df -m /sdcard | grep /sdcard | awk '{print$4}'`
    if [ ! -z `echo $FREEBLOCKS | grep %` ]; then #fs name must be too long
    	FREEBLOCKS=`df -m /sdcard | grep /sdcard | awk '{print$3}'`
    fi
    REQBLOCKS=0
    if [ $NODATA != 1 ]; then
      DATABLOCKS=`du -sm /data | awk '{print$1}'`
      let REQBLOCKS=$REQBLOCKS+$DATABLOCKS
    fi  
    if [ $NOSYSTEM != 1 ]; then
      SYSBLOCKS=`du -sm /system | awk '{print$1}'`
      let REQBLOCKS=$REQBLOCKS+$SYSBLOCKS
    fi  
    if [ $NOSECURE != 1 ]; then
      SECBLOCKS=`du -sm /sdcard/.android_secure | awk '{print$1}'`
      let REQBLOCKS=$REQBLOCKS+$SECBLOCKS
    fi
    if [ $NOCACHE != 1 ]; then
      CACHEBLOCKS=`du -sm /cache | awk '{print$1}'`
      let REQBLOCKS=$REQBLOCKS+$CACHEBLOCKS
    fi
    REQBLOCKSSTRING=`echo $REQBLOCKS | sed -e :a -e 's/\(.*[0-9]\)\([0-9]\{3\}\)/\1,\2/;ta'`
    FREEBLOCKSSTRING=`echo $FREEBLOCKS | sed -e :a -e 's/\(.*[0-9]\)\([0-9]\{3\}\)/\1,\2/;ta'`
    echo "* print $REQBLOCKSSTRING MB Required!"
    echo "* print $FREEBLOCKSSTRING MB Available!"
    if [ "$FREEBLOCKS" -le "$REQBLOCKS" ]; then
	echo "* print Error: not enough free space available on sdcard, aborting."
	umount /system 2>/dev/null
	umount /data 2>/dev/null
	rm -rf $DESTDIR
	umount /sdcard 2>/dev/null
	exit 34
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
	
	if [ ! -z "$bootIsMountable" ]; then
		cd /boot
		PTOTAL=$(find . | wc -l)
	 	[ "$PROGRESS" == "1" ] tar $TAR_OPTS $DESTDIR/boot.tar . 2>/dev/null | pipeline $PTOTAL

		cd /
		sync
	else
		sleep 1s
		MD5RESULT=1
		ATTEMPT=0
		DEVICEMD5=`dump_image $image - | md5sum | awk '{print$1}'`
		while [ $MD5RESULT -eq 1 ]; do
	    		let ATTEMPT=$ATTEMPT+1
	    		dump_image $image $DESTDIR/$image.img
	    		sync
	    		echo -n "* print Verifying $image dump..."
	    		IMGMD5=`md5sum $DESTDIR/$image.img | awk '{print$1}'`
	    	if [ "$IMGMD5" -eq "$DEVICEMD5" ]; then
			MD5RESULT=1
		else
			MD5RESULT=0
	    	fi
	   	 if [ "$ATTEMPT" == "5" ]; then
			echo "* print Fatal error while trying to dump $image, aborting."
			umount /system 2>/dev/null
			umount /data 2>/dev/null
			umount /sdcard 2>/dev/null
			exit 35
	    	fi
		done
	fi	
	echo " complete!"
    done

# 6
    for image in system data cache secure; do
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
            secure)
		if [ "$NOSECURE" == 1 ]; then
                    echo "* print Dump of .android_secure suppressed."
                    continue
		elif [ "$NOSECURE" != 1 ]; then
					image="/sdcard/.android_secure"
					dest="secure"
		fi
		;;
	esac

	echo "* print Dumping $image... "

	cd /$image

	PTOTAL=$(find . | wc -l)
	[ "$COMPRESS" == 0 ] && tar $TAR_OPTS $DESTDIR/$dest.tar . 2>/dev/null | pipeline $PTOTAL
	[ "$COMPRESS" == 1 ] && tar $TAR_OPTS $DESTDIR/$dest.tar.gz . | pipeline $PTOTAL
	sync
    done

    echo "* print Unmounting..."
			umount /system 2>/dev/null
			umount /data 2>/dev/null
			umount /data/data
			TOTALSIZE=`du -sm $DESTDIR | awk '{print$1}'`
			umount /sdcard 2>/dev/null
			umount /boot 2>/dev/null
    echo "* print Backup successful."
    B_END=`date +%s`
    ELAPSED_SECS=$(( $B_END - $B_START ))
    echo "* reset_progress"
    echo "* print Backup operation took $ELAPSED_SECS seconds."
    echo "* print Total size of backup: $TOTALSIZE MB."
    echo "* print Thanks for using RZRecovery."
    exit 0
fi
