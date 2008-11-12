#!/bin/ash

# This script is typically used to start Qtopia outside of inittab. Ideally both Qtopia and syslogd 
#  a run as respawn'd processes by inittab

export QPEDIR=/opt/Qtopia
if [ ! -f "$QPEDIR/qpe.env" ]; then
    logger -t 'Qtopia' "Error: Missing $QPEDIR/qpe.env, unable to start Qtopia with $0"
    exit 1
fi
OLDQPEDIR=$QPEDIR
. $QPEDIR/qpe.env
if [ "$QPEDIR" != "$OLDQPEDIR" ]; then
   logger -t 'Qtopia' "Error: qpe.env and qpe.sh have different values for \$QPEDIR"
   exit 1
fi

KILLPROGS="syslogd qpe quicklauncher qss mediaplayer"

touch /tmp/restart-qtopia
while [ -e /tmp/restart-qtopia ]; do
    killall $KILLPROGS 2>/dev/null

    # load startup flags from conf file
    # typically the startup flags mechanism is useful for development
    #  purposes only
    eval `startupflags.sh`

    if [ "$PHONEDUMMY_FLAG" = 1 ]; then
        export QTOPIA_PHONE_DUMMY=1
    else
        export QTOPIA_PHONE_DUMMY=0
    fi

    if [ "$PHONEDEVICE_FLAG" != "" ]; then
        export QTOPIA_PHONE_DEVICE=$PHONEDEVICE_FLAG
    fi

    if [ "$PHONEBOUNCE_FLAG" = 1 ] ; then
        phonebounce $QTOPIA_PHONE_DEVICE 12345 &
        export QTOPIA_PHONE_DUMMY=1
    fi

    if [ ! -d $HOME/.helix ]; then
        mkdir -p $HOME/.helix
    fi
    if [ -f $HOME/.helix/HelixSDK_10_0 ]; then
        if [  "`cat $HOME/.helix/HelixSDK_10_0|grep -c volume`" = 0 ] ; then
            echo "volume=95">>$HOME/.helix/HelixSDK_10_0
        fi
    fi

    # Running ipcrm is not required but a good idea
    # The two awk commands are needed because there may or may not be a space
    # separating the items in the first two columns if the ipcs output.
    if [ -n `which ipcs`  -a -n `which ipcrm` ]; then
        for i in `ipcs -m | grep root | awk '{print $2}' | awk '{print $1}'`
            do ipcrm -m $i;
        done
        for j in `ipcs -s | grep root | awk '{print $2}' | awk '{print $1}'`
            do ipcrm -s $j;
        done
    else
        logger -t "Qtopia" "Information: Did not find the ipcs command, skipping ipcs related cleanup"
    fi

    # There are several options for syslogd
    #   1) file based logging only 
    #   2) circular logging (required by the Logging application)
    #   3) remote logging and circular logging
    #   4) remote, file based and circular logging 
    # Change the value of QTOPIA_LOGGING to suit the device
    QTOPIA_LOGGING=2
    case $QTOPIA_LOGGING in
        1) # file based logging only
            /sbin/syslogd -n -S -s 1024 -b 5 -O /tmp/qtopia.log &
          ;;

        2) # circular logging, recommended on commercial devices
            sbin/syslogd -n -S -s 1024 -b 5 -C 512 &
          ;;
    
        3) # remote logging and circular logging
           # hint add "hostpc" to /etc/hosts
            syslogd -n -S -C 512 -L -s 1024 -b 5 -R hostpc &
          ;;

        4) #remote, file based and circular logging 
           # hints: 
           #  *  add "hostpc" to /etc/hosts
           #  *  ensure busybox is patched with
           #     devices/greenphone/rootfs/busybox/dual-local-logging.patch
            syslogd -n -S -C 512 -L -s 1024 -b 5 -O /tmp/qtopia.log -R hostpc &
          ;;

        *) logger -t "Qtopia" "Unknown value of \$QTOPIA_LOGGING of $QTOPIA_LOGGING, defaulting to circular logging" 
            sbin/syslogd -n -S -s 1024 -b 5 -C 512 &
    esac

    # start Qtopia
    qpe 2>&1 | logger -t 'Qtopia'
done


