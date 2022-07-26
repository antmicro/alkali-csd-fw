BUILD_DIR ?= ${PWD}/build

BUILDROOT_DIR = ${PWD}/third-party/buildroot
BUILDROOT_BUILD_DIR = ${BUILD_DIR}/buildroot
LINUX_DIR = ${PWD}/third-party/linux
LINUX_BUILD_DIR = ${BUILD_DIR}/linux

BUILDROOT_OPTS = O=${BUILDROOT_BUILD_DIR} -C ${BUILDROOT_DIR} BR2_EXTERNAL=${PWD}/br2-external
buildroot: ## build buildroot
	make ${BUILDROOT_OPTS} zynqmp_nvme_defconfig
	make ${BUILDROOT_OPTS} -j$(nproc)

buildroot-menuconfig:
	make ${BUILDROOT_OPTS} menuconfig

linux: ## build linux
	make O=${LINUX_BUILD_DIR} -C ${LINUX_DIR} zynqmp_nvme_defconfig
	make O=${LINUX_BUILD_DIR} -C ${LINUX_DIR} make -j$(nproc)
	make O=${LINUX_BUILD_DIR} -C ${LINUX_DIR} make modules -j$(nproc)
	make O=${LINUX_BUILD_DIR} -C ${LINUX_DIR} make dtbs -j$(nproc)


HELP_COLUMN_SPAN = 20
HELP_FORMAT_STRING = "\033[36m%-${HELP_COLUMN_SPAN}s\033[0m %s\n"
help: ## show this help
	@echo Here is the list of available targets:
	@echo ""
	@grep -E '^[a-zA-Z_\/-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf ${HELP_FORMAT_STRING}, $$1, $$2}'
	@echo ""

.PHONY: help
.DEFAULT_GOAL := help

