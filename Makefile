# Helper macros ---------------------------------------------------------------
ROOT_DIR = $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
NVME_SPEC_NAME = NVM-Express-1_4-2019.06.10-Ratified.pdf
NVME_SPEC_FILE = $(REGGEN_DIR)/$(NVME_SPEC_NAME)

# Build directories -----------------------------------------------------------
BUILD_DIR ?= $(ROOT_DIR)/build
APUAPP_BUILD_DIR = $(BUILD_DIR)/apu-app
RPUAPP_BUILD_DIR = $(BUILD_DIR)/rpu-app
BUILDROOT_BUILD_DIR = $(BUILD_DIR)/buildroot

# Helper directories ----------------------------------------------------------
THIRD_PARTY_DIR = $(ROOT_DIR)/third-party
BUILDROOT_DIR = $(PWD)/third-party/buildroot
REGGEN_DIR = $(ROOT_DIR)/third-party/registers-generator


# -----------------------------------------------------------------------------
# All -------------------------------------------------------------------------
# -----------------------------------------------------------------------------
.PHONY: all
all: buildroot apu-app rpu-app ## build all binaries (buildroot, apu-app, rpu-app)


# -----------------------------------------------------------------------------
# Clean -----------------------------------------------------------------------
# -----------------------------------------------------------------------------
.PHONY: clean
clean: ## clean build artifacts
	$(RM) -r build


# -----------------------------------------------------------------------------
# Buildroot -------------------------------------------------------------------
# -----------------------------------------------------------------------------
BUILDROOT_OPTS = O=$(BUILDROOT_BUILD_DIR) -C $(BUILDROOT_DIR) BR2_EXTERNAL=$(PWD)/br2-external
BUILDROOT_TOOLCHAIN_TAR_FILE = $(BUILDROOT_BUILD_DIR)/images/aarch64-buildroot-linux-gnu_sdk-buildroot.tar.gz
BUILDROOT_TOOLCHAIN_OUTPUT_DIR = $(BUILD_DIR)/aarch64-buildroot-linux-gnu_sdk-buildroot
BUILDROOT_TOOLCHAIN_CMAKE_FILE = $(BUILDROOT_TOOLCHAIN_OUTPUT_DIR)/share/buildroot/toolchainfile.cmake

# Buildroot rules -------------------------------------------------------------
.PHONY: buildroot
buildroot: apu-app ## build buildroot
	cp $(APUAPP_BUILD_DIR)/libvta-delegate.so $(PWD)/br2-external/board/basalt/overlay/lib/libvta-delegate.so
	cp $(APUAPP_BUILD_DIR)/apu-app $(PWD)/br2-external/board/basalt/overlay/bin/apu-app
	$(MAKE) $(BUILDROOT_OPTS) zynqmp_nvme_defconfig
	$(MAKE) $(BUILDROOT_OPTS) -j$(nproc)

.PHONY: buildroot/distclean
buildroot/distclean: ## clean buildroot build
	$(MAKE) $(BUILDROOT_OPTS) distclean

.PHONY: buildroot/sdk
buildroot/sdk: $(BUILDROOT_TOOLCHAIN_TAR_FILE) ## generate buildroot toolchain

.PHONY: buildroot/sdk-untar
buildroot/sdk-untar: $(BUILDROOT_TOOLCHAIN_DIR) ## untar buildroot toolchain

.PHONY: buildroot//%
buildroot//%: ## forward rule to invoke buildroot rules directly e.g. `make buildroot//menuconfig
	$(MAKE) $(BUILDROOT_OPTS) $*

# Buildroot dependencies-------------------------------------------------------
$(BUILDROOT_TOOLCHAIN_CMAKE_FILE): $(BUILDROOT_TOOLCHAIN_TAR_FILE)
	tar mxf $(BUILDROOT_TOOLCHAIN_TAR_FILE) -C $(BUILD_DIR)

$(BUILDROOT_TOOLCHAIN_TAR_FILE):
	$(MAKE) $(BUILDROOT_OPTS) zynqmp_nvme_defconfig
	$(MAKE) $(BUILDROOT_OPTS) sdk


# -----------------------------------------------------------------------------
# APU App ---------------------------------------------------------------------
# -----------------------------------------------------------------------------
APUAPP_SRC_DIR = apu-app/src
APUAPP_INSTALL_DIR = $(BUILD_DIR)/apu-app/install
APUAPP_OUTPUTS = $(APUAPP_BUILD_DIR)/libvta-delegate.so $(APUAPP_BUILD_DIR)/apu-app

# APU App rules ---------------------------------------------------------------
.PHONY: apu-app
apu-app: $(APUAPP_OUTPUTS) ## build apu app

.PHONY: apu-app/clean
apu-app/clean: ## clean apu-app build artifacts
	$(RM) -r $(APUAPP_BUILD_DIR)

# APU App dependencies --------------------------------------------------------
$(APUAPP_OUTPUTS): $(BUILDROOT_TOOLCHAIN_CMAKE_FILE)
$(APUAPP_OUTPUTS): $(wildcard $(APUAPP_SRC_DIR)/*.cpp)
$(APUAPP_OUTPUTS): $(wildcard $(APUAPP_SRC_DIR)/*.hpp)
$(APUAPP_OUTPUTS): $(wildcard $(APUAPP_SRC_DIR)/*.h)
$(APUAPP_OUTPUTS): $(wildcard $(APUAPP_SRC_DIR)/vm/*.cpp)
$(APUAPP_OUTPUTS): $(wildcard $(APUAPP_SRC_DIR)/cmd/*.cpp)
$(APUAPP_OUTPUTS): $(wildcard $(APUAPP_SRC_DIR)/vta/*.cc)
$(APUAPP_OUTPUTS): $(wildcard $(APUAPP_SRC_DIR)/vta/*.cpp)
$(APUAPP_OUTPUTS): $(wildcard $(APUAPP_SRC_DIR)/vta/*.h)
$(APUAPP_OUTPUTS): $(wildcard $(APUAPP_SRC_DIR)/vta/*.hpp)
	@mkdir -p $(APUAPP_BUILD_DIR)
	cmake \
	      -DCMAKE_TOOLCHAIN_FILE=$(BUILDROOT_TOOLCHAIN_CMAKE_FILE) \
	      -DCMAKE_INSTALL_PREFIX=$(APUAPP_INSTALL_DIR) \
	      -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
	      -DUBPF_ENABLE_INSTALL=ON \
	      -DCMAKE_BUILD_TYPE=Debug \
	      -DNO_HARDWARE=OFF \
	      -DBUILD_TESTS=ON \
	      -S apu-app -B $(APUAPP_BUILD_DIR)
	$(MAKE) -C $(APUAPP_BUILD_DIR) -j all


# -----------------------------------------------------------------------------
# zephyr ----------------------------------------------------------------------
# -----------------------------------------------------------------------------
ZEPHYR_SDK_VERSION = zephyr-sdk-0.10.3
ZEPHYR_SDK_DOWNLOAD_URL = https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.10.3/zephyr-sdk-0.10.3-setup.run
ZEPHYR_SDK_DOWNLOAD_PATH = $(BUILD_DIR)/zephyr-sdk.run
ZEPHYR_SDK_INSTALL_DIR = $(BUILD_DIR)/$(ZEPHYR_SDK_VERSION)

ZEPHYR_SOURCES= \
	$(BUILD_DIR)/zephyr \
	$(BUILD_DIR)/modules/hal/libmetal \
	$(BUILD_DIR)/modules build/tools \
	$(BUILD_DIR)/modules/hal/atmel \
	$(BUILD_DIR)/modules/lib/civetweb \
	$(BUILD_DIR)/modules/hal/esp-idf \
	$(BUILD_DIR)/modules/fs/fatfs \
	$(BUILD_DIR)/modules/hal/cypress \
	$(BUILD_DIR)/modules/hal/nordic \
	$(BUILD_DIR)/modules/hal/openisa \
	$(BUILD_DIR)/modules/hal/microchip \
	$(BUILD_DIR)/modules/hal/silabs \
	$(BUILD_DIR)/modules/hal/st \
	$(BUILD_DIR)/modules/hal/stm32 \
	$(BUILD_DIR)/modules/hal/ti \
	$(BUILD_DIR)/modules/lib/gui/lvgl \
	$(BUILD_DIR)/modules/crypto/mbedtls \
	$(BUILD_DIR)/modules/lib/mcumgr \
	$(BUILD_DIR)/modules/fs/nffs \
	$(BUILD_DIR)/modules/hal/nxp \
	$(BUILD_DIR)/modules/lib/open-amp \
	$(BUILD_DIR)/modules/lib/openthread \
	$(BUILD_DIR)/modules/debug/segger \
	$(BUILD_DIR)/modules/lib/tinycbor \
	$(BUILD_DIR)/modules/fs/littlefs \
	$(BUILD_DIR)/modules/debug/mipi-sys-t

# Zephyr rules ----------------------------------------------------------------
.PHONY: zephyr/sdk
zephyr/sdk: $(ZEPHYR_SDK_INSTALL_DIR) ## install Zephyr SDK locally (helper)
	@echo "To use local installation of the toolchain set the following environment variables:"
	@echo "  - ZEPHYR_TOOLCHAIN_VARIANT=zephyr"
	@echo "  - ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK_INSTALL_DIR)"

.PHONY: zephyr/setup
zephyr/setup: $(ZEPHYR_SOURCES) ## clone main zephyr repositories and modules

.PHONY: zephyr/deps
zephyr/deps: $(ZEPHYR_SOURCES)
	pip3 install -r $(BUILD_DIR)/zephyr/scripts/requirements.txt

.PHONY: zephyr/clean
zephyr/clean:
	$(RM) -r build/zephyr*

# Zephyr dependencies ---------------------------------------------------------
$(ZEPHYR_SDK_DOWNLOAD_PATH):
	@mkdir -p $(BUILD_DIR)
	wget -q $(ZEPHYR_SDK_DOWNLOAD_URL) -O $(ZEPHYR_SDK_DOWNLOAD_PATH)

$(ZEPHYR_SDK_INSTALL_DIR): $(ZEPHYR_SDK_DOWNLOAD_PATH)
	chmod u+rwx $(ZEPHYR_SDK_DOWNLOAD_PATH)
	bash $(ZEPHYR_SDK_DOWNLOAD_PATH) --quiet -- -d $(ZEPHYR_SDK_INSTALL_DIR)

$(ZEPHYR_SOURCES): .west/config rpu-app/west.yml
	bash -c "for i in {1..5}; do west update && break || sleep 1; done"


# -----------------------------------------------------------------------------
# RPU App ---------------------------------------------------------------------
# -----------------------------------------------------------------------------
RPUAPP_SRC_DIR = $(ROOT_DIR)/rpu-app/src
RPUAPP_BUILD_DIR = $(BUILD_DIR)/rpu-app
RPUAPP_ZEPHYR_ELF = $(RPUAPP_BUILD_DIR)/zephyr/zephyr.elf
RPUAPP_REG_HEADERS = \
	$(RPUAPP_SRC_DIR)/nvme_ident_fields.h \
	$(RPUAPP_SRC_DIR)/nvme_reg_fields.h \
	$(RPUAPP_SRC_DIR)/nvme_reg_map.h

IN_ZEPHYR_ENV = source $(BUILD_DIR)/zephyr/zephyr-env.sh
IN_SDK_ENV = \
	source $(BUILD_DIR)/zephyr/zephyr-env.sh && \
	export ZEPHYR_TOOLCHAIN_VARIANT=zephyr && \
	export ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK_INSTALL_DIR)

# RPU App rules ---------------------------------------------------------------
.PHONY: rpu-app
rpu-app: $(RPUAPP_REG_HEADERS)
rpu-app: $(RPUAPP_ZEPHYR_ELF) ## build rpu-app

.PHONY: rpu-app/with-sdk
rpu-app/with-sdk: $(RPUAPP_REG_HEADERS)
rpu-app/with-sdk: SHELL:=/bin/bash
rpu-app/with-sdk: zephyr/deps zephyr/sdk zephyr/setup  ## build rpu-app with local Zephyr SDK (helper)
	$(IN_SDK_ENV) && west build -b zcu106 -d $(RPUAPP_BUILD_DIR) rpu-app

.PHONY: rpu-app/clean
rpu-app/clean:
	$(RM) -r $(RPUAPP_BUILD_DIR)
	$(RM) $(RPUAPP_SRC_DIR)/nvme_reg_fields.h
	$(RM) $(RPUAPP_SRC_DIR)/nvme_reg_map.h
	$(RM) $(RPUAPP_SRC_DIR)/nvme_ident_fields.h

# RPU App dependencies --------------------------------------------------------
$(RPUAPP_REG_HEADERS): $(REGGEN_DIR)/get_reg_fields.py
$(RPUAPP_REG_HEADERS): $(NVME_SPEC_FILE)
	$(REGGEN_DIR)/get_reg_fields.py $(NVME_SPEC_FILE) -f $(BUILD_DIR)/registers.json
	$(REGGEN_DIR)/get_reg_fields_zephyr.py $(BUILD_DIR)/registers.json -f $(RPUAPP_SRC_DIR)/nvme_reg_fields.h
	$(REGGEN_DIR)/get_reg_map_zephyr.py $(NVME_SPEC_FILE) -f $(RPUAPP_SRC_DIR)/nvme_reg_map.h
	$(REGGEN_DIR)/get_identify_struct.py $(NVME_SPEC_FILE) -f $(RPUAPP_SRC_DIR)/nvme_ident_fields.h

$(RPUAPP_ZEPHYR_ELF): SHELL := /bin/bash
$(RPUAPP_ZEPHYR_ELF): $(ZEPHYR_SOURCES)
	$(IN_ZEPHYR_ENV) && west build -b zcu106 -d $(RPUAPP_BUILD_DIR) rpu-app


# -----------------------------------------------------------------------------
# Help ------------------------------------------------------------------------
# -----------------------------------------------------------------------------
HELP_COLUMN_SPAN = 20
HELP_FORMAT_STRING = "\033[36m%-$(HELP_COLUMN_SPAN)s\033[0m %s\n"
.PHONY: help
help: ## show this help
	@echo Here is the list of available targets:
	@echo ""
	@grep -E '^[^#[:blank:]]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf $(HELP_FORMAT_STRING), $$1, $$2}'
	@echo ""

.DEFAULT_GOAL := help
