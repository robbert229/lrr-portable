# SPDX-License-Identifier: LGPL-3.0-or-later

SHELL := /bin/bash

BUILD_DIR ?= build
WAD_DIR ?= $(CURDIR)/reference
PLATFORM ?= native
CMAKE_TOOLCHAIN_FILE ?=
TEST_WADS_DIR ?=
MOVIES ?= auto

.PHONY: all setup build run clean

all: build

# Configure the build directory: bootstrap sparse miniwin for native builds,
# then configure CMake. An existing build directory is reconfigured in place.
setup:
	@if [ "$(PLATFORM)" = native ]; then tools/setup-miniwin.sh; fi
	cmake -S . -B "$(BUILD_DIR)" -DPLATFORM=$(PLATFORM) -DMOVIES=$(MOVIES) -DTEST_WADS_DIR="$(TEST_WADS_DIR)" $(if $(CMAKE_TOOLCHAIN_FILE),-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE),)

build:
	cmake --build "$(BUILD_DIR)" --target LegoRR

run: build
	LEGORRR_WAD_DIR="$(WAD_DIR)" ./$(BUILD_DIR)/LegoRR

clean:
	cmake --build "$(BUILD_DIR)" --target clean
