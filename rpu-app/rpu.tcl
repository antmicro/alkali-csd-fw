# Copyright 2021-2022 Western Digital Corporation or its affiliates
# Copyright 2021-2022 Antmicro
#
# SPDX-License-Identifier: Apache-2.0

connect -url TCP:localhost:6000

source /opt/Xilinx/Vitis/2019.2/scripts/vitis/util/zynqmp_utils.tcl

targets -set -nocase -filter {name =~ "RPU*"}

enable_split_mode

targets -set -nocase -filter {name =~ "APU*"}

set mode [expr [mrd -value 0xff5e0200] & 0xf]

targets -set -nocase -filter {name =~ "*R5*#0"}

rst -processor

dow build/zephyr/zephyr.elf
