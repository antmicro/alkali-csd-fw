#ifndef NVME_CMD_H
#define NVME_CMD_H

#include <stdint.h>
#include <string.h>

#include "tc.h"

void nvme_cmd_handle_adm(void *tc_priv, void *cmd);
void nvme_cmd_handle_io(void *tc_priv, void *cmd);

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

#define NVME_ADM_CMD_GET_LOG		0x02
#define NVME_ADM_CMD_IDENTIFY		0x06
#define NVME_ADM_CMD_SET_FEATURES	0x09
#define NVME_ADM_CMD_CREATE_IO_SQ	0x01
#define NVME_ADM_CMD_CREATE_IO_CQ	0x05
#define NVME_ADM_CMD_KEEP_ALIVE		0x18

void nvme_cmd_adm_identify(nvme_tc_priv_t *tc, void *buf, nvme_cq_entry_t *cq_buf);
void nvme_cmd_adm_get_log(nvme_tc_priv_t *tc, void *buf, nvme_cq_entry_t *cq_buf);
void nvme_cmd_adm_set_features(nvme_tc_priv_t *tc, void *buf, nvme_cq_entry_t *cq_buf);
void nvme_cmd_adm_create_sq(nvme_tc_priv_t *tc, void *buf, nvme_cq_entry_t *cq_buf);
void nvme_cmd_adm_create_cq(nvme_tc_priv_t *tc, void *buf, nvme_cq_entry_t *cq_buf);

void nvme_cmd_return(nvme_tc_priv_t *tc, nvme_sq_entry_base_t *cmd, nvme_cq_entry_t *cq_buf);
void nvme_cmd_return_data(nvme_tc_priv_t *tc, nvme_sq_entry_base_t *cmd, void *ret_buf, uint32_t ret_len, nvme_cq_entry_t *cq_buf);

#endif
