#include "cmd.h"

#include <string.h>
#include <stdint.h>
#include <assert.h>

#define IDENTIFY_STRUCT_BUF_SIZE 4096

#define BUFFER_SIZE (64*1024*1024)
#define ID_FW_EXCHANGE	0
#define ID_IN_BUFFER	1
#define ID_OUT_BUFFER	2

static unsigned char identify_struct[IDENTIFY_STRUCT_BUF_SIZE];

typedef struct ident_head {
	uint8_t magic[4];
	uint32_t len;
	uint8_t rsvd[8];
} ident_head_t;

typedef struct desc_list {
	uint32_t len;
	uint32_t id : 16;
	uint32_t rsvd : 16;
} desc_list_t;

typedef struct cap_entry {
	uint32_t id;
	uint32_t rsvd;
	uint64_t buf_size;
} cap_entry_t;

void setup_identify(void)
{
	int off;
	memset(identify_struct, 0x55, IDENTIFY_STRUCT_BUF_SIZE);

	assert(sizeof(ident_head_t) == 16);
	assert(sizeof(cap_entry_t) == 16);
	assert(sizeof(desc_list_t) == 8);

	ident_head_t head = {magic : {'W', 'D', 'C', '0'}};

	desc_list_t desc = {
		id : 0,
	};

	cap_entry_t cap = {
		id : 0,
	};

	off = sizeof(head) + sizeof(desc);

	cap.id = ID_FW_EXCHANGE;

	memcpy(&identify_struct[off], &cap, sizeof(cap));
	off += sizeof(cap);

	cap.id = ID_IN_BUFFER;
	cap.buf_size = BUFFER_SIZE;

	memcpy(&identify_struct[off], &cap, sizeof(cap));
	off += sizeof(cap);

	cap.id = ID_OUT_BUFFER;
	cap.buf_size = BUFFER_SIZE;

	memcpy(&identify_struct[off], &cap, sizeof(cap));
	off += sizeof(cap);

	head.len = off;

	memcpy(identify_struct, &head, sizeof(head));

	desc.len = off - sizeof(head);

	memcpy(&identify_struct[sizeof(head)], &desc, sizeof(desc));
}

void adm_cmd_identify(payload_t *recv, unsigned char *buf)
{
	memcpy(buf, identify_struct, (recv->buf_len > 4096) ? 4096 : recv->buf_len);
}
