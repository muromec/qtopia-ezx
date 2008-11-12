#!/bin/sh

case $1 in
    unload)
        rmmod g_ether
        rmmod g_file_storage
        rmmod g_serial
        ;;
    ether)
        modprobe g_ether
        ;;
    serial)
        modprobe g_serial
        ;;
    storage)
        if grep /dev/hda6 /proc/mounts; then
            umount /dev/hda6
        fi
        modprobe g_file_storage file=/dev/hda6
        ;;
    *)
        echo "$0 {unload|ether|serial|storage}"
        exit 1;
        ;;
esac

