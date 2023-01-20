#! /bin/sh

module="shortio"
device="shortio"
mode="664"
group=0

# invoke insmod with all arguments we got
# and use a pathname, as insmod doesn't look in.

function load() {
    insmod ./$module.ko $* || exit 1

    rm -f /dev/${device}[0]

    # retrieve major number
    major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)
    mknod /dev/${device}0 c $major 0

    chgrp $group /dev/$device[0]
    chmod $mode /dev/$device[0]
}

function unload() {
    rm -f /dev/${device}[0]
    rmmod $module $* || exit 1
}

arg=${1:-"load"}
case $arg in
    load)
        load
        ;;
    unload)
        unload
        ;;
    reload)
        ( unload )
        load
        ;;
    *)
        echo "Usage: $0 {load | unload | reload}"
        echo "Default is load"
        exit 1
        ;;
esac