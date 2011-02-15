#!/sbin/sh

echo $*

##
## nandroid v2.2.1 ported to the Droid by SirPsychoS, modded by raidzero.  99% of credit or more goes to infernix, brainaid, and cyanogen!  My modifications are surrounded by lines starting with double-hashes, and my comment lines start with two hashes.
##


# nandroid v2.2.1 - an Android backup tool for the  by infernix and brainaid
# restore capability added by cyanogen

# pensive modified to allow to add prefixes to backups, and to restore specific backups
# pensive added the ability to exclude various images from the restore/backup operations, allows to preserve the newer
# recovery image if an older backup is being restored or to preserve user data. Also, saves space by not backing up
# partitions which change rarely.
# pensive added compressing backups and restoring compressed backups
# pensive added fetching system updates directly from the web into /sdcard/update.zip
# pensive added fetching system updates directly from the web into /cache and applying it.
# pensive added moving *update*.zip from /sdcard/download where a browser puts it to /sdcard/update.zip
# pensive added deletion of stale backups
# pensive added backup for ext2 partition on the sdcard to switch roms
# pensive added composite options --save NAME and --switchto NAME to switch ROMS
# pensive added list backup anywhere on the sdcard
# pensive added list updates (more precisely *.zip) anywhere on the sdcard

# Requirements:

# - a modded android in recovery mode (JF 1.3 will work by default)
# - adb shell as root in recovery mode if not using a pre-made recovery image
# - busybox in recovery mode
# - dump_image-arm-uclibc compiled and in path on phone
# - mkyaffs2image-arm-uclibc compiled and installed in path on phone
# - flash_image-arm-uclibc compiled and in path on phone
# - unyaffs-arm-uclibc compiled and in path on phone
# - for [de]compression needs gzip or bzip2, part of the busybox
# - wget for the wireless updates

##
## This reference data does not apply to the droid.  Corrected data is below.
##

# Reference data:

# dev:    size   erasesize  name
#mtd0: 00040000 00020000 "misc"
#mtd1: 00500000 00020000 "recovery"
#mtd2: 00280000 00020000 "boot"
#mtd3: 04380000 00020000 "system"
#mtd4: 04380000 00020000 "cache"
#mtd5: 04ac0000 00020000 "userdata"
#mtd6 is everything, dump splash1 with: dd if=/dev/mtd/mtd6ro of=/sdcard/splash1.img skip=19072 bs=2048 count=150


# Logical steps (v2.2.1):
#
# 0.  test for a target dir and the various tools needed, if not found then exit with error.
# 1.  check "adb devices" for a device in recovery mode. set DEVICEID variable to the device ID. abort when not found.
# 2.  mount system and data partitions read-only, set up adb portforward and create destdir
# 3.  check free space on /cache, exit if less blocks than 20MB free
# 4.  push required tools to device in /cache
# 5   for partitions boot recovery misc:
# 5a  get md5sum for content of current partition *on the device* (no data transfered)
# 5b  while MD5sum comparison is incorrect (always is the first time):
# 5b1 dump current partition to a netcat session
# 5b2 start local netcat to dump image to current dir
# 5b3 compare md5sums of dumped data with dump in current dir. if correct, contine, else restart the loop (6b1)
# 6   for partitions system data:
# 6a  get md5sum for tar of content of current partition *on the device* (no data transfered)
# 6b  while MD5sum comparison is incorrect (always is the first time):
# 6b1 tar current partition to a netcat session
# 6b2 start local netcat to dump tar to current dir
# 6b3 compare md5sums of dumped data with dump in current dir. if correct, contine, else restart the loop (6b1)
# 6c  if i'm running as root:
# 6c1 create a temp dir using either tempdir command or the deviceid in /tmp
# 6c2 extract tar to tempdir
# 6c3 invoke mkyaffs2image to create the img
# 6c4 clean up
# 7.  remove tools from device /cache
# 8.  umount system and data on device
# 9.  print success.


DEVICEID=foo
RECOVERY=foo

SUBNAME=""
NOBOOT=0
NODATA=0
NOSYSTEM=0
NOSECURE=0
NOSPLASH1=0
NOSPLASH2=0
EXT2=0

COMPRESS=0
GETUPDATE=0
RESTORE=0
BACKUP=0
DELETE=0
INSTALL_ROM=0
BUNDLE_ROM=0
WEBGET=0
LISTBACKUP=0
LISTUPDATE=0
AUTOREBOOT=0
AUTOAPPLY=0
ITSANUPDATE=0
ITSANIMAGE=0
WEBGETSOURCE=""
WEBGETTARGET="/sdcard"

DEFAULTUPDATEPATH="/sdcard/download"

DEFAULTWEBUPDATE=http://n0rp.chemlab.org/android/update-cm-3.6.8.1-signed.zip
# There really should be a clone link always pointing to the latest
#DEFAULTWEBUPDATE="http://n0rp.chemlab.org/android/latestupdate-signed.zip"
DEFAULTWEBIMAGE=http://n0rp.chemlab.org/android/cm-recovery-1.4-signed.zip

# Set up the default (for pensive at least) nameservers
NAMESERVER1=64.46.128.3
NAMESERVER2=64.46.128.4

# WiFi works, rmnet0 setup ???
# Do not know how to start the rmnet0 interface in recovery
# If in normal mode "ifconfig rmnet0 down" kills rild too
# /system/bin/rild& exits immediately, todo?


DEVICEID=.

# This is the default repository for backups
BACKUPPATH="/sdcard/nandroid/$DEVICEID"


# Boot, Cache, Data, Ext2, Misc, Recovery, System, Splash1, Splash2
# BACKUPLEGEND, If all the partitions are backed up it should be "CBDEMRS12"
# Enables the user to figure at a glance what is in the backup
BACKUPLEGEND=""

# Normally we want tar to be verbose for confidence building.
TARFLAGS="v"

DEFAULTCOMPRESSOR=gzip
DEFAULTEXT=.gz
DEFAULTLEVEL="-1"

ASSUMEDEFAULTUSERINPUT=0

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

for option in $(getopt --name="nandroid-mobile v2.2.1" -l progress -l bundle-rom: -l verbose -l install-rom: -l noboot -l nodata -l nosystem -l nosecure -l nosplash1 -l nosplash2 -l subname: -l backup -l restore -l compress -l getupdate -l delete -l path -l webget: -l webgettarget: -l nameserver: -l nameserver2: -l bzip2: -l defaultinput -l autoreboot -l autoapplyupdate -l ext2 -l save: -l switchto: -l listbackup -l listupdate -l silent -l quiet -l help -- "cbruds:p:eqli:" "$@"); do
    case $option in
        --silent)
            ECHO=echo2log
            ASSUMEDEFAULTUSERINPUT=1
            TARFLAGS=""
            OUTPUT=>>/cache/recovery/log
            shift
            ;;
        --quiet)
            ECHO=echo2log
            ASSUMEDEFAULTUSERINPUT=1
            TARFLAGS=""
            OUTPUT=>>/cache/recovery/log
            shift
            ;;
        -q)
            ECHO=echo2log
            ASSUMEDEFAULTUSERINPUT=1
            TARFLAGS=""
            OUTPUT=>>/cache/recovery/log
            shift
            ;;
	--uiprint)
	    ECHO='echo "ui_print "'
	    shift
	    ;;
        --help)
            ECHO=echo
            echo "* print Usage: $0 {--backup|--restore|--getupdate|--delete|--compress|--bzip2:ARG|--webget:URL|--listbackup|--listupdate} [options]"
            echo "* print "
            $ECHO "--help                     Display this help"
            echo "* print "
            $ECHO "-s | --subname: SUBSTRING  In case of --backup the SUBSTRING is"
            echo "* print                            the prefix used with backup name"
            echo "* print                            in case of --restore or -c|--compress|--bzip2 or"
            echo "* print                            --delete SUBSTRING specifies backups on which to"
            echo "* print                            operate"
            echo "* print "
            $ECHO "-u | --getupdate           Will search /sdcard/download for files named"
            echo "* print                            *update*.zip, will prompt the user"
            echo "* print                            to narrow the choice if more than one is found,"
            echo "* print                            and then move the latest, if not unique,"
            echo "* print                            to sdcard root /sdcard with the update.zip name"
            echo "* print                            It is assumed the browser was used to put the *.zip"
            echo "* print                            in the /sdcard/download folder. -p|--path option"
            echo "* print                            would override /sdcard/download with the path of your"
            echo "* print                            choosing."
            echo "* print "
            $ECHO "-p | --path DIR            Requires an ARGUMENT, which is the path to where "
            echo "* print                            the backups are stored, can be used"
            echo "* print                            when the default path /sdcard/nandroid/$DEVICEID "
            echo "* print                            needs to be changed"
            echo "* print "
            $ECHO "-b | --backup              Will store a full system backup on $BACKUPPATH"
            echo "* print                            One can suppress backup of any image however with options"
            echo "* print                            starting with --no[partionname]"
            echo "* print "
            $ECHO "-r | --restore             Will restore the last made backup which matches --subname"
            echo "* print                            ARGUMENT for boot, system, recovery and data"
            echo "* print                            unless suppressed by other options"
            echo "* print                            if no --subname is supplied and the user fails to"
            echo "* print                            provide any input then the latest backup is used"
            echo "* print                            When restoring compressed backups, the images will remain"
            echo "* print                            decompressed after the restore, you need to use -c|-compress"
            echo "* print                            or --bzip2 to compress the backup again"
            echo "* print "
            $ECHO "-d | --delete              Will remove backups whose names match --subname ARGUMENT"
            echo "* print                            Will allow to narrow down, will ask if the user is certain."
            echo "* print                            Removes one backup at a time, repeat to remove multiple backups"
            echo "* print "
            $ECHO "-c | --compress            Will operate on chosen backups to compress image files,"
            echo "* print                            resulting in saving of about 40MB out of 90+mb,"
            echo "* print                            i.e. up to 20 backups can be stored in 1 GIG, if this "
            echo "* print                            option is turned on with --backup, the resulting backup will"
            echo "* print                            be compressed, no effect if restoring since restore will"
            echo "* print                            automatically uncompress compressed images if space is available"
            echo "* print "
            $ECHO "--bzip2: -#                Turns on -c|--compress and uses bzip2 for compression instead"
            echo "* print                            of gzip, requires an ARG -[1-9] compression level"
            echo "* print "
            $ECHO "--noboot                   Will suppress restore/backup of the boot partition"
            echo "* print "
            $ECHO "--nodata                   Will suppress restore/backup of the data partition"
            echo "* print "
            $ECHO "--nosystem                 Will suppress restore/backup of the system partition"
            echo "* print "
            $ECHO "--defaultinput             Makes nandroid-mobile non-interactive, assumes default"
            echo "* print                            inputs from the user."
            echo "* print "
            $ECHO "--autoreboot               Automatically reboot the phone after a backup, especially"
            echo "* print                            useful when the compression options are on -c|--compress| "
            echo "* print                            --bzip2 -level since the compression op takes time and"
            echo "* print                            you may want to go to sleep or do something else, and"
            echo "* print                            when a backup is over you want the calls and mms coming"
            echo "* print                            through, right?"
            echo "* print "
            $ECHO "--autoapplyupdate          Once the specific update is downloaded or chosen from the"
            echo "* print                            sdcard, apply it immediately. This option is valid as a "
            echo "* print                            modifier for either --webget or --getupdate options."
            echo "* print "
            $ECHO "--save: ROMTAG             Preserve EVERYTHING under ROMTAG name, a composite option,"
            echo "* print                            it uses several other options."
            echo "* print "
            $ECHO "--switchto: ROMTAG         Restores your entire environment including app2sd apps, cache"
            echo "* print                            to the ROM environment named ROMTAG."
            echo "* print "
            $ECHO "-q|--quiet|--silent        Direct all the output to the recovery log, and assume default"
            echo "* print                            user inputs."
            echo "* print "
            $ECHO "-l|--listbackup            Will search the entire SDCARD for backup folders and will dump"
            echo "* print                            the list to stdout for use by the UI. Should be run with --silent"
            echo "* print "
            $ECHO "--listupdate               Will search the entire SDCARD for updates and will dump"
            echo "* print                            the list to stdout for use by the UI. Should be run with --silent"
	    echo "* print "
	    $ECHO "-i|--install-tar: FILE     Attempts to install the file specified as a ROM"
	    echo "* print "
	    $ECHO "--bundle-rom               Turns the current /system and /data into a ROM and prompts for some metadata"
	    echo "* print "
	    $ECHO "--verbose                  Enables verbose output from tar when making backups so that"
	    echo "* print                            hung files can be diagnosed and fixed"
            exit 0
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
        -r)
            RESTORE=1
            #$ECHO "restore"
            shift
            ;;
        --compress)
            COMPRESS=1
            #$ECHO "Compress"
            shift
            ;;
        -c)
            COMPRESS=1
            #$ECHO "Compress"
            shift
            ;;
        --bzip2)
            COMPRESS=1
            DEFAULTCOMPRESSOR=bzip2
            DEFAULTEXT=.bz2
            if [ "$2" == "$option" ]; then
                shift
            fi
            DEFAULTLEVEL="$2"
            shift
            ;;
        --getupdate)
            GETUPDATE=1
            shift
            ;;
        -u)
            GETUPDATE=1
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
	--bundle-rom)
	    if [ "$2" == "$option" ]; then
		shift
	    fi
	    BUNDLE_ROM=1
	    ROM_FILE=$2
	    ;;
	--progress)
	    PROGRESS=1
	    ;;
	-i)
	    if [ "$2" == "$option" ]; then
		shift
	    fi
	    INSTALL_ROM=1
	    ROM_FILE=$2
	    ;;
        --path)
            if [ "$2" == "$option" ]; then
                shift
            fi
            #$ECHO $2
            BACKUPPATH="$2"
            DEFAULTUPDATEPATH="$2"
            shift 2
            ;;
        -p)
            if [ "$2" == "$option" ]; then
                shift
            fi
            #$ECHO $2
            BACKUPPATH="$2"
            shift 2
            ;;
        --delete)
            DELETE=1
            shift
            ;;
        -d)
            DELETE=1
            shift
            ;;

        --defaultinput)
            ASSUMEDEFAULTUSERINPUT=1
            shift
            ;;
        --autoreboot)
            AUTOREBOOT=1
            shift
            ;;
        --autoapplyupdate)
            AUTOAPPLY=1
            shift
            ;;
        --save)
            if [ "$2" == "$option" ]; then
                shift
            fi
            SUBNAME="$2"
            BACKUP=1
            ASSUMEDEFAULTUSERINPUT=1
            EXT2=1
            COMPRESS=1
            shift
            ;;
        --switchto)
            if [ "$2" == "$option" ]; then
                shift
            fi
            SUBNAME="$2"
            RESTORE=1
            ASSUMEDEFAULTUSERINPUT=1
            EXT2=1
            shift
            ;;
        -l)
            shift
            LISTBACKUP=1
            ;;
        --listbackup)
            shift
            LISTBACKUP=1
            ;;
        --listupdate)
            shift
            LISTUPDATE=1
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

[ "$PROGRESS" == "1" ] && $ECHO "* show_indeterminate_progress"

pipeline() {
    if [ "$PROGRESS" == "1" ]; then
	$ECHO "* show_indeterminate_progress"
	awk "NR==1 {print \"* ptotal $1\"} {print \"* pcur \" NR}"
    else
	cat
    fi
}

let OPS=$BACKUP
let OPS=$OPS+$RESTORE
let OPS=$OPS+$DELETE
let OPS=$OPS+$WEBGET
let OPS=$OPS+$GETUPDATE
let OPS=$OPS+$LISTBACKUP
let OPS=$OPS+$LISTUPDATE
let OPS=$OPS+$INSTALL_ROM
let OPS=$OPS+$BUNDLE_ROM

if [ "$OPS" -ge 2 ]; then
    ECHO=echo
    $ECHO "--backup, --restore, --delete, --webget, --getupdate, --listbackup, --listupdate  are mutually exclusive operations."
    echo "* print Please, choose one and only one option!"
    echo "* print Aborting."
    exit 2
fi

let OPS=$OPS+$COMPRESS

if [ "$OPS" -lt 1 ]; then
    ECHO=echo
    echo "* print Usage: $0 {-b|--backup|-r|--restore|-d|--delete|-u|--getupdate|--webget|-c|--compress|--bzip2 -level|-l|--listbackup|--listupdate} [options]"
    echo "* print At least one operation must be defined, try $0 --help for more information."
    exit 0
fi


if [ "$LISTBACKUP" == 1 ]; then
    umount /sdcard 2>/dev/null
    mount /sdcard 2>/dev/null
    CHECK=`mount | grep /sdcard`
    if [ "$CHECK" == "" ]; then
        echo "Error: sdcard not mounted, aborting."
        exit 3
    else
        find /sdcard | sed s/.gz//g | sed s/.bz2//g 
        umount /sdcard 2>/dev/null
        exit 0
    fi
fi

if [ "$LISTUPDATE" == 1 ]; then
    umount /sdcard 2>/dev/null
    mount /sdcard 2>/dev/null
    CHECK=`mount | grep /sdcard`
    if [ "$CHECK" == "" ]; then
        echo "Error: sdcard not mounted, aborting."
        exit 4
    else
        find /sdcard | grep .zip
        umount /sdcard 2>/dev/null
        exit 0
    fi
fi

# Make sure it exists
touch /cache/recovery/log


if [ "$AUTOAPPLY" == 1 -a "$WEBGET" == 0 -a "$GETUPDATE" == 0 ]; then
    ECHO=echo
    echo "* print The --autoapplyupdate option is valid only in conjunction with --webget or --getupdate."
    echo "* print Aborting."
    exit 5
fi

if [ "$COMPRESS" == 1 ]; then
    echo "* print Compressing with $DEFAULTCOMPRESSOR, level $DEFAULTLEVEL"
fi

if [ "$WEBGET" == 1 ]; then
    echo "* print Fetching $WEBGETSOURCE to target folder: $WEBGETTARGET"
fi

if [ ! "$SUBNAME" == "" ]; then
    if [ "$BACKUP" == 1 ]; then
        if [ "$COMPRESS" == 1 ]; then
            echo "* print Using $SUBNAME- prefix to create a compressed backup folder"
        else
            echo "* print Using $SUBNAME- prefix to create a backup folder"
        fi
    else
        if [ "$RESTORE" == 1 -o "$DELETE" == 1 -o "$COMPRESS" == 1 ]; then
            echo "* print Searching for backup directories, matching $SUBNAME, to delete or restore"
            echo "* print or compress"
            echo "* print "
        fi
    fi
else
    if [ "$BACKUP" == 1 ]; then
        if [ "$ASSUMEDEFAULTUSERINPUT" == 0 ]; then
            read SUBNAME
        fi
        echo "* print "
        if [ "$COMPRESS" == 1 ]; then
            echo "* print Using $SUBNAME- prefix to create a compressed backup folder"
        else
            echo "* print Using $SUBNAME- prefix to create a backup folder"
        fi
        echo "* print "
    else
        if [ "$RESTORE" == 1 -o "$DELETE" == 1 -o "$COMPRESS" == 1 ]; then
            if [ "$ASSUMEDEFAULTUSERINPUT" == 0 ]; then
                read SUBNAME
            fi

        fi
    fi
fi

if [ "$BACKUP" == 1 ]; then
##
    if [ "$VERBOSE" == "1" ]
    then
        tar="busybox tar cvf"
    else
	tar="busybox tar cf"
    fi
##
    mkyaffs2image=`which mkyaffs2image`
    if [ "$mkyaffs2image" == "" ]; then
	mkyaffs2image=`which mkyaffs2image-arm-uclibc`
	if [ "$mkyaffs2image" == "" ]; then
	    echo "* print error: mkyaffs2image or mkyaffs2image-arm-uclibc not found in path"
	    exit 6
	fi
    fi
    dump_image=`which dump_image`
    if [ "$dump_image" == "" ]; then
	dump_image=`which dump_image-arm-uclibc`
	if [ "$dump_image" == "" ]; then
	    echo "* print error: dump_image or dump_image-arm-uclibc not found in path"
	    exit 7
	fi
    fi
fi

if [ "$RESTORE" == 1 ]; then
##
    if [ "$VERBOSE" == "1" ]
    then
	untar="busybox tar xvf"
    else
        untar="busybox tar xf"
    fi
##
    flash_image=`which flash_image`
    if [ "$flash_image" == "" ]; then
	flash_image=`which flash_image-arm-uclibc`
	if [ "$flash_image" == "" ]; then
	    echo "* print error: flash_image or flash_image-arm-uclibc not found in path"
	    exit 8
	fi
    fi
    unyaffs=`which unyaffs`
    if [ "$unyaffs" == "" ]; then
	unyaffs=`which unyaffs-arm-uclibc`
	if [ "$unyaffs" == "" ]; then
	    echo "* print error: unyaffs or unyaffs-arm-uclibc not found in path"
	    exit 9
	fi
    fi
fi
if [ "$COMPRESS" == 1 ]; then
    compressor=`busybox | grep $DEFAULTCOMPRESSOR`
    if [ "$compressor" == "" ]; then
        echo "* print Warning: busybox/$DEFAULTCOMPRESSOR is not found"
        echo "* print No compression operations will be performed"
        COMPRESS=0
    else
        echo "* print Found $DEFAULTCOMPRESSOR, will compress the backup"
    fi
fi

# 1
DEVICEID=`cat /proc/cmdline | sed "s/.*serialno=//" | cut -d" " -f1`
RECOVERY=`cat /proc/cmdline | grep "androidboot.mode=recovery"`
if [ "$RECOVERY" == "foo" ]; then
    echo "* print Error: Must be in recovery mode, aborting"
    exit 10
fi
if [ "$DEVICEID" == "foo" ]; then
    echo "* print Error: device id not found in /proc/cmdline, aborting"
    exit 11
fi
if [ ! "`id -u 2>/dev/null`" == "0" ]; then
    if [ "`whoami 2>&1 | grep 'uid 0'`" == "" ]; then
	echo "* print Error: must run as root, aborting"
	exit 12
    fi
fi


## added install-rom
if [ "$INSTALL_ROM" == 1 ]; then
    batteryAtLeast 30

    $ECHO "* show_indeterminate_progress"

    echo "* print Installing ROM from $ROM_FILE..."

    umount /sdcard 2>/dev/null
    mount /sdcard 2>/dev/null

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

	$ECHO
	$ECHO "--==AUTHORS==--"
	$ECHO $(cat metadata/* | grep "author=" | $GETVAL)
	$ECHO "--==NAME==--"
	$ECHO $(cat metadata/* | grep "name=" | $GETVAL)
	$ECHO "--==DESCRIPTION==--"
	$ECHO $(cat metadata/* | grep "description=" | $GETVAL)
	$ECHO "--==HELP==--"
	$ECHO $(cat metadata/* | grep "help=" | $GETVAL)
	$ECHO "--==URL==--"
	$ECHO $(cat metadata/* | grep "url=" | $GETVAL)
	$ECHO "--==EMAIL==--"
	$ECHO $(cat metadata/* | grep "email=" | $GETVAL)
	$ECHO
    fi

    if [ -d pre.d ]; then
	echo "* print Executing pre-install scripts..."
	for sh in pre.d/[0-9]*.sh; do
	    if [ -r "$sh" ]; then
		. "$sh"
	    fi
	done
    fi

    for image in system data;
    do
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

if [ "$BUNDLE_ROM" == 1 ]; then
    echo "* print Packaging current system to $ROM_FILE."
    echo "* print This will create a ROM that installs your /system and wipes /data when installed"
    echo "* print To include your own metadata or pre- and post-install scripts, or to avoid wiping /data on install, please make the tar archive manually."
    
    umount /sdcard 2>/dev/null
    mount /sdcard 2>/dev/null
    mount /system

    TAR_OPTS="c"
    [ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
    TAR_OPTS="${TAR_OPTS}f"

    if [ -z "$(mount | grep sdcard)" ]; then
	echo "* print error: unable to mount /sdcard, aborting"
	exit 17
    fi

    if [ -z "$(mount | grep system)" ]; then
	echo "* print error: unable to mount /system, aborting"
	exit 18
    fi

    mkdir /tmp/rom
    cd /tmp/rom
    tar $TAR_OPTS data.tar . 2>/dev/null | pipeline # make an empty data.tar
    
    cd /system

    PROTAL=$(find . | wc -l)
    [ "$PROGRESS" == "1" ] && $ECHO "* print Creating system.tar..."

    tar $TAR_OPTS /tmp/rom/system.tar . | pipeline $PTOTAL
    
    cd /tmp/rom
    mkdir metadata
    echo "description=generated with nandroid-mobile.sh" >> metadata/info

    PTOTAL=$(find . | wc -l)
    tar $TAR_OPTS "$ROM_FILE" . | pipeline $PTOTAL
fi

if [ "$RESTORE" == 1 ]; then
    batteryAtLeast 30

    umount /sdcard 2>/dev/null
    mount /sdcard 2>/dev/null
    if [ "`mount | grep sdcard`" == "" ]; then
	echo "* print error: unable to mount /sdcard, aborting"
	exit 20
    fi



    RESTOREPATH=`ls -trd $BACKUPPATH/*$SUBNAME* 2>/dev/null | tail -1`
    echo "* print  "

    if [ "$RESTOREPATH" = "" ];
    then
	echo "* print Error: no backups found"
	exit 21
    else
        echo "* print Default backup is the latest: $RESTOREPATH"
        ls -trd $BACKUPPATH/*$SUBNAME* 2>/dev/null | grep -v $RESTOREPATH $OUTPUT
        echo "* print "

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
    
    echo "* print Restore path: $RESTOREPATH"
    echo "* print "

    umount /system /data 2>/dev/null
    mount /system 2>/dev/null
    mount /data 2>/dev/null
    if [ "`mount | grep data`" == "" ]; then
	echo "* print error: unable to mount /data, aborting"	
	exit 23
    fi
    if [ "`mount | grep system`" == "" ]; then
	echo "* print error: unable to mount /system, aborting"	
	exit 24
    fi
    
    CWD=$PWD
    cd $RESTOREPATH

    DEFAULTEXT=""
    if [ `ls *.bz2 2>/dev/null|wc -l` -ge 1 ]; then
        DEFAULTCOMPRESSOR=bzip2
        DEFAULTDECOMPRESSOR=bunzip2
        DEFAULTEXT=.bz2
    fi
    if [ `ls *.gz 2>/dev/null|wc -l` -ge 1 ]; then
        DEFAULTCOMPRESSOR=gzip
        DEFAULTDECOMPRESSOR=gunzip
        DEFAULTEXT=.gz
    fi



    if [ `ls *.bz2 2>/dev/null|wc -l` -ge 1 -o `ls *.gz 2>/dev/null|wc -l` -ge 1 ]; then
        echo "* print This backup is compressed with $DEFAULTCOMPRESSOR."

                    # Make sure that $DEFAULT[DE]COMPRESSOR exists
        if [ `busybox | grep $DEFAULTCOMPRESSOR | wc -l` -le 0 -a\
                            `busybox | grep $DEFAULTDECOMPRESSOR | wc -l` -le 0 ]; then

            echo "* print You do not have either the $DEFAULTCOMPRESSOR or the $DEFAULTDECOMPRESSOR"
            echo "* print to unpack this backup, cleaning up and aborting!"
            umount /system 2>/dev/null
            umount /data 2>/dev/null
            umount /sdcard 2>/dev/null
            exit 26
        fi
        echo "* print Checking free space /sdcard for the decompression operation."
        FREEBLOCKS="`df -k /sdcard| grep sdcard | awk '{ print $4 }'`"
                    # we need about 100MB for gzip to uncompress the files
        if [ $FREEBLOCKS -le 100000 ]; then
            echo "* print Error: not enough free space available on sdcard (need about 100mb)"
            echo "* print to perform restore from the compressed images, aborting."
            umount /system 2>/dev/null
            umount /data 2>/dev/null
            umount /sdcard 2>/dev/null
            exit 27
        fi
        echo "* print Decompressing images, please wait...."
        echo "* print "
                    # Starting from the largest while we still have more space to reduce
                    # space requirements
        $DEFAULTCOMPRESSOR -d `ls -S *$DEFAULTEXT`
        echo "* print Backup images decompressed"
        echo "* print "
    fi

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

    for image in boot; do
        if [ "$NOBOOT" == "1" -a "$image" == "boot" ]; then
            echo "* print "
            echo "* print Not flashing boot image!"
            echo "* print "
            continue
        fi
        echo "* print Flashing $image..."
	$flash_image $image $image.img $OUTPUT
    done

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
        elif [ "$NOSYSTEM" == 0 -a "$image" == "system" ]; then
			image="system"	
			tarfile="system"
		fi
        if [ "$NOSECURE" == 1 -a "$image" == "secure" ]; then
            echo "* print "
            echo "* print Not restoring .android_secure!"
            echo "* print "
            continue
        elif [ "$NOSECURE" == 0 -a "$image" == "secure" ]; then
			image="sdcard/.android_secure"	
			tarfile="secure"
		fi
	echo "* print Erasing /$image..."
	cd /$image
	rm -rf * 2>/dev/null

	TAR_OPTS="x"
	[ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
	TAR_OPTS="${TAR_OPTS}f"

	PTOTAL=$(tar tf $RESTOREPATH/$image.tar | wc -l)
	[ "$PROGRESS" == "1" ] && $ECHO "* print Unpacking $image image..."

	tar $TAR_OPTS $RESTOREPATH/$tarfile.tar -C /$image | pipeline $PTOTAL
	if [ "$image" == "data" ]
	then
	    rm /data/misc/ril/pppd-notifier.fifo
	fi

##
	cd /
	sync
	umount /$image 2> /dev/null
    done
    
    echo "* print Restore done"
    exit 0
fi


# 2.
if [ "$BACKUP" == 1 ]; then

    TAR_OPTS="c"
    [ "$PROGRESS" == "1" ] && TAR_OPTS="${TAR_OPTS}v"
    TAR_OPTS="${TAR_OPTS}f"

    if [ "$COMPRESS" == 1 ]; then
        ENERGY=`cat /sys/class/power_supply/battery/capacity`
        if [ "`cat /sys/class/power_supply/battery/status`" == "Charging" ]; then
            ENERGY=100
        fi
        if [ ! $ENERGY -ge 30 ]; then
            echo "* print Warning: Not enough battery power to perform compression."
            COMPRESS=0
            echo "* print Turning off compression option, you can compress the backup later"
            echo "* print with the compression options."
        fi
    fi

    echo "* print mounting system and data read-only, sdcard read-write"
    umount /system 2>/dev/null
    umount /data 2>/dev/null
    umount /sdcard 2>/dev/null
    mount /system #|| FAIL=1
    mount /data #|| FAIL=2
    mount /sdcard 2> /dev/null || mount /dev/block/mmcblk0p1 /sdcard 2> /dev/null #|| FAIL=3
    #case $FAIL in
	#1) echo "* print Error mounting system read-only"; umount /system /data /sdcard; exit 29;;
	#2) echo "* print Error mounting data read-only"; umount /system /data /sdcard; exit 30;;
	#3) echo "* print Error mounting sdcard read-write"; umount /system /data /sdcard; exit 31;;
    #esac

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
    if [ "$EXT2" == 1 ]; then
	BACKUPLEGEND=$BACKUPLEGEND"E"
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
    if [ ! -d $DESTDIR ]; then 
	mkdir -p $DESTDIR
	if [ ! -d $DESTDIR ]; then 
	    echo "* print error: cannot create $DESTDIR"
	    umount /system 2>/dev/null
	    umount /data 2>/dev/null
	    umount /sdcard 2>/dev/null
	    exit 32
	fi
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

# 3.
    echo "* print checking free space on sdcard"
    FREEBLOCKS="`df -k /sdcard| grep sdcard | awk '{ print $4 }'`"
# we need about 130MB for the dump
    if [ $FREEBLOCKS -le 130000 ]; then
	echo "* print Error: not enough free space available on sdcard (need 130mb), aborting."
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

	DEVICEMD5=`$dump_image $image - | md5sum | awk '{ print $1 }'`
	sleep 1s
	MD5RESULT=1
	# 5b
	echo "* print Dumping $image to $DESTDIR/$image.img..."
	ATTEMPT=0
	while [ $MD5RESULT -eq 1 ]; do
	    let ATTEMPT=$ATTEMPT+1
		# 5b1
	    $dump_image $image $DESTDIR/$image.img $OUTPUT
	    sync
		# 5b3
	    echo "${DEVICEMD5}  $DESTDIR/$image.img" | md5sum -c -s - $OUTPUT
	    if [ $? -eq 1 ]; then
		true
	    else
		MD5RESULT=0
	    fi
	    if [ "$ATTEMPT" == "5" ]; then
		echo "* print Fatal error while trying to dump $image, aborting."
		umount /system
		umount /data
		umount /sdcard
		exit 35
	    fi
	done
	echo "* print done."
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

	# 6a
	echo "* print Dumping $image to $DESTDIR/$image.tar..."
## Modified to use tar
##

	cd /$image

	PTOTAL=$(find . | wc -l)
	[ "$PROGRESS" == "1" ]

	tar $TAR_OPTS $DESTDIR/$dest.tar . 2>/dev/null | pipeline $PTOTAL
	

##
	sync
	echo "* print done"
    done


# 7b.
    if [ "$COMPRESS" == 1 ]; then
	echo "* print Compressing the backup, may take a bit of time, please wait..."
##
##	echo "* print checking free space on sdcard for the compression operation."
##
	FREEBLOCKS="`df -k /sdcard| grep sdcard | awk '{ print $4 }'`"
    # we need about 70MB for the intermediate storage needs
	if [ $FREEBLOCKS -le 70000 ]; then
	    echo "* print error: not enough free space available on sdcard for compression operation (need 70mb)"
            echo "* print leaving this backup uncompressed."
	else
        # we are already in $DESTDIR, start compression from the smallest files
        # to maximize space for the largest's compression, less likely to fail.
        # To decompress reverse the order.
            $DEFAULTCOMPRESSOR $DEFAULTLEVEL `ls -S -r * | grep -v ext2`
	fi
    fi

    cd $CWD
    echo "* print done"

# 8.
    echo "* print unmounting system, data and sdcard"
    umount /system
    umount /data
    umount /sdcard

# 9.
    echo "* print Backup successful."
    if [ "$AUTOREBOOT" == 1 ]; then
	reboot
    fi
    exit 0
fi



# -------------------------------------DELETION, COMPRESSION OF BACKUPS---------------------------------
if [ "$COMPRESS" == 1 -o "$DELETE" == 1 ]; then
    echo "* print Unmounting /system and /data to be on the safe side, mounting /sdcard read-write."
    umount /system 2>/dev/null
    umount /data 2>/dev/null
    umount /sdcard 2>/dev/null

    FAIL=0
    # Since we are in recovery, these file-system have to be mounted
    echo "* print Mounting /sdcard to look for backups."
    mount /sdcard || mount /dev/block/mmcblk0 /sdcard || FAIL=1

    if [ "$FAIL" == 1 ]; then
	echo "* print Error mounting /sdcard read-write, cleaning up..."; umount /system /data /sdcard; exit 36
    fi

    echo "* print The current size of /sdcard FAT32 filesystem is `du /sdcard | tail -1 | cut -f 1 -d '/'`Kb"
    echo "* print "

    # find the oldest backup, but show the user other options
    echo "* print Looking for the oldest backup to delete, newest to compress,"
    echo "* print will display all choices!"
    echo "* print "
    echo "* print Here are the backups you have picked within this repository $BACKUPPATH:"

    if [ "$DELETE" == 1 ]; then
        RESTOREPATH=`ls -td $BACKUPPATH/*$SUBNAME* 2>/dev/null | tail -1`
        ls -td $BACKUPPATH/*$SUBNAME* 2>/dev/null $OUTPUT
    else
        RESTOREPATH=`ls -trd $BACKUPPATH/*$SUBNAME* 2>/dev/null | tail -1`
        ls -trd $BACKUPPATH/*$SUBNAME* 2>/dev/null $OUTPUT
    fi
    echo "* print  "

    if [ "$RESTOREPATH" = "" ];	then
	echo "* print Error: no backups found"
	exit 37
    else
        if [ "$DELETE" == 1 ]; then
            echo "* print Default backup to delete is the oldest: $RESTOREPATH"
            echo "* print "
            echo "* print Other candidates for deletion are: "
            ls -td $BACKUPPATH/*$SUBNAME* 2>/dev/null | grep -v $RESTOREPATH $OUTPUT
        fi
        if [ "$COMPRESS" == 1 ]; then
            echo "* print Default backup to compress is the latest: $RESTOREPATH"
            echo "* print "
            echo "* print Other candidates for compression are: "
            ls -trd $BACKUPPATH/*$SUBNAME* 2>/dev/null | grep -v $RESTOREPATH $OUTPUT
        fi

        echo "* print "
        if [ "$ASSUMEDEFAULTUSERINPUT" == 0 ]; then
            read SUBSTRING
        else
            SUBSTRING=""
        fi

        if [ ! "$SUBSTRING" == "" ]; then
            RESTOREPATH=`ls -td $BACKUPPATH/*$SUBNAME* 2>/dev/null | grep $SUBSTRING | tail -1`
        else
            RESTOREPATH=`ls -td $BACKUPPATH/*$SUBNAME* 2>/dev/null | tail -1`
        fi
        if [ "$RESTOREPATH" = "" ]; then
            echo "* print Error: no matching backup found, aborting"
            exit 38
        fi
    fi
    
    if [ "$DELETE" == 1 ]; then
        echo "* print Deletion path: $RESTOREPATH"
        echo "* print "
        echo "* print WARNING: Deletion of a backup is an IRREVERSIBLE action!!!"
        $ECHO -n "Are you absolutely sure? {yes | YES | Yes | no | NO | No}: "
        if [ "$ASSUMEDEFAULTUSERINPUT" == 0 ]; then
            read ANSWER
        else
            ANSWER=yes
        fi
        echo "* print "
        if [ "$ANSWER" == "yes" -o "$ANSWER" == "YES" -o "$ANSWER" == "Yes" ]; then
            rm -rf $RESTOREPATH
            echo "* print "
            $ECHO "$RESTOREPATH has been permanently removed from your SDCARD."
            echo "* print Post deletion size of the /sdcard FAT32 filesystem is `du /sdcard | tail -1 | cut -f 1 -d '/'`Kb"
        else 
            if [ "$ANSWER" == "no" -o "$ANSWER" == "NO" -o "$ANSWER" == "No" ]; then
                echo "* print The chosen backup will NOT be removed."
            else 
                echo "* print Invalid answer: assuming NO."
            fi
        fi
    fi

    if [ "$COMPRESS" == 1 ]; then

        CWD=`pwd`
        cd $RESTOREPATH

        if [ `ls *.bz2 2>/dev/null|wc -l` -ge 1 -o `ls *.gz 2>/dev/null|wc -l` -ge 1 ]; then
            echo "* print This backup is already compressed, cleaning up, aborting..."
            cd $CWD
            umount /sdcard 2>/dev/null
            exit 0
        fi

        echo "* print checking free space on sdcard for the compression operation."
        FREEBLOCKS="`df -k /sdcard| grep sdcard | awk '{ print $4 }'`"
         # we need about 70MB for the intermediate storage needs
        if [ $FREEBLOCKS -le 70000 ]; then
            echo "* print Error: not enough free space available on sdcard for compression operation (need 70mb)"
            echo "* print leaving this backup uncompressed."
        else
             # we are already in $DESTDIR, start compression from the smallest files
             # to maximize space for the largest's compression, less likely to fail.
             # To decompress reverse the order.
            echo "* print Pre compression size of the /sdcard FAT32 filesystem is `du /sdcard | tail -1 | cut -f 1 -d '/'`Kb"
            echo "* print "
            echo "* print Compressing the backup may take a bit of time, please wait..."
            $DEFAULTCOMPRESSOR $DEFAULTLEVEL `ls -S -r *`
            echo "* print "
            echo "* print Post compression size of the /sdcard FAT32 filesystem is `du /sdcard | tail -1 | cut -f 1 -d '/'`Kb"
        fi
    fi

    echo "* print Cleaning up."
    cd $CWD
    umount /sdcard 2>/dev/null
    exit 0

fi

if [ "$GETUPDATE" == 1 ]; then
    echo "* print Unmounting /system and /data to be on the safe side, mounting /sdcard read-write."
    umount /system 2>/dev/null
    umount /data 2>/dev/null
    umount /sdcard 2>/dev/null

    FAIL=0
    # Since we are in recovery, these file-system have to be mounted
    echo "* print Mounting /sdcard to look for updates to flash."
    mount /sdcard || mount /dev/block/mmcblk0 /sdcard || FAIL=1

    if [ "$FAIL" == 1 ]; then
	echo "* print Error mounting /sdcard read-write, cleaning up..."; umount /system /data /sdcard; exit 39
    fi

    echo "* print The current size of /sdcard FAT32 filesystem is `du /sdcard | tail -1 | cut -f 1 -d '/'`Kb"
    echo "* print "

    # find all the files with update in them, but show the user other options
    echo "* print Looking for all *update*.zip candidate files to flash."
    echo "* print "
    echo "* print Here are the updates limited by the subname $SUBNAME found"
    echo "* print within the repository $DEFAULTUPDATEPATH:"
    echo "* print "
    RESTOREPATH=`ls -trd $DEFAULTUPDATEPATH/*$SUBNAME*.zip 2>/dev/null | grep update | tail -1`
    if [ "$RESTOREPATH" == "" ]; then
        echo "* print Error: found no matching updates, cleaning up, aborting..."
        umount /sdcard 2>/dev/null
        exit 40
    fi
    ls -trd $DEFAULTUPDATEPATH/*$SUBNAME*.zip 2>/dev/null | grep update $OUTPUT
    echo "* print "
    echo "* print The default update is the latest $RESTOREPATH"
    echo "* print "
    if [ "$ASSUMEDEFAULTUSERINPUT" == 0 ]; then
        read SUBSTRING
    else
        SUBSTRING=""
    fi
    echo "* print "

    if [ ! "$SUBSTRING" == "" ]; then
        RESTOREPATH=`ls -trd $DEFAULTUPDATEPATH/*$SUBNAME*.zip 2>/dev/null | grep update | grep $SUBSTRING | tail -1`
    else
        RESTOREPATH=`ls -trd $DEFAULTUPDATEPATH/*$SUBNAME*.zip 2>/dev/null | grep update | tail -1`
    fi
    if [ "$RESTOREPATH" = "" ]; then
        echo "* print Error: no matching backups found, aborting"
        exit 41
    fi

    if [ "$RESTOREPATH" == "/sdcard/update.zip" ]; then
        echo "* print You chose update.zip, it is ready for flashing, there nothing to do."
    else

        # Things seem ok so far.

        # Move the previous update aside, if things go badly with the new update, it is good
        # have the last one still around :-)

        # If we cannot figure out what the file name used to be, create this new one with a time stamp
        OLDNAME="OLD-update-`date +%Y%m%d-%H%M`"

        if [ -e /sdcard/update.zip ]; then
            echo "* print There is already an update.zip in /sdcard, backing it up to"
            if [ -e /sdcard/update.name ]; then
                OLDNAME=`cat /sdcard/update.name`
                # Backup the name file (presumably contains the old name of the update.zip
                mv -f /sdcard/update.name /sdcard/`basename $OLDNAME .zip`.name
            fi
            $ECHO "`basename $OLDNAME .zip`.zip"
            mv -f /sdcard/update.zip /sdcard/`basename $OLDNAME .zip`.zip

            # Backup the MD5sum file
            if [ -e /sdcard/update.MD5sum ]; then
                mv -f /sdcard/update.MD5sum /sdcard/`basename $OLDNAME .zip`.MD5sum
            fi
        fi

        if [ -e $DEFAULTUPDATEPATH/`basename $RESTOREPATH .zip`.MD5sum ]; then
            mv -f $DEFAULTUPDATEPATH/`basename $RESTOREPATH .zip`.MD5sum /sdcard/update.MD5sum
        else
            $ECHO `md5sum $RESTOREPATH | tee /sdcard/update.MD5sum`
            echo "* print "
            echo "* print MD5sum has been stored in /sdcard/update.MD5sum"
            echo "* print "
        fi
        if [ -e $DEFAULTUPDATEPATH/`basename $RESTOREPATH .zip`.name ]; then
            mv -f $DEFAULTUPDATEPATH/`basename $RESTOREPATH .zip`.name /sdcard/update.name
        else
            echo "`basename $RESTOREPATH`" > /sdcard/update.name
        fi

        mv -i $RESTOREPATH /sdcard/update.zip


        echo "* print Your file $RESTOREPATH has been moved to the root of sdcard, and is ready for flashing!!!"

    fi

    echo "* print You may want to execute 'reboot recovery' and then choose the update option to flash the update."
    echo "* print Or in the alternative, shutdown your phone with reboot -p, and then press <CAMERA>+<POWER> to"
    echo "* print initiate a standard update procedure if you have stock SPL."
    echo "* print "
    echo "* print Cleaning up and exiting."
    umount /sdcard 2>/dev/null
    exit 0
fi
