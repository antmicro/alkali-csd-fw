/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// // pynq
#define VTA_COHERENT_ACCESSES false
#define VTA_COMPUTE_ADDR 0xB0002000     ///< Address in mmaped memory with VTA pointing to compute data for compute module
#define VTA_COMPUTE_BIAS_ADDR_OFFSET 40 ///< Offset from VTA_COMPUTE_ADDR pointing to place where address to bias vector will be stored
#define VTA_COMPUTE_DONE_RD_OFFSET 24
#define VTA_COMPUTE_DONE_WR_OFFSET 16
#define VTA_COMPUTE_UOP_ADDR_OFFSET 32
#define VTA_FETCH_ADDR 0xB0000000       ///< Address in mmapped memory with VTA pointing to place for fetch instructions
#define VTA_FETCH_INSN_ADDR_OFFSET 24   ///< Offset from VTA_FETCH_ADDR pointing to place where address to instructions will be stored
#define VTA_FETCH_INSN_COUNT_OFFSET 16  ///< Offset from VTA_FETCH_ADDR pointing to place where instruction count will be stored
#define VTA_HW_VER 0.0.2                ///< HW design version
#define VTA_IP_REG_MAP_RANGE 0x1000     ///< Size of the MMap register used for communication with VTA
#define VTA_LOAD_ADDR 0xB0001000        ///< Load module address
#define VTA_LOAD_INP_ADDR_OFFSET 16     ///< Input address offset
#define VTA_LOAD_WGT_ADDR_OFFSET 24     ///< Weights' address offset
#define VTA_LOG_ACC_BUFF_SIZE 17        ///< Accumulator on-chip buffer size in bytes
#define VTA_LOG_ACC_WIDTH 5             ///< Accumulator data type signed integer width
#define VTA_LOG_BATCH 0                 ///< Batch size
#define VTA_LOG_BLOCK 4                 ///< GEMM inner dimensions
#define VTA_LOG_BLOCK_IN 4              ///< Input tensor width (channels)
#define VTA_LOG_BLOCK_OUT 4             ///< Output tensor width (channels)
#define VTA_LOG_BUS_WIDTH 6             ///< Memory bus width
#define VTA_LOG_INP_BUFF_SIZE 15        ///< Input on-chip buffer size in bytes
#define VTA_LOG_INP_WIDTH 3             ///< Input data type signed integer width
#define VTA_LOG_OUT_BUFF_SIZE 15        ///< Output on-chip buffer size in bytes
#define VTA_LOG_OUT_WIDTH 3             ///< Output data type signed integer width
#define VTA_LOG_UOP_BUFF_SIZE 15        ///< Micro-op on-chip buffer size in bytes
#define VTA_LOG_WGT_BUFF_SIZE 18        ///< Weights' on-chip buffer size in bytes
#define VTA_LOG_WGT_WIDTH 3             ///< Weights' data signed integer width
#define VTA_STORE_ADDR 0xB0003000       ///< Store memory address
#define VTA_STORE_OUT_ADDR_OFFSET 16    ///< Store address offset

// sim
// #define VTA_COHERENT_ACCESSES true
// #define VTA_COMPUTE_ADDR 0x43C02000
// #define VTA_COMPUTE_BIAS_ADDR_OFFSET 40
// #define VTA_COMPUTE_DONE_RD_OFFSET 24
// #define VTA_COMPUTE_DONE_WR_OFFSET 16
// #define VTA_COMPUTE_UOP_ADDR_OFFSET 32
// #define VTA_FETCH_ADDR 0x43C00000
// #define VTA_FETCH_INSN_ADDR_OFFSET 24
// #define VTA_FETCH_INSN_COUNT_OFFSET 16
// #define VTA_HW_VER 0.0.2
// #define VTA_IP_REG_MAP_RANGE 0x1000
// #define VTA_LOAD_ADDR 0x43C01000
// #define VTA_LOAD_INP_ADDR_OFFSET 16
// #define VTA_LOAD_WGT_ADDR_OFFSET 24
// #define VTA_LOG_ACC_BUFF_SIZE 17
// #define VTA_LOG_ACC_WIDTH 5
// #define VTA_LOG_BATCH 0
// #define VTA_LOG_BLOCK 4
// #define VTA_LOG_BLOCK_IN 4
// #define VTA_LOG_BLOCK_OUT 4
// #define VTA_LOG_BUS_WIDTH 6
// #define VTA_LOG_INP_BUFF_SIZE 15
// #define VTA_LOG_INP_WIDTH 3
// #define VTA_LOG_OUT_BUFF_SIZE 15
// #define VTA_LOG_OUT_WIDTH 3
// #define VTA_LOG_UOP_BUFF_SIZE 15
// #define VTA_LOG_WGT_BUFF_SIZE 18
// #define VTA_LOG_WGT_WIDTH 3
// #define VTA_STORE_ADDR 0x43C03000
// #define VTA_STORE_OUT_ADDR_OFFSET 16
