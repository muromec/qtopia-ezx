#!/bin/sh

MNTPNTS=$( awk '/^\/dev\/mmcblk0p1/ { print $2 }' /proc/mounts )
[ -z "$MNTPNTS" ] && exit

# lazy unmount SD card so Qtopia will not be able to see it
umount -l $MNTPNTS

# signal Qtopia to release all open file handles on SD card
qcop QPE/Card 'mtabChanged()'

