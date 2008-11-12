#!/bin/sh

# This script is for only running on the Marvel Zylonite which
#   has a read+write / 

# set -x

help() {
    echo ""
    echo "FORMAT: $0 <options>"
    echo 
    echo "Options:"
    echo " -h          display command help"
    echo " -q          quiet mode" 
    echo " -hostip     <host ip address>"
    echo " -rootfs     <rootfs image directory>"
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
    
    -hostip | --hostip)
        shift
        HOSTIP=$1
        ;;
    
    -rootfs | --rootfs)
    	shift
	IMAGEDIR=$1
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

killall qpe.sh syslogd qpe quicklauncher qss mediaplayer udevd telnetd udhcpc > /dev/null 2>&1
echo "Progress: Removing old Qtopia image - for Qtopia version changes."
rm -rf /opt
echo "Progress: Copying new rootfs image"
cp -af /tmp/mount/* /
if [ $? -ne "0" ]; then
    echo "Error: Unable to copy image file onto the target device"
    exit 1
#else #the else had been to check it got into the if statement - as no indication was given
      #after the copy command and before reboot - probably due files being overwritten
      #this proved the script was still running though.
#    echo "Update Good" > /success
fi
    
umount /tmp/mount
if [ $? -eq "0" ]; then
    echo "Success: Update complete, Rebooting Littleton"
    echo "Please Observe Device's Serial Console To See It Came Up Properly"
    reboot
else
    echo "Error: Update rootfs failed"
fi

exit $?
