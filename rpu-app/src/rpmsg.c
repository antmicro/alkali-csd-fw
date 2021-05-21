#include "rpmsg.h"

#include <openamp/open_amp.h>
#include <metal/device.h>
#include <metal/sys.h>
#include <metal/log.h>
#include <metal/irq.h>
#include <drivers/ipm.h>
#include "platform_info.h"

#include "cmd.h"

#define RPMSG_SERVICE_NAME         "rpmsg-openamp-nvme-channel"

static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
		u32_t src, void *priv)
{
	nvme_rpmsg_payload_t *payload = (nvme_rpmsg_payload_t*)data;
	printk("id: %u, len: %u, priv: %08x\n", payload->id, payload->len, payload->priv);

	nvme_cmd_priv_t *cmd = (nvme_cmd_priv_t*)payload->priv;

	if(payload->id == RPMSG_SEND_COMPLETION)
		nvme_cmd_return(cmd);

	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	printk("rpmsg endpoint destroyed\n");
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
		printk("Mailbox binding failed!\n");
	}

	ipm_register_callback(tc->ipm_dev_rx, ipi_callback, tc->ipm_dev_rx);

	ret = platform_init(&tc->platform);
	if(ret) {
		printk("Failed to initialize platform!\n");
		return ret;
	}

	tc->rpdev = platform_create_rpmsg_vdev(tc->platform, 0,
			VIRTIO_DEV_SLAVE,
			NULL, NULL);

	ret = rpmsg_create_ept(&tc->lept, tc->rpdev, RPMSG_SERVICE_NAME,
			       0, RPMSG_ADDR_ANY, rpmsg_endpoint_cb,
			       rpmsg_service_unbind);

	if(ret)
		printk("Failed to create endpoint!\n");

	return ret;
}
