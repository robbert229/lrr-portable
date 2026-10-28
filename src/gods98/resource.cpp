// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.hpp"
#include "heap.hpp"

#define RESOURCE_MAX_WADS 10
#define RESOURCE_PATH_MAX 4096
#define WAD_MAGIC         "WWAD"
#define ENTRY_FLAG_EXPANDED 0x2u
#define EXPAND_HEADER_SZ  0x12u

typedef struct WadEntry {
    uint32_t flags;
    uint32_t orig;    /* uncompressed length */
    uint32_t stored;  /* length on disk */
    uint32_t off;     /* absolute file offset of the payload */
    uint32_t name_off;/* byte offset of the name inside RESOURCE_wads[].names */
} WadEntry;

typedef struct ResourceWad {
    FILE*     fp;         /* binary handle, kept open for reads */
    char      path[RESOURCE_PATH_MAX];
    uint32_t  file_size;
    uint32_t  entry_count;
    char*     names;      /* copied name blob (archive-relative paths) */
    WadEntry* entries;
} ResourceWad;

static ResourceWad s_wads[RESOURCE_MAX_WADS];
static int       s_wad_count = 0;
static char      s_wad_prefix[RESOURCE_PATH_MAX] = "LegoRR";

/* CD/STD fallback state (finding 01). */
#ifdef LEGORRR_BACKEND_WINAPI
#  define RES_PATH_SEP '\\' /* the corpus's "%c:\%s\%s" uses DOS separators */
#else
#  define RES_PATH_SEP '/'  /* host-native on the POSIX port */
#endif
static char  s_cd_drive    = 'A'; /* DAT_005343b4 analog; default mirrors .rdata */
static char  s_cd_data_dir[RESOURCE_PATH_MAX] = "Data";
static char  s_cd_root[RESOURCE_PATH_MAX] = ""; /* "" = "%c:\" default */

static const char* ares_cd_path(const char* name, char* out, size_t out_sz)
{
    size_t off;
    if (s_cd_root[0] != '\0') {
        off = (size_t)snprintf(out, out_sz, "%s%c%s%c", s_cd_root,
                               RES_PATH_SEP, s_cd_data_dir, RES_PATH_SEP);
    } else {
        off = (size_t)snprintf(out, out_sz, "%c:%c%s%c", s_cd_drive,
                               RES_PATH_SEP, s_cd_data_dir, RES_PATH_SEP);
    }
    if (off >= out_sz) {
        return out;
    }
    for (size_t i = 0; name[i] != '\0' && off + 1 < out_sz; ++i) {
        out[off++] = name[i] == '\\' ? RES_PATH_SEP : name[i];
    }
    out[off] = '\0';
    return out;
}

/* SoundPathFallbackFormat (FUN_0047f660): only format the fallback when a
 * drive is configured. The port permits a mounted-root override and applies
 * host separators/bounds checks inside ares_cd_path. */
// FUNCTION: LEGORR 0x0047f660
static int ares_cd_fallback_path(const char* name, char* out, size_t out_sz)
{
    if (s_cd_drive == 0) {
        return 0;
    }
    ares_cd_path(name, out, out_sz);
    return 1;
}

void resource_set_cd_drive(char drive)
{
    s_cd_drive = drive;
}

void resource_set_cd_data_dir(const char* dir)
{
    if (dir != NULL && dir[0] != '\0') {
        snprintf(s_cd_data_dir, sizeof(s_cd_data_dir), "%s", dir);
    }
}

void resource_set_cd_root(const char* root)
{
    if (root == NULL) {
        s_cd_root[0] = '\0';
    } else {
        snprintf(s_cd_root, sizeof(s_cd_root), "%s", root);
    }
}

static const char* ares_cd_root_path(const char* name, char* out, size_t out_sz)
{
    if (s_cd_root[0] != '\0') {
        snprintf(out, out_sz, "%s%c%s", s_cd_root, RES_PATH_SEP, name);
    } else {
        snprintf(out, out_sz, "%c:%c%s", s_cd_drive, RES_PATH_SEP, name);
    }
    return out;
}

int resource_cd_present(void)
{
    char path[RESOURCE_PATH_MAX];
    ares_cd_root_path("cd.key", path, sizeof(path));
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

int resource_data_present(void)
{
    char path[RESOURCE_PATH_MAX];
    ares_cd_root_path("delme.dat", path, sizeof(path));
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

/* ------------------------------------------------------------------ *
 *  Small helpers (no STL, no locale — ASCII fold only).
 * ------------------------------------------------------------------ */

static int ares_islower(int c) { return c >= 'a' && c <= 'z'; }

static int ares_strcasecmp(const char* a, const char* b)
{
    for (;;) {
        int ca = (unsigned char)*a++;
        int cb = (unsigned char)*b++;
        if (ares_islower(ca)) ca -= 0x20;
        if (ares_islower(cb)) cb -= 0x20;
        if (ca != cb) {
            return ca - cb;
        }
        if (ca == 0) {
            return 0;
        }
    }
}

static uint32_t ares_le32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static char* ares_strdup(const char* s)
{
    size_t n = strlen(s);
    char*  d = (char*)malloc(n + 1);
    if (d == NULL) {
        return NULL;
    }
    memcpy(d, s, n);
    d[n] = '\0';
    return d;
}

/* ------------------------------------------------------------------ *
 *  WWAD header + TOC parse (finding 01):
 *    +0x00 "WWAD"
 *    +0x04 u32 entry_count
 *    names    : entry_count x NUL-terminated archive-relative paths
 *    unc      : entry_count x NUL-terminated dev-tree paths (unused)
 *    toc      : entry_count x 16-byte { flags, orig, stored, off }
 *    then the data area at the recorded (absolute) offsets.
 * ------------------------------------------------------------------ */

static uint32_t ares_count_names(FILE* fp, uint32_t n)
{
    /* Returns the byte size of the two NUL-terminated name blocks
     * (archive-relative paths + UNC paths) read sequentially from the
     * current position. Leaves the stream positioned at the TOC. */
    uint32_t total = 0;
    for (int pass = 0; pass < 2; ++pass) {
        for (uint32_t i = 0; i < n; ++i) {
            uint32_t len = 0;
            int c;
            do {
                c = fgetc(fp);
                if (c == EOF) {
                    ferror(fp); /* clear error */
                    return total; /* truncated file */
                }
                ++len;
            } while (c != 0);
            total += len;
        }
    }
    return total;
}

static int ares_parse(FILE* fp, ResourceWad* out)
{
    uint8_t hdr[8];
    if (fseek(fp, 0, SEEK_END) != 0) {
        return 0;
    }
    long size = ftell(fp);
    if (size < 0) {
        return 0;
    }
    out->file_size = (uint32_t)size;

    if (fseek(fp, 0, SEEK_SET) != 0 || fread(hdr, 1, 8, fp) != 8) {
        return 0;
    }
    if (memcmp(hdr, WAD_MAGIC, 4) != 0) {
        return 0;
    }
    uint32_t n = ares_le32(hdr + 4);
    if (n == 0 || n > 0x100000) {
        return 0; /* entry-count sanity bound */
    }

    /* Count the name blocks (archive-relative + UNC) so we can slurp the
     * whole region in one allocation. */
    uint32_t names_bytes = ares_count_names(fp, n);

    /* Now slurp the name blob and record per-entry name offsets. */
    out->entry_count = n;
    out->entries = (WadEntry*)calloc(n, sizeof(WadEntry));
    out->names = (char*)malloc(names_bytes > 0 ? names_bytes : 1);
    if (out->entries == NULL || out->names == NULL) {
        free(out->entries);
        free(out->names);
        out->entries = NULL;
        out->names = NULL;
        return 0;
    }
    if (names_bytes > 0) {
        if (fseek(fp, 8, SEEK_SET) != 0
            || fread(out->names, 1, names_bytes, fp) != names_bytes) {
            return 0;
        }
    }
    uint32_t blob_pos = 0;
    for (int pass = 0; pass < 2; ++pass) {
        for (uint32_t i = 0; i < n; ++i) {
            if (pass == 0) {
                out->entries[i].name_off = blob_pos;
            }
            while (out->names[blob_pos] != '\0') {
                ++blob_pos;
            }
            ++blob_pos; /* skip the NUL */
        }
    }
    /* names[] positions now point into the blob at the archive-relative
     * name for each entry (pass 0 recorded them). */

    /* TOC: 16 bytes per entry. */
    uint32_t toc_off = 8 + names_bytes;
    for (uint32_t i = 0; i < n; ++i) {
        uint8_t row[16];
        if (fseek(fp, toc_off + (long)i * 16, SEEK_SET) != 0
            || fread(row, 1, 16, fp) != 16) {
            return 0;
        }
        out->entries[i].flags  = ares_le32(row + 0);
        out->entries[i].orig   = ares_le32(row + 4);
        out->entries[i].stored = ares_le32(row + 8);
        out->entries[i].off    = ares_le32(row + 12);
    }
    return 1;
}

/* ------------------------------------------------------------------ *
 *  Public API
 * ------------------------------------------------------------------ */

/* Partial WadMountStartup (FUN_0047f130): try the numbered WAD archive
 * paths and mount every archive that exists. The original startup routine
 * also handles CD detection, retry dialogs, and missing-data exits. */
// FUNCTION: LEGORR 0x0047f130
int resource_mount(const char* dir, const char* prefix)
{
    for (int i = 0; i < RESOURCE_MAX_WADS; ++i) {
        char path[RESOURCE_PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s%u.wad", dir, prefix,
                 (unsigned)i);
        FILE* fp = fopen(path, "rb");
        if (fp == NULL) {
            continue;
        }
        ResourceWad* w = &s_wads[s_wad_count];
        w->fp = fp;
        if (!ares_parse(fp, w)) {
            fprintf(stderr, "resource_mount: bad or truncated wad: %s\n",
                    path);
            fclose(fp);
            continue;
        }
        snprintf(w->path, sizeof(w->path), "%s", path);
        ++s_wad_count;
    }
    return s_wad_count;
}

void resource_set_wad_prefix(const char* prefix)
{
    if (prefix == NULL || prefix[0] == '\0') {
        return;
    }
    snprintf(s_wad_prefix, sizeof(s_wad_prefix), "%s", prefix);
}

const char* resource_get_wad_prefix(void)
{
    return s_wad_prefix;
}

int resource_mounted_count(void)
{
    return s_wad_count;
}

void resource_unmount_all(void)
{
    for (int i = 0; i < s_wad_count; ++i) {
        ResourceWad* w = &s_wads[i];
        if (w->fp != NULL) {
            fclose(w->fp);
            w->fp = NULL;
        }
        free(w->names);
        free(w->entries);
        w->names = NULL;
        w->entries = NULL;
    }
    s_wad_count = 0;
}

/* WadEntryNameLookup: case-insensitive scan of one mounted archive's names;
 * return its entry index or -1. Bounds checks are port safeguards. */
// FUNCTION: LEGORR 0x0048bda0
int resource_wad_lookup(int wad_idx, const char* name)
{
    if (wad_idx < 0 || wad_idx >= s_wad_count) {
        return -1;
    }
    ResourceWad* w = &s_wads[wad_idx];
    for (uint32_t i = 0; i < w->entry_count; ++i) {
        const char* cand = w->names + w->entries[i].name_off;
        if (ares_strcasecmp(cand, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

uint32_t resource_wad_entry_count(int wad_idx)
{
    if (wad_idx < 0 || wad_idx >= s_wad_count) {
        return 0;
    }
    return s_wads[wad_idx].entry_count;
}

uint32_t resource_wad_entry_stored(int wad_idx, int entry_idx)
{
    if (wad_idx < 0 || wad_idx >= s_wad_count
        || entry_idx < 0
        || (uint32_t)entry_idx >= s_wads[wad_idx].entry_count) {
        return 0;
    }
    return s_wads[wad_idx].entries[entry_idx].stored;
}

const char* resource_wad_entry_name(int wad_idx, int entry_idx)
{
    if (wad_idx < 0 || wad_idx >= s_wad_count
        || entry_idx < 0
        || (uint32_t)entry_idx >= s_wads[wad_idx].entry_count) {
        return NULL;
    }
    ResourceWad* w = &s_wads[wad_idx];
    return w->names + w->entries[entry_idx].name_off;
}

/* Open an entry inside mounted wad `wad_idx` (no payload yet). */
static Resource* ares_open_wad(int wad_idx, int entry_idx, const char* name)
{
    Resource* r = (Resource*)calloc(1, sizeof(Resource));
    if (r == NULL) {
        return NULL;
    }
    r->name = ares_strdup(name);
    if (r->name == NULL) {
        free(r);
        return NULL;
    }
    r->branch    = RESOURCE_WAD_BRANCH;
    r->wad_idx   = wad_idx;
    r->entry_idx = entry_idx;
    r->size      = -1;
    return r;
}

/* Partial WadPathOpen (FUN_0048bf70): scan mounted archives from newest to
 * oldest, resolve the entry, and return its resource handle. The original
 * opens the entry into a fixed stream slot immediately; this port reads its
 * payload lazily through resource_read. */
// FUNCTION: LEGORR 0x0048bf70
static Resource* ares_open_wad_path(const char* name, int mode)
{
    for (int w = s_wad_count - 1; w >= 0; --w) {
        int e = resource_wad_lookup(w, name);
        if (e >= 0) {
            Resource* r = ares_open_wad(w, e, name);
            if (r != NULL) {
                r->mode = mode;
            }
            return r;
        }
    }
    return NULL;
}

/* Open a loose file (STD branch) — payload read lazily by resource_read. */
static Resource* ares_open_std(const char* name, int mode)
{
    FILE* fp = fopen(name, "rb");
    if (fp == NULL) {
        return NULL;
    }
    long size = -1;
    if (fseek(fp, 0, SEEK_END) == 0) {
        size = ftell(fp);
    }
    fclose(fp);
    Resource* r = (Resource*)calloc(1, sizeof(Resource));
    if (r == NULL) {
        return NULL;
    }
    r->name = ares_strdup(name);
    if (r->name == NULL) {
        free(r);
        return NULL;
    }
    r->branch    = RESOURCE_STD_BRANCH;
    r->wad_idx   = -1;
    r->entry_idx = -1;
    r->mode      = mode;
    r->size      = (int32_t)size;
    return r;
}

/* GameFileOpenDispatch (FUN_0047f6e0): choose the WAD or loose-file path
 * for the requested mode. Payload reading is deferred to resource_read. */
// FUNCTION: LEGORR 0x0047f6e0
Resource* resource_open(const char* name, int mode)
{
    if (name == NULL || name[0] == '\0') {
        return NULL;
    }

    /* STD branch first for AUTO when the loose file exists. */
    if (mode == RESOURCE_AUTO || mode == RESOURCE_STD_ONLY) {
        Resource* r = ares_open_std(name, mode);
        if (r != NULL) {
            return r;
        }
        if (mode == RESOURCE_STD_ONLY) {
            return NULL;
        }
    }

    /* WAD branch: later mounts shadow earlier ones. */
    Resource* wad = ares_open_wad_path(name, mode);
    if (wad != NULL) {
        return wad;
    }

    /* CD/STD fallback (AUTO only): "%c:\%s\%s" with the drive letter +
     * data dir (finding 01, FUN_0047f660). */
    if (mode == RESOURCE_AUTO) {
        char path[RESOURCE_PATH_MAX];
        if (ares_cd_fallback_path(name, path, sizeof(path))) {
            Resource* r = ares_open_std(path, mode);
            if (r != NULL) {
                return r;
            }
        }
    }
    return NULL;
}

int32_t resource_read(Resource* r)
{
    if (r == NULL || r->data != NULL) {
        return r != NULL ? r->size : -1;
    }
    if (r->branch == RESOURCE_STD_BRANCH) {
        FILE* fp = fopen(r->name, "rb");
        if (fp == NULL || r->size < 0) {
            if (fp != NULL) {
                fclose(fp);
            }
            return -1;
        }
        uint8_t* buf = (uint8_t*)malloc((size_t)r->size > 0 ? (size_t)r->size : 1u);
        if (buf == NULL || fread(buf, 1, (size_t)r->size, fp) != (size_t)r->size) {
            free(buf);
            fclose(fp);
            return -1;
        }
        fclose(fp);
        r->data = buf;
        return r->size;
    }

    ResourceWad* w = &s_wads[r->wad_idx];
    WadEntry*    e = &w->entries[r->entry_idx];

    /* Read the stored bytes. */
    long payload_off = (long)e->off;
    if (payload_off < 0 || (uint32_t)payload_off + e->stored > w->file_size) {
        return -1;
    }
    uint8_t* stored = (uint8_t*)malloc(e->stored > 0 ? e->stored : 1u);
    if (stored == NULL || fseek(w->fp, payload_off, SEEK_SET) != 0
        || fread(stored, 1, e->stored, w->fp) != e->stored) {
        free(stored);
        return -1;
    }

    if ((e->flags & ENTRY_FLAG_EXPANDED) == 0) {
        r->data = stored;
        r->size = (int32_t)e->stored;
        return r->size;
    }

    /* Expanded entry (finding 01): 4-byte magic, type byte at [3],
     * big-endian ("byte-swapped") length at [4], payload from +0x12. */
    if (e->stored < EXPAND_HEADER_SZ) {
        free(stored);
        return -1;
    }
    uint32_t type = stored[3];
    uint32_t len  = ((uint32_t)stored[4] << 24) | ((uint32_t)stored[5] << 16)
                  | ((uint32_t)stored[6] << 8) | (uint32_t)stored[7];
    if (type != 0) {
        /* FUN_0049c8e0 (1) / FUN_0049ca60 (2) — dormant: no shipped entry
         * sets the expanded flag, so the codecs are intentionally not
         * ported yet (finding 01 §"dormant"). */
        free(stored);
        return -1;
    }
    uint8_t* buf = (uint8_t*)malloc(len > 0 ? len : 1u);
    if (buf == NULL) {
        free(stored);
        return -1;
    }
    if (e->stored < EXPAND_HEADER_SZ + len) {
        free(stored);
        free(buf);
        return -1;
    }
    memcpy(buf, stored + EXPAND_HEADER_SZ, len);
    free(stored);
    r->data = buf;
    r->size = (int32_t)len;
    return r->size;
}

void resource_free(Resource* r)
{
    if (r == NULL) {
        return;
    }
    free(r->data);
    free(r->name);
    free(r);
}

/* --- the fixed-record slot resources (see resource.hpp) -------------------- */
#define FIXED_RECORD_SLOTS 16

/* The fixed-record slot band: sixteen 16-byte records {resource@+0x00,
 * in-use flag@+0x04} at DAT_005756a0 (all-zero image bytes; the corpus
 * scales the index shl 4 and strides blindly). The +0x08/+0x0c words
 * belong to the unported reader (FUN_0048c010) — pad only. */
typedef struct FixedRecordSlot {
    void*    resource;  /* +0x00 */
    uint32_t in_use;    /* +0x04 */
    uint8_t  pad[8];    /* +0x08..0x0f */
} FixedRecordSlot;

typedef char fixed_record_slot_stride_check[sizeof(FixedRecordSlot) == 16u ? 1 : -1];

// GLOBAL: LEGORR 0x005756a0
static FixedRecordSlot s_fixed_slots[FIXED_RECORD_SLOTS];

// FUNCTION: LEGORR 0x0048bfc0
void fixed_record_slot_resource_release(int slot)
{
    /* The corpus scales the index shl 4 and strides blindly (the tests only
     * release in-range slots) and clears ONLY the flag — the stale pointer
     * is unobservable because the get path checks the flag first. */
    if (s_fixed_slots[slot].in_use != 0) {
        engine_heap_free(s_fixed_slots[slot].resource);
        s_fixed_slots[slot].in_use = 0;
    }
}

// FUNCTION: LEGORR 0x0048bff0
void* fixed_record_slot_resource_get(int slot)
{
    if (slot < 0 || slot >= FIXED_RECORD_SLOTS) {
        return NULL; /* deviation: the corpus strides blindly */
    }
    return s_fixed_slots[slot].in_use != 0 ? s_fixed_slots[slot].resource : NULL;
}

void fixed_record_slot_install(int slot, void* resource)
{
    if (slot >= 0 && slot < FIXED_RECORD_SLOTS) {
        s_fixed_slots[slot].resource = resource;
        s_fixed_slots[slot].in_use = 1;
    }
}

int fixed_record_slot_flag_peek(int slot)
{
    return (slot >= 0 && slot < FIXED_RECORD_SLOTS) ? (int)s_fixed_slots[slot].in_use : 0;
}

/* WadMemoryStreamBufferGet (FUN_0048c030): the corpus wrapper routes the
 * slot through the fixed-record slot reader (FUN_0048bff0) and drops the
 * result — the observable half is the slot reader's resolution. */
// FUNCTION: LEGORR 0x0048c030
void wad_memory_stream_buffer_get(int slot)
{
    (void)fixed_record_slot_resource_get(slot);
}

/* --- wad payload band (2026-10-25; finding 01 codec story) ---------------
 * The corpus decodes expanded wad entries through the RNC-header dispatch
 * (the DAT_004b2d08 global holds the three-byte magic "RNC"), builds the
 * stream-buffer handle pair, and clears the type-1 Huffman table between
 * streams. The codecs themselves stay dormant (no shipped entry sets the
 * expanded flag), as documented above. */

/* WadPayloadDecodeDispatch (FUN_0049c840): verify the "RNC" magic, then
 * branch on the type byte at [3]. Type 0 is a raw pass-through: the size
 * dword at +4 read big-endian, payload from +0x12, copied in DWORD chunks
 * plus the trailing (size & 3) bytes. Types 1/2 hand over to the dormant
 * codecs; any other byte answers the unknown-type verdict. */
#define WAD_DECODE_MAGIC_FAIL 0xffff
#define WAD_DECODE_TYPE_FAIL  0xfffe

// FUNCTION: LEGORR 0x0049c840
int wad_payload_decode_dispatch(const void* header, void* dest)
{
    if (header == NULL || dest == NULL) {
        /* deviation: the corpus strides both operands blindly */
        return WAD_DECODE_MAGIC_FAIL;
    }
    const unsigned char* h = (const unsigned char*)header;
    if (memcmp(h, "RNC", 3) != 0) {
        return WAD_DECODE_MAGIC_FAIL;
    }
    const unsigned char type = h[3];
    if (type == 0u) {
        /* (uVar >> 8 & 0xff + uVar * 0x100) * 0x100 + ... = big-endian read */
        const uint32_t size = ((uint32_t)h[4] << 24) | ((uint32_t)h[5] << 16)
                            | ((uint32_t)h[6] << 8) | (uint32_t)h[7];
        /* deviation: the corpus copies in dword+byte stages; the whole-span
         * memcpy is byte-identical and bounds-safe on both sides. */
        memcpy(dest, h + EXPAND_HEADER_SZ, size);
        return 0;
    }
    if (type == 1u || type == 2u) {
        /* The codecs FUN_0049c8e0 / FUN_0049ca60 (portable, dormant) own
         * these paths; no shipped wad reaches them. Deviation: without the
         * codec bodies built, the port answers the type marker itself so
         * the verdict (non-zero == decode failed) stays observable. */
        return (int)type;
    }
    return WAD_DECODE_TYPE_FAIL;
}

/* ExpandWadPayload (FUN_0049c7c0): probe or materialize an expanded entry
 * payload. The size is the big-endian length dword at +4; a NULL out-pointer
 * is the corpus's size probe (the size alone answers). Otherwise the engine
 * heap allocates, the dispatch decodes, and a failed dispatch frees. */
// FUNCTION: LEGORR 0x0049c7c0
long wad_expand_payload(const void* header, void** out_payload)
{
    if (header == NULL) {
        return 0; /* deviation: the corpus strides the payload blindly */
    }
    const unsigned char* h = (const unsigned char*)header;
    /* corpus mov eax,[ebx+4]: one dword load, and the byteswap rides
     * arithmetically off the register so MSVC5 emits its and/shl/add
     * expansion rather than four movzx byte loads */
    const uint32_t raw = *(const uint32_t*)(h + 4);
    const uint32_t swapped = ((raw << 24) & 0xff000000u)
                           | ((raw << 8) & 0x00ff0000u)
                           | ((raw >> 8) & 0x0000ff00u) | (raw >> 24);
    const long size = (long)swapped;
    if (out_payload == NULL) {
        return size;
    }
    void* buf = engine_heap_alloc(size); /* FUN_0048dbd0 */
    if (buf == NULL) {
        return 0;
    }
    if (wad_payload_decode_dispatch(h, buf) != 0) { /* corpus tests ax,ax */
        engine_heap_free(buf); /* FUN_0048db80 */
        return 0;
    }
    *out_payload = buf;
    return size;
}

/* WadHuffmanTableClear (FUN_0049cf50): zero the decode table's `count`
 * 16-byte entries: DWORD 0 at +0, the WORD 0xffff at +4, DWORD 0 at +8,
 * and the WORD 0 at +12. A zero count clears nothing. Deviations: the
 * corpus's count is char-truncated (multiples of 256 alias to nothing),
 * documented rather than replayed, and the blind table stride is guarded. */
// FUNCTION: LEGORR 0x0049cf50
void wad_huffman_table_clear(void* table, int count)
{
    if (table == NULL || count <= 0) {
        return;
    }
    for (int i = 0; i != count; ++i) {
        unsigned char* e = (unsigned char*)table + (size_t)i * 16u;
        const uint32_t zero = 0u;
        const uint16_t full = 0xffffu;
        const uint16_t empty = 0u;
        memcpy(e + 0, &zero, 4);
        memcpy(e + 4, &full, 2);
        memcpy(e + 8, &zero, 4);
        memcpy(e + 12, &empty, 2);
    }
}

/* GameFileHandleAllocate (FUN_0047fed0): the file-layer handle pair. Kind 1
 * (wads) is the 8-byte record flagged 1 with no stream buffer; kind 0 (the
 * loose-file route) flags 0 and carries the 12-byte stream buffer behind,
 * releasing the record if the buffer alloc fails. Deviations: the record
 * fields widen to (int32_t flag; void* stream) for LP64 (the corpus's
 * dwords, 8 bytes, alias this record's front half) and the corpus's
 * blind-first store for kind 1 is guarded. The corpus's wrapper chain
 * 0x47ff30 is a bare call into the heap's alloc entry. */
// FUNCTION: LEGORR 0x0047fed0
void* game_file_handle_allocate(int kind)
{
    if (kind != 0 && kind != 1) {
        return NULL;
    }
    GameFileHandle* h = (GameFileHandle*)engine_heap_alloc(
        (long)sizeof(GameFileHandle));
    if (h == NULL) {
        return NULL;
    }
    h->flag = kind;
    /* Deviation: the corpus leaves +4 uninitialized for kind 1; the port
     * zeroes the stream field for deterministic handles. */
    h->stream = NULL;
    if (kind == 1) {
        return h;
    }
    h->stream = engine_heap_alloc(12);
    if (h->stream != NULL) {
        return h;
    }
    engine_heap_free_wrapper(h);
    return NULL;
}

/* WadEntryReadIntoStreamSlot (FUN_0047fea0): zero the slot pair behind the
 * handle, resolve the entry through the corpus's wad path opener with the
 * search-all selector, and answer whether the entry exists. Deviations:
 * the opener's record-slot handle (its slot table is modeled in
 * fixed_record_slot_resource_get) rides as the opened resource pointer and
 * the corpus's -1 sentinel becomes NULL; the search-all index arrives as
 * the wad-only open mode with the corpus's global at -1 inlined. */
// FUNCTION: LEGORR 0x0047fea0
int wad_entry_read_into_stream_slot(WadStreamSlots* slots, const char* name)
{
    if (slots == NULL) {
        return 0; /* deviation: the corpus strides the slots blindly */
    }
    slots->word2 = 0u;
    slots->stream = NULL;
    const Resource* handle = resource_open(name, RESOURCE_WAD_ONLY);
    slots->handle = handle;
    return handle != NULL;
}

/* --- resource-handle release band (2026-10-25; corpus .data band) ---------
 * ResourceHandleSetRelease (FUN_0041a850) frees the corpus's global handle
 * set: the DAT_004ddc04 stride-8 record band (the first DWORD of each pair,
 * while the walker stays below DAT_004ddcc4) followed by twelve named
 * globals, freed in corpus order. Deviations: the band lands as
 * own storage (install/peek hooks for the named globals), the
 * corpus frees without clearing, and the port nulls each freed slot. */
static struct {
    void* handle;      /* corpus band dword (stride 8; the word behind rides) */
    uint32_t word2;
} s_handle_set_band[24]; /* DAT_004ddc04..DAT_004ddcc4, stride 8 */
static void* s_handle_set_named[12]; /* DAT_004dd734..9a8/DAT_004ddad4/d8 */

/* Port hook: slots 0..23 are the band (stride-8 units), 24..35 the named
 * globals in address order: 734, 738, 73c, 740, 744, 748, 99c, 9a0, 9a4,
 * 9a8, ad4, ad8. */
void resource_handle_set_slot_install(int slot, void* res)
{
    if (slot < 24) {
        s_handle_set_band[slot].handle = res;
    } else if (slot >= 24 && slot < 36) {
        s_handle_set_named[slot - 24] = res;
    }
}

void* resource_handle_set_slot_peek(int slot)
{
    if (slot < 24) {
        return s_handle_set_band[slot].handle;
    }
    if (slot >= 24 && slot < 36) {
        return s_handle_set_named[slot - 24];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0041a850
void resource_handle_set_release(void)
{
    for (int i = 0; i != 24; ++i) {
        /* corpus: stride the band, free the non-zero dwords under cc4 */
        if (s_handle_set_band[i].handle != NULL) {
            engine_heap_free(s_handle_set_band[i].handle); /* FUN_0048db80 */
            s_handle_set_band[i].handle = NULL; /* deviation: the corpus dangles */
        }
    }
    /* corpus free order: 734 73c 744 ad4 99c 9a4 738 740 748 ad8 9a0 9a8 */
    static const size_t order[12] = { 0, 2, 4, 10, 6, 8, 1, 3, 5, 11, 7, 9 };
    for (size_t k = 0; k != 12; ++k) {
        void** p = &s_handle_set_named[order[k]];
        if (*p != NULL) {
            engine_heap_free(*p);
            *p = NULL;
        }
    }
}

/* --- the key-group action-code band (see resource.hpp) ---------------------
 * InputKeyGroupRegister (FUN_0041a7d0) stages one interaction-action group:
 * the count word rides in at the pair's +4 slot FIRST (corpus order), then
 * a count*4 heap array takes the code dwords. The codes ride the stack as
 * the corpus's own cdecl varargs and are read out with va_arg in index
 * order. Deviations: the staging copy of the variadic window into locals
 * (and the always-zero byte tail) fold into a direct dword copy, and the
 * group guard answers where the corpus strides the band blindly.
 * Registering the same group twice re-stages in place; the corpus leaks the
 * superseded array, and so does the port. */
// FUNCTION: LEGORR 0x0041a7d0
void resource_input_key_group_register(int group, int count, ...)
{
    va_list ap;

    if (group < 0 || group >= 24) {
        return; /* deviation: the corpus strides the band blindly */
    }
    s_handle_set_band[group].word2 = (uint32_t)count;
    void* array = engine_heap_alloc((long)count * 4); /* FUN_0048dbd0 */
    s_handle_set_band[group].handle = array;
    va_start(ap, count);
    if (array != NULL) {
        uint32_t* dst = (uint32_t*)array;
        const uint32_t n = (uint32_t)count & 0x3fffffffu;
        for (uint32_t i = 0; i != n; ++i) {
            dst[i] = (uint32_t)va_arg(ap, int);
        }
    }
    va_end(ap);
}

/* InteractionActionCodeMapInitialize (FUN_0041a660): registers the fixed
 * eighteen interaction-action groups in the corpus's call order — groups
 * 0..5, 7, 8, then 0xa, 0xb, 0xc, with group 9 landing between 0xc and
 * 0xd, and group 6 left unregistered (the corpus skips it). The action
 * codes ride as the corpus's own pushed constants, one push per code. */
// FUNCTION: LEGORR 0x0041a660
void interaction_action_code_map_initialize(void)
{
    resource_input_key_group_register(0, 4, 1, 2, 3, 4);
    resource_input_key_group_register(1, 3, 5, 6, 0xe);
    resource_input_key_group_register(2, 1, 7);
    resource_input_key_group_register(3, 1, 0xe);
    resource_input_key_group_register(4, 1, 0x10);
    resource_input_key_group_register(5, 2, 9, 0xe);
    resource_input_key_group_register(7, 4, 0xb, 0xc, 0xd, 0xf);
    resource_input_key_group_register(8, 2, 0x2b, 0x2f);
    resource_input_key_group_register(0xa, 7, 0x16, 0x14, 0x42, 0x19,
                                      0x2d, 0x2e, 0x29);
    resource_input_key_group_register(0xb, 6, 0x16, 0x14, 0x19, 0x2d, 0x2e,
                                      0x1a);
    resource_input_key_group_register(0xc, 2, 0x18, 0x29);
    resource_input_key_group_register(9, 10, 0x1f, 0x17, 0x15, 0x30, 0x21,
                                      0x40, 0x39, 0x2d, 0x2e, 0x20);
    resource_input_key_group_register(0xd, 4, 0x24, 0x22, 0x41, 0x28);
    resource_input_key_group_register(0xe, 1, 0x2a);
    resource_input_key_group_register(0xf, 3, 0x2c, 0x2e, 0x2d);
    resource_input_key_group_register(0x10, 8, 0x31, 0x32, 0x33, 0x34, 0x35,
                                      0x36, 0x37, 0x38);
    resource_input_key_group_register(0x11, 6, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
                                      0x3f);
    resource_input_key_group_register(0x12, 4, 0x43, 0x44, 0x45, 0x46);
}

void resource_handle_set_band_word_install(int unit, uint32_t value)
{
    if (unit >= 0 && unit < 24) {
        s_handle_set_band[unit].word2 = value;
    }
}

uint32_t resource_handle_set_band_word_peek(int unit)
{
    return (unit >= 0 && unit < 24) ? s_handle_set_band[unit].word2 : 0u;
}

/* ResourceHandleListDispose (FUN_00410ae0): free the list's non-zero
 * handles (the count lives behind), then the array, then the record — the
 * record is freed even when its array word is zero. Deviations: the record
 * fields widen LP64-safe and the NULL record is guarded (the corpus strides
 * blindly). */
// FUNCTION: LEGORR 0x00410ae0
void resource_handle_list_dispose(ResourceHandleList* list)
{
    if (list == NULL) {
        return; /* deviation: the corpus derefs blindly */
    }
    if (list->handles != NULL) {
        for (int32_t i = 0; i < list->count; ++i) {
            if (list->handles[i] != NULL) {
                engine_heap_free(list->handles[i]); /* FUN_0048db80 */
            }
        }
        engine_heap_free(list->handles);
    }
    engine_heap_free(list);
}

/* ResourceHandleListAppend (FUN_00410aa0): append a heap-owned string only
 * while the preallocated handle array has capacity. */
// FUNCTION: LEGORR 0x00410aa0
void resource_handle_list_append(ResourceHandleList* list, const char* value)
{
    if (list == NULL || list->handles == NULL
        || list->count >= (int32_t)(uint32_t)list->word1 || value == NULL) {
        return;
    }
    const size_t length = strlen(value);
    if (length >= 0x7fffffffU) return;
    char* copy = (char*)engine_heap_alloc((long)length + 1);
    if (copy == NULL) return;
    memcpy(copy, value, length + 1);
    list->handles[list->count] = copy;
    ++list->count;
}
