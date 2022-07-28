BUILD_DIR ?= ${PWD}/build

# buildroot

BUILDROOT_DIR = ${PWD}/third-party/buildroot
BUILDROOT_BUILD_DIR = ${BUILD_DIR}/buildroot

BUILDROOT_OPTS = O=${BUILDROOT_BUILD_DIR} -C ${BUILDROOT_DIR} BR2_EXTERNAL=${PWD}/br2-external
buildroot/all: ## build buildroot
	$(MAKE) ${BUILDROOT_OPTS} zynqmp_nvme_defconfig
	$(MAKE) ${BUILDROOT_OPTS} -j$(nproc)

buildroot/distclean: ## clean buildroot build
	$(MAKE) ${BUILDROOT_OPTS} distclean

buildroot/sdk: ${BUILDROOT_TOOLCHAIN_TAR_PATH} ## generate buildroot toolchain
buildroot/sdk-untar: ${BUILDROOT_TOOLCHAIN_DIR} ## untar buildroot toolchain

buildroot//%: ## forward rule to invoke buildroot rules directly e.g. `make buildroot//menuconfig
	$(MAKE) ${BUILDROOT_OPTS} $*

.PHONY: buildroot/all buildroot/distclean buildroot/sdk buildroot/sdk-untar

# buildroot toolchain

BUILDROOT_TOOLCHAIN_TAR_PATH = ${BUILDROOT_BUILD_DIR}/images/aarch64-buildroot-linux-gnu_sdk-buildroot.tar.gz
BUILDROOT_TOOLCHAIN_OUTPUT_DIR = ${BUILD_DIR}/aarch64-buildroot-linux-gnu_sdk-buildroot
BUILDROOT_TOOLCHAIN_CMAKE_FILE = ${BUILDROOT_TOOLCHAIN_OUTPUT_DIR}/share/buildroot/toolchainfile.cmake

${BUILDROOT_TOOLCHAIN_CMAKE_FILE}: ${BUILDROOT_TOOLCHAIN_TAR_PATH}
	tar mxf ${BUILDROOT_TOOLCHAIN_TAR_PATH} -C ${BUILD_DIR}

${BUILDROOT_TOOLCHAIN_TAR_PATH}:
	$(MAKE) ${BUILDROOT_OPTS} sdk

# apu app

APUAPP_SRC_DIR = apu-app/src
APUAPP_BUILD_DIR = build/apu-app
APUAPP_INSTALL_DIR = build/apu-app/install
APUAPP_OUTPUTS = ${APUAPP_BUILD_DIR}/libvta-delegate.so ${APUAPP_BUILD_DIR}/apu-app

apu-app/all: ${APUAPP_OUTPUTS} ## build apu app
apu-app/clean: ## clean apu app build artifacts
	-rm -rf ${APUAPP_BUILD_DIR}

.PHONY: apu-app

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
	make -C ${APUAPP_BUILD_DIR} -j all

# zephyr

ZEPHYR_SDK_VERSION=zephyr-sdk-0.14.2
ZEPHYR_SDK_DOWNLOAD_PATH = ${BUILD_DIR}/zephyr-sdk.tar.gz
ZEPHYR_SDK_DIR = ${BUILD_DIR}/${ZEPHYR_SDK_VERSION}

${ZEPHYR_SDK_DOWNLOAD_PATH}:
	@mkdir -p ${BUILD_DIR}
	wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.14.2/${ZEPHYR_SDK_VERSION}.2_linux-x86_64.tar.gz \
		-O ${ZEPHYR_SDK_DOWNLOAD_PATH}

${ZEPHYR_SDK_DIR}: ${ZEPHYR_SDK_DOWNLOAD_PATH}
	tar mxf ${ZEPHYR_SDK_DOWNLOAD_PATH} -C ${BUILD_DIR}

zephyr-sdk: ${ZEPHYR_SDK_DIR}
	bash ${ZEPHYR_SDK_DIR}/setup.sh

# help

HELP_COLUMN_SPAN = 20
HELP_FORMAT_STRING = "\033[36m%-${HELP_COLUMN_SPAN}s\033[0m %s\n"
help: ## show this help
	@echo Here is the list of available targets:
	@echo ""
	@grep -E '^[a-zA-Z_\/-\%]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf ${HELP_FORMAT_STRING}, $$1, $$2}'
	@echo ""

.PHONY: help
.DEFAULT_GOAL := help
