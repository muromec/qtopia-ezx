#!/bin/sh
#
# This script will create rootfs.tgz and rootfs.jffs2 for the Marvel Zylonite
#   based on pristine rootfs.tgz

LOGFILE="log.txt"
:> $LOGFILE

RUN_OK=

DISTDIR=$PWD


usage()
{
    echo -e "Usage: `basename $0`  [--source-url <url>]" 
    echo -e "   --source-url     Location to download prestine rootfs.tgz from." 
}

# Package location for pristine rootfs
PACKAGE_LOCATION=http://qtopiaweb.trolltech.com.au/dist/input/qtopia/zylonite_eabi/

while [ $# -ne 0 ]; do
    case $1 in
        --source-url)
            if [ $# -ge 2 ]; then
                PACKAGE_LOCATION="$2"
                shift 1
            else
                echo "$1 requires an argument"
                usage
                exit 1
            fi
            ;;
        
        --help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
    shift
done

echo "Create a new rootfs.tgz and rootfs.jffs2 for the Marvel Zylonite"

rm -f mkfs.jffs2
if [ -n "$QPEDIR" ]; then
    if [ ! -f $QPEDIR/image/bin/qpe ]; then
        echo "Missing $QPEDIR/image/bin/qpe, try \"make install\" first"
    else
        wget $PACKAGE_LOCATION/mkfs.jffs2 >> $LOGFILE 2>&1 
        if [ -f mkfs.jffs2 ]; then
            chmod +x mkfs.jffs2  >> $LOGFILE 2>&1 
            for model in a1 littleton; do
                if [ ! -d $DISTDIR/$model ]; then
                    mkdir $DISTDIR/$model
                fi
                cd $DISTDIR/$model
                sudo rm -rf rootfs*
                wget $PACKAGE_LOCATION/$model/rootfs.tgz  >> $LOGFILE 2>&1 
                if [ -f rootfs.tgz ]; then
                    sudo tar -xzf rootfs.tgz   >> $LOGFILE 2>&1 
                    sudo rm -rf rootfs/opt/Qtopia   >> $LOGFILE 2>&1 
                    sudo cp -a $QPEDIR/image rootfs/opt/Qtopia  >> $LOGFILE 2>&1 
                    sudo $DISTDIR/mkfs.jffs2 -r ./rootfs -o rootfs.jffs2 -e 0x20000 --pad=0x03000000  >> $LOGFILE 2>&1 
                    sudo tar -czf rootfs.tgz rootfs  >> $LOGFILE 2>&1 

                    # Check that the size is 48m
                    SIZE=`du -b rootfs.jffs2 | awk '{print $1}'`
                    if [ $SIZE != 50331648 ] ; then
                        echo "Failed: Size of rootfs.jffs2 for the Zylonite $model is not 48mb!"
                        exit 1
                    fi
                    RUN_OK=1
                fi
            done
        fi
    fi
fi

3>&1

if [ -z "$RUN_OK" ]; then
    echo "Failed: Unable to ceate a new rootfs.tgz and rootfs.jffs2 for the Marvel Zylonite"
    cat $LOGFILE
    exit 1
else
    echo "Success: Created rootfs.jffs2 and rootfs.tgz for Marvel Zylonites"
    exit 0
fi
