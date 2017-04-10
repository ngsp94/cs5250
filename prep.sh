#!bin/sh

rm /dev/lcd
rmmod my_driver
make
insmod ./my_driver.ko
mknod /dev/lcd c 61 0

