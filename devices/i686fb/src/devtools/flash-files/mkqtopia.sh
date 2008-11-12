#!/bin/bash
QPEDIR=`pwd`

test -z $QPEDIR && QPEDIR=$(dirname $0)

# search backward until the build directory is found
while [ ! -f "$QPEDIR/config.cache" -a -n "$QPEDIR" ]; do
    QPEDIR=${QPEDIR%/*}
done
if [ -z $QPEDIR ]; then
    "Could not find build directory - set \$QPEDIR location of config.cache (or do configure; make)";
    exit 1;
fi

PREFIX="$(grep '^opt\.prefix\.default=' $QPEDIR/config.cache | sed 's/^opt\.prefix\.default=//')"
if [ -z $PREFIX ]; then
    echo "Could not find opt.prefix.default in $QPEDIR/config.cache - redo configure?"
    exit 1
fi

if [ ! -d "$PREFIX" ]; then
    echo "No image directory $PREFIX - run make install"
    exit 1;
fi

if [ $1 ]; then
    build_cramfs=1
    echo "Building Qtopia cramfs loop fs"
else
    echo "Building Qtopia ext2 loop fs"
fi

# The operations need root privileges
test `id -u` == 0 || { echo "Run this script as root" && exit 1; }

#clean up after any aborted run
test -d qtopia_fs && rm -Rf qtopia_fs
test -d cdrom_fs && rm -Rf cdrom_fs

# build the qtopia user file-system - will go in a tarball
rm -Rf Qtopia.user
mkdir Qtopia.user
cd Qtopia.user
mkdir apps bin lib plugins
mv $PREFIX/etc .
mv $PREFIX/qpe.sh .
tar -czf $PREFIX/qtopia-user.tgz .
cd ..

# build the qtopia filesystem
mkdir -p qtopia_fs
if [ ! $build_cramfs ]; then
    dd if=/dev/zero of=qtopia.lp bs=1M count=256
    /sbin/mke2fs -F qtopia.lp
    mount -t ext2 qtopia.lp qtopia_fs -o loop
fi
cp -dpR $PREFIX/* qtopia_fs
ln -s /opt/Qtopia.user/etc qtopia_fs/etc
if [ $build_cramfs ]; then
    /sbin/mkfs.cramfs qtopia_fs qtopia.rom
else
    umount qtopia_fs
fi
rm -Rf qtopia_fs

# build the cd with the cramfs in it
mkdir cdrom_fs
if [ $build_cramfs ]; then
    mv qtopia.rom cdrom_fs
else
    mv qtopia.lp cdrom_fs
fi

mkisofs -o qtopiacd.iso cdrom_fs
rm -Rf cdrom_fs

# Put things back how they were
mv Qtopia.user/etc $PREFIX/.
mv Qtopia.user/qpe.sh $PREFIX/.
rm -Rf Qtopia.user

cat <<END

qtopiacd.iso created
END
