cmake_minimum_required(VERSION 3.16)

project(apu-app)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)

set(USE_SIM_DRIVER OFF CACHE BOOL "Use simulated driver for VTA")
set(BUILD_TESTS OFF CACHE BOOL "Build tests for the VTA delegate")

add_subdirectory(third-party/tensorflow/tensorflow/lite/)
add_subdirectory(third-party/ubpf/)

include_directories(src third-party/tensorflow)

add_library(pynqlib
    src/vta/pynqlib.cpp
)

# Select driver to use
if (NOT USE_SIM_DRIVER)
    add_library(vta-driver SHARED
        src/vta/tf_driver.cc
    )
    target_link_libraries(vta-driver pynqlib)
else()
    add_library(vta-driver SHARED
        src/vta/sim_driver.cc
        src/vta/sim_tlpp.cc
        src/vta/virtual_memory.cc
    )
endif()

# Build VTA delegate
add_library(vta-delegate SHARED
    src/tflite-delegate.cpp
    third-party/tensorflow/tensorflow/lite/delegates/utils/simple_delegate.cc
)
target_link_libraries(vta-delegate
    tensorflow-lite
    vta-driver
)

# Build APU APP
add_executable(apu-app
    src/main.cpp
    src/cmd.cpp
    src/rpmsg.cpp

    src/vm/print.cpp
    src/vm/tf.cpp
    src/vm/register.cpp

    src/cmds/adm_acc_ctl.cpp
    src/cmds/fw.cpp
    src/cmds/identify.cpp
    src/cmds/io_acc_ctl.cpp
    src/cmds/lba.cpp
    src/cmds/qspi.cpp
    src/cmds/status.cpp

    src/vta/vta_runtime.cc
)
target_link_libraries(apu-app
    ubpf
    Threads::Threads
    vta-delegate
    pynqlib
)

# Build TFLite delegate test app
add_executable(tflite-delegate-test
    src/vta/vta_runtime.cc
    src/tflite-test.cpp
)
target_link_libraries(tflite-delegate-test
    vta-delegate
)
