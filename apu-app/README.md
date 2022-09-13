NVMe APU application
====================

Copyright 2021-2022 Western Digital Corporation or its affiliates<br>
Copyright 2021-2022 Antmicro

This repository contains APU userspace application that is responsible for handling the custom NVMe commands.

Building
--------

To build this app you need to have Buildroot toolchain prepared and Tensorflow sources cloned.
You might also need to update `buildroot.cmake` file to point to your Buildroot toolchain.
Once that is ready, you can build the app with:

    mkdir build && cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../buildroot.cmake -DTENSORFLOW_SOURCE_DIR=<tensorflow-dir>
    cmake --build . -j

This should create a file `apu-app` which can be executed on the APU.
