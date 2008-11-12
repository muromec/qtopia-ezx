#!/bin/sh

# This script is for onlu running on the Marvel Zylonite which
#   has a read+write /opt/Qtopia

# set -x
REBOOT=0

help() {
    echo ""
    echo "FORMAT: $0 <options>"
    echo 
    echo "Options:"
    echo " -h          display command help"
    echo " -q          quiet mode"
    echo " -imagedir   <Qtopia image directory>" 
    echo " -hostip     <host ip address>"
    echo " -reboot     <reboot instead of restart qtopia>"
}

while [ -n "$1" ]; do
  case $1 in
    -h | -help | --help | -?)
        help
        exit 1
        ;;

    -q | -quiet | --quiet) 
        VERBOSE=0
        OUTPUT="/dev/null"
        ;;

    -imagedir | --imagedir)
        shift
        IMAGEDIR=$1
        ;;
    
    -hostip | --hostip)
        shift
        HOSTIP=$1
        ;;
	
    -reboot | --reboot)
    	REBOOT=1
	;;
	
  *)
      echo $1: unknown argument
      ;;
  esac
  shift
done

if [ -z "$IMAGEDIR" -o -z "$HOSTIP" ]; then
    echo "Error: Missing arguments to $0"
    help
    exit 1
fi

if [ ! -d /tmp/mount ]; then
    mkdir /tmp/mount 
fi

mount -t nfs -o nolock,tcp,rsize=8192,wsize=8192 $HOSTIP:$IMAGEDIR /tmp/mount
if [ $? -ne "0" ]; then
    echo "Error: Unable to mount $HOSTIP:$IMAGEDIR on the target device"
    exit 1
fi

killall qpe.sh syslogd qpe quicklauncher qss mediaplayer > /dev/null 2>&1
echo "Progress: Removing old Qtopia image"
rm -r /opt/Qtopia
echo "Progress: Copying new Qtopia image"
cp -a /tmp/mount /opt/Qtopia
if [ $? -ne "0" ]; then
    echo "Error: Unable to copy image file onto the target device"
    exit 1
fi

umount /tmp/mount
if [ $? -eq "0" ]; then
    if [ $REBOOT -eq "1" ]; then
    	echo "Success: Update complete, rebooting the device"
    	reboot
    else
        echo "Success: Update complete, restarting qtopia"
	/opt/Qtopia/qpe.sh </dev/null > /dev/null 2>&1 &
	sleep 1
	exit 0
    fi
else
    echo "Error: Update Qtopia failed"
fi

exit $?
