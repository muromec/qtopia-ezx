#!/bin/sh

#set -x

# suggested host and target IP values
HOSTIP=$( hostname -i )
TARGETIP="192.168.8.2"
OUTPUT=""
REBOOT=0

if [ -n "$QPEDIR" ]; then
    IMAGEDIR=$QPEDIR/image
else
    IMAGEDIR=$PWD/image
fi


install(){

  if ! ping -c 1 -i 2 $TARGETIP >/dev/null 2>&1 ; then
      echo "Networking failed. Please check network connection and settings"
      return 1
  fi

  if [ ! -f $IMAGEDIR/bin/qpe ]; then
    echo "Missing $IMAGEDIR/bin/qpe, check \$QPEDIR, see also -qpedir option"
    return 1
  fi

  # cd $IMAGEDIR
  QTOPIA_DEPOT_PATH=`dirname $0`
  if [ ! -f $QTOPIA_DEPOT_PATH/update-qtopia-target.sh ];then
    echo "Missing $QTOPIA_DEPOT_PATH/update-qtopia-target.sh"
    return 1
  fi
  scp $QTOPIA_DEPOT_PATH/update-qtopia-target.sh root@$TARGETIP:/tmp/
  # assume that  no input is required by this script and
  #    ensure that ssh execution completes as expected
  if [ $REBOOT -eq "1" ]; then
    ssh -T -l root $TARGETIP "/tmp/update-qtopia-target.sh -hostip $HOSTIP -imagedir $IMAGEDIR -reboot" < /dev/null
    SSH_RETURN=$?
    if [ $SSH_RETURN -ne "0" ] && [ $SSH_RETURN -ne "255" ]; then
      echo "Failed to update target device"
      return 1
    fi  	
  else
    ssh -T -l root $TARGETIP "/tmp/update-qtopia-target.sh -hostip $HOSTIP -imagedir $IMAGEDIR" < /dev/null
    if [ $? -ne "0" ]; then
      echo "Failed to update target device"
      return 1
    fi
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
    echo " -qpedir     <Qtopia build directory>, see also \$QPEDIR" 
    echo " -hostip     <host ip address>"
    echo " -targetip   <target ip address>"
    echo " -reboot     <reboot device rather than restart qtopia>"
    echo
    echo " Usage: -targetip is required either as IP or DNS name "
    echo "        -hostip is used where the Qtopia image dir, is being shared via an IP other than given by: hostname -i"
    echo "        -qpedir is used to specify the path to the direcotry called image - if not specified the script"
    echo "               defaults to current path in \$QPEDIR, if defined, or current path of current direcotry"
    echo "        -reboot causes device to reboot after Qtopia is updated, rather than just restarting Qtopia"
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

    -qpedir | --qpedir)
        shift
        IMAGEDIR=$1/image
        ;;
    
    -hostip | --hostip)
        shift
        HOSTIP=$1
        ;;

    -targetip | --targetip)
        shift
        TARGETIP=$1
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

install

exit $?
