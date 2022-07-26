#!/bin/sh

CNT=$(dmesg | grep nvme | wc -l)

# stop RPU
echo stop > /sys/class/remoteproc/remoteproc0/state
# select firmware
echo zephyr.elf > /sys/class/remoteproc/remoteproc0/firmware
# disable IPI communication for the RPU
devmem 0xff31001c 32 0xffffffff
# start RPU
echo start > /sys/class/remoteproc/remoteproc0/state

while [ $CNT -eq $(dmesg | grep nvme | wc -l) ]; do sleep 0.5; done

apu-app
