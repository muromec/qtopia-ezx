#!/bin/bash
#
# Build SDK greenphone Qtopia 4
#

if [ ! -e `pwd`/scripts/functions ] ; then
  echo "ERROR: This script must be run from the base directory of Qtopia source package" 
  exit
fi

. `pwd`/scripts/functions

output_error()
{
  make -k >/dev/null 2>&1
  make -j 1>>$LOGFILE 2>&1
  tail -n 20 $LOGFILE
  echo "For more information on this error look at $LOGFILE for details"
}

output_install_error()
{
  make install -k >/dev/null 2>&1
  make install -j 1>>$LOGFILE 2>&1
  tail -n 20 $LOGFILE
  echo "For more information on this error look at $LOGFILE for details"
}

skip_host_tools()
{
  if [ -e $QPEDIR/qtopiacore/host/bin/qmake ] ; then
    SKIP="-skip-dqt"
    if [ ! -e qtopiacore/host ] ; then
      ln -s ../qtopiacore/host qtopiacore/host
    fi
  else
    SKIP=""
  fi 
}

make_greenphone_sdk()
{
  if [ "$CLEAN" = "1" ] ;then  
      rm -Rf $QPEDIR/../greenphone-sdk
      mkdir -p $QPEDIR/../greenphone-sdk/qtopiacore
      if [ ! -e $QPEDIR/../greenphone-sdk/qtopiacore/qtopiacore/qt ] ; then
        ln -s $QPEDIR/qtopiacore/qt $QPEDIR/../greenphone-sdk/qtopiacore/qt
      fi
  else
    if [ ! -d $QPEDIR/../greenphone-sdk ]; then
        die" Cannot do a \"-no-clean\" build as $QPEDIR/../greenphone-sdk does not exist yet"
    fi
  fi
  cd $QPEDIR/../greenphone-sdk
  LOGFILE=`pwd`/build.log
  echo 0>$LOGFILE
  skip_host_tools
  echo "TIME:`date "+%H:%M:%S"` Configuring Qtopia for Greenphone"

  EXTRA_OPTIONS=""
  test "$DRM" = "1" && EXTRA_OPTIONS="-drm"
  test "$SXE" = "0" && EXTRA_OPTIONS=$EXTRA_OPTIONS" -no-sxe"


  if [ "$CLEAN" = "1" ]; then
      if $QPEDIR/configure -device greenphone -platform $QPEDIR/devices/gcc411/mkspecs/linux-g++ -sdk /opt/Qtopia/SDK/$QPEVER/greenphone -image /opt/Qtopia/SDK/$QPEVER/greenphone/image -extra-qt-config -no-glib -confirm-license $SKIP $EXTRA_OPTIONS >${OUTPUT} 2>&1; then
        echo "DONE"
      else
        echo "$0 FAILED in configure for greenphone SDK"
        if [ "$QUIET" = "1" ] ; then
            echo "Try running $0 without the --quiet option to get more details on why configure failed" 
        fi
        exit 1
      fi
  else
    if [ ! -f $PWD/config.status ]; then
        echo "$0 FAILED in configure for Greenphone SDK, \"-no-clean\" option selected but no build was found in $PWD"
        exit 1
    fi
  fi

  echo "TIME:`date "+%H:%M:%S"` Building..."

  make -k >>${OUTPUT} 2>&1

  if make >>${OUTPUT} 2>&1; then
    echo "DONE"
  else
    output_error
    echo "$0 FAILED in make for greenphone SDK"
    exit 1
  fi
  echo "TIME:`date "+%H:%M:%S"` Installing..."
  if make install >>${OUTPUT} 2>&1; then
    echo "DONE"
  else
    output_install_error
    echo "$0 FAILED in make install for greenphone SDK"
    exit 1
  fi

  # Remove some helix file formats
  cd /opt/Qtopia/SDK/$QPEVER/greenphone/image/lib/helix
  rm -f amrff.so
  rm -f amrn.so
  rm -f amrw.so
  rm -f d263.so
  rm -f h263render.so
  rm -f mp3fformat.so
  rm -f mp3render.so
  rm -f mp4arender.so
  rm -f mp4fformat.so

  rm -Rf /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/host
  cd /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore
  ln -s /opt/Qtopia/SDK/$QPEVER/x86/qtopiacore/host

  # Optimise the space requirements for the docs.
  # Most of the Qt docs exist in the Qtopia doc directory so we save about 55MB.
  cd /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/qt/doc/html
  rm -r images
  ln -s ../../../../doc/html/images .
  for file in *.html; do
      if [ "$file" != "*.html" ]; then
          if diff -q $file ../../../../doc/html/$file >/dev/null 2>&1; then
              rm $file
              ln -s ../../../../doc/html/$file .
          fi
      fi
  done
}

make_x86_sdk()
{
  if [ "$CLEAN" = "1" ] ;then  
      rm -Rf $QPEDIR/../x86-sdk
      mkdir -p $QPEDIR/../x86-sdk/qtopiacore
      if [ ! -e $QPEDIR/../x86-sdk/qtopiacore/qtopiacore/qt ] ; then
        ln -s $QPEDIR/qtopiacore/qt $QPEDIR/../x86-sdk/qtopiacore/qt
      fi
    else
    if [ ! -d $QPEDIR/../x86-sdk ]; then
        die" Cannot do a \"-no-clean\" build as $QPEDIR/../x86-sdk does not exist yet"
    fi
  fi
  cd $QPEDIR/../x86-sdk
  LOGFILE=`pwd`/build.log
  echo 0>$LOGFILE
  skip_host_tools
  echo "TIME:`date "+%H:%M:%S"` Configuring Qtopia for x86 GCC 4.1.1"

  EXTRA_OPTIONS="-no-drm"   # Current default is to specify no-drm to match with the greenphone default
  test "$DRM" = "1" && EXTRA_OPTIONS="-drm"
  test "$SXE" = "0" && EXTRA_OPTIONS=$EXTRA_OPTIONS" -no-sxe"
  test "$SXE" = "1" && EXTRA_OPTIONS=$EXTRA_OPTIONS" -sxe"

  if [ $CLEAN = "1" ]; then      
      if $QPEDIR/configure -device gcc411 -prefix /opt/Qtopia/SDK/$QPEVER/x86/image -sdk /opt/Qtopia/SDK/$QPEVER/x86 -image /opt/Qtopia/SDK/$QPEVER/x86/image -qtopiamedia -mediaengines helix -helix-system-id linux-2.2-libc6-gcc32-i586 -ssl -debug -confirm-license -no-infrared -dbus -dbuspath /usr/local -bluetooth -extra-qt-config="-release" -extra-qt-config="-qt-libpng" -extra-qt-config -no-glib $EXTRA_OPTIONS >${OUTPUT} 2>&1; then
        echo "DONE"
      else
        echo "$0 FAILED in configure for x86 SDK"
        if [ "$QUIET" = "1" ] ; then
            echo "Try running $0 without the --quiet option to get more details on why configure failed" 
        fi
        exit 1
      fi
  else
    if [ ! -f $PWD/config.status ]; then
        echo "$0 FAILED in configure for x86 SDK, \"-no-clean\" option selected but no build was found in $PWD"
        exit 1
    fi
  fi

  echo "TIME:`date "+%H:%M:%S"` Building..."

  make -k >>${OUTPUT} 2>&1

  if make >>${OUTPUT} 2>&1; then
    echo "DONE"
  else
    output_error
    echo "$0 FAILED in make for x86 SDK"
    exit 1
  fi
  echo "TIME:`date "+%H:%M:%S"` Installing..."
  if make install >>${OUTPUT} 2>&1; then
    echo "DONE"
  else
    output_install_error
    echo "$0 FAILED in make install for x86 SDK"
    exit 1
  fi

  cd devices/greenphone/src/plugins/phonevendors/greenphone
  make >>${OUTPUT} 2>&1
  make install >>${OUTPUT} 2>&1
  cd /opt/Qtopia/SDK/$QPEVER/x86/qtopiacore/target/lib
  rm -f libQtCore.so.${QTVER}
  rm -f libQtGui.so.${QTVER}
  rm -f libQtNetwork.so.${QTVER}
  rm -f libQtSql.so.${QTVER}
  rm -f libQtSvg.so.${QTVER}
  rm -f libQtXml.so.${QTVER}
  ln -s ../../../image/lib/libQtCore.so.${QTVER}
  ln -s ../../../image/lib/libQtGui.so.${QTVER}
  ln -s ../../../image/lib/libQtNetwork.so.${QTVER}
  ln -s ../../../image/lib/libQtSql.so.${QTVER}
  ln -s ../../../image/lib/libQtSvg.so.${QTVER}
  ln -s ../../../image/lib/libQtXml.so.${QTVER}

  # Remove some helix file formats
  cd /opt/Qtopia/SDK/$QPEVER/x86/image/lib/helix
  rm -f amrff.so
  rm -f amrn.so
  rm -f amrw.so
  rm -f d263.so
  rm -f h263render.so
  rm -f mp3fformat.so
  rm -f mp3render.so
  rm -f mp4arender.so
  rm -f mp4fformat.so

  # Remove the docs and symlink to the greenphone's
  cd /opt/Qtopia/SDK/$QPEVER/x86
  rm -rf /opt/Qtopia/SDK/$QPEVER/x86/doc
  ln -s /opt/Qtopia/SDK/$QPEVER/greenphone/doc

  rm -rf /opt/Qtopia/SDK/$QPEVER/x86/qtopiacore/qt/doc
  ln -s /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/qt/doc /opt/Qtopia/SDK/$QPEVER/x86/qtopiacore/qt/doc
  ln -s /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/qt/tools /opt/Qtopia/SDK/$QPEVER/x86/qtopiacore/qt/tools

  # Update the x86 version of the sxe_qtopia script to point to the x86 path
  if [ "$SXE" = "1" ] ; then
    cd /opt/Qtopia/SDK/$QPEVER/x86/image/etc/sxe_qtopia
    chmod +w sxe_qtopia
    sed -e "/^#!/a test -z \$QTOPIA_DIR && export QTOPIA_DIR=/opt/Qtopia/SDK/$QPEVER/x86" sxe_qtopia >sxe_qtopia.tmp
    cat sxe_qtopia.tmp > sxe_qtopia
    rm sxe_qtopia.tmp
    chmod -w sxe_qtopia
  fi

  cd $QPEDIR
}

make_greenphone_img()
{
		rm -Rf $QPEDIR/$IMAGE_PATH
		cp -dpR /opt/Qtopia/SDK/$QPEVER/greenphone/image $QPEDIR/$IMAGE_PATH
}

update_extras()
{
  mkdir -p /opt/Qtopia/extras/bin
  cd /opt/Qtopia/extras/bin
  rm -f /opt/Qtopia/extras/bin/*
  wget -nv http://qtopiaweb.trolltech.com.au/dist/input/qtopia/$QPEVER/greenphone/extras/uic3
  chmod 555 uic3
  wget -nv http://qtopiaweb.trolltech.com.au/dist/input/qtopia/$QPEVER/greenphone/extras/qt3to4
  chmod 555 qt3to4
  wget -nv http://qtopiaweb.trolltech.com.au/dist/input/qtopia/$QPEVER/greenphone/extras/usbflash
  chmod 555 usbflash


#addional device specific extra add-ons here 
# to be run at update time
		if [ -e $QTOPIA_DEPOT_PATH/devices/greenphone/scripts/extras.sh ]; then
				cp -a $QTOPIA_DEPOT_PATH/devices/greenphone/scripts/extras.sh /opt/Qtopia/extras/bin
				chmod +x /opt/Qtopia/extras/bin/extras.sh
		fi

}

make_flash()
{
echo "make_flash "
  mkdir -p /opt/Qtopia/extras/images
  EXTRA_FLASH_OPTIONS=

  cd $QPEDIR

  if [ "$ROOTFS" = "0" ] ; then
    mkdir -p $QPEDIR/rootfs
    cd rootfs
    cp /opt/Qtopia/extras/images/greenphone-rootfs.tar.gz .
    cp /opt/Qtopia/extras/images/greenphone-kernel .
    cd ..
    EXTRA_FLASH_OPTIONS=" --rootfs $QPEDIR/rootfs/greenphone-rootfs.tar.gz --kernel $QPEDIR/rootfs/greenphone-kernel"
  else
    sudo cp -f $BUILD_PATH/greenphone-rootfs.tar.gz /opt/Qtopia/extras/images/greenphone-rootfs.tar.gz
    sudo cp -f $BUILD_PATH/greenphone-kernel /opt/Qtopia/extras/images/greenphone-kernel
  fi

  echo "Make flash now..."
  sleep 2
  $QPEDIR/scripts/greenphone-make-flash.sh --qtopia-build $QPEDIR/$IMAGE_PATH --flash $EXTRA_FLASH_OPTIONS

  if [ -s $QPEDIR/qtopia-greenphone-flash ]; then
    sudo cp -f $QPEDIR/qtopia-greenphone-flash /opt/Qtopia/extras/images/qtopia-greenphone-flash-$QPEVER
    echo ""
    echo "To flash the whole device run : usbflash /opt/Qtopia/extras/images/qtopia-greenphone-flash-$QPEVER"
  else
    echo "Error: Missing $QPEDIR/qtopia-greenphone-flash"
    exit 1
  fi
  if [ -s $QPEDIR/qtopia-greenphone-update.tar.gz ]; then
      sudo cp -f $QPEDIR/qtopia-greenphone-update.tar.gz /opt/Qtopia/extras/images/qtopia-greenphone-update-$QPEVER.tar.gz
      cd /opt/Qtopia/extras/images
      sudo rm -f qtopia-greenphone-update.tar.gz
      sudo ln -s qtopia-greenphone-update-$QPEVER.tar.gz qtopia-greenphone-update.tar.gz
      sudo rm -f qtopia-greenphone-flash
      sudo ln -s qtopia-greenphone-flash-$QPEVER qtopia-greenphone-flash
      echo ""
      echo "To install updateqtopia image onto the device run : updatedevice"
  else
    echo "Error: Missing $QPEDIR/qtopia-greenphone-update.tar.gz"
    exit 1
  fi
  cd $QPEDIR
}

HELP=yes
OUTPUT="/dev/stdout"
SDKGRN=0
SDKX86=0
SDKIMG=0
FLASH=0
EXTRAS=0
VMWARE=0
ROOTFS=1
QUIET=0
SXE=0
DRM=0
IMAGE_PATH=image
BUILD_PATH=$PWD/rootfs
CLEAN=1

while [ -n "$1" ] ; do
  case $1 in
  -h | -help | --help)
    HELP=yes
    ;;
  -q | -quiet | --quiet)
    OUTPUT="build.log"
    HELP=no
    QUIET=1
    ;;
  -sdk | --sdk)
    SDKGRN=1
    SDKX86=1
    SDKIMG=1
    HELP=no
    ;;
  -sdk-greenphone | --sdk-greenphone)
    SDKGRN=1
    HELP=no
    ;;
  -sdk-x86 | --sdk-x86)
    SDKX86=1
    HELP=no
    ;;
  -sdk-img | --sdk-img)
    SDKIMG=1
    HELP=no
    ;;
  -flash | --flash)
    FLASH=1
    HELP=no
    ;;
  -extras | --extras)
    EXTRAS=1
    ROOTFS=1
    HELP=no
    ;;
  -sxe | --sxe)
    SXE=1
    ;;
  -drm | --drm)
    DRM=1
    ;;
  --image-path)
	IMAGE_PATH="$2"
	shift 1
	;;
  --build-path)
    BUILD_PATH="$2"
    shift 1
    ;;
  -no-clean | --no-clean)
    CLEAN=0
    ;;
  *)
    echo $1: unknown argument
    ;;
  esac
  shift
done

if [ "$HELP" = "yes" ] ; then
  echo
  echo "FORMAT: $0 <options>"
  echo
  echo "Options:"
  echo 
  echo " --help            display command help"
  echo " --quiet           quiet mode"
  echo " --sdk             Build x86, greenphone and image for SDK"
  echo " --sdk-greenphone  Build greenphone SDK"
  echo " --sdk-x86         Build x86 SDK"
  echo " --sdk-img         Create Qtopia dir from greenphone SDK binaries."
  echo " --flash           Create greenphone_flash file"
  echo " --extras          Update qt3to4, uic3 etc"
  echo " --sxe             Builds the specified SDK with SXE"
  echo " --drm             Builds the specified SDK with DRM"
  echo " --image-path      Specify the path where the Qtopia filesystem is."
  echo " --build-path      Specify the path where the rootfs is."
  echo " --no-clean        Build without first cleaning the build directory"
  echo
  exit
fi

if [ "`hostname`" = "greenphonesdk" ] ; then
  #We force if detect building inside vmware since don't have access to extras
  EXTRAS=0
  ROOTFS=0
  echo "Extras option has been disabled"
fi

export QPEVER=`version`
export QTVER=`qt_version`

export PATH=/opt/teambuilder/bin:/usr/local/gcc-4.1.1/bin:/opt/toolchains/greenphone/gcc-4.1.1-glibc-2.3.6/arm-linux/bin:$PATH
export TEAMBUILDER=1
export TEAMBUILDER_CC_VERSION=4.1.1

export QPEDIR=`pwd`
export PATH=${QPEDIR}/bin:${QPEDIR}/scripts:${PATH}

if [ "$SDKX86" = "1" ] ; then
  if [ $CLEAN = "1" ]; then
      rm -Rf /opt/Qtopia/SDK/$QPEVER/x86
  fi
  make_x86_sdk
fi

if [ "$SDKGRN" = "1" ] ; then
  if [ $CLEAN = "1" ]; then
      rm -Rf /opt/Qtopia/SDK/$QPEVER/greenphone
  fi
  make_greenphone_sdk
fi

if [ "$SDKIMG" = "1" ] ; then
  make_greenphone_img
fi

if [ "$EXTRAS" = "1" ] ; then
  rm -Rf /opt/Qtopia/extras
  update_extras
fi

if [ "$FLASH" = "1" ] ; then
  make_flash
fi

if [ -e /opt/Qtopia/extras/bin/uic3 ] ; then
  if [ -e /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/host/bin ] ; then
    rm -f /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/host/bin/uic3
    rm -f /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/host/bin/qt3to4
    rm -f /opt/Qtopia/SDK/scripts/usbflash
    ln -s /opt/Qtopia/extras/bin/uic3 /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/host/bin/uic3
    ln -s /opt/Qtopia/extras/bin/qt3to4 /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/host/bin/qt3to4
    ln -s /opt/Qtopia/extras/bin/usbflash /opt/Qtopia/SDK/scripts/usbflash
  fi
fi

if [ "$SDKGRN" = "1" ] ; then
if [ ! -e /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/qt/tools/qvfb/Greenphone.skin ] ; then
  mkdir -p /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/qt/tools/qvfb 
  cp -dpR $QPEDIR/devices/greenphone/Greenphone.skin /opt/Qtopia/SDK/$QPEVER/greenphone/qtopiacore/qt/tools/qvfb 
fi
ln -s /opt/Qtopia/SDK/$QPEVER/x86/image/pics /opt/Qtopia/SDK/pics

mkdir -p /opt/Qtopia/SDK/scripts
cp $QPEDIR/bin/mkPackages /opt/Qtopia/SDK/scripts
cp -dpR $QPEDIR/scripts/greenphone-sdk/scripts/* /opt/Qtopia/SDK/scripts

rm -f /opt/Qtopia/SDK/scripts/functions
echo -en "\043\041/bin/bash\nversion()\n{\necho \"$QPEVER\"\n}\n\nqt_version()\n{\necho \"$QTVER\"\n}\n">/opt/Qtopia/SDK/scripts/functions
cat $QPEDIR/scripts/greenphone-sdk/scripts/functions >>/opt/Qtopia/SDK/scripts/functions
chmod 755 /opt/Qtopia/SDK/scripts/functions

echo "Built: `date`">/opt/Qtopia/SDK/versioninfo
echo "By:    `whoami`">>/opt/Qtopia/SDK/versioninfo
echo "Host:  `hostname`">>/opt/Qtopia/SDK/versioninfo
echo "Device: Greenphone" >> /opt/Qtopia/SDK/versioninfo
fi
