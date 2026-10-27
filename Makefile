# SPDX-License-Identifier: LGPL-3.0-or-later

SHELL := /bin/bash

BUILD_DIR ?= build
WAD_DIR ?= $(CURDIR)/reference
PLATFORM ?= native
CMAKE_TOOLCHAIN_FILE ?=
TEST_WADS_DIR ?=
MOVIES ?= auto
RECCMP_BUILD_DIR ?= $(CURDIR)/build-reccmp
RECCMP_LEDGER ?= docs/coverage/ledger.csv
RECCMP_WINE_BIN ?=
RECCMP_RECCMP ?=
REPROBIT_MSVC_5_0_RTM_ROOT ?=

.PHONY: all setup build run clean reccmp reccmp-report reccmp-lint

all: build

# Configure the build directory: bootstrap sparse miniwin for native builds,
# then configure CMake. An existing build directory is reconfigured in place.
setup:
	@if [ "$(PLATFORM)" = native ]; then tools/setup-miniwin.sh; fi
	cmake -S . -B "$(BUILD_DIR)" -DPLATFORM="$(PLATFORM)" -DMOVIES="$(MOVIES)" -DTEST_WADS_DIR="$(TEST_WADS_DIR)" $(if $(CMAKE_TOOLCHAIN_FILE),-DCMAKE_TOOLCHAIN_FILE="$(CMAKE_TOOLCHAIN_FILE)",)

build: setup
	cmake --build "$(BUILD_DIR)" --target program

# Build the MSVC 5.0 candidate with a linker PDB, then generate the Reccmp
# HTML and JSON reports. Override the Wine/tool paths when they are not on PATH.
reccmp: reccmp-report

reccmp-report:
	RECCMP_BUILD_DIR="$(RECCMP_BUILD_DIR)" \
	RECCMP_WINE_BIN="$(RECCMP_WINE_BIN)" \
	RECCMP_RECCMP="$(RECCMP_RECCMP)" \
	REPROBIT_MSVC_5_0_RTM_ROOT="$(REPROBIT_MSVC_5_0_RTM_ROOT)" \
	  ./tools/reccmp.sh

reccmp-lint:
	python3 ./tools/lint_reccmp_annotations.py --ledger "$(RECCMP_LEDGER)"

run: build
	LEGORRR_WAD_DIR="$(WAD_DIR)" "$(BUILD_DIR)/LegoRR"

clean:
	cmake --build "$(BUILD_DIR)" --target clean
