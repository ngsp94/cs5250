#!bin/sh

rm /dev/scull
rmmod my_driver
make
insmod ./my_driver.ko
mknod /dev/scull c 61 0

