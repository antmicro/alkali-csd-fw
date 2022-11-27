# NVMe APU application

Copyright 2021-2022 Western Digital Corporation or its affiliates<br>
Copyright 2021-2022 Antmicro

This repository contains APU userspace application that is responsible for handling the custom NVMe commands.

# Building

To build this app you need to have Buildroot toolchain prepared and Tensorflow sources cloned.
You might also need to update `buildroot.cmake` file to point to your Buildroot toolchain.
Once that is ready, you can build the app with:

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../buildroot.cmake
cmake --build . -j`nproc`
```

This should create a file `apu-app` which can be executed on the APU.

# VTA delegate

APU app contains a TensorFlow Lite delegate for VTA accelerator, build as `libvta-delegate.so`.
It requires VTA driver (`libvta-driver.so`), which interfaces with real or simulated VTA accelerator.

## Building a driver for simulated VTA delegate

To build driver for simulated VTA (for testing purposes), run CMake with the `-DNO_HARDWARE=ON` flag:

```bash
mkdir build && cd build
cmake -DNO_HARDWARE=ON ..
make -j`nproc`
```

It is not a hardware-accurate simulation, it is only a C++ implementation of VTA operations.

## Building tests for VTA delegate

Tests are implemented with Google Test framework.
To build tests for VTA-accelerated operations in VTA delegate, run CMake with the `-DBUILD_TESTS=ON` flag:

```bash
mkdir build && cd build
cmake -DNO_HARDWARE=ON -DBUILD_TESTS=ON ..
make -j`nproc`
```

`NOTE:` Tests can be executed both with simulated VTA and actual hardware.

After successful build, the runner for tests appears in the `build` directory as `vta-delegate-test-runner`.
To show the help for the program run:

```bash
./vta-delegate-test-runner --help
```

To list all the available tests, use `--gtest_list_tests` flag:

```bash
./vta-delegate-test-runner --gtest_list_tests
```

To run tests which names follow a certain regex, use `--gtest_filter` flag, e.g.:

```bash
./vta-delegate-test-runner --gtest_filter="VTAAddTestGroup/VTAAddTest.*"
```

To run all tests for VTA Add implementation.
