/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NVME_H
#define NVME_H

#include <stdint.h>

#define NVME_TC_SQ_ENTRY_SIZE	64
#define NVME_TC_CQ_ENTRY_SIZE	16

typedef struct nvme_cmd_cdw0 {
	uint32_t opc : 8;
	uint32_t fuse : 2;
	uint32_t rsvd : 4;
	uint32_t psdt : 2;
	uint32_t cid : 16;
} nvme_cmd_cdw0_t;

typedef struct nvme_cmd_prp {
	uint64_t prp1;
	uint64_t prp2;
} nvme_cmd_prp_t;

typedef struct __attribute__((packed)) nvme_cmd_sgl {
	uint8_t sgl_desc_type : 4;
	uint8_t sgl_desc_sub_type : 4;
	uint8_t rsvd[15];
} nvme_cmd_sgl_t;

typedef union nvme_cmd_dptr {
	nvme_cmd_prp_t prp;
	nvme_cmd_sgl_t sgl;
} nvme_cmd_dptr_t;

typedef struct nvme_sq_entry_base {
	nvme_cmd_cdw0_t cdw0;
	uint32_t nsid;
	uint32_t rsvd[2];
	uint64_t mptr;
	nvme_cmd_dptr_t dptr;
} nvme_sq_entry_base_t;

typedef struct nvme_cq_entry {
	uint32_t cdw0;
	uint32_t rsvd;
	uint32_t sq_head : 16;
	uint32_t sq_id : 16;
	uint32_t cid : 16;
	uint32_t p : 1;
	uint32_t sc : 8;
	uint32_t sct : 3;
	uint32_t crd : 2;
	uint32_t m : 1;
	uint32_t dnr : 1;
} nvme_cq_entry_t;

#define CMD_ADM_IDENTIFY	0xC2
#define CMD_ADM_STATUS		0xC6
#define CMD_ADM_CTL		0xC0

#define CMD_IO_SEND_DATA	0x81
#define CMD_IO_READ_DATA	0x82

#define CMD_IO_SEND_FW		0x85
#define CMD_IO_READ_FW		0x86

#define CMD_IO_READ_LBA		0x88
#define CMD_IO_WRITE_LBA	0x8c

#define CMD_IO_CTL		0x91

#endif
