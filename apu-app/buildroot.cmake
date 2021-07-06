set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_SYSROOT ${CMAKE_CURRENT_LIST_DIR}/../buildroot/output/host/aarch64-buildroot-linux-gnu/sysroot)
set(CMAKE_STAGING_PREFIX ${CMAKE_CURRENT_LIST_DIR}/../stage)

set(CMAKE_C_COMPILER ${CMAKE_CURRENT_LIST_DIR}/../buildroot/output/host/bin/aarch64-buildroot-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER ${CMAKE_CURRENT_LIST_DIR}/../buildroot/output/host/bin/aarch64-buildroot-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
