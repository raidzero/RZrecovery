#!/sbin/sh
set -o verbose
SUBNAME=""
NOBOOT=0
NODATA=0
NOSYSTEM=0
NOSECURE=0

RESTORE=0
BACKUP=0
INSTALL_ROM=0

BACKUPPATH="/sdcard/nandroid/"


# Boot, Data, System, Android-secure
# Enables the user to figure at a glance what is in the backup
BACKUPLEGEND=""

# Normally we want tar to be verbose for confidence building.
TARFLAGS="v"

ASSUMEDEFAULTUSERINPUT=0

sd_mounted=`mount | grep "/sdcard" | wc -l`
data_mounted=`mount | grep "/data" | wc -l`
system_mounted=`mount | grep "/system" | wc -l`

	
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

bootP=`cat /etc/recovery.fstab | grep boot | awk '{print$2}'`
if [ "$bootP" -eq "vfat" ]; then
	bootIsMountable=1 #boot is not raw
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

for option in $(getopt --name="nandroid-mobile v2.2.1" -l progress -l install-rom: -l noboot -l nodata -l nosystem -l nosecure -l subname: -l backup -l restore -l defaultinput -- "cbruds:p:eqli:" "$@"); do
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

if [ "$BACKUP" == 1 ]; then
    if [ "$VERBOSE" == "1" ]; then
        tar="busybox tar cvf"
    else
	tar="busybox tar cf"
    fi
fi

if [ "$RESTORE" == 1 ]; then
    if [ "$VERBOSE" == "1" ]; then
		untar="busybox tar xvf"
    else
        untar="busybox tar xf"
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
		$ECHO "* print Decompressing ROM..."
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
	
	[ "$HGREV" -ge "$MIN_REV" ] || ($ECHO "ERROR: your installed version of the recovery image is too old. Please upgrade."; exit 1) || exit 154

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
    datadata_present= `cat /etc/fstab | grep datadata | wc -l`
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
        LATEST=1
	ls -trd $BACKUPPATH/*$SUBNAME* 2>/dev/null | grep -v $RESTOREPATH $OUTPUT

        if [ "$ASSUMEDEFAULTUSERINPUT" == 0 ]; then
            read SUBSTRING
        else
            SUBSTRING=""
        fi
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
    
    RESTOREPATHSTRING=`echo $RESTOREPATH | sed 's/\/\//\//g'`
    echo "* print Restore path: $RESTOREPATHSTRING"

    if [ $LATEST == 1 ]; then
    	echo "* print is the latest."
    fi
    echo "* print "

    mount /system 2>/dev/null
    mount /data 2>/dev/null
    if [ ! -z "$bootIsMountable" ]; then
    	mount /boot 2>/dev/null
    fi
    if [ "$datadata" != "0" && -z "$(mount | grep datadata)" ]; then
	mount /datadata 2>/dev/null
    fi
    if [ -z "$(mount | grep datadata)" && "$datadata_present" != "0" ]; then
		echo "* print error: unable to mount /data/data, aborting"	
		exit 23
    fi    
    if [ -z "$(mount | grep data)" ]; then
		echo "* print error: unable to mount /data, aborting"	
		exit 23
    fi
    if [ -z "$(mount | grep system)" ]; then
		echo "* print error: unable to mount /system, aborting"	
		exit 24
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
    if [ `ls system* 2>/dev/null | wc -l` == 0 ]; then
        NOSYSTEM=1
    fi
	if [ `ls secure* 2>/dev/null | wc -l` == 0 ]; then
        NOSECURE=1
    fi

    if [ -z "$bootIsMountable"]; then 
    	for image in boot; do
        	if [ "$NOBOOT" == "1" -a "$image" == "boot" ]; then
            	echo "* print "
            	echo "* print Not flashing boot image!"
            	echo "* print "
            	continue
        	fi
        	echo -n "* print Flashing $image..."
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

    for image in data system secure; do
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
		if [ "$image" -eq "sdcard/.android_secure"] ; then
			cd /$image
			rm -rf * 2>/dev/null
		fi
		if [ "$image" -eq "system"] ; then
			format /$image
		fi
		if [ "$image" -eq "data"] ; then
			format /$image
		fi	
		
		TAR_OPTS="x"
		[ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
		TAR_OPTS="${TAR_OPTS}f"

		PTOTAL=$(tar tf $RESTOREPATH/$image.tar | wc -l)
		[ "$PROGRESS" == "1" ] && $ECHO "* print Unpacking $image..."

		tar $TAR_OPTS $RESTOREPATH/$tarfile.tar -C /$image | pipeline $PTOTAL
		if [ "$image" == "data" ]; then
			if [ -e /data/misc/ril/pppd-notifier.fifo ]; then
				rm /data/misc/ril/pppd-notifier.fifo
			fi
		fi

		cd /
		sync
		umount /$image 2> /dev/null

    done
    
    exit 0
    echo "* print Thanks for using RZRecovery."
fi


# 2.
if [ "$BACKUP" == 1 ]; then
    datadata_present= `cat /etc/fstab | grep datadata | wc -l`
	
    TAR_OPTS="c"
    [ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
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
	if [ "$datadata" != "0" ]; then
		mount /datadata 2>/dev/null
	fi
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
		umount /sdcard 2>/dev/null
	    exit 32
#	fi
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
    DATABLOCKS=`df -m /data | grep /data | awk '{print$3}'`
    SYSBLOCKS=`df -m /system | grep /system | awk '{print$3}'`
    SECBLOCKS=`du -sm /sdcard/.android_secure | awk '{print$1}'`
    REQBLOCKS=`expr $DATABLOCKS + $SYSBLOCKS + $SECBLOCKS`
    REQBLOCKSSTRING=`echo $REQBLOCKS | sed -e :a -e 's/\(.*[0-9]\)\([0-9]\{3\}\)/\1,\2/;ta'`
    FREEBLOCKSSTRING=`echo $FREEBLOCKS | sed -e :a -e 's/\(.*[0-9]\)\([0-9]\{3\}\)/\1,\2/;ta'`
    echo "* print $REQBLOCKSSTRING MB Required!"
    echo "* print $FREEBLOCKSSTRING MB Available!"
    if [ "$FREEBLOCKS" -le "$REQBLOCKS" ]; then
	echo "* print Error: not enough free space available on sdcard, aborting."
	umount /system 2>/dev/null
	umount /data 2>/dev/null
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
	
	if [ ! -z $"bootIsMountable" ]; then
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
    for image in system data secure; do
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

	echo "* print Dumping $image..."

	cd /$image

	PTOTAL=$(find . | wc -l)
	[ "$PROGRESS" == "1" ]

	tar $TAR_OPTS $DESTDIR/$dest.tar . 2>/dev/null | pipeline $PTOTAL
	
	sync
    done

    echo "* print Unmounting..."
			umount /system 2>/dev/null
			umount /data 2>/dev/null
			umount /sdcard 2>/dev/null
			umount /boot 2>/dev/null
    echo "* print Backup successful."
	echo "* print Thanks for using RZRecovery."
    if [ "$AUTOREBOOT" == 1 ]; then
	reboot
    fi
    exit
fi
