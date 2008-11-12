#!/bin/sh

#set -x

# suggested host and target IP values
HOSTIP=$( hostname -i )
TARGETIP=""
OUTPUT=""

install(){

  if ! ping -c 1 -i 2 $TARGETIP >/dev/null 2>&1 ; then
      echo "Networking failed. Please check network connection and settings" 
      return 1
  fi

  ROOTFS_TARGET_SH_PATH=`dirname $0`
  if [ ! -f $ROOTFS_TARGET_SH_PATH/update-rootfs-target.sh ];then
    echo "Missing $ROOTFS_TARGET_SH_PATH/update-rootfs-target.sh"
    return 1
  fi
  scp $ROOTFS_TARGET_SH_PATH/update-rootfs-target.sh root@$TARGETIP:/tmp/
  # assume that  no input is required by this script
  ssh -T -l root $TARGETIP "/tmp/update-rootfs-target.sh -hostip $HOSTIP -rootfs $IMAGEDIR" < /dev/null
  # connection is closed by remote host giving 255 due to reboot command in script - so check to see
  # if we get a ssh return value other than 0 or 255 and report it - the if will overwrite it so copy
  # copy it first
  SSH_RETURN=$?
  if [ $SSH_RETURN -ne "0" ] && [ $SSH_RETURN -ne "255" ]; then
    echo "ssh reports this error value from script: $SSH_RETURN"
    return 1
  else
    echo "Update Appears Succesful - You Might Want To Check The"
    echo "Serial Console Of The Device To See It Reboots Properly"
  fi

  return 0;
}

help() {
    echo ""
    echo "FORMAT: $0 -targetip targetip <options>"
    echo 
    echo "Options:"
    echo " -h          display command help"
    echo " -q          quiet mode" 
    echo " -hostip     <host ip address>"
    echo " -targetip   <target ip address>"
    echo " -rootfs     <rootfs image dir>"
    echo
    echo " Usage: -targetip is required either as IP or DNS name "
    echo "        -hostip is used where the rootfs dir, is being shared via an IP other than given by: hostname -i"
    echo "        -rootfs is used to specify the fullpath to the files (your /etc/exports entry needs to match)"
    echo "               if not given then the script will check the current dir for a rootfs dir and mount it."
}


while [ -n "$1" ]; do
  case $1 in
    -h | -help | --help | -?)
        help
        exit 
        ;;

    -q | -quiet | --quiet) 
        VERBOSE=0
        OUTPUT="/dev/null"
        ;;
    
    -hostip | --hostip)
        shift
        HOSTIP=$1
        ;;

    -targetip | --targetip)
        shift
        TARGETIP=$1
        ;;

    -rootfs | --rootfs)
    	shift
	IMAGEDIR=$1
	ROOTFS=1
	;;
	
    *)
      echo $1: unknown argument
      ;;
  esac
  shift
done

if [ -z $IMAGEDIR ]; then
    if [ -d $PWD/rootfs ]; then
        IMAGEDIR=$PWD/rootfs
    #elif [  ]  #add logic for teamtest here
    #       IMAGEDIR=$DISTDIR/$model/rootfs  #give path here
    else
        echo "Error: rootfs image dir not found, please specify using -rootfs"
	exit 1
    fi
else
    if [ ! -d $IMAGEDIR ]; then
    	echo "Error: Specified image dir does not appear to exist"
	exit 1
    fi
fi

install

exit $?
