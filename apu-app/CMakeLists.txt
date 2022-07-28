# Copyright 2021-2022 Western Digital Corporation or its affiliates
# Copyright 2021-2022 Antmicro

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)

project(apu-app)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../" ABSOLUTE)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)

set(NO_HARDWARE OFF CACHE BOOL "Use simulated driver for VTA")
set(BUILD_TESTS OFF CACHE BOOL "Build tests for the VTA delegate")

# Enable installing ubpf
set(UBPF_ENABLE_INSTALL ON)

add_subdirectory(third-party/tensorflow/tensorflow/lite/)
add_subdirectory(third-party/ubpf/)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(third-party/googletest/)
    message(STATUS "Added testing")
endif()

include_directories(src third-party/tensorflow third-party/ubpf/vm/inc)

add_library(bpf-vm-utils SHARED
        src/vm/print.cpp
        src/vm/tf.cpp
        src/vm/register.cpp
)
target_link_libraries(bpf-vm-utils
    tensorflow-lite
    ubpf
)

# Select driver to use
if (NO_HARDWARE)
    add_library(vta-driver SHARED
        src/vta/sim_driver.cc
        src/vta/sim_tlpp.cc
        src/vta/virtual_memory.cc

        src/vta/vta_runtime.cc
    )
else()
    add_library(vta-driver SHARED
        src/cmds/adm_acc_ctl.cpp
        src/cmds/fw.cpp
        src/cmds/identify.cpp
        src/cmds/io_acc_ctl.cpp
        src/cmds/lba.cpp
        src/cmds/qspi.cpp
        src/cmds/status.cpp

        src/vta/tf_driver.cc
        src/vta/pynqlib.cpp
        src/cmd.cpp
        src/rpmsg.cpp
        src/acc.cpp

        src/vta/vta_runtime.cc
    )
    target_link_libraries(vta-driver
        bpf-vm-utils
    )
endif()

# Build VTA delegate
add_library(vta-delegate SHARED
    src/vta-delegate.cpp
    src/vta-delegate-ops.cpp
    third-party/tensorflow/tensorflow/lite/delegates/utils/simple_delegate.cc
)
target_link_libraries(vta-delegate
    tensorflow-lite
    vta-driver
)

if (NOT NO_HARDWARE)
    # Build APU APP only if hardware is available
    # TODO make apu-app work in a simulated environment
    add_executable(apu-app
        src/main.cpp
    )
    target_link_libraries(apu-app
        ubpf
        Threads::Threads
        vta-delegate
    )
    install(
        TARGETS
            apu-app
        DESTINATION bin
    )
endif()

# Build TFLite delegate test app
add_executable(vta-delegate-test
    src/tflite-vta-test.cpp
)
target_link_libraries(vta-delegate-test
    vta-delegate
)

if (BUILD_TESTS)
    add_executable(vta-delegate-test-runner
        tests/basic-vta-delegate-tests.cpp
        tests/add-tests.cpp
        tests/conv2d-tests.cpp
        tests/tests-main.cpp
    )
    target_link_libraries(vta-delegate-test-runner
        gmock
        gtest
        stdc++fs
        vta-delegate
    )
    add_test(vta-delegate-test vta-delegate-test-runner)
endif()

install(
    TARGETS
        vta-delegate
        bpf-vm-utils
        vta-driver
    DESTINATION lib
)
install(
    TARGETS
        vta-delegate-test
    DESTINATION bin
)
