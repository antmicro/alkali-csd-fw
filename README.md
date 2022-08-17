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
