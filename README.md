# Alkali Firmware

This repository contains firmware sources of the Alkali project, which can be
used to generate the bitstream for the Western Digital NVMe accelerator test
platform (AN300 board). The project includes Linux built with Buildroot that
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
│   │   └── an300
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

This repository contains the `fw.dockerfile` which can be used to simplify
the process of installing dependencies. Before running other rules from
this repository make sure that you build the docker image by using:
```
make docker
```
In case you want to install all the prerequisites directly on your machine,
follow the instructions from the `fw.dockerfile`.

# Usage

If you use docker workflow, use `make enter` to open the docker container
before running other commands.

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
make rpu-app
```

The final output files can be found in `build/` directory.

# Managing Subprojects

The projects available in this repository use different build systems
which are all together managed by the main `Makefile`. To extend the existing
project with add new files one has to understand how they are organized.

* `apu-app` uses `CMake` to manage its dependencies. To add sources to
  the project, include the new files in the proper list of either
  [library](https://cmake.org/cmake/help/latest/command/add_library.html) or
  [executable](https://cmake.org/cmake/help/latest/command/add_executable.html)
  targets inside the `apu-app/CMakeLists.txt` file. For details about
  the CMake file format check [CMake documentation](https://cmake.org/cmake/help/latest/)

* rpu-app uses `west` and `CMake` to manage its dependencies. To add sources
  to the application extend the list of
  [target_sources (https://cmake.org/cmake/help/latest/command/target_sources.html)
  in the `rpu-app/CMakeLists.txt`. For details about the CMake file format check
  [CMake documentation](https://cmake.org/cmake/help/latest/) All the changes
  related to the device tree settings should be applied to
  `rpu-app/nvme.overlay` file. The list of the Zephyr subprojects used to build
  the app is managed by `west` and is contained in the `rpu-app/west.yml` file.
  For details about the West check [West documentation](https://docs.zephyrproject.org/latest/develop/west/index.html)

* Buildroot uses `make` and `Kconfig` for building the projects with
  the given configuration. The most important part of the project includes
  the configuration located in the `br2-external/configs/zynqmp_nvme_defconfig`
  which is used to select all the components of the system including U-Boot,
  Linux kernel, ARM trusted firmware, and root file system. Besides the packages
  specified in the config, all the files from the `br2-external/board/alkali`
  directory are copied to the final root file system. Details about
  the Buildroot may be found in the [Buildroot's documentation](https://buildroot.org/downloads/manual/manual.html)
