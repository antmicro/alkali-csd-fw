cmake_minimum_required(VERSION 3.13.1)

set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nvme.overlay")

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(tc_sw)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/dma.c)
target_sources(app PRIVATE src/tc.c)
target_sources(app PRIVATE src/ramdisk.c)

target_sources(app PRIVATE src/rpmsg.c)
target_sources(app PRIVATE src/platform_info.c)
target_sources(app PRIVATE src/rsc_table.c)
target_sources(app PRIVATE src/zynqmp_r5_a53_rproc.c)

target_sources(app PRIVATE src/cmd.c)
target_sources(app PRIVATE src/cmds/identify.c)
target_sources(app PRIVATE src/cmds/get_log.c)
target_sources(app PRIVATE src/cmds/set_features.c)
target_sources(app PRIVATE src/cmds/queues.c)
target_sources(app PRIVATE src/cmds/read.c)
target_sources(app PRIVATE src/cmds/write.c)
target_sources(app PRIVATE src/cmds/vendor.c)

target_include_directories(app PRIVATE src)