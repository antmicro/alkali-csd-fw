cmake_minimum_required(VERSION 3.13.1)

set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nvme.overlay")

find_package(Zephyr)

project(tc_sw)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/dma.c)
target_sources(app PRIVATE src/tc.c)

target_sources(app PRIVATE src/cmd.c)
target_sources(app PRIVATE src/cmds/identify.c)
target_sources(app PRIVATE src/cmds/get_log.c)
target_sources(app PRIVATE src/cmds/set_features.c)
