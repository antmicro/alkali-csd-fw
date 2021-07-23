cmake_minimum_required(VERSION 3.16)

project(apu-app C CXX)

add_executable(apu-app src/main.cpp)

#target_sources(app PRIVATE src/main.c)
target_sources(apu-app PRIVATE src/acc.cpp)
target_sources(apu-app PRIVATE src/cmd.cpp)
target_sources(apu-app PRIVATE src/rpmsg.cpp)

target_sources(apu-app PRIVATE src/vm/print.cpp)
target_sources(apu-app PRIVATE src/vm/tf.cpp)
target_sources(apu-app PRIVATE src/vm/register.cpp)

target_sources(apu-app PRIVATE src/cmds/adm_acc_ctl.cpp)
target_sources(apu-app PRIVATE src/cmds/fw.cpp)
target_sources(apu-app PRIVATE src/cmds/identify.cpp)
target_sources(apu-app PRIVATE src/cmds/io_acc_ctl.cpp)
target_sources(apu-app PRIVATE src/cmds/lba.cpp)
target_sources(apu-app PRIVATE src/cmds/qspi.cpp)
target_sources(apu-app PRIVATE src/cmds/status.cpp)

include_directories(src/)

target_link_libraries(apu-app ${CMAKE_SOURCE_DIR}/src/libubpf.a)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(apu-app Threads::Threads)

set(TENSORFLOW_SOURCE_DIR "" CACHE PATH
  "Directory that contains the TensorFlow project" )
if(NOT TENSORFLOW_SOURCE_DIR)
  get_filename_component(TENSORFLOW_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../../../../" ABSOLUTE)
endif()


add_subdirectory(
  "${TENSORFLOW_SOURCE_DIR}/tensorflow/lite"
  "${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite" EXCLUDE_FROM_ALL)

target_link_libraries(apu-app tensorflow-lite)

add_library(custom-delegate SHARED
  src/tflite-delegate.cpp
  ${TENSORFLOW_SOURCE_DIR}/tensorflow/lite/delegates/utils/simple_delegate.cc
)
target_link_libraries(custom-delegate tensorflow-lite)

add_executable(tflite-delegate-test
  src/vta/sim_driver.cc
  src/vta/vta_runtime.cc
  src/vta/sim_tlpp.cc
  src/vta/virtual_memory.cc
  src/tflite-test.cpp
)
target_link_libraries(tflite-delegate-test custom-delegate tensorflow-lite)
