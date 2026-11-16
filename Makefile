# SPDX-License-Identifier: LGPL-3.0-or-later

SHELL := /bin/bash

BUILD_DIR ?= build
WAD_DIR ?= $(CURDIR)/reference
CMAKE_TOOLCHAIN_FILE ?=
TEST_WADS_DIR ?=
MOVIES ?= auto
# The shipped game, run under the A.2 harness (tools/compat/README.md).
REFERENCE_DIR ?= $(CURDIR)/reference
PROTON_BIN ?= $(RR_A2_PROTON)
COMPATDATA ?= $(if $(RR_A2_HOME),$(RR_A2_HOME),$(HOME)/.local/share/rock-raider-a2)/compatdata

.PHONY: all setup build run test clean reference-run

all: build

# Configure the build directory: bootstrap sparse miniwin, then configure
# CMake. An existing build directory is reconfigured in place.
setup:
	tools/setup-miniwin.sh
	cmake -S . -B "$(BUILD_DIR)" -DMOVIES="$(MOVIES)" -DTEST_WADS_DIR="$(TEST_WADS_DIR)" $(if $(CMAKE_TOOLCHAIN_FILE),-DCMAKE_TOOLCHAIN_FILE="$(CMAKE_TOOLCHAIN_FILE)",)

build: setup
	cmake --build "$(BUILD_DIR)" --target program

test: setup
	cmake --build "$(BUILD_DIR)"
	ctest --test-dir "$(BUILD_DIR)" --output-on-failure

run: build
	LEGORRR_WAD_DIR="$(WAD_DIR)" "$(BUILD_DIR)/LegoRR"

clean:
	cmake --build "$(BUILD_DIR)" --target clean

# Run the shipped LegoRR.exe from $(REFERENCE_DIR) under Proton, with the A.2
# harness's runner and prefix.
reference-run:
	@test -s "$(REFERENCE_DIR)/LegoRR.exe" || { \
	  echo "Missing $(REFERENCE_DIR)/LegoRR.exe" >&2; \
	  exit 2; \
	}
	@proton_bin="$(PROTON_BIN)"; \
	if [ -z "$$proton_bin" ]; then \
	  proton_bin="$$(python3 -c 'from tools.compat.a2 import find_runner; r = find_runner(); print(r["path"] if r and r["kind"] == "proton" else "")')"; \
	fi; \
	if [ -z "$$proton_bin" ] || [ ! -x "$$proton_bin" ]; then \
	  echo "Proton was not found; set PROTON_BIN (or RR_A2_PROTON) to its executable path" >&2; \
	  exit 2; \
	fi; \
	cd "$(REFERENCE_DIR)" && \
	  STEAM_COMPAT_DATA_PATH="$(COMPATDATA)" \
	  STEAM_COMPAT_CLIENT_INSTALL_PATH="$(HOME)" \
	    "$$proton_bin" runinprefix ./LegoRR.exe
