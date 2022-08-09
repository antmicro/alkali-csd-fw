# Helper macros ---------------------------------------------------------------
ROOT_DIR = $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
NVME_SPEC_NAME = NVM-Express-1_4-2019.06.10-Ratified.pdf
NVME_SPEC_FILE = $(REGGEN_DIR)/$(NVME_SPEC_NAME)

DOCKER_IMAGE_BASE ?= debian:bullseye
DOCKER_TAG_NAME=fw:1.0
DOCKER_TAG = $(DOCKER_IMAGE_PREFIX)$(DOCKER_TAG_NAME)

# Build directories -----------------------------------------------------------
BUILD_DIR ?= $(ROOT_DIR)/build
APUAPP_BUILD_DIR = $(BUILD_DIR)/apu-app
RPUAPP_BUILD_DIR = $(BUILD_DIR)/rpu-app
BUILDROOT_BUILD_DIR = $(BUILD_DIR)/buildroot

# Helper directories ----------------------------------------------------------
THIRD_PARTY_DIR = $(ROOT_DIR)/third-party
BUILDROOT_DIR = $(ROOT_DIR)/third-party/buildroot
REGGEN_DIR = $(ROOT_DIR)/third-party/registers-generator
RPUAPP_DIR = $(ROOT_DIR)/rpu-app
DOCKER_BUILD_DIR = $(BUILD_DIR)/docker
WEST_INIT_DIR ?= $(RPUAPP_DIR)


# -----------------------------------------------------------------------------
# All -------------------------------------------------------------------------
# -----------------------------------------------------------------------------
.PHONY: all
all: buildroot apu-app rpu-app ## Build all binaries (Buildroot, APU App, RPU App)


# -----------------------------------------------------------------------------
# Clean -----------------------------------------------------------------------
# -----------------------------------------------------------------------------
.PHONY: clean
clean: ## Remove ALL build artifacts
	$(RM) -r $(BUILD_DIR)
	$(RM) -r $(WEST_DIR)


# -----------------------------------------------------------------------------
# Buildroot -------------------------------------------------------------------
# -----------------------------------------------------------------------------
BR2_EXTERNAL_DIR = $(ROOT_DIR)/br2-external
BR2_EXTERNAL_BUILD_DIR = $(BUILDROOT_BUILD_DIR)/br2-external
BR2_BASALT_OVERLAY_BUILD_DIR = $(BR2_EXTERNAL_BUILD_DIR)/board/basalt/overlay
BUILDROOT_OPTS = O=$(BUILDROOT_BUILD_DIR) -C $(BUILDROOT_DIR) BR2_EXTERNAL=$(BR2_EXTERNAL_BUILD_DIR)
BUILDROOT_TOOLCHAIN_TAR_FILE = $(BUILDROOT_BUILD_DIR)/images/aarch64-buildroot-linux-gnu_sdk-buildroot.tar.gz
BUILDROOT_TOOLCHAIN_OUTPUT_DIR = $(BUILD_DIR)/aarch64-buildroot-linux-gnu_sdk-buildroot
BUILDROOT_TOOLCHAIN_CMAKE_FILE = $(BUILDROOT_TOOLCHAIN_OUTPUT_DIR)/share/buildroot/toolchainfile.cmake

# Buildroot rules -------------------------------------------------------------
.PHONY: buildroot
buildroot: apu-app ## Build Buildroot
	cp -r $(BR2_EXTERNAL_DIR) $(BUILDROOT_BUILD_DIR)
	cp $(APUAPP_BUILD_DIR)/libvta-delegate.so $(BR2_BASALT_OVERLAY_BUILD_DIR)/lib/libvta-delegate.so
	cp $(APUAPP_BUILD_DIR)/apu-app $(BR2_BASALT_OVERLAY_BUILD_DIR)/bin/apu-app
	$(MAKE) $(BUILDROOT_OPTS) zynqmp_nvme_defconfig
	$(MAKE) $(BUILDROOT_OPTS) -j$(nproc)

.PHONY: buildroot/distclean
buildroot/distclean: ## Remove Buildroot build
	$(MAKE) $(BUILDROOT_OPTS) distclean

.PHONY: buildroot/sdk
buildroot/sdk: $(BUILDROOT_TOOLCHAIN_TAR_FILE) ## Generate Buildroot toolchain

.PHONY: buildroot/sdk-untar
buildroot/sdk-untar: $(BUILDROOT_TOOLCHAIN_DIR) ## Untar Buildroot toolchain (helper)

.PHONY: buildroot//%
buildroot//%: ## Forward rule to invoke Buildroot rules directly e.g. `make buildroot//menuconfig`
	$(MAKE) $(BUILDROOT_OPTS) $*

# Buildroot dependencies-------------------------------------------------------
$(BUILDROOT_TOOLCHAIN_CMAKE_FILE): $(BUILDROOT_TOOLCHAIN_TAR_FILE)
	tar mxf $(BUILDROOT_TOOLCHAIN_TAR_FILE) -C $(BUILD_DIR)

$(BUILDROOT_TOOLCHAIN_TAR_FILE):
	mkdir -p $(BR2_EXTERNAL_BUILD_DIR)
	cp -r $(BR2_EXTERNAL_DIR) $(BUILDROOT_BUILD_DIR)
	$(MAKE) $(BUILDROOT_OPTS) zynqmp_nvme_defconfig
	$(MAKE) $(BUILDROOT_OPTS) sdk


# -----------------------------------------------------------------------------
# APU App ---------------------------------------------------------------------
# -----------------------------------------------------------------------------
APUAPP_DIR = $(ROOT_DIR)/apu-app
APUAPP_SRC_DIR = $(ROOT_DIR)/apu-app/src
APUAPP_INSTALL_DIR = $(BUILD_DIR)/apu-app/install
APUAPP_OUTPUTS = $(APUAPP_BUILD_DIR)/libvta-delegate.so $(APUAPP_BUILD_DIR)/apu-app
APUAPP_BUILD_TYPE ?= Debug

# APU App rules ---------------------------------------------------------------
.PHONY: apu-app
apu-app: $(APUAPP_OUTPUTS) ## Build APU App

.PHONY: apu-app/clean
apu-app/clean: ## Remove APU App build files
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
	      -DCMAKE_BUILD_TYPE=$(APUAPP_BUILD_TYPE) \
	      -DNO_HARDWARE=OFF \
	      -DBUILD_TESTS=ON \
	      -S $(APUAPP_DIR) -B $(APUAPP_BUILD_DIR)
	$(MAKE) -C $(APUAPP_BUILD_DIR) -j all


# -----------------------------------------------------------------------------
# Zephyr ----------------------------------------------------------------------
# -----------------------------------------------------------------------------
WEST_DIR = $(ROOT_DIR)/.west
WEST_CONFIG ?= $(WEST_DIR)/config
WEST_YML ?= $(RPUAPP_DIR)/west.yml
ZEPHYR_SDK_VERSION = 0.10.3
ZEPHYR_SDK_NAME = zephyr-sdk-$(ZEPHYR_SDK_VERSION)
ZEPHYR_SDK_DOWNLOAD_URL = https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v$(ZEPHYR_SDK_VERSION)/$(ZEPHYR_SDK_NAME)-setup.run
ZEPHYR_SDK_DOWNLOAD_PATH = $(BUILD_DIR)/zephyr-sdk.run
ZEPHYR_SDK_LOCAL_INSTALL_DIR = $(BUILD_DIR)/$(ZEPHYR_SDK_NAME)

ZEPHYR_SOURCES= \
	$(BUILD_DIR)/zephyr \
	$(BUILD_DIR)/tools \
	$(BUILD_DIR)/modules/hal/libmetal \
	$(BUILD_DIR)/modules \
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
zephyr/sdk: $(ZEPHYR_SDK_LOCAL_INSTALL_DIR) ## Install Zephyr SDK locally (helper)
	@echo "To use local installation of the toolchain set the following environment variables:"
	@echo "  - ZEPHYR_TOOLCHAIN_VARIANT=zephyr"
	@echo "  - ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK_LOCAL_INSTALL_DIR)"

.PHONY: zephyr/setup
zephyr/setup: $(WEST_YML)
zephyr/setup: $(ZEPHYR_SOURCES) ## Install Zephyr dependencies and get Zephyr sources

.PHONY: zephyr/clean
zephyr/clean: ## Remove Zephyr installed files
	$(RM) -r $(BUILD_DIR)/zephyr*
	$(RM) -r $(WEST_DIR)

# Zephyr dependencies ---------------------------------------------------------
$(ZEPHYR_SDK_DOWNLOAD_PATH):
	@mkdir -p $(BUILD_DIR)
	wget -q $(ZEPHYR_SDK_DOWNLOAD_URL) -O $(ZEPHYR_SDK_DOWNLOAD_PATH)

$(ZEPHYR_SDK_LOCAL_INSTALL_DIR): $(ZEPHYR_SDK_DOWNLOAD_PATH)
	chmod u+rwx $(ZEPHYR_SDK_DOWNLOAD_PATH)
	bash $(ZEPHYR_SDK_DOWNLOAD_PATH) --quiet -- -d $(ZEPHYR_SDK_LOCAL_INSTALL_DIR)

$(ZEPHYR_SOURCES) &: $(WEST_CONFIG)
	@:

$(WEST_CONFIG): SHELL := /bin/bash
$(WEST_CONFIG):
	@echo "Initialize west for Zephyr."; \
	if west init -l --mf $(WEST_YML) $(WEST_INIT_DIR); then \
		west update; \
		pip3 install -r $(BUILD_DIR)/zephyr/scripts/requirements.txt; \
		echo "Done."; \
	else \
		echo ""; \
		echo -e "\e[31mError:\e[0m West initialization failed. It might be caused by another west config instance"; \
		echo -e "\e[31mError:\e[0m in a parent directory. Remove an existing '.west' and try again."; \
		echo -e "\e[31mError:\e[0m "; \
		echo -e "\e[31mError:\e[0m If you want to have west initialized in multiple directories of one tree you must"; \
		echo -e "\e[31mError:\e[0m first initialize it in a directory that is lower in hierarchy."; \
		echo ""; \
		exit -1; \
	fi;

# -----------------------------------------------------------------------------
# RPU App ---------------------------------------------------------------------
# -----------------------------------------------------------------------------
RPUAPP_MAIN_DIR ?= rpu-app
RPUAPP_SRC_DIR = $(RPUAPP_DIR)/src
RPUAPP_GENERATED_DIR = $(RPUAPP_BUILD_DIR)/generated
RPUAPP_ZEPHYR_ELF = $(RPUAPP_BUILD_DIR)/zephyr/zephyr.elf

IN_ZEPHYR_ENV = source $(BUILD_DIR)/zephyr/zephyr-env.sh
IN_SDK_ENV = \
	source $(BUILD_DIR)/zephyr/zephyr-env.sh && \
	export ZEPHYR_TOOLCHAIN_VARIANT=zephyr && \
	export ZEPHYR_SDK_INSTALL_DIR=$(ZEPHYR_SDK_LOCAL_INSTALL_DIR)

CMAKE_OPTS = -DGENERATED_DIR=$(RPUAPP_GENERATED_DIR) -DREGGEN_DIR=$(REGGEN_DIR) \
	-DNVME_SPEC_FILE=$(NVME_SPEC_FILE) -DRPUAPP_GENERATED_DIR=$(RPUAPP_GENERATED_DIR)
WEST_BUILD = west build -b zcu106 -d $(RPUAPP_BUILD_DIR) $(RPUAPP_MAIN_DIR) $(CMAKE_OPTS)

# RPU App rules ---------------------------------------------------------------
.PHONY: rpu-app
rpu-app: $(RPUAPP_ZEPHYR_ELF) ## Build RPU App

.PHONY: rpu-app/with-sdk
rpu-app/with-sdk: SHELL:=/bin/bash
rpu-app/with-sdk: zephyr/sdk zephyr/setup ## Build RPU App with local Zephyr SDK (helper)
	$(IN_SDK_ENV) && $(WEST_BUILD)

.PHONY: rpu-app/clean
rpu-app/clean: ## Remove RPU App build files
	$(RM) -r $(RPUAPP_BUILD_DIR)

# RPU App dependencies --------------------------------------------------------
$(RPUAPP_ZEPHYR_ELF): SHELL := /bin/bash
$(RPUAPP_ZEPHYR_ELF): $(ZEPHYR_SOURCES)
	$(IN_ZEPHYR_ENV) && $(WEST_BUILD)


# -----------------------------------------------------------------------------
# Docker ----------------------------------------------------------------------
# -----------------------------------------------------------------------------

REGGEN_REL_DIR=$(shell realpath --relative-to $(ROOT_DIR) $(REGGEN_DIR))
DOCKER_BUILD_PYTHON_REQS_DIR=$(DOCKER_BUILD_DIR)/$(REGGEN_REL_DIR)

.PHONY: docker
docker: $(DOCKER_BUILD_DIR)/docker.ok ## Build the development docker image

$(DOCKER_BUILD_DIR):
	@mkdir -p $(DOCKER_BUILD_DIR)

$(DOCKER_BUILD_DIR)/docker.ok: fw.dockerfile requirements.txt $(REGGEN_DIR)/requirements.txt | $(DOCKER_BUILD_DIR)
	cp $(ROOT_DIR)/fw.dockerfile $(DOCKER_BUILD_DIR)/Dockerfile
	cp $(ROOT_DIR)/requirements.txt $(DOCKER_BUILD_DIR)/requirements.txt
	mkdir -p $(DOCKER_BUILD_PYTHON_REQS_DIR)
	cp $(REGGEN_DIR)/requirements.txt $(DOCKER_BUILD_PYTHON_REQS_DIR)/requirements.txt
	cd $(DOCKER_BUILD_DIR) && docker build \
		--build-arg IMAGE_BASE="$(DOCKER_IMAGE_BASE)" \
		--build-arg REPO_ROOT="$(PWD)" \
		-t $(DOCKER_TAG) . && touch docker.ok

.PHONY: docker/clean
docker/clean:
	$(RM) -r $(DOCKER_BUILD_DIR)


# -----------------------------------------------------------------------------
# Enter -----------------------------------------------------------------------
# -----------------------------------------------------------------------------
.PHONY: enter
enter: $(DOCKER_BUILD_DIR)/docker.ok ## enter the development docker image
	docker run \
		--rm \
		-v $(PWD):$(PWD) \
		-v /etc/passwd:/etc/passwd \
		-v /etc/group:/etc/group \
		-u $(shell id -u):$(shell id -g) \
		-h docker-container \
		-w $(PWD) \
		-it \
		$(DOCKER_RUN_EXTRA_ARGS) \
		$(DOCKER_TAG)


# -----------------------------------------------------------------------------
# Help ------------------------------------------------------------------------
# -----------------------------------------------------------------------------
HELP_COLUMN_SPAN = 25
HELP_FORMAT_STRING = "\033[36m%-$(HELP_COLUMN_SPAN)s\033[0m %s \033[34m%s\033[0m\n"
USED_IN_BUILD_MESSAGE = (used to configure build inside 'alkali-csd-build')
.PHONY: help
help: ## Show this help message
	@echo Here is the list of available targets:
	@echo ""
	@grep -E '^[^#[:blank:]]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf $(HELP_FORMAT_STRING), $$1, $$2, ""}'
	@echo ""
	@echo "Additionally, you can use the following environment variables:"
	@echo ""
	@printf $(HELP_FORMAT_STRING) "DOCKER_RUN_EXTRA_ARGS" "Extra arguments to pass to container on 'make enter'" " "
	@printf $(HELP_FORMAT_STRING) "DOCKER_IMAGE_PREFIX" "Custom registry prefix with '/' at the end" " "
	@printf $(HELP_FORMAT_STRING) "DOCKER_IMAGE_BASE" "Custom docker image base" " "
	@printf $(HELP_FORMAT_STRING) "BUILD_DIR" "Absolute path to desired build directory" "$(USED_IN_BUILD_MESSAGE)"
	@printf $(HELP_FORMAT_STRING) "APUAPP_BUILD_TYPE" "APU application build type, Debug (default) or Release" "$(USED_IN_BUILD_MESSAGE)"
	@printf $(HELP_FORMAT_STRING) "WEST_INIT_DIR" "Relative path to directory where west should be initialized" "$(USED_IN_BUILD_MESSAGE)"
	@printf $(HELP_FORMAT_STRING) "WEST_CONFIG" "Absolute path to '.west/config' configuration file" "$(USED_IN_BUILD_MESSAGE)"
	@printf $(HELP_FORMAT_STRING) "WEST_YML" "Absolute path to 'west.yml' manifest file" "$(USED_IN_BUILD_MESSAGE)"
	@printf $(HELP_FORMAT_STRING) "RPUAPP_MAIN_DIR" "Absolute path to RPU application directory" "$(USED_IN_BUILD_MESSAGE)"
	@echo ""

.DEFAULT_GOAL := help
