NVMe RPU firmware
=================

Copyright 2021-2022 Western Digital Corporation or its affiliates<br>
Copyright 2021-2022 Antmicro

This repository contains RPU firmware that is responsible for handling the NVMe communitation with host.

Building
--------

To build this firmware you need to first install Zephyr SDK and clone the Zephyr repository with submodules.

`source ~/.zephyrrc`
`source path/to/zephyr/repo/zephyr-env.sh`
`west build -b zcu106 .`

This should create a file `build/zephyr/zephyr.elf` which can be used as firmware for the RPU.
