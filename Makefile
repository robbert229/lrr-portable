# SPDX-License-Identifier: LGPL-3.0-or-later

SHELL := /bin/bash

BUILD_DIR ?= build
WAD_DIR ?= $(CURDIR)/reference
PLATFORM ?= native
# Extra flags for `meson setup`, e.g. for the winapi reference build:
#   make setup CROSS_FILE=--cross-file\ cross/i686.ini PLATFORM=winapi
CROSS_FILE ?=

.PHONY: all setup build run clean

all: build

# Configure the build directory: bootstrap the sparse miniwin submodule
# (idempotent, no-op once present) and run meson setup. An already-configured
# BUILD_DIR is reconfigured instead of failing.
setup:
	tools/setup-miniwin.sh
	@if [ -f "$(BUILD_DIR)/build.ninja" ]; then \
		echo "meson setup --reconfigure $(BUILD_DIR)"; \
		meson setup --reconfigure "$(BUILD_DIR)" $(CROSS_FILE) -Dplatform=$(PLATFORM); \
	else \
		meson setup "$(BUILD_DIR)" $(CROSS_FILE) -Dplatform=$(PLATFORM); \
	fi

# Meson's CMake bridge maps miniwin's EXCLUDE_FROM_ALL library to an empty
# archive rule with no inputs. Build miniwin with its own CMake project first;
# the rule then sees its output present and up to date, and meson compiles and
# links LegoRR against the real archive.
build:
	tools/build-miniwin.sh
	meson compile -C "$(BUILD_DIR)" src/LegoRR

run: build
	LEGORRR_WAD_DIR="$(WAD_DIR)" ./$(BUILD_DIR)/src/LegoRR

clean:
	meson compile -C "$(BUILD_DIR)" --clean
