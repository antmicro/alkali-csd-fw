BUILD_DIR ?= ${PWD}/build

all: buildroot apu-app rpu-app ## build all binaries (buildroot, apu-app, rpu-app)

clean: ## clean build artifacts
	-rm -rf build

APUAPP_BUILD_DIR = ${BUILD_DIR}/apu-app

# buildroot

BUILDROOT_DIR = ${PWD}/third-party/buildroot
BUILDROOT_BUILD_DIR = ${BUILD_DIR}/buildroot
BUILDROOT_OPTS = O=${BUILDROOT_BUILD_DIR} -C ${BUILDROOT_DIR} BR2_EXTERNAL=${PWD}/br2-external

BUILDROOT_TOOLCHAIN_TAR_PATH = ${BUILDROOT_BUILD_DIR}/images/aarch64-buildroot-linux-gnu_sdk-buildroot.tar.gz
BUILDROOT_TOOLCHAIN_OUTPUT_DIR = ${BUILD_DIR}/aarch64-buildroot-linux-gnu_sdk-buildroot
BUILDROOT_TOOLCHAIN_CMAKE_FILE = ${BUILDROOT_TOOLCHAIN_OUTPUT_DIR}/share/buildroot/toolchainfile.cmake

buildroot: apu-app ## build buildroot
	cp ${APUAPP_BUILD_DIR}/libvta-delegate.so ${PWD}/br2-external/board/basalt/overlay/lib/libvta-delegate.so
	cp ${APUAPP_BUILD_DIR}/apu-app ${PWD}/br2-external/board/basalt/overlay/bin/apu-app
	$(MAKE) ${BUILDROOT_OPTS} zynqmp_nvme_defconfig
	$(MAKE) ${BUILDROOT_OPTS} -j$(nproc)

buildroot/distclean: ## clean buildroot build
	$(MAKE) ${BUILDROOT_OPTS} distclean

buildroot/sdk: ${BUILDROOT_TOOLCHAIN_TAR_PATH} ## generate buildroot toolchain
buildroot/sdk-untar: ${BUILDROOT_TOOLCHAIN_DIR} ## untar buildroot toolchain

buildroot//%: ## forward rule to invoke buildroot rules directly e.g. `make buildroot//menuconfig
	$(MAKE) ${BUILDROOT_OPTS} $*

.PHONY: buildroot buildroot/distclean buildroot/sdk buildroot/sdk-untar

${BUILDROOT_TOOLCHAIN_CMAKE_FILE}: ${BUILDROOT_TOOLCHAIN_TAR_PATH}
	tar mxf ${BUILDROOT_TOOLCHAIN_TAR_PATH} -C ${BUILD_DIR}

${BUILDROOT_TOOLCHAIN_TAR_PATH}:
	$(MAKE) ${BUILDROOT_OPTS} zynqmp_nvme_defconfig
	$(MAKE) ${BUILDROOT_OPTS} sdk

# apu app

APUAPP_SRC_DIR = apu-app/src
APUAPP_INSTALL_DIR = ${BUILD_DIR}/apu-app/install
APUAPP_OUTPUTS = ${APUAPP_BUILD_DIR}/libvta-delegate.so ${APUAPP_BUILD_DIR}/apu-app

apu-app: ${APUAPP_OUTPUTS} ## build apu app

apu-app/clean: ## clean apu-app build artifacts
	-rm -rf ${APUAPP_BUILD_DIR}

.PHONY: apu-app apu-app/clean

${APUAPP_OUTPUTS}: ${BUILDROOT_TOOLCHAIN_CMAKE_FILE}
${APUAPP_OUTPUTS}: $(wildcard ${APUAPP_SRC_DIR}/*.cpp)
${APUAPP_OUTPUTS}: $(wildcard ${APUAPP_SRC_DIR}/*.hpp)
${APUAPP_OUTPUTS}: $(wildcard ${APUAPP_SRC_DIR}/*.h)
${APUAPP_OUTPUTS}: $(wildcard ${APUAPP_SRC_DIR}/vm/*.cpp)
${APUAPP_OUTPUTS}: $(wildcard ${APUAPP_SRC_DIR}/cmd/*.cpp)
${APUAPP_OUTPUTS}: $(wildcard ${APUAPP_SRC_DIR}/vta/*.cc)
${APUAPP_OUTPUTS}: $(wildcard ${APUAPP_SRC_DIR}/vta/*.cpp)
${APUAPP_OUTPUTS}: $(wildcard ${APUAPP_SRC_DIR}/vta/*.h)
${APUAPP_OUTPUTS}: $(wildcard ${APUAPP_SRC_DIR}/vta/*.hpp)
	@mkdir -p ${APUAPP_BUILD_DIR}
	cmake \
	      -DCMAKE_TOOLCHAIN_FILE=${BUILDROOT_TOOLCHAIN_CMAKE_FILE} \
	      -DCMAKE_INSTALL_PREFIX=${APUAPP_INSTALL_DIR} \
	      -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
	      -DUBPF_ENABLE_INSTALL=ON \
	      -DCMAKE_BUILD_TYPE=Debug \
	      -DNO_HARDWARE=OFF \
	      -DBUILD_TESTS=ON \
	      -S apu-app -B ${APUAPP_BUILD_DIR}
	$(MAKE) -C ${APUAPP_BUILD_DIR} -j all

# zephyr

ZEPHYR_SDK_VERSION=zephyr-sdk-0.10.3
ZEPHYR_SDK_DOWNLOAD_URL=https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.10.3/zephyr-sdk-0.10.3-setup.run
ZEPHYR_SDK_DOWNLOAD_PATH=${BUILD_DIR}/zephyr-sdk.run
ZEPHYR_SDK_INSTALL_DIR=${BUILD_DIR}/${ZEPHYR_SDK_VERSION}

ZEPHYR_PROJECTS= \
	${BUILD_DIR}/zephyr \
	${BUILD_DIR}/modules/hal/libmetal \
	${BUILD_DIR}/modules build/tools \
	${BUILD_DIR}/modules/hal/atmel \
	${BUILD_DIR}/modules/lib/civetweb \
	${BUILD_DIR}/modules/hal/esp-idf \
	${BUILD_DIR}/modules/fs/fatfs \
	${BUILD_DIR}/modules/hal/cypress \
	${BUILD_DIR}/modules/hal/nordic \
	${BUILD_DIR}/modules/hal/openisa \
	${BUILD_DIR}/modules/hal/microchip \
	${BUILD_DIR}/modules/hal/silabs \
	${BUILD_DIR}/modules/hal/st \
	${BUILD_DIR}/modules/hal/stm32 \
	${BUILD_DIR}/modules/hal/ti \
	${BUILD_DIR}/modules/lib/gui/lvgl \
	${BUILD_DIR}/modules/crypto/mbedtls \
	${BUILD_DIR}/modules/lib/mcumgr \
	${BUILD_DIR}/modules/fs/nffs \
	${BUILD_DIR}/modules/hal/nxp \
	${BUILD_DIR}/modules/lib/open-amp \
	${BUILD_DIR}/modules/lib/openthread \
	${BUILD_DIR}/modules/debug/segger \
	${BUILD_DIR}/modules/lib/tinycbor \
	${BUILD_DIR}/modules/fs/littlefs \
	${BUILD_DIR}/modules/debug/mipi-sys-t

zephyr/sdk: ${ZEPHYR_SDK_INSTALL_DIR} ## install Zephyr SDK locally (helper)
	@echo "To use local installation of the toolchain set the following environment variables:"
	@echo "  - ZEPHYR_TOOLCHAIN_VARIANT=zephyr"
	@echo "  - ZEPHYR_SDK_INSTALL_DIR=${ZEPHYR_SDK_INSTALL_DIR}"

zephyr/setup: ${ZEPHYR_PROJECTS} ## clone main zephyr repositories and modules

zephyr/deps: ${ZEPHYR_PROJECTS}
	pip3 install -r ${BUILD_DIR}/zephyr/scripts/requirements.txt

.PHONY: zephyr/sdk zephyr/setup zephyr/deps

${ZEPHYR_SDK_DOWNLOAD_PATH}:
	@mkdir -p ${BUILD_DIR}
	wget -q ${ZEPHYR_SDK_DOWNLOAD_URL} -O ${ZEPHYR_SDK_DOWNLOAD_PATH}

${ZEPHYR_SDK_INSTALL_DIR}: ${ZEPHYR_SDK_DOWNLOAD_PATH}
	chmod u+rwx ${ZEPHYR_SDK_DOWNLOAD_PATH}
	bash ${ZEPHYR_SDK_DOWNLOAD_PATH} --quiet -- -d ${ZEPHYR_SDK_INSTALL_DIR}

${ZEPHYR_PROJECTS}: .west/config rpu-app/west.yml
	bash -c "for i in {1..5}; do west update && break || sleep 1; done"

# rpu-app

RPUAPP_BUILD_DIR=${BUILD_DIR}/rpu-app
RPUAPP_BUILD_PATH=${RPUAPP_BUILD_DIR}/zephyr/zephyr.elf

rpu-app: ${RPUAPP_BUILD_PATH} ## build rpu-app

IN_SDK_ENV = \
	source ${BUILD_DIR}/zephyr/zephyr-env.sh && \
	export ZEPHYR_TOOLCHAIN_VARIANT=zephyr && \
	export ZEPHYR_SDK_INSTALL_DIR=${ZEPHYR_SDK_INSTALL_DIR}
rpu-app/with-sdk: SHELL:=/bin/bash
rpu-app/with-sdk: zephyr/deps zephyr/sdk zephyr/setup  ## build rpu-app with local Zephyr SDK (helper)
	${IN_SDK_ENV} && west build -b zcu106 -d ${RPUAPP_BUILD_DIR} rpu-app

IN_ZEPHYR_ENV = source ${BUILD_DIR}/zephyr/zephyr-env.sh
${RPUAPP_BUILD_PATH}: SHELL:=/bin/bash
${RPUAPP_BUILD_PATH}: ${ZEPHYR_PROJECTS}
	$(IN_ZEPHYR_ENV) && west build -b zcu106 -d ${RPUAPP_BUILD_DIR} rpu-app

# help

HELP_COLUMN_SPAN = 20
HELP_FORMAT_STRING = "\033[36m%-${HELP_COLUMN_SPAN}s\033[0m %s\n"
help: ## show this help
	@echo Here is the list of available targets:
	@echo ""
	@grep -E '^[^#[:blank:]]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf ${HELP_FORMAT_STRING}, $$1, $$2}'
	@echo ""

.PHONY: help
.DEFAULT_GOAL := help
