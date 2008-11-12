#!/bin/ash

export QWS_MOUSE_PROTO="tslib"
export QWS_KEYBOARD="ficgta01kbdhandler"
export QTOPIA_PHONE_DEVICE="/dev/ttySAC0:115200"
export QPEDIR=/opt/Nokia/Qtopia
export PATH=$QPEDIR/bin:$PATH
export LD_LIBRARY_PATH=$QPEDIR/lib:$LD_LIBRARY_PATH

# export SXE_DISCOVERY_MODE=1
export QTOPIA_ZONEINFO_PATH=$QPEDIR/etc/zoneinfo
export QTOPIA_PHONE_VENDOR=ficgta01

export TSLIB_TSDEVICE=/dev/input/touchscreen0
# export TSLIB_TSDEVICE=/dev/input/event1
export TSLIB_CONFFILE=/etc/ts.conf

export USER=root
export HOME=/home/$USER

echo '1' > /proc/sys/kernel/printk

restore_default()
{
#    startupflags.sh RESTOREDEFAULTS_FLAG 0
    # Restart this script. It is possible that it was changed.
    exec $0
}

case $1 in
'start')

KILLPROGS="qpe quicklauncher mediaserver mediaplayer sipagent"
    kill $( pidof $KILLPROGS ) 2>/dev/null

touch /tmp/restart-qtopia
while [ -e /tmp/restart-qtopia ]; do

    # load startup flags from conf file
#    eval `startupflags.sh`

    # Restore default settings.
    if [ "$RESTOREDEFAULTS_FLAG" = 1 ]; then
        restore_default
    fi

    if [ "$PHONEDUMMY_FLAG" = 1 ]; then
        export QTOPIA_PHONE_DUMMY=1
    else
        export QTOPIA_PHONE_DUMMY=0
    fi

    if [ "$PHONEDEVICE_FLAG" != "" ]; then
        export QTOPIA_PHONE_DEVICE=$PHONEDEVICE_FLAG
    fi

    
    # clean up shared memory and semaphores
    # but not for resources created by syslogd
#    clearipc $(pidof syslogd)

#    chvol SYSTEM 100
#    chvol CALL 60

# power gsm on
    echo 1 > /sys/bus/platform/devices/neo1973-pm-gsm.0/power_on
    echo 1 > /sys/bus/platform/devices/neo1973-pm-bt.0/power_on

# fix for gta02 bt
if [ -e /sys/devices/platform/s3c2440-i2c ]; then
    echo 3300 > /sys/devices/platform/s3c2440-i2c/i2c-adapter/i2c-0/0-0073/voltage_ldo4
    echo 1 > /sys/bus/platform/drivers/neo1973-pm-bt/neo1973-pm-bt.0/reset
    echo 0 > /sys/bus/platform/drivers/neo1973-pm-bt/neo1973-pm-bt.0/reset
fi



	echo "starting clock" > $HOME/log
	echo `date` >> $HOME/log

chvt 3
#	qpe 2>>$HOME/log >>$HOME/log
    qpe 2>&1 | logger -t 'Qtopia'

 done

#gifanim $QTOPIA_TOOLS/splash-shutdown-exit.gif &

;;
'stop')

KILLPROGS="qpe quicklauncher mediaserver mediaplayer sipagent"

rm -f /tmp/restart-qtopia

kill $( pidof $KILLPROGS ) 2>/dev/null
;;

'restart')
KILLPROGS="qpe quicklauncher mediaserver mediaplayer sipagent"
kill $( pidof $KILLPROGS ) 2>/dev/null

;;

esac
