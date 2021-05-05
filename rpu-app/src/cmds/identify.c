#include "../cmd.h"
#include "../nvme_ident_fields.h"

#include <zephyr.h>
#include <sys/printk.h>

#include <string.h>

typedef struct cmd_cdw10 {
	uint32_t cns : 8;
	uint32_t rsvd : 8;
	uint32_t cntid : 16;
} cmd_cdw10_t;

typedef struct cmd_cdw11 {
	uint32_t nvmsetid : 16;
	uint32_t rsvd : 16;
} cmd_cdw11_t;

typedef struct cmd_cdw14 {
	uint32_t uuid_idx : 7;
	uint32_t rsvd : 25;
} cmd_cdw14_t;

typedef struct cmd_sq {
	nvme_sq_entry_base_t base;
	cmd_cdw10_t cdw10;
	cmd_cdw11_t cdw11;
	uint32_t rsvd[2];
	cmd_cdw14_t cdw14;
} cmd_sq_t;

#define NVME_CMD_IDENTIFY_RESP_SIZE	4096

#define CNS_IDENTIFY_CONTROLLER		0x01

#define VID 0x1b96
#define SSVID VID
#define SN "0123456789"
#define MN "DEADBEEF"
#define FR "12.34"

#define OUI0 0x00
#define OUI1 0x90
#define OUI2 0xA9

#define VER 0x00010304

#define IO_CNTRL 1

#define SUBNQN "NVMe Open Source Controller"

static inline void clear_buf(uint8_t *buf, int len)
{
	for(int i = 0; i < len; i++)
		buf[i] = 0;
}

static void fill_identify_struct(uint8_t *ptr)
{
	mem_addr_t buf = (mem_addr_t)ptr;
	clear_buf(ptr, NVME_CMD_IDENTIFY_RESP_SIZE);

	sys_write16(VID, buf + NVME_ID_FIELD_VID);
	sys_write16(SSVID, buf + NVME_ID_FIELD_SSVID);

	strncat(ptr + NVME_ID_FIELD_SN, SN, NVME_ID_FIELD_SN_SIZE-1);
	strncat(ptr + NVME_ID_FIELD_MN, MN, NVME_ID_FIELD_MN_SIZE-1);
	strncat(ptr + NVME_ID_FIELD_FR, FR, NVME_ID_FIELD_FR_SIZE-1);

	sys_write8(0, buf + NVME_ID_FIELD_RAB);	

	sys_write16(OUI0, buf + NVME_ID_FIELD_IEEE);
	sys_write16(OUI1, buf + NVME_ID_FIELD_IEEE + 2);
	sys_write16(OUI2, buf + NVME_ID_FIELD_IEEE + 4);

	sys_write8(0, buf + NVME_ID_FIELD_MDTS);
	sys_write16(0, buf + NVME_ID_FIELD_CNTLID);

	sys_write32(VER, buf + NVME_ID_FIELD_VER);
	
	sys_write32(0, buf + NVME_ID_FIELD_RTD3R);
	sys_write32(0, buf + NVME_ID_FIELD_RTD3E);

	sys_write16(0, buf + NVME_ID_FIELD_OAES);	

	sys_write32(0, buf + NVME_ID_FIELD_CTRATT);

	sys_write8(IO_CNTRL, buf + NVME_ID_FIELD_CNTRLTYPE);

	sys_write32(0, buf + NVME_ID_FIELD_OACS);

	sys_write8(3, buf + NVME_ID_FIELD_ACL);
	sys_write8(3, buf + NVME_ID_FIELD_AERL);

	sys_write8(3, buf + NVME_ID_FIELD_FRMW);

	sys_write8(0, buf + NVME_ID_FIELD_LPA);

	sys_write8(0, buf + NVME_ID_FIELD_ELPE);

	sys_write8(0, buf + NVME_ID_FIELD_NPSS);

	sys_write8(1, buf + NVME_ID_FIELD_AVSCC);

	sys_write16(0x0157, buf + NVME_ID_FIELD_WCTEMP);

	sys_write16(0x0300, buf + NVME_ID_FIELD_CCTEMP);

	sys_write8(1, buf + NVME_ID_FIELD_FWUG);

	sys_write8(0, buf + NVME_ID_FIELD_KAS);

	sys_write8(0x66, buf + NVME_ID_FIELD_SQES);

	sys_write8(0x44, buf + NVME_ID_FIELD_CQES);

	sys_write8(0, buf + NVME_ID_FIELD_MAXCMD);

	sys_write32(0, buf + NVME_ID_FIELD_NN);

	sys_write16(0, buf + NVME_ID_FIELD_ONCS);

	sys_write16(0, buf + NVME_ID_FIELD_FUSES);
	
	sys_write8(1, buf + NVME_ID_FIELD_FNA);

	sys_write8(0, buf + NVME_ID_FIELD_VWC);

	sys_write16(0xFFFF, buf + NVME_ID_FIELD_AWUN);

	sys_write16(0xFFFF, buf + NVME_ID_FIELD_AWUPF);

	sys_write8(1, buf + NVME_ID_FIELD_NVSCC);

	sys_write8(0, buf + NVME_ID_FIELD_NWPC);

	strncat(ptr + NVME_ID_FIELD_SUBNQN, SUBNQN, NVME_ID_FIELD_SUBNQN_SIZE-1);

	__DMB();
}

static void fill_cq_resp(volatile nvme_cq_entry_t *cq_buf, uint16_t sq_head, uint16_t cmd_id)
{
	clear_buf((uint8_t*)cq_buf, NVME_TC_ADM_CQ_ENTRY_SIZE);

	cq_buf->sq_head = sq_head;
	cq_buf->sq_id = 0;

	cq_buf->cid = cmd_id;

	cq_buf->sc = 0;
	cq_buf->sct = 0;

	cq_buf->crd = 0;
	cq_buf->m = 0;
	cq_buf->dnr = 0;

	__DMB();
}

static void identify_controller(nvme_tc_priv_t *tc, cmd_sq_t *cmd)
{
	static uint8_t resp_buf[NVME_CMD_IDENTIFY_RESP_SIZE];
	volatile nvme_cq_entry_t *cq_buf;

	if(k_mem_slab_alloc(&tc->adm_cq_slab, (void**)&cq_buf, K_NO_WAIT) == 0) {

		fill_identify_struct(resp_buf);
		fill_cq_resp(cq_buf, tc->adm_sq_head, cmd->base.cdw0.cid);

		nvme_cmd_return_data(tc, &cmd->base, resp_buf, NVME_CMD_IDENTIFY_RESP_SIZE, cq_buf);
	}
}

void nvme_cmd_adm_identify(nvme_tc_priv_t *tc, void *buf)
{
	cmd_sq_t *cmd = (cmd_sq_t*)buf;	

	switch(cmd->cdw10.cns) {
		case CNS_IDENTIFY_CONTROLLER:
			identify_controller(tc, cmd);
			break;
		default:
			printk("Invalid Identify CNS value! (%d)\n", cmd->cdw10.cns);
	}
}
