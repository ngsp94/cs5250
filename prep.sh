#!bin/sh

rm /dev/fourmb
rmmod my_driver
make
insmod ./my_driver.ko
mknod /dev/fourmb c 61 0

