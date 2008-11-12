#!/bin/sh
# 
#  qtopia start-up script for Nokia 770
# 
# echo '1' >> /proc/sys/kernel/printk
export LOGNAME=user
export HOME=/home/$LOGNAME
export QPEDIR=/opt/Qtopia
export QTDIR=$QPEDIR
export LD_LIBRARY_PATH=$QTDIR/lib
export PATH=$QPEDIR/bin:$PATH
export QTOPIA_PHONE_DUMMY=1
export SXE_DISCOVERY_MODE=1

export QWS_MOUSE_PROTO="tslib:/dev/input/event3"
export QWS_KEYBOARD="nokiakbdhandler"
export QWS_SIZE="800x480"

if [ -e /sys/devices/platform/gpio-switch/slide/state ]; then
# n810
		export QWS_DISPLAY=Transformed:nokia:mmWidth70:mmHeight40:Rot270:0
		export QWS_KEYBOARD="nokiakbdhandler"
else
#n800
		export QWS_DISPLAY=Transformed:nokia:mmWidth70:mmHeight40:Rot0:0
		export QWS_KEYBOARD="n800kbdhandler"
fi

killproc() {
        pid=`/bin/ps -e | /bin/sed -n -e '/\<'$1'\>/ s/^ *\([0-9][0-9]*\).*/\1/p'`
        [ "$pid" != "" ] && kill $pid
}

case $1 in
'start')
	echo "Starting QPE..."

	cd $HOME
	KILLPROGS="quicklauncher qpe mediaserverXomap sapwood-server matchbox-window-manager clipboard-manager gconfd-2 maemo-launcher"

	touch /tmp/restart-qtopia
	while [ -e /tmp/restart-qtopia ]; do

	killall $KILLPROGS 2>/dev/null

    /sbin/rmmod /mnt/initfs/lib/modules/2.6.21-omap1/g_file_storage.ko
    /sbin/insmod /mnt/initfs/lib/modules/2.6.21-omap1/g_ether.ko
    /sbin/ifup usb0

	echo "starting clock" > $HOME/log
	echo `date` >> $HOME/log
	qpe 2>>$HOME/log >>$HOME/log

	done

#	$QPEDIR/bin/qpe 2>/opt/log >/opt/log;
	;;
'stop')
        echo "Killing QPE..."
    	killall $KILLPROGS 2>/dev/null
	rm /tmp/restart-qtopia
        ;;
*)
	echo "Starting QPE..."

	KILLPROGS="quicklauncher qpe mediaserver"

     	killall $KILLPROGS 2>/dev/null
     	qpe 2>log >log
	;;
esac

