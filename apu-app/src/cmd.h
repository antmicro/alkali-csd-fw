#ifndef CMD_H
#define CMD_H

#include "rpmsg.h"

#define DEBUG

extern bool global_enable;

int mmap_buffer(uint32_t base, uint32_t len, int *fd, unsigned char **buf);
void mmap_cleanup(uint32_t len, int fd, unsigned char *buf);

void send_ack(int fd, payload_t *data, uint32_t id);
void handle_adm_cmd(int fd, payload_t *recv);
void handle_io_cmd(int fd, payload_t *recv);

void adm_cmd_identify(payload_t *recv, unsigned char *buf);
void adm_cmd_acc_ctl(payload_t *recv);
void adm_cmd_status(payload_t *recv, unsigned char *buf);

void adm_cmd_fw_commit(payload_t *recv);
void adm_cmd_fw_download(payload_t *recv, unsigned char *buf);

void setup_identify(void);
void setup_status(void);

void io_cmd_send_fw(payload_t *recv, unsigned char *buf);
void io_cmd_read_lba(payload_t *recv);
void io_cmd_write_lba(payload_t *recv);
void io_cmd_acc_ctl(payload_t *recv);

#endif
