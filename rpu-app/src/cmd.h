/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NVME_CMD_H
#define NVME_CMD_H

#include <stdint.h>
#include <string.h>

#include "tc.h"

void nvme_cmd_handler(void *cmd_priv, void *cmd);

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

typedef struct nvme_sq_entry_vendor_base {
	nvme_sq_entry_base_t base;
	uint32_t ndt;
	uint32_t ndm;
} nvme_sq_entry_vendor_base_t;

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

#define NVME_ADM_CMD_GET_LOG		0x02
#define NVME_ADM_CMD_IDENTIFY		0x06
#define NVME_ADM_CMD_SET_FEATURES	0x09
#define NVME_ADM_CMD_CREATE_IO_SQ	0x01
#define NVME_ADM_CMD_CREATE_IO_CQ	0x05
#define NVME_ADM_CMD_DELETE_IO_SQ	0x00
#define NVME_ADM_CMD_DELETE_IO_CQ	0x04
#define NVME_ADM_CMD_FW_COMMIT		0x10
#define NVME_ADM_CMD_FW_DOWNLOAD	0x11
#define NVME_ADM_CMD_KEEP_ALIVE		0x18

#define NVME_ADM_CMD_VENDOR		0xC0

#define NVME_IO_CMD_FLUSH		0x00
#define NVME_IO_CMD_WRITE		0x01
#define NVME_IO_CMD_READ		0x02

#define NVME_IO_CMD_VENDOR		0x80

#define NVME_CMD_XFER_NONE		0x00
#define NVME_CMD_XFER_FROM_HOST		0x01
#define NVME_CMD_XFER_TO_HOST		0x02
#define NVME_CMD_XFER_BIDIR		0x03
#define NVME_CMD_XFER_MASK		0x03

int nvme_cmd_transfer_data(nvme_cmd_priv_t *priv);

void nvme_cmd_vendor(nvme_cmd_priv_t *priv, int zero_based);

void nvme_cmd_adm_identify(nvme_cmd_priv_t *priv);
void nvme_cmd_adm_get_log(nvme_cmd_priv_t *priv);
void nvme_cmd_adm_set_features(nvme_cmd_priv_t *priv);

void nvme_cmd_adm_create_sq(nvme_cmd_priv_t *priv);
void nvme_cmd_adm_create_cq(nvme_cmd_priv_t *priv);

void nvme_cmd_adm_delete_sq(nvme_cmd_priv_t *priv);
void nvme_cmd_adm_delete_cq(nvme_cmd_priv_t *priv);

void nvme_cmd_io_write(nvme_cmd_priv_t *priv);
void nvme_cmd_io_read(nvme_cmd_priv_t *priv);

void nvme_cmd_return(nvme_cmd_priv_t *priv);
void nvme_cmd_return_data(nvme_cmd_priv_t *priv, void *ret_buf, uint32_t ret_len);
void nvme_cmd_get_data(nvme_cmd_priv_t *priv, void *ret_buf, uint32_t ret_len);

#endif
