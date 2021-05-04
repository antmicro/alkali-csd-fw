#include "cmd.h"
#include "dma.h"

#include <zephyr.h>
#include <sys/printk.h>

void nvme_cmd_handle_adm(void *tc_priv, void *buf)
{
	nvme_sq_entry_base_t *cmd = (nvme_sq_entry_base_t*)buf;
	nvme_tc_priv_t *tc = (nvme_tc_priv_t*)tc_priv;

	switch(cmd->cdw0.opc) {
		case NVME_ADM_CMD_IDENTIFY:
			nvme_cmd_adm_identify(tc, buf);
			break;
		case NVME_ADM_CMD_SET_FEATURES:
		case NVME_ADM_CMD_CREATE_IO_SQ:
		case NVME_ADM_CMD_CREATE_IO_CQ:
		case NVME_ADM_CMD_KEEP_ALIVE:
		default:
			printk("Unsupported Admin command! (Opcode: %d)\n", cmd->cdw0.opc);
	}

	k_mem_slab_free(&tc->adm_sq_slab, &buf);
}

static void adm_cq_cb(void *tc_priv, void *buf)
{
	nvme_tc_priv_t *tc = (nvme_tc_priv_t*)tc_priv;
	printk("Sending completion interrupt to host\n");
	nvme_tc_cq_notify(tc, ADM_QUEUE_ID);
}

void nvme_cmd_return_data(nvme_tc_priv_t *tc, nvme_sq_entry_base_t *cmd, void *ret_buf, uint32_t ret_len, volatile nvme_cq_entry_t *cq_buf)
{
	uint64_t ret_addr = cmd->dptr.prp.prp1;
	uint64_t cq_addr = nvme_tc_get_cq_addr(tc);

	if(!ret_addr) {
		printk("%s: Return value host memory address is a invalid!\n", __func__);
		return;
	}

	if(!cq_addr) {
		printk("%s: Completion Queue host memory address is invalid!\n", __func__);
		return;
	}

	// We know the correct phase only after obtaining next CQ entry address
	cq_buf->p = tc->adm_cq_phase;

	__DMB();

	nvme_dma_xfer_mem_to_host(tc->dma_priv, (uint32_t)ret_buf, ret_addr, ret_len, NULL, NULL); // We don't need to do anything after transferring data, all will happen in the CQ transfer callback

	nvme_dma_xfer_mem_to_host(tc->dma_priv, (uint32_t)cq_buf, cq_addr, NVME_TC_ADM_CQ_ENTRY_SIZE, adm_cq_cb, (void*)tc);
}
