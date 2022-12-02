/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd.h"
#include "main.h"
#include "rpmsg.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(NVME_LOGGER_NAME, NVME_LOGGER_LEVEL);

static int send_cmd(nvme_cmd_priv_t *priv)
{
	nvme_sq_entry_vendor_base_t *cmd = (nvme_sq_entry_vendor_base_t*)priv->sq_buf;
	const int msg_size = sizeof(nvme_rpmsg_payload_t) + sizeof(priv->sq_buf);
	nvme_rpmsg_payload_t *msg = (nvme_rpmsg_payload_t*)k_malloc(msg_size);
	const int buffer_size = cmd->ndt * 4;

	if(msg == NULL) {
		LOG_ERR("Failed to allocate rpmsg buffer!");
		// TODO: send response stating that comand failed
		return -1;
	}

	msg->id = (priv->qid > 0) ? RPMSG_HANDLE_CUSTOM_IO_COMMAND : RPMSG_HANDLE_CUSTOM_ADM_COMMAND;
	msg->len = sizeof(priv->sq_buf);
	msg->priv = (uint32_t)priv;
	msg->buf = (uint32_t)priv->block.data;
	msg->buf_len = buffer_size;

	memcpy(msg->data, priv->sq_buf, msg->len);

	LOG_DBG("vendor buffer %x, %d", msg->buf, msg->buf_len);

	int ret = rpmsg_send(&priv->tc->lept, msg, msg_size);
	if(ret != msg_size) {
		LOG_ERR("Failed to send rpmsg message: %d", ret);
		k_free(msg);
		return -1;
	}

	k_free(msg);

	return 0;
}

static void vendor_cb(void *cmd_priv, void *buf)
{
	nvme_cmd_priv_t *priv = (nvme_cmd_priv_t*)cmd_priv;

	send_cmd(priv);
}

void nvme_cmd_vendor(nvme_cmd_priv_t *priv, int zero_based)
{
	nvme_sq_entry_vendor_base_t *cmd = (nvme_sq_entry_vendor_base_t*)priv->sq_buf;
	const uint8_t opc = cmd->base.cdw0.opc;
	const int dir = opc & NVME_CMD_XFER_MASK;

	if(zero_based)
		cmd->ndt++;

	const int buffer_size = cmd->ndt * 4;

	LOG_INF("Vendor %s command (Opcode: %d, priv: %08x)", (priv->qid > 0) ? "IO" : "Admin", opc, (uint32_t)priv);

	if((dir != NVME_CMD_XFER_NONE) && (buffer_size > 0)) {
		int ret = k_mem_pool_alloc(priv->tc->buffer_pool, &priv->block, buffer_size, 0);
		if(ret) {
			LOG_ERR("Failed to allocate vendor command data buffer! (size: %d, ret: %d)", buffer_size, ret);
			// TODO: send response stating that comand failed
			return;
		}

	}

	if(buffer_size == 0 || dir == NVME_CMD_XFER_TO_HOST) { // No data transfer from host required, we can send rpmsg now
		send_cmd(priv);
	} else {
		priv->xfer_base = priv->xfer_buf = (uint32_t)priv->block.data;
		priv->xfer_size = priv->xfer_len = buffer_size;

		priv->dir = DIR_FROM_HOST;
		priv->xfer_cb = vendor_cb;

		nvme_cmd_transfer_data(priv);
	}
}
