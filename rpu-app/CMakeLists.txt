cmake_minimum_required(VERSION 3.13.1)

set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nvme.overlay")

find_package(Zephyr)

project(tc_sw)

target_sources(app PRIVATE src/main.c)
