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
APUAPP_OUTPUTS = ${APUAPP_BUILD_DIR}/libvta-delegate.so

apu-app/all: ${APUAPP_OUTPUTS} ## build apu app
apu-app/clean: ## clean apu app build artifacts
	-rm -r ${APUAPP_BUILD_DIR}

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
	      -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
	      -DCMAKE_BUILD_TYPE=Debug \
	      -DNO_HARDWARE=ON \
	      -DBUILD_TESTS=ON \
	      -DUBPF_ENABLE_INSTALL=ON \
	      -S apu-app -B ${APUAPP_BUILD_DIR}
	make -C ${APUAPP_BUILD_DIR} -j$(nproc) all

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
