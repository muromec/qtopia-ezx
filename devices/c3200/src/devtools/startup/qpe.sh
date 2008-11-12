#!/bin/ash

export QWS_MOUSE_PROTO="c3200mousehandler"
export QWS_KEYBOARD="c3200kbdhandler"
export QWS_SIZE="240x320"
export QTOPIA_PHONE_DUMMY=1

KILLPROGS="qpe quicklauncher qss mediaplayer"

dbmigrate --systemupgrade

touch /tmp/restart-qtopia
while [ -e /tmp/restart-qtopia ]; do
    killall $KILLPROGS 2>/dev/null

    # load startup flags from conf file
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
        # Turn on the modem
        # apm --modem-poweron
        # apm --wakeup

        # Bring up networking
        # usbnet.sh up &

        phonebounce $QTOPIA_PHONE_DEVICE 12345 &
        #modem_keep_alive.sh &

        export QTOPIA_PHONE_DUMMY=1
    fi

    if [ "`cat $HOME/.helix/HelixSDK_10_0|grep -c volume`" = 0 ] ; then
        echo "volume=95">>$HOME/.helix/HelixSDK_10_0
    fi
    chvt 3
    clear>/dev/tty3
    echo -e '\033[?1;2;3c'>/dev/tty3
    export QWS_DISPLAY=Transformed:LinuxFb:mWidth75:mmHeight57:Rot270:0

    # clearipc - clear shared memory segments and semaphores

    if [ "`whoami`" == "root" ]; then
        us="0x";      # "0x" is the hex key value in every line
    else
        us="$USER";
    fi
    # The two awk commands are needed because there may or may not be a space
    # separating the items in the first two columns if the ipcs output.
    for i in `ipcs -m | grep $us | awk '{print $2}' | awk '{print $1}'`
        do ipcrm -m $i;
    done
    for j in `ipcs -s | grep $us | awk '{print $2}' | awk '{print $1}'`
        do ipcrm -s $j;
    done

    qpe 2>&1 | logger -t 'Qtopia'
    clear>/dev/tty3
    chvt 1
done


