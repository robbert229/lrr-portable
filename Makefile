# SPDX-License-Identifier: LGPL-3.0-or-later

SHELL := /bin/bash

BUILD_DIR ?= build
WAD_DIR ?= $(CURDIR)/source

.PHONY: all build run clean

all: build

# Meson's CMake bridge emits an empty STATIC_LINKER rule for miniwin. Build
# miniwin with its own CMake project, then run the generated Ninja commands
# for LegoRR while omitting only that empty archive command. Ninja's command
# list includes the normal compile, archive, and link steps for the executable.
build:
	tools/build-miniwin.sh
	@set -o pipefail; \
	  ninja -C "$(BUILD_DIR)" -t commands src/LegoRR | \
	  sed '/^rm -f subprojects\/miniwin\/libminiwin\.a && .*ar csr.*/d' | \
	  (cd "$(BUILD_DIR)" && sh -e)

run: build
	LEGORRR_WAD_DIR="$(WAD_DIR)" ./$(BUILD_DIR)/src/LegoRR

clean:
	meson compile -C "$(BUILD_DIR)" --clean
