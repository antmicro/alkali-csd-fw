NVMe APU application
====================

Copyright (c) 2021 [Antmicro](https://www.antmicro.com)

This repository contains APU userspace application that is responsible for handling the custom NVMe commands.

Building
--------

To build this app you need to have `aarch64-linux-gnu-` toolchain installed.
Once that is installed, you can build the app with:

    export CROSS_COMPILE=aarch64-linux-gnu-
    make

This should create a file `apu-app` which can be executed on the APU.
