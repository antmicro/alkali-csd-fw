# Alkali Firmware

This repository contains firmware sources of the Alkali project, which can be
used to generate the bitstream for the Western Digital NVMe accelerator test
platform (Basalt board). The project includes Linux built with Buildroot that
contains application for APU and there is a separate Zephyrproject application
for RPU.

The register description headers are auto-generated from the NVMe documentation.
It is used by RPU application to communicate with NVMe control registers.

# Repository structure

The diagram below presents the simplified structure of this repository and
includes the most important files and directories.

```
.
├── apu-app
│   ├── CMakeLists.txt
│   ├── src
│   │   ├── cmds
│   │   ├── vm
│   │   ├── vta
│   │   └── ...
│   └── third-party -> ../third-party
├── br2-external
│   ├── board
│   │   └── basalt
│   │       └── ...
│   ├── configs
│   │   └── zynqmp_nvme_defconfig
├── LICENSE
├── Makefile
├── README.md
├── requirements.txt
├── rpu-app
│   ├── CMakeLists.txt
│   ├── nvme.overlay
│   ├── prj.conf
│   ├── rpu.tcl
│   ├── src
│   │   └── ...
│   └── west.yml
└── third-party (submodules)
    ├── buildroot
    ├── googletest
    ├── linux
    ├── registers-generator
    ├── tensorflow
    └── ubpf
```

* [apu-app/](apu-app) - contains source files for APU Application. The APU
  App will be responsible for processing the custom NVMe commands. It is
  generated with use of Buildroot.

* [br2-external/](br2-external) - configuration files for Buildroot to build
  the Linux system for APU. Its rootfs contains an APU Application.

* [rpu-app/](rpu-app) - contains source file for RPU Application. The RPU
  software is responsible for handling the functionality required by the NVMe
  standard including regular read/write transactions. Application is built using
  Zephyr RTOS as an operating system.

* [third-party/](third-party) - the directory used to store all external
  projects used as a part of the `alkali-csd-hw` project. The
  `register-generator` directory contains scripts used to parse the NVMe
  specification and generate the headers with registers descriptions.
  The `buildroot`, `googletest`, `tensorflow` and `ubpf` are used to build APU
  software, including an operating system and applications.

* [Makefile](Makefile) - file containing all the rules used to manage the
  repository. Type `make help` to display all the documented rules that can be
  used inside the project.


# Prerequisites

To build the whole APU and RPU software you don't need any oddly specific
dependencies installed but there are few packages required. You might also need
to install CMake in order to build Zephyr application.
You can install them using the following commands:

1. Install dependencies:
```bash
sudo apt update -y
sudo apt install -y bc bison build-essential cpio curl default-jdk flex git \
    gperf libcurl4-openssl-dev libelf-dev libffi-dev libjpeg-dev libpcre3-dev \
    libssl-dev make ninja-build python3 python3-pip python3-sphinx rsync rustc \
    unzip wget
```

2. Install CMake:
```bash
git clone -b v3.16.7 https://gitlab.kitware.com/cmake/cmake.git cmake
cd cmake
./bootstrap --system-curl
make -j$(nproc)
make install
```

After succesful installation you might want to delete not needed files with:
```bash
cd ..
rm -rf cmake
```

3. Install required python packages:
```bash
pip3 install -r requirements.txt
```

# Usage

To generate software for the alkali board you can simply use:
```bash
make all
```
This will run all builds one by one in order `APU App`, `Linux system`, `RPU App`.
However it might take a lot of time (~2 hours) so you can also choose which part
would you like to build. Note that in order to build Linux system, you must build
an APU App first.
* Build APU App:
```bash
make apu-app
```
* Build Linux system:
```bash
make buildroot
```
* Build RPU App:
```bash
make rpu-app/with-sdk
```
If you have Zephyr installed and configured already, you can also omit `/with-sdk` suffix.

The final output files can be found in `build/` directory.
