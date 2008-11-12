#!/bin/sh
# This script will make an SDK iso out of files created by make-sdk.sh
# you must have working vmdk files zipped on a server.
# you must edit this script!
DEVICE=""
HELP=no
QPE_VERSION=`cat $QTOPIA_DEPOT_PATH/src/libraries/qtopiabase/version.h | grep QPE_VERSION |awk '{print $3}'|sed 's/"//g'`
BUILDDIR=`pwd`

# CHANGENO=
QPE_URL_VERSION=`echo $QPE_VERSION | sed 's/\.[0-9]*$//g'`

# put your own url here where your vmdk's zip files are
URL=http://qtopiaweb.trolltech.com.au:/dist/input/qtopia/sdk

# URL=$1

function fix_versions
{

echo "Fix version in docs"
echo $QPE_VERSION

  sed -n 's/!CHANGENO!/$CHANGENO &/p' isofs/sdk.nsi
  perl -i.bak -pe "s/\!CHANGENO\!/$CHANGENO/;" isofs/sdk.nsi
  perl -i.bak -pe "s/\!QPE_VERSION\!/$QPE_VERSION/;" isofs/sdk.nsi
#  type $QPE_DEPOT_DIR/scripts/greenphone-sdk/windows/greenphone-sdk.nsi


  chmod +w isofs/release.txt
  perl -i.bak -pe "s/\!CHANGENO\!/$CHANGENO/;" isofs/release.txt
  perl -i.bak -pe "s/\!QPE_VERSION\!/$QPE_VERSION/;" isofs/release.txt
  perl -i.bak -pe "s/\!QPE_URL_VERSION\!/$QPE_URL_VERSION/;" isofs/release.txt

  chmod +w isofs/release.html
  perl -i.bak -pe "s/\!CHANGENO\!/$CHANGENO/;" isofs/release.html
  perl -i.bak -pe "s/\!QPE_VERSION\!/$QPE_VERSION/;" isofs/release.html
  perl -i.bak -pe "s/\!QPE_URL_VERSION\!/$QPE_URL_VERSION/;" isofs/release.html
  rm -f isofs/*.bak

}

DEVICE=""

while [ -n "$1" ] ; do
  case $1 in
  -h | -help | --help)
    HELP=yes
    ;;
  -d | --device)
    DEVICE="$2"
    shift 1;
    ;;
  *)
  HELP="yes";
    ;;
  esac
  shift
done

if [ "$HELP" = "yes" ] ; then
  echo
  echo "FORMAT: $0 version [options]"
  echo
  echo "Options:"
  echo 
  echo " --help            display command help"
  echo " --device          set device configuration profile"
  echo
  exit
fi


if [ -z $DEVICE ]; then
		echo "SET DEVICE!"
exit 1
fi


. $QTOPIA_DEPOT_PATH/devices/$DEVICE/environment

if [ -z $TOOLCHAIN ]; then
		echo "SET TOOLCHAIN!"
exit 1
fi


mkdir -p $BUILDDIR/"$DEVICE"_sdk_build/
rm -rf $BUILDDIR/"$DEVICE"_sdk_build/*
cd $BUILDDIR/"$DEVICE"_sdk_build

# Copy the pdf docs into the sdk
cp $QTOPIA_DEPOT_PATH/devices/$DEVICE/sdk/docs/*.pdf /opt/Qtopia/SDK/.

#create qtopia.tar.bz2
cd /opt/Qtopia
echo "Create Qtopia sdk and extras zip file"

if ! tar -cjf qtopia.tar.bz2 SDK extras; then
		echo "creating qtopia SDK tarball failed."
exit 1
fi

cd $BUILDDIR

mkdir -p $BUILDDIR/"$DEVICE"_sdk_build/sdk-files
mv /opt/Qtopia/qtopia.tar.bz2 $BUILDDIR/"$DEVICE"_sdk_build/sdk-files

# cd  $BUILDDIR/"$DEVICE"_sdk_build
# wget -nv http://qtopiaweb.trolltech.com.au:/dist/toolchains/$DEVICE.tgz
# official toolchain:
# http://downloads.openmoko.org/toolchains/openmoko-i686-arm-linux-gnueabi-toolchain.tar.bz2
# tar -xvpzf $DEVICE.tgz


#create tools.tar.bz2
cd $TOOLCHAIN
TOOLS=`basename $TOOLCHAIN`
echo $TOOLS
cd ../

echo "Create toolchain zip file"
if ! tar -cjf $BUILDDIR/"$DEVICE"_sdk_build/sdk-files/tools.tar.bz2 $TOOLS; then
		echo "Creating toolchain tarball failed"
		exit 1
fi

#create extras.tar.bz2
cp -dPR $QTOPIA_DEPOT_PATH/devices/$DEVICE/sdk/extras $BUILDDIR/"$DEVICE"_sdk_build/sdk-files/
cd $BUILDDIR/"$DEVICE"_sdk_build/sdk-files/extras
echo "Create extras zip file"
if ! tar -cjf $BUILDDIR/"$DEVICE"_sdk_build/sdk-files/extras.tar.bz2 *; then
		echo "Creating extras tarball failed"
		exit 1
fi


cp -f $QTOPIA_DEPOT_PATH/scripts/sdk/windows/manual-install.sh $BUILDDIR/"$DEVICE"_sdk_build/sdk-files/
chmod +x $BUILDDIR/"$DEVICE"_sdk_build/sdk-files/manual-install.sh


cd $BUILDDIR/"$DEVICE"_sdk_build/sdk-files
#rm -Rf $BUILDDIR/"$DEVICE"_sdk_build/opt


cp -f /opt/Qtopia/SDK/versioninfo version

#cp -f $QTOPIA_DEPOT_PATH/scripts/sdk/windows/ks.cfg .
cd ..
echo "Creating Qtopia binary and toolchain iso"
 mkisofs -o qtopia.iso -graft-points -volid "SDK update" -publisher "Trolltech" -full-iso9660-filenames -iso-level 2 -rational -J sdk-files/

cd  $BUILDDIR/"$DEVICE"_sdk_build



VMDKFILES="home rootfs toolchain qtopiasrc qtopia"

for FILE in $VMDKFILES; do
		wget -nv $URL/$FILE.zip
		unzip $FILE.zip
    rm $FILE.zip
		tar -vcjf $FILE.dat $FILE.vmdk
    rm $FILE.vmdk
done


rm -rf isofs
mkdir isofs
cp -av $QTOPIA_DEPOT_PATH/scripts/sdk/windows/* isofs
chmod +x isofs/install.sh

char=${DEVICE:0:1}
string=${DEVICE:1}
CHAR=`echo "$char" | tr a-z A-Z`

DEVICENAME="$CHAR$string"

chmod +rw isofs/sdk.vmx
echo "displayName = \"$DEVICENAME\"">> isofs/sdk.vmx

#TODO fixme 
mv isofs/sdk.vmx isofs/Qtopia.vmx


fix_versions


rm -f isofs/sdk.nsi
rm -f isofs/splash.bmp
rm -f isofs/ks.cfg

mv *.dat isofs
mv *.iso isofs

## SDK documents here
cp $QTOPIA_DEPOT_PATH/devices/$DEVICE/sdk/docs/*.pdf isofs

# FIXME
# cp $QTOPIA_DEPOT_PATH/doc/src/$DEVICE/*.pdf isofs
# rm -f isofs/"$DEVICE"_userguide_wifi.pdf

wget -nv $URL/extras/autorun.exe
mv autorun.exe isofs
echo "Creating final iso"


mkisofs -o qtopia-"$DEVICE"-sdk-"$QPE_VERSION".iso -graft-points -volid "$DEVICE SDK" -publisher "Trolltech" -full-iso9660-filenames -iso-level 2 -rational -J isofs/

SIZE=`ls -al qtopia-"$DEVICE"-sdk-"$QPE_VERSION".iso |awk '{print $5}'`
if [ $SIZE -gt 734003200 ] ; then
  echo "ERROR: $DEVICE SDK has a size of $SIZE. I expected at most 650 Mb."
  exit 1
fi

echo "make iso FINISHED!"
