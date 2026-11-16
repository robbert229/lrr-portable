# SPDX-License-Identifier: LGPL-3.0-or-later
# Inner build of the seam recorder; runs inside the lrr-seamrec-tools image
# (see Containerfile). From the repo root use `make seamrec`.
CC      := i686-w64-mingw32-gcc
OUT     := build
GEN     := $(OUT)/gen
CFLAGS  := -O2 -std=gnu11 -Wall -Wno-unused-variable -Werror=incompatible-pointer-types \
           -Werror=implicit-function-declaration -D__USE_MINGW_ANSI_STDIO=1 -Isrc -I$(GEN) \
           -fno-ident -ffile-prefix-map=$(CURDIR)=.
LDFLAGS := -shared -static-libgcc -Wl,--no-insert-timestamp -s
PROXIES := ddraw d3drm dinput dsound winmm avifil32
CORE_SRC := src/core.c src/hooks.c src/guids.c $(GEN)/gen_wrappers.c $(GEN)/gen_flat.c $(GEN)/gen_reals.c

all: $(OUT)/seamrec.dll $(PROXIES:%=$(OUT)/%.dll)

$(GEN)/stamp: gen.py methods.txt flat.txt $(wildcard exports/*.txt)
	python3 gen.py $(GEN)
	touch $@

$(GEN)/gen_wrappers.c $(GEN)/gen_flat.c $(GEN)/gen_reals.c $(GEN)/gen_hooks.h: $(GEN)/stamp

$(OUT)/seamrec.dll: $(CORE_SRC) src/sr.h src/dxall.h $(GEN)/gen_hooks.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CORE_SRC) -Wl,--kill-at -Wl,--out-implib,$(OUT)/libseamrec.a -luser32

$(OUT)/%.dll: $(GEN)/stamp $(OUT)/seamrec.dll
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(GEN)/proxy_$*.c $(GEN)/$*.def -L$(OUT) -lseamrec

# Re-extract methods.txt from the container's headers (only when adding interfaces).
methods:
	$(CC) -E -P -x c src/dxall.h | python3 extract.py > methods.txt

clean:
	rm -rf $(OUT)

.PHONY: all clean methods
