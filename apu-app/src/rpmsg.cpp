/*
Software License Agreement (BSD License)
========================================

Copyright (c) 2014, Mentor Graphics Corporation. All rights reserved.
Copyright (c) 2015 - 2016 Xilinx, Inc. All rights reserved.
Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. The names of its contributors may not be used to endorse or promote
   products derived from this software without specific prior written
   permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <linux/rpmsg.h>
#include <spdlog/spdlog.h>

#define DEV_NAME "virtio0.rpmsg-openamp-nvme-channel.-1.0"
#define EPT_NAME "rpmsg-openamp-nvme-channel"
#define RPMSG_BUS_SYS "/sys/bus/rpmsg"

static int rpmsg_create_ept(int rpfd, struct rpmsg_endpoint_info *eptinfo)
{
	int ret;

	ret = ioctl(rpfd, RPMSG_CREATE_EPT_IOCTL, eptinfo);
	if (ret)
		spdlog::error("Failed to create endpoint.");
	return ret;
}

static char *get_rpmsg_ept_dev_name(const char *rpmsg_char_name,
				    const char *ept_name,
				    char *ept_dev_name)
{
	char sys_rpmsg_ept_name_path[64];
	char svc_name[64];
	const char *sys_rpmsg_path = "/sys/class/rpmsg";
	FILE *fp;
	int i;
	int ept_name_len;

	for (i = 0; i < 128; i++) {
		sprintf(sys_rpmsg_ept_name_path, "%s/%s/rpmsg%d/name",
			sys_rpmsg_path, rpmsg_char_name, i);
		spdlog::info("checking {}", sys_rpmsg_ept_name_path);
		if (access(sys_rpmsg_ept_name_path, F_OK) < 0)
			continue;
		fp = fopen(sys_rpmsg_ept_name_path, "r");
		if (!fp) {
			spdlog::error("failed to open {}", sys_rpmsg_ept_name_path);
			break;
		}
		fgets(svc_name, sizeof(svc_name), fp);
		fclose(fp);
		spdlog::debug("svc_name: {}.",svc_name);
		ept_name_len = strlen(ept_name);
		if (ept_name_len > (int)sizeof(svc_name))
			ept_name_len = sizeof(svc_name);
		if (!strncmp(svc_name, ept_name, ept_name_len)) {
			sprintf(ept_dev_name, "rpmsg%d", i);
			return ept_dev_name;
		}
	}

	spdlog::warn("not able to rpmsg endpoint file for {}:{}.",
	       rpmsg_char_name, ept_name);
	return NULL;
}

static int bind_rpmsg_chrdev(const char *rpmsg_dev_name)
{
	char fpath[256];
	const char *rpmsg_chdrv = "rpmsg_chrdev";
	int fd;
	int ret;


	/* rpmsg dev overrides path */
	sprintf(fpath, "%s/devices/%s/driver_override",
		RPMSG_BUS_SYS, rpmsg_dev_name);
	fd = open(fpath, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s, %s\n",
			fpath, strerror(errno));
		return -EINVAL;
	}
	ret = write(fd, rpmsg_chdrv, strlen(rpmsg_chdrv) + 1);
	if (ret < 0) {
		fprintf(stderr, "Failed to write %s to %s, %s\n",
			rpmsg_chdrv, fpath, strerror(errno));
		return -EINVAL;
	}
	close(fd);

	/* bind the rpmsg device to rpmsg char driver */
	sprintf(fpath, "%s/drivers/%s/bind", RPMSG_BUS_SYS, rpmsg_chdrv);
	fd = open(fpath, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s, %s\n",
			fpath, strerror(errno));
		return -EINVAL;
	}
	ret = write(fd, rpmsg_dev_name, strlen(rpmsg_dev_name) + 1);
	if (ret < 0) {
		fprintf(stderr, "Failed to write %s to %s, %s\n",
			rpmsg_dev_name, fpath, strerror(errno));
		return -EINVAL;
	}
	close(fd);
	return 0;
}

static int get_rpmsg_chrdev_fd(const char *rpmsg_dev_name,
			       char *rpmsg_ctrl_name)
{
	char dpath[256];
	char fpath[384];
	const char *rpmsg_ctrl_prefix = "rpmsg_ctrl";
	DIR *dir;
	struct dirent *ent;
	int fd;

	sprintf(dpath, "%s/devices/%s/rpmsg", RPMSG_BUS_SYS, rpmsg_dev_name);
	dir = opendir(dpath);
	if (dir == NULL) {
		fprintf(stderr, "Failed to open dir %s\n", dpath);
		return -EINVAL;
	}
	while ((ent = readdir(dir)) != NULL) {
		if (!strncmp(ent->d_name, rpmsg_ctrl_prefix,
			    strlen(rpmsg_ctrl_prefix))) {
			spdlog::debug("Opening file {}.", ent->d_name);
			sprintf(fpath, "/dev/%s", ent->d_name);
			fd = open(fpath, O_RDWR | O_NONBLOCK);
			if (fd < 0) {
				fprintf(stderr,
					"Failed to open rpmsg char dev %s,%s\n",
					fpath, strerror(errno));
				return fd;
			}
			sprintf(rpmsg_ctrl_name, "%s", ent->d_name);
			return fd;
		}
	}

	fprintf(stderr, "No rpmsg char dev file is found\n");
	return -EINVAL;
}

int rpmsg_init()
{
	struct rpmsg_endpoint_info eptinfo;
	char rpmsg_char_name[16];
	char ept_dev_name[16];
	char ept_dev_path[32];
	char fpath[256];
	int charfd;
	int ret;
	int fd;

	spdlog::debug("rpmsg initialization start");

	sprintf(fpath, "%s/devices/%s", RPMSG_BUS_SYS, DEV_NAME);
	if (access(fpath, F_OK)) {
		fprintf(stderr, "Not able to access rpmsg device %s, %s\n",
			fpath, strerror(errno));
		return -EINVAL;
	}

	ret = bind_rpmsg_chrdev(DEV_NAME);
	if (ret < 0)
		return ret;

	charfd = get_rpmsg_chrdev_fd(DEV_NAME, rpmsg_char_name);
	if (charfd < 0)
		return charfd;

	/* Create endpoint from rpmsg char driver */
	strcpy(eptinfo.name, "rpmsg-openamp-nvme-channel");
	eptinfo.src = 0;
	eptinfo.dst = 1;
	ret = rpmsg_create_ept(charfd, &eptinfo);
	if (ret) {
		spdlog::error("failed to create RPMsg endpoint.");
		return -EINVAL;
	}
	if (!get_rpmsg_ept_dev_name(rpmsg_char_name, eptinfo.name,
				    ept_dev_name))
		return -EINVAL;
	sprintf(ept_dev_path, "/dev/%s", ept_dev_name);
	fd = open(ept_dev_path, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror("Failed to open rpmsg device.");
		close(charfd);
		return -1;
	}

	return fd;
}
