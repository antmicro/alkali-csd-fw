#include "cmd.h"
#include "dma.h"

#include <zephyr.h>
#include <sys/printk.h>

static inline void fill_cq_resp(nvme_cmd_priv_t *priv) /*nvme_cq_entry_t *cq_buf, uint16_t sq_head, uint16_t cmd_id) */
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

	switch(cmd->cdw0.opc) {
		case NVME_ADM_CMD_GET_LOG:
			nvme_cmd_adm_get_log(priv);
			break;
		case NVME_ADM_CMD_IDENTIFY:
			nvme_cmd_adm_identify(priv);
			break;
		case NVME_ADM_CMD_SET_FEATURES:
			nvme_cmd_adm_set_features(priv);
			break;
		case NVME_ADM_CMD_CREATE_IO_SQ:
			nvme_cmd_adm_create_sq(priv);
			break;
		case NVME_ADM_CMD_CREATE_IO_CQ:
			nvme_cmd_adm_create_cq(priv);
			break;
		case NVME_ADM_CMD_DELETE_IO_SQ:
			nvme_cmd_adm_delete_sq(priv);
			break;
		case NVME_ADM_CMD_DELETE_IO_CQ:
			nvme_cmd_adm_delete_cq(priv);
			break;
		case NVME_ADM_CMD_KEEP_ALIVE:
		default:
			printk("Unsupported Admin command! (Opcode: %d)\n", cmd->cdw0.opc);
			nvme_cmd_return(priv);
	}	
}

static void handle_io(nvme_cmd_priv_t *priv)
{
	printk("IO Command\n");
	nvme_cmd_return(priv);
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
	printk("Sending completion interrupt to host\n");
	nvme_tc_cq_notify(priv->tc, priv->qid);
	k_mem_slab_free(&priv->tc->cmd_slab, &cmd_priv);
}

void nvme_cmd_return(nvme_cmd_priv_t *priv)
{
	nvme_cq_entry_t *cq = (nvme_cq_entry_t*)&priv->cq_buf;

	uint64_t cq_addr = nvme_tc_get_cq_addr(priv->tc, priv->qid);

	if(!cq_addr) {
		printk("%s: Completion Queue host memory address is invalid!\n", __func__);
		return;
	}

	// We know the correct phase only after obtaining next CQ entry address
	cq->p = priv->tc->cq_phase[priv->qid];

	nvme_dma_xfer_mem_to_host(priv->tc->dma_priv, (uint32_t)cq, cq_addr, NVME_TC_CQ_ENTRY_SIZE, cq_cb, (void*)priv);
}

void nvme_cmd_return_data(nvme_cmd_priv_t *priv, void *ret_buf, uint32_t ret_len)
{
	nvme_sq_entry_base_t *cmd = (nvme_sq_entry_base_t*)priv->sq_buf;
	uint64_t ret_addr = cmd->dptr.prp.prp1;

	if(!ret_addr) {
		printk("%s: Return value host memory address is a invalid!\n", __func__);
		return;
	}

	nvme_dma_xfer_mem_to_host(priv->tc->dma_priv, (uint32_t)ret_buf, ret_addr, ret_len, NULL, NULL); // We don't need to do anything after transferring data, all will happen in the CQ transfer callback
	nvme_cmd_return(priv);
}
