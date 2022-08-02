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
target_include_directories(app PRIVATE ${GENERATED_DIR})

find_package( PythonInterp REQUIRED )

add_custom_target(
    generate_headers ALL
    COMMAND ${PYTHON_EXECUTABLE} ${REGGEN_DIR}/get_reg_fields.py
            ${NVME_SPEC_FILE} -f ${RPUAPP_GENERATED_DIR}/registers.json

    COMMAND ${PYTHON_EXECUTABLE} ${REGGEN_DIR}/get_reg_fields_zephyr.py
            ${RPUAPP_GENERATED_DIR}/registers.json -f ${RPUAPP_GENERATED_DIR}/nvme_reg_fields.h

    COMMAND ${PYTHON_EXECUTABLE} ${REGGEN_DIR}/get_reg_map_zephyr.py
            ${NVME_SPEC_FILE} -f ${RPUAPP_GENERATED_DIR}/nvme_reg_map.h

    COMMAND ${PYTHON_EXECUTABLE} ${REGGEN_DIR}/get_identify_struct.py
            ${NVME_SPEC_FILE} -f ${RPUAPP_GENERATED_DIR}/nvme_ident_fields.h

    BYPRODUCTS ${RPUAPP_GENERATED_DIR}/registers.json
                ${RPUAPP_GENERATED_DIR}/nvme_reg_fields.h
                ${RPUAPP_GENERATED_DIR}/nvme_reg_map.h
                ${RPUAPP_GENERATED_DIR}/nvme_ident_fields.h

    COMMENT "Generating NVMe headers"
)

add_dependencies(app generate_headers)
