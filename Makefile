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
BINARY_PROGRESS_PYTHON ?= python3
RECCMP_BASELINE ?= docs/coverage/match-baseline.txt
RECCMP_WINE_BIN ?=
RECCMP_PROTON_BIN ?= $(RR_A2_PROTON)
RECCMP_COMPATDATA ?= $(if $(RR_A2_HOME),$(RR_A2_HOME),$(HOME)/.local/share/rock-raider-a2)/compatdata
RECCMP_RECCMP ?=
REPROBIT_MSVC_5_0_RTM_ROOT ?=

.PHONY: match-progress binary-progress binary-progress-compare all setup build run clean reccmp reccmp-report reccmp-run reference-run reccmp-lint reccmp-check reccmp-compare reccmp-baseline-update match-function match-function-parity convert-unit tools-test

all: build

# Configure the build directory: bootstrap sparse miniwin for native builds,
# then configure CMake. An existing build directory is reconfigured in place.
setup:
	@if [ "$(PLATFORM)" = native ]; then tools/setup-miniwin.sh; fi
	cmake -S . -B "$(BUILD_DIR)" -DPLATFORM="$(PLATFORM)" -DMOVIES="$(MOVIES)" -DTEST_WADS_DIR="$(TEST_WADS_DIR)" $(if $(CMAKE_TOOLCHAIN_FILE),-DCMAKE_TOOLCHAIN_FILE="$(CMAKE_TOOLCHAIN_FILE)",)

build: setup
	cmake --build "$(BUILD_DIR)" --target program

# Rebuild the matching build once, then refresh the baseline, the listing
# digests and the whole-binary comparison from it; review the gate and
# entropy policy before accepting drops.
match-progress: reccmp-report
	$(MAKE) reccmp-baseline-update
	$(MAKE) binary-progress-compare

# Rebuild the matching build and refresh the whole-binary comparison.
binary-progress: reccmp-report
	$(MAKE) binary-progress-compare

# Compare the existing matching build with the original, without rebuilding.
binary-progress-compare:
	$(BINARY_PROGRESS_PYTHON) tools/binary_progress.py --rebuilt "$(RECCMP_BUILD_DIR)/LegoRR.exe" \
	  --report-dir "$(RECCMP_BUILD_DIR)/reccmp-report"

# Build the MSVC 5.0 candidate (non-incremental link, with a linker PDB and
# $(RECCMP_BUILD_DIR)/LegoRR.map), then generate the Reccmp HTML and JSON
# reports. Override the Wine/tool paths when they are not on PATH.
reccmp: reccmp-report

reccmp-report:
	RECCMP_BUILD_DIR="$(RECCMP_BUILD_DIR)" \
	RECCMP_WINE_BIN="$(RECCMP_WINE_BIN)" \
	RECCMP_RECCMP="$(RECCMP_RECCMP)" \
	REPROBIT_MSVC_5_0_RTM_ROOT="$(REPROBIT_MSVC_5_0_RTM_ROOT)" \
	  ./tools/reccmp.sh 2>&1

# Match baseline gate (see GLOSSARY.md). reccmp-check rebuilds, then fails if
# any function's score fell below the committed baseline. reccmp-compare does
# the same against an existing report without rebuilding. reccmp-baseline-update
# accepts the current report as the new baseline (run it after a rebuild).
reccmp-check: reccmp-report reccmp-compare

reccmp-compare:
	python3 ./tools/match_baseline.py check \
	  --report-dir "$(RECCMP_BUILD_DIR)/reccmp-report" --baseline "$(RECCMP_BASELINE)"

reccmp-baseline-update:
	python3 ./tools/match_baseline.py update \
	  --report-dir "$(RECCMP_BUILD_DIR)/reccmp-report" --baseline "$(RECCMP_BASELINE)"

# Single-function loop (tools/match_function.py): compile only the translation
# unit holding ADDR with the matching-build flags and score it with Reccmp's
# comparator, without linking. ADDR takes one or more original addresses;
# SOURCE=<file> alone scores every annotated function in that file; JSON=1
# prints JSON; MATCH_ARGS passes extra options (e.g. --side-by-side).
match-function:
	@if [ -z "$(ADDR)$(SOURCE)" ]; then \
	  echo "usage: make match-function ADDR=0x00478e60 [SOURCE=src/...] [JSON=1] [MATCH_ARGS=...]" >&2; \
	  exit 2; \
	fi
	@REPROBIT_MSVC_5_0_RTM_ROOT="$(REPROBIT_MSVC_5_0_RTM_ROOT)" \
	  python3 ./tools/match_function.py $(ADDR) $(if $(SOURCE),--source "$(SOURCE)") \
	  $(if $(JSON),--json) $(MATCH_ARGS)

# Scaffold an original-shape conversion of one unit (ADR 0005, step 0).
# UNIT=G98-029 [CONVERT_ARGS="--stem registry --dry-run"]; see tools/README.md.
convert-unit:
	@if [ -z "$(UNIT)" ]; then \
	  echo "usage: make convert-unit UNIT=G98-029 [CONVERT_ARGS=\"--stem name --dry-run\"]" >&2; \
	  exit 2; \
	fi
	@REPROBIT_MSVC_5_0_RTM_ROOT="$(REPROBIT_MSVC_5_0_RTM_ROOT)" \
	  python3 ./tools/convert_unit.py $(UNIT) $(CONVERT_ARGS)

# Unit tests of the Python tooling (needs reference/LegoRR.exe for the
# validation tests, which are skipped without it).
tools-test:
	python3 -m unittest discover -s tools -p 'test_convert_unit.py'
	python3 -m unittest discover -s tools/tests -p 'test_msvc_permuter.py'
	python3 -m unittest discover -s tools/tests -p 'test_permuter_cohort.py'
	python3 -m unittest discover -s tools/tests -p 'test_binary_progress.py'
	python3 -m unittest discover -s tools/tests -p 'test_pr_match_progress.py'
	python3 -m unittest discover -s tools/tests -p 'test_match_listings.py'

# Check match_function.py against an existing full report from the same tree.
match-function-parity:
	REPROBIT_MSVC_5_0_RTM_ROOT="$(REPROBIT_MSVC_5_0_RTM_ROOT)" \
	  python3 ./tools/match_function_parity.py \
	  --report "$(RECCMP_BUILD_DIR)/reccmp-report/results.json"

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
