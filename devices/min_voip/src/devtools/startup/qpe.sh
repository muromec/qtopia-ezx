#!/bin/ash

QTOPIA_IMAGE_PATH=""
QTOPIA_TOOLS=/mnt/user/tools

# Set generic Greenphone Qtopia variables here.
# Set Qtopia image location dependent variables in
# the appropriate trolltech_*_startup.sh script or
# the appropriate trolltech_*_flash.sh script.
export QWS_MOUSE_PROTO="linuxtp"
export QWS_KEYBOARD="greenphonekbdhandler greenphonedetecthandler"
export QWS_SIZE="240x320"
export QTOPIA_PHONE_DEVICE="/dev/ttyS0:115200"

restore_default()
{
    echo "TROLL: Restoring system default"

    # Restore $HOME
    rm -rf /mnt/disk2/home/*

    # Restore writeable Qtopia filesystem
    rm -rf $QPEDIR/*
    if tar -xzf /opt/Qtopia.rom/qtopia_default.tgz -C /mnt/disk2/Qtopia; then
        md5sum /opt/Qtopia.rom/qtopia_default.tgz > /mnt/disk2/.qtopia_default.m
    else
        echo "TROLL: Error extracting qtopia_default.tgz"
    fi

    startupflags.sh RESTOREDEFAULTS_FLAG 0

    # Restart this script. It is possible that it was changed.
    exec $0
}

# Certain database access patterns in Qtopia 4.1 exposed a performance
# limitation in the disc-on-chip IO subsystem. This problem is evident from the
# kernel process "tffs1" consuming nearly 100% CPU usage during filesystem
# access. Qtopia 4.2 no longer triggers this issue. The following code removes
# the symlink magic that worked around the problem for Qtopia 4.1 builds.
if [ -L $HOME/Applications/Qtopia ]; then
    echo "TROLL: Disabling fast boot fix, not required for Qtopia 4.2+"

    rm -f $HOME/Applications/Qtopia

    if [ -d $HOME/Applications/.Qtopia ]; then
        mv $HOME/Applications/.Qtopia $HOME/Applications/Qtopia
    fi
fi


KILLPROGS="qpe quicklauncher qss mediaplayer sipagent phonebounce modem_keep_alive.sh"

touch /tmp/restart-qtopia
while [ -e /tmp/restart-qtopia ]; do
    kill $( pidof $KILLPROGS ) 2>/dev/null

    # load startup flags from conf file
    eval `startupflags.sh`

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

    if [ "$PHONEBOUNCE_FLAG" = 1 ] ; then
        # Turn on the modem
        apm --modem-poweron
        apm --wakeup

        # Bring up networking
        usbnet.sh up &

        phonebounce $QTOPIA_PHONE_DEVICE 12345 &
        modem_keep_alive.sh &

        export QTOPIA_PHONE_DUMMY=1
    fi

    clearipc

    chvol SYSTEM 100
    chvol CALL 60

    qpe 2>&1 | logger -p local5.notice -t 'Qtopia'
done

gifanim $QTOPIA_TOOLS/splash-shutdown-exit.gif &

