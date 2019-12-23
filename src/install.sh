#!/bin/sh
module="translate"
device="trans"
mode="777"

# remove stale nodes
rm -f /dev/${device}[0-1]

# delete module
/sbin/rmmod $module

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
# call with arguments to set bufSize and transOffset
# example:
# sudo ./install.sh bufSize=10 transOffset=5
/sbin/insmod ./$module.ko $* || exit 1

major=$(grep /proc/devices -e $module | cut -d\  -f1)

mknod /dev/${device}0 c $major 0
mknod /dev/${device}1 c $major 1

# give appropriate group/permissions, and change the group.
# not all distributions have staff, some have "wheel" instead.
group="staff"
grep -q '^staff:' /etc/group || group="wheel"

chgrp $group /dev/${device}[0-1]
chmod $mode /dev/${device}[0-1]
