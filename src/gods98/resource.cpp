// SPDX-License-Identifier: LGPL-3.0-or-later
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

    /* WAD branch: scan mounted wads highest-first so later mounts shadow
     * earlier ones (finding 01, FUN_0048bf70 with idx == -1). */
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

    /* CD/STD fallback (AUTO only): "%c:\%s\%s" with the drive letter +
     * data dir (finding 01, FUN_0047f660). */
    if (mode == RESOURCE_AUTO && s_cd_drive != 0) {
        char path[RESOURCE_PATH_MAX];
        ares_cd_path(name, path, sizeof(path));
        Resource* r = ares_open_std(path, mode);
        if (r != NULL) {
            return r;
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
static void* s_fixed_slot[FIXED_RECORD_SLOTS];
static int   s_fixed_flag[FIXED_RECORD_SLOTS];

void fixed_record_slot_resource_release(int slot)
{
    if (slot < 0 || slot >= FIXED_RECORD_SLOTS) {
        return; /* deviation: the corpus strides the index blindly */
    }
    if (s_fixed_flag[slot] != 0) {
        engine_heap_free(s_fixed_slot[slot]);
        s_fixed_slot[slot] = NULL;
        s_fixed_flag[slot] = 0;
    }
}

void* fixed_record_slot_resource_get(int slot)
{
    if (slot < 0 || slot >= FIXED_RECORD_SLOTS) {
        return NULL; /* deviation: the corpus strides blindly */
    }
    return s_fixed_flag[slot] != 0 ? s_fixed_slot[slot] : NULL;
}

void fixed_record_slot_install(int slot, void* resource)
{
    if (slot >= 0 && slot < FIXED_RECORD_SLOTS) {
        s_fixed_slot[slot] = resource;
        s_fixed_flag[slot] = 1;
    }
}

int fixed_record_slot_flag_peek(int slot)
{
    return (slot >= 0 && slot < FIXED_RECORD_SLOTS) ? s_fixed_flag[slot] : 0;
}

/* WadMemoryStreamBufferGet (FUN_0048c030): the corpus wrapper routes the
 * slot through the fixed-record slot reader (FUN_0048bff0) and drops the
 * result — the observable half is the slot reader's resolution. */
void wad_memory_stream_buffer_get(int slot)
{
    (void)fixed_record_slot_resource_get(slot);
}
