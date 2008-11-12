#!/bin/sh
# this is a flash creating script for Neo 1973
# It requires a working root filesystem which can either be downloaded
# or built using openembedded.

DEVICE=ficgta01
# 
if [ -z $QPEVER ]; then
	QPEVER=`cat $QTOPIA_DEPOT_PATH/src/libraries/qtopiabase/version.h | grep QPE_VERSION |awk '{print $3}'|sed 's/"//g'`

	#	exit 1
fi

BUILDDIR=`pwd`

function make_flash
{
#make jffs flash image system

		rm -f neo-rootfs.tgz
		if ! wget http://qtopiaweb.trolltech.com.au/dist/input/qtopia/neo/neo-rootfs.tgz; then
				echo "wget failed to get rootfs"
				exit 1
		fi

		if [ -e rootfs ]; then 
				rm -rf rootfs
		fi

				mkdir rootfs 
				echo `pwd`
				echo "Unpack rootfs"
				sudo tar -C rootfs -xpzf neo-rootfs.tgz
        rm neo-rootfs.tgz
   
				if [ ! -e rootfs/etc/init.d/qpe ]; then
						cp $QTOPIA_DEPOT_PATH/devices/$DEVICE/src/devtools/startup/qpe.sh rootfs/etc/init.d/qpe
						chmod 775 rootfs/etc/init.d/qpe
				fi


		if [ -e rootfs/opt/Qtopia ]; then
				rm -rf rootfs/opt/Qtopia
		fi

    mkdir -p rootfs/opt/Qtopia

		if [ ! -e /opt/Qtopia/SDK/${QPEVER}/$DEVICE/image ]; then
    cp -a /opt/Qtopia/SDK/${QPEVER}/$DEVICE/image/* rootfs/opt/Qtopia
    create_flash
else

    cp -a image/* rootfs/opt/Qtopia
    create_flash
fi
echo " make_flash DONE"

}

function create_flash()
{
		echo "Creating jffs2 file"
    LASTDATETIME=`/bin/date -u +%m%d%H%M`
		sudo mkfs.jffs2 --pad=0x700000 -o qtopia-neo-flash.jffs2 -e 0x4000 -n -drootfs
		if !  sudo rm -rf rootfs; then
				echo "removing rootfs failed"
				exit 1
		fi

}

function create_update()
{
		if ! tar -cpzf $BUILDDIR/qtopia-$DEVICE-update.tar.gz *; then
				echo "creating update failed"
				cd $BUILDDIR
				exit 1
		fi
}

function make_update 
{
echo "Creating update file "$QPEVER
if [ -e /opt/Qtopia/SDK/${QPEVER}/$DEVICE/image ]; then
		cd  /opt/Qtopia/SDK/${QPEVER}/$DEVICE/image
		create_update
else
		cd image
		create_update
fi
cd $BUILDDIR

}


# make_flash
make_update

echo "make flash DONE!"
