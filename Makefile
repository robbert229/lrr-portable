# SPDX-License-Identifier: LGPL-3.0-or-later

SHELL := /bin/bash

BUILD_DIR ?= build
WAD_DIR ?= $(CURDIR)/reference
PLATFORM ?= native
CMAKE_TOOLCHAIN_FILE ?=
TEST_WADS_DIR ?=
MOVIES ?= auto
RECCMP_BUILD_DIR ?= $(CURDIR)/build-reccmp
RECCMP_REFERENCE_DIR ?= $(CURDIR)/reference
RECCMP_LEDGER ?= docs/coverage/ledger.csv
RECCMP_WINE_BIN ?=
RECCMP_PROTON_BIN ?= $(RR_A2_PROTON)
RECCMP_COMPATDATA ?= $(if $(RR_A2_HOME),$(RR_A2_HOME),$(HOME)/.local/share/rock-raider-a2)/compatdata
RECCMP_RECCMP ?=
REPROBIT_MSVC_5_0_RTM_ROOT ?=

.PHONY: all setup build run clean reccmp reccmp-report reccmp-run reference-run reccmp-lint

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
	  ./tools/reccmp.sh 2>&1

# Both executables use the same Proton runner, compatdata, and reference
# working directory so the candidate sees the same game assets and settings.
reference-run: _RUN_EXE := $(abspath $(RECCMP_REFERENCE_DIR)/LegoRR.exe)
reference-run: _RUN_ARGUMENT := ./LegoRR.exe
reference-run: _RUN_MISSING_HINT :=
reccmp-run: _RUN_EXE := $(abspath $(RECCMP_BUILD_DIR)/LegoRR.exe)
reccmp-run: _RUN_ARGUMENT := $(abspath $(RECCMP_BUILD_DIR)/LegoRR.exe)
reccmp-run: _RUN_MISSING_HINT :=; build it with make reccmp-report

reference-run reccmp-run:
	@test -s "$(_RUN_EXE)" || { \
	  echo "Missing $(_RUN_EXE)$(_RUN_MISSING_HINT)" >&2; \
	  exit 2; \
	}
	@proton_bin="$(RECCMP_PROTON_BIN)"; \
	if [ -z "$$proton_bin" ]; then \
	  proton_bin="$$(python3 -c 'from tools.compat.a2 import find_runner; r = find_runner(); print(r["path"] if r and r["kind"] == "proton" else "")')"; \
	fi; \
	if [ -z "$$proton_bin" ] || [ ! -x "$$proton_bin" ]; then \
	  echo "Proton was not found; set RECCMP_PROTON_BIN (or RR_A2_PROTON) to its executable path" >&2; \
	  exit 2; \
	fi; \
	cd "$(RECCMP_REFERENCE_DIR)" && \
	  STEAM_COMPAT_DATA_PATH="$(RECCMP_COMPATDATA)" \
	  STEAM_COMPAT_CLIENT_INSTALL_PATH="$(HOME)" \
	    "$$proton_bin" runinprefix "$(_RUN_ARGUMENT)"

reccmp-lint:
	python3 ./tools/lint_reccmp_annotations.py --ledger "$(RECCMP_LEDGER)"

run: build
	LEGORRR_WAD_DIR="$(WAD_DIR)" "$(BUILD_DIR)/LegoRR"

clean:
	cmake --build "$(BUILD_DIR)" --target clean
