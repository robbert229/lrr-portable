# SPDX-License-Identifier: LGPL-3.0-or-later

SHELL := /bin/bash

BUILD_DIR ?= build
# The instrumented build for `make coverage`, and where its report goes.
COVERAGE_BUILD_DIR ?= build-coverage
COVERAGE_DIR ?= $(COVERAGE_BUILD_DIR)/report
WAD_DIR ?= $(CURDIR)/reference
CMAKE_TOOLCHAIN_FILE ?=
# The shipped exe's flags. Without -best, the display settings come from
# LegoRR.ini in place of the original's mode-selection dialog.
RUN_ARGS ?=
# The shipped game, run under the A.2 harness (tools/compat/README.md).
REFERENCE_DIR ?= $(CURDIR)/reference
PROTON_BIN ?= $(RR_A2_PROTON)
COMPATDATA ?= $(if $(RR_A2_HOME),$(RR_A2_HOME),$(HOME)/.local/share/rock-raider-a2)/compatdata

# The packaged build on macOS is an app bundle (packaging/macos).
LEGORR = $(if $(wildcard $(abspath $(BUILD_DIR))/LegoRR.app/Contents/MacOS/LegoRR),$(abspath $(BUILD_DIR))/LegoRR.app/Contents/MacOS/LegoRR,$(abspath $(BUILD_DIR))/LegoRR)

.PHONY: all setup build run test coverage clean reference-run

all: build

# Configure the build directory: bootstrap sparse miniwin, then configure
# CMake. An existing build directory is reconfigured in place.
setup:
	tools/setup-miniwin.sh
	cmake -S . -B "$(BUILD_DIR)" $(if $(CMAKE_TOOLCHAIN_FILE),-DCMAKE_TOOLCHAIN_FILE="$(CMAKE_TOOLCHAIN_FILE)",)

build: setup
	cmake --build "$(BUILD_DIR)" --target program

test: setup
	cmake --build "$(BUILD_DIR)"
	ctest --test-dir "$(BUILD_DIR)" --output-on-failure

# Line coverage of src/ (docs/building.md, "Coverage"): a Debug build with
# gcov instrumentation in COVERAGE_BUILD_DIR, the tests, then the report in
# COVERAGE_DIR (html/index.html). Needs GCC, gcovr and diff-cover. Counts from
# an earlier run are cleared first.
coverage:
	tools/setup-miniwin.sh
	cmake -S . -B "$(COVERAGE_BUILD_DIR)" -DCMAKE_BUILD_TYPE=Debug -DLRR_COVERAGE=ON -DLRR_PACKAGING=OFF $(if $(CMAKE_TOOLCHAIN_FILE),-DCMAKE_TOOLCHAIN_FILE="$(CMAKE_TOOLCHAIN_FILE)",)
	cmake --build "$(COVERAGE_BUILD_DIR)"
	find "$(COVERAGE_BUILD_DIR)" -name '*.gcda' -delete
	ctest --test-dir "$(COVERAGE_BUILD_DIR)" --output-on-failure
	python3 tools/coverage.py "$(COVERAGE_BUILD_DIR)" "$(COVERAGE_DIR)" --compare-branch origin/main

run: build
	cd "$(WAD_DIR)" && "$(LEGORR)" $(RUN_ARGS)

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

# Seam recorder (ADR 0007 "Validation"): proxy DLLs that record the shipped
# LegoRR.exe's DX6/Win32 seam calls. Needs podman; see tools/seamrec/README.md.
.PHONY: seamrec seamrec-check
seamrec:
	python3 tools/seamrec/seamrec.py build

seamrec-check: seamrec
	python3 tools/seamrec/seamrec.py check boot-menu-quit
