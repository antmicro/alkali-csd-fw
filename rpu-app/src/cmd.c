#include "cmd.h"
#include "dma.h"

#include <zephyr.h>
#include <sys/printk.h>

static inline void fill_cq_resp(nvme_cq_entry_t *cq_buf, uint16_t sq_head, uint16_t cmd_id)
{
	memset((void*)cq_buf, 0, NVME_TC_ADM_CQ_ENTRY_SIZE);

	cq_buf->sq_head = sq_head;
	cq_buf->sq_id = 0;

	cq_buf->cid = cmd_id;

	cq_buf->sc = 0;
	cq_buf->sct = 0;

	cq_buf->crd = 0;
	cq_buf->m = 0;
	cq_buf->dnr = 0;
}

static void dump_sq_entry(void *buf)
{
	uint32_t *cmd = (uint32_t*)buf;

	printk("SQ Entry:\n");
	
	for(int i = 0; i < NVME_TC_ADM_SQ_ENTRY_SIZE/4; i++)
		printk("CDW%d: %08x\n", i, cmd[i]);
}

void nvme_cmd_handle_adm(void *tc_priv, void *buf)
{
	nvme_sq_entry_base_t *cmd = (nvme_sq_entry_base_t*)buf;
	nvme_cq_entry_t *cq;
	nvme_tc_priv_t *tc = (nvme_tc_priv_t*)tc_priv;

	dump_sq_entry(buf);

	if(k_mem_slab_alloc(&tc->adm_cq_slab, (void**)&cq, K_NO_WAIT) == 0) {
		fill_cq_resp(cq, tc->adm_sq_head, cmd->cdw0.cid);
	} else {
		printk("Failed to allocate completion queue entry!\n");
		goto cleanup;
	}

	switch(cmd->cdw0.opc) {
		case NVME_ADM_CMD_GET_LOG:
			nvme_cmd_adm_get_log(tc, buf, cq);
			break;
		case NVME_ADM_CMD_IDENTIFY:
			nvme_cmd_adm_identify(tc, buf, cq);
			break;
		case NVME_ADM_CMD_SET_FEATURES:
			nvme_cmd_adm_set_features(tc, buf, cq);
			break;
		case NVME_ADM_CMD_CREATE_IO_SQ:
			nvme_cmd_adm_create_sq(tc, buf, cq);
			break;
		case NVME_ADM_CMD_CREATE_IO_CQ:
			nvme_cmd_adm_create_cq(tc, buf, cq);
			break;
		case NVME_ADM_CMD_KEEP_ALIVE:
		default:
			printk("Unsupported Admin command! (Opcode: %d)\n", cmd->cdw0.opc);
	}

cleanup:
	k_mem_slab_free(&tc->adm_sq_slab, &buf);
}

static void adm_cq_cb(void *tc_priv, void *buf)
{
	nvme_tc_priv_t *tc = (nvme_tc_priv_t*)tc_priv;
	printk("Sending completion interrupt to host\n");
	nvme_tc_cq_notify(tc, ADM_QUEUE_ID);
	k_mem_slab_free(&tc->adm_cq_slab, &buf);
}

void nvme_cmd_return(nvme_tc_priv_t *tc, nvme_sq_entry_base_t *cmd, nvme_cq_entry_t *cq_buf)
{
	uint64_t cq_addr = nvme_tc_get_cq_addr(tc);

	if(!cq_addr) {
		printk("%s: Completion Queue host memory address is invalid!\n", __func__);
		return;
	}

	// We know the correct phase only after obtaining next CQ entry address
	cq_buf->p = tc->adm_cq_phase;

	nvme_dma_xfer_mem_to_host(tc->dma_priv, (uint32_t)cq_buf, cq_addr, NVME_TC_ADM_CQ_ENTRY_SIZE, adm_cq_cb, (void*)tc);
}

void nvme_cmd_return_data(nvme_tc_priv_t *tc, nvme_sq_entry_base_t *cmd, void *ret_buf, uint32_t ret_len, nvme_cq_entry_t *cq_buf)
{
	uint64_t ret_addr = cmd->dptr.prp.prp1;

	if(!ret_addr) {
		printk("%s: Return value host memory address is a invalid!\n", __func__);
		return;
	}

	nvme_dma_xfer_mem_to_host(tc->dma_priv, (uint32_t)ret_buf, ret_addr, ret_len, NULL, NULL); // We don't need to do anything after transferring data, all will happen in the CQ transfer callback
	nvme_cmd_return(tc, cmd, cq_buf);
}
