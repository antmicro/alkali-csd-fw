/*
 * Copyright 2021-2022 Western Digital Corporation or its affiliates
 * Copyright 2021-2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rpmsg.h"
#include "main.h"
#include "cmd.h"

#include <openamp/open_amp.h>
#include <metal/device.h>
#include <metal/sys.h>
#include <metal/log.h>
#include <metal/irq.h>
#include <drivers/ipm.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(NVME_LOGGER_NAME, NVME_LOGGER_LEVEL);

#define RPMSG_SERVICE_NAME         "rpmsg-openamp-nvme-channel"

static void rpmsg_cmd_return_cb(void *cmd_priv, void *buf)
{
	nvme_cmd_priv_t *priv = (nvme_cmd_priv_t*)cmd_priv;

	k_mem_pool_free(&priv->block);

	nvme_cmd_return(priv);
}

static void rpmsg_cmd_return_data(nvme_cmd_priv_t *priv, void *ret_buf, uint32_t ret_len)
{
	nvme_sq_entry_base_t *cmd = (nvme_sq_entry_base_t*)priv->sq_buf;
	uint8_t psdt = cmd->cdw0.psdt;

	priv->xfer_base = priv->xfer_buf = (uint32_t)ret_buf;
	priv->xfer_size = priv->xfer_len = ret_len;

	priv->dir = DIR_TO_HOST;
	priv->xfer_cb = rpmsg_cmd_return_cb;

	if(psdt != 0) {
		LOG_ERR("Invalid PSDT value! (%d != 0)", psdt);
		nvme_cmd_return(priv);
	} else {
		nvme_cmd_transfer_data(priv);
	}
}

static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
		u32_t src, void *priv)
{
	nvme_rpmsg_payload_t *payload = (nvme_rpmsg_payload_t*)data;
	LOG_INF("id: %x, len: %u, priv: %08x", payload->id, payload->len, payload->priv);

	nvme_cmd_priv_t *cmd = (nvme_cmd_priv_t*)payload->priv;

	switch(payload->id) {
		case RPMSG_CMD_RETURN:
			LOG_DBG("Handling RPMSG_CMD_RETURN");
			nvme_cmd_return(cmd);
			if(cmd->block.data)
				k_mem_pool_free(&cmd->block);
			break;
		case RPMSG_CMD_RETURN_DATA:
			LOG_DBG("Handling RPMSG_CMD_RETURN_DATA");
			rpmsg_cmd_return_data(cmd, payload->buf, payload->buf_len);
			break;
		default:
			LOG_ERR("Unsupported payload ID! (%d) received from APU", payload->id);
	}

	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	LOG_INF("rpmsg endpoint destroyed");
}

static void ipi_callback(void *context, u32_t cpu_id,
		volatile void *data)
{
	irq_handler(cpu_id);
}

int rpmsg_init(nvme_tc_priv_t *tc)
{
	int ret;

	/* Initialize HW system components */
	struct metal_init_params metal_param = METAL_INIT_DEFAULTS;

	/* Low level abstraction layer for openamp initialization */
	metal_init(&metal_param);

	/* mailbox init */
	tc->ipm_dev_tx = device_get_binding("MAILBOX_0");
	tc->ipm_dev_rx = device_get_binding("MAILBOX_1");

	if (!tc->ipm_dev_tx || !tc->ipm_dev_rx) {
		LOG_ERR("Mailbox binding failed!");
	}

	ipm_register_callback(tc->ipm_dev_rx, ipi_callback, tc->ipm_dev_rx);

	ret = platform_init(&tc->platform);
	if(ret) {
		LOG_ERR("Failed to initialize platform!");
		return ret;
	}

	tc->rpdev = platform_create_rpmsg_vdev(tc->platform, 0,
			VIRTIO_DEV_SLAVE,
			NULL, NULL);

	ret = rpmsg_create_ept(&tc->lept, tc->rpdev, RPMSG_SERVICE_NAME,
			       0, RPMSG_ADDR_ANY, rpmsg_endpoint_cb,
			       rpmsg_service_unbind);

	if (ret)
		LOG_ERR("Failed to create endpoint!");

	return ret;
}
