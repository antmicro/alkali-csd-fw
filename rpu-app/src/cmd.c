/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd.h"
#include "dma.h"
#include "main.h"

#include <zephyr.h>
#include <sys/printk.h>
#include <openamp/open_amp.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(NVME_LOGGER_NAME, NVME_LOGGER_LEVEL);

static inline void fill_cq_resp(nvme_cmd_priv_t *priv)
{
	nvme_sq_entry_base_t *sq = (nvme_sq_entry_base_t*)priv->sq_buf;
	nvme_cq_entry_t *cq = (nvme_cq_entry_t*)&priv->cq_buf;

	memset((void*)cq, 0, NVME_TC_CQ_ENTRY_SIZE);
	cq->sq_head = priv->tc->sq_head[priv->qid];
	cq->cid = sq->cdw0.cid;
}

static void handle_adm(nvme_cmd_priv_t *priv)
{
	nvme_sq_entry_base_t *cmd = (nvme_sq_entry_base_t*)priv->sq_buf;

	// LOG_DBG("Admin command (Opcode: %d)", cmd->cdw0.opc);
	switch(cmd->cdw0.opc) {
		case NVME_ADM_CMD_GET_LOG:
			LOG_DBG("Handling NVME_ADM_CMD_GET_LOG");
			nvme_cmd_adm_get_log(priv);
			break;
		case NVME_ADM_CMD_IDENTIFY:
			LOG_DBG("Handling NVME_ADM_CMD_IDENTIFY");
			nvme_cmd_adm_identify(priv);
			break;
		case NVME_ADM_CMD_SET_FEATURES:
			LOG_DBG("Handling NVME_ADM_CMD_SET_FEATURES");
			nvme_cmd_adm_set_features(priv);
			break;
		case NVME_ADM_CMD_CREATE_IO_SQ:
			LOG_DBG("Handling NVME_ADM_CMD_CREATE_IO_SQ");
			nvme_cmd_adm_create_sq(priv);
			break;
		case NVME_ADM_CMD_CREATE_IO_CQ:
			LOG_DBG("Handling NVME_ADM_CMD_CREATE_IO_CQ");
			nvme_cmd_adm_create_cq(priv);
			break;
		case NVME_ADM_CMD_DELETE_IO_SQ:
			LOG_DBG("Handling NVME_ADM_CMD_DELETE_IO_SQ");
			nvme_cmd_adm_delete_sq(priv);
			break;
		case NVME_ADM_CMD_DELETE_IO_CQ:
			LOG_DBG("Handling NVME_ADM_CMD_DELETE_IO_CQ");
			nvme_cmd_adm_delete_cq(priv);
			break;
		case NVME_ADM_CMD_FW_COMMIT: // Vendor command layout is compatible with FW commands and in the end FW commands are handled by the APU
			LOG_DBG("Handling NVME_ADM_CMD_FW_COMMIT");
			nvme_cmd_vendor(priv, 0);
			break;
		case NVME_ADM_CMD_FW_DOWNLOAD:
			LOG_DBG("Handling NVME_ADM_CMD_GET_LOG");
			nvme_cmd_vendor(priv, 1);
			break;
		case NVME_ADM_CMD_KEEP_ALIVE:
			LOG_DBG("Handling NVME_ADM_CMD_FW_DOWNLOAD");
		default:
			if(cmd->cdw0.opc >= NVME_ADM_CMD_VENDOR) {
				LOG_DBG("Handling NVME_ADM_CMD_VENDOR");
				nvme_cmd_vendor(priv, 0);
				break;
			}
			LOG_ERR("Unsupported Admin command! (Opcode: %d)", cmd->cdw0.opc);
			nvme_cmd_return(priv);
	}
}

static void handle_io(nvme_cmd_priv_t *priv)
{
	nvme_sq_entry_base_t *cmd = (nvme_sq_entry_base_t*)priv->sq_buf;

	// LOD_DBG("IO command (Opcode: %d)", cmd->cdw0.opc);
	switch(cmd->cdw0.opc) {
		case NVME_IO_CMD_FLUSH:
			LOG_DBG("Handling NVME_IO_CMD_FLUSH");
			nvme_cmd_return(priv);
			break;
		case NVME_IO_CMD_WRITE:
			LOG_DBG("Handling NVME_IO_CMD_WRITE");
			nvme_cmd_io_write(priv);
			break;
		case NVME_IO_CMD_READ:
			LOG_DBG("Handling NVME_IO_CMD_READ");
			nvme_cmd_io_read(priv);
			break;
		default:
			if(cmd->cdw0.opc >= NVME_IO_CMD_VENDOR) {
				LOG_DBG("Handling NVME_IO_CMD_VENDOR");
				nvme_cmd_vendor(priv, 0);
				break;
			}
			LOG_ERR("Unsupported IO Command! (Opcode: %d)", cmd->cdw0.opc);
			nvme_cmd_return(priv);
	}
}

void nvme_cmd_handler(void *cmd_priv, void *buf)
{
	nvme_cmd_priv_t *priv = (nvme_cmd_priv_t*)cmd_priv;
	fill_cq_resp(priv);

	if(priv->qid == ADM_QUEUE_ID)
		handle_adm(priv);
	else
		handle_io(priv);
}

static void cq_cb(void *cmd_priv, void *buf)
{
	nvme_cmd_priv_t *priv = (nvme_cmd_priv_t*)cmd_priv;
	LOG_INF("Sending completion interrupt to host");
	nvme_tc_cq_notify(priv->tc, priv->qid);
	k_mem_slab_free(&priv->tc->cmd_slab, &cmd_priv);
}

void nvme_cmd_return_cb(void *cmd_priv, void *buf)
{
	nvme_cmd_priv_t *priv = (nvme_cmd_priv_t*)cmd_priv;
	nvme_cmd_return(priv);
}

void nvme_cmd_return(nvme_cmd_priv_t *priv)
{
	nvme_cq_entry_t *cq = (nvme_cq_entry_t*)&priv->cq_buf;

	uint64_t cq_addr = nvme_tc_get_cq_addr(priv->tc, priv->qid);

	if (!cq_addr) {
		LOG_ERR("Completion Queue host memory address is invalid!");
		return;
	}

	// We know the correct phase only after obtaining next CQ entry address
	cq->p = priv->tc->cq_phase[priv->qid];

	nvme_dma_xfer_mem_to_host(priv->tc->dma_priv, (uint32_t)cq, cq_addr, NVME_TC_CQ_ENTRY_SIZE, cq_cb, (void*)priv);
}

static void transfer_chunk(nvme_cmd_priv_t *priv, uint64_t host_addr, uint32_t local_addr, uint32_t len)
{
	nvme_dma_xfer_cb *cb = NULL;
	void *arg = NULL;

	// const uint32_t lo = host_addr & 0xffffffff;
	// const uint32_t hi = (host_addr >> 32) & 0xffffffff;
	// LOG_DBG("Transferring data, host: %08x %08x, local: %08x, len: %d", hi, lo, local_addr, len);

	if(priv->xfer_len == len) {
		cb = priv->xfer_cb;
		arg = priv;
	}

	if(priv->dir == DIR_TO_HOST) {
		nvme_dma_xfer_mem_to_host(priv->tc->dma_priv, local_addr, host_addr, len, cb, arg);
	} else {
		nvme_dma_xfer_host_to_mem(priv->tc->dma_priv, host_addr, local_addr, len, cb, arg);
	}

	priv->xfer_len -= len;
	priv->xfer_buf += len;
}

static uint32_t calc_prp_size(uint64_t base, uint32_t mps, uint32_t len)
{
	uint32_t total = ((len + mps - 1) / mps) * sizeof(uint64_t);
	uint32_t page = mps - (base % mps);
	return (page > total) ? total : page;
}

static void prp_cb(void *cmd_priv, void *buf)
{
	nvme_cmd_priv_t *priv = (nvme_cmd_priv_t*)cmd_priv;
	uint64_t *prp_list = (uint64_t*)buf;
	const uint32_t mps = priv->tc->memory_page_size;
	const int prp_last = (priv->prp_size / sizeof(uint64_t)) - 1;
	int i = 0;

	LOG_DBG("List of PRPs transferred");

	while(priv->xfer_len > 0) {
		const int xfer_len = (mps > priv->xfer_len) ? priv->xfer_len : mps;

		if((i == prp_last) && (priv->xfer_len > mps)) { // We need to fetch another PRP list
			priv->prp_size = calc_prp_size(prp_list[prp_last], mps, priv->xfer_len);
			nvme_dma_xfer_host_to_mem(priv->tc->dma_priv, prp_list[prp_last], (uint32_t)buf, priv->prp_size, prp_cb, priv);
			LOG_DBG("Fetching list of PRPs (%d bytes)", priv->prp_size);
			return;
		}

		transfer_chunk(priv, prp_list[i++], priv->xfer_buf, xfer_len);
	}

	LOG_DBG("Finished transferring %d pages", i);

	k_mem_slab_free(&priv->tc->prp_slab, &buf);
}

static void transfer_data_with_prps(nvme_cmd_priv_t *priv)
{
	nvme_sq_entry_base_t *cmd = (nvme_sq_entry_base_t*)priv->sq_buf;
	uint64_t host_addr = cmd->dptr.prp.prp1;
	const int mps = priv->tc->memory_page_size;
	const int off = host_addr % mps;

	uint32_t xfer_len = ((mps - off) > priv->xfer_len) ? priv->xfer_len : (mps - off);

	transfer_chunk(priv, host_addr, priv->xfer_buf, xfer_len);

	if(priv->xfer_len == 0) {
		return;
	}

	if(priv->xfer_len > mps) { // We need to fetch list of PRPs
		void *prp_buf;
		if(k_mem_slab_alloc(&priv->tc->prp_slab, (void**)&prp_buf, K_NO_WAIT) == 0) {
			priv->prp_size = calc_prp_size(cmd->dptr.prp.prp2, mps, priv->xfer_len);
			nvme_dma_xfer_host_to_mem(priv->tc->dma_priv, cmd->dptr.prp.prp2, (uint32_t)prp_buf, priv->prp_size, prp_cb, priv);
			LOG_DBG("Fetching list of PRPs (%d bytes)", priv->prp_size);
			return;
		} else {
			LOG_ERR("Failed to allocate PRP buffer!");
			nvme_cmd_return(priv);
			return;
		}
	} else { // Second PRP is in PRP2
		host_addr = cmd->dptr.prp.prp2;
		xfer_len = priv->xfer_len;
		transfer_chunk(priv, host_addr, priv->xfer_buf, xfer_len);
		return;
	}
}

int nvme_cmd_transfer_data(nvme_cmd_priv_t *priv)
{
	nvme_sq_entry_base_t *cmd = (nvme_sq_entry_base_t*)priv->sq_buf;
	uint8_t psdt = cmd->cdw0.psdt;

	if(psdt != 0) {
		LOG_ERR("Invalid PSDT value! (%d != 0)", psdt);
		return 1;
	} else {
		transfer_data_with_prps(priv);
		return 0;
	}

}

void nvme_cmd_get_data(nvme_cmd_priv_t *priv, void *ret_buf, uint32_t ret_len)
{
	priv->xfer_base = priv->xfer_buf = (uint32_t)ret_buf;
	priv->xfer_size = priv->xfer_len = ret_len;

	priv->dir = DIR_FROM_HOST;
	priv->xfer_cb = nvme_cmd_return_cb;

	if(nvme_cmd_transfer_data(priv))
		nvme_cmd_return(priv);
}

void nvme_cmd_return_data(nvme_cmd_priv_t *priv, void *ret_buf, uint32_t ret_len)
{
	priv->xfer_base = priv->xfer_buf = (uint32_t)ret_buf;
	priv->xfer_size = priv->xfer_len = ret_len;

	priv->dir = DIR_TO_HOST;
	priv->xfer_cb = nvme_cmd_return_cb;

	if(nvme_cmd_transfer_data(priv))
		nvme_cmd_return(priv);
}
