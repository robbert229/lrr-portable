// SPDX-License-Identifier: LGPL-3.0-or-later
/* Block-text (Lego*) config: port-only conveniences over the original
 * Config.c records (config.c, finding 12), plus ConfigBooleanParse and
 * RegistryPathSplit from other original units. See block.hpp. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.hpp"
#include "string.hpp"

/* ConfigBooleanParse compares these file-backed tokens directly. Their
 * contents and addresses are confirmed by the mapped DAT inventory and
 * decompile references. */
// GLOBAL: LEGORR 0x004aabec
static const char s_config_bool_yes[] = "YES";
// GLOBAL: LEGORR 0x004aabe4
static const char s_config_bool_true[] = "TRUE";
// GLOBAL: LEGORR 0x004a4124
static const char s_config_bool_on[] = "ON";
// GLOBAL: LEGORR 0x004aabe0
static const char s_config_bool_no[] = "NO";
// GLOBAL: LEGORR 0x004aabd8
static const char s_config_bool_false[] = "FALSE";
// GLOBAL: LEGORR 0x004a4120
static const char s_config_bool_off[] = "OFF";

/* --- accessors ------------------------------------------------------------ */

const char* block_name(const BlockRec* r)  { return r ? r->key : NULL; }
const char* block_value(const BlockRec* r) { return r ? r->value : NULL; }

/* An item follows its container directly, one level deeper. */
BlockRec* block_first_child(const BlockRec* r)
{
    if (r == NULL || r->next == NULL || r->next->depth <= r->depth) {
        return NULL;
    }
    return r->next;
}

BlockRec* block_next_sibling(const BlockRec* r)
{
    const BlockRec* next;
    if (r == NULL) {
        return NULL;
    }
    for (next = r->next; next != NULL; next = next->next) {
        if (next->depth < r->depth) {
            return NULL;
        }
        if (next->depth == r->depth) {
            return next->key != NULL ? (BlockRec*)next : NULL;
        }
    }
    return NULL;
}

int block_has_children(const BlockRec* r) { return block_first_child(r) != NULL; }

size_t block_record_count(const BlockFile* bf)
{
    size_t count = 0;
    for (; bf != NULL; bf = bf->next) {
        if (bf->key != NULL) {
            count++;
        }
    }
    return count;
}

BlockRec* block_record_at(const BlockFile* bf, size_t index)
{
    for (; bf != NULL; bf = bf->next) {
        if (bf->key != NULL) {
            if (index == 0) {
                return (BlockRec*)bf;
            }
            index--;
        }
    }
    return NULL;
}

/* --- typed getters --------------------------------------------------------- */

int block_get_str(BlockFile* bf, const char* path, char* buf, size_t sz)
{
    const char* value = block_get_strptr(bf, path);
    if (value == NULL) {
        return 0;
    }
    snprintf(buf, sz, "%s", value);
    return 1;
}

int block_get_int(BlockFile* bf, const char* path, int dflt)
{
    const char* value = block_get_strptr(bf, path);
    if (value == NULL) {
        return dflt;
    }
    char* end = NULL;
    long v = strtol(value, &end, 10);
    return (end == value) ? dflt : (int)v;
}

float block_get_float(BlockFile* bf, const char* path, float dflt)
{
    const char* value = block_get_strptr(bf, path);
    if (value == NULL) {
        return dflt;
    }
    char* end = NULL;
    float v = strtof(value, &end);
    return (end == value) ? dflt : v;
}

/* ConfigBooleanParse (FUN_00477710): YES/TRUE/ON → 1; NO/FALSE/OFF → 0;
 * otherwise 2. */
// FUNCTION: LEGORR 0x00477710
int block_get_bool(const char* value)
{
    if (value == NULL) return 2;
    if (strcasecmp_lite(value, s_config_bool_yes) == 0
        || strcasecmp_lite(value, s_config_bool_true) == 0
        || strcasecmp_lite(value, s_config_bool_on) == 0) {
        return 1;
    }
    if (strcasecmp_lite(value, s_config_bool_no) == 0
        || strcasecmp_lite(value, s_config_bool_false) == 0
        || strcasecmp_lite(value, s_config_bool_off) == 0) {
        return 0;
    }
    return 2;
}

/* RegistryPathSplit (FUN_0048b330, finding 402): the corpus copies the
 * leading backslash component, terminates both ends, and skips one
 * separator. The registry's 100-byte caller buffer bound is the
 * caller's business (finding 402 notes the helper itself is unbounded —
 * reproduced, caller checks live where the registry lands). */
// FUNCTION: LEGORR 0x0048b330
const char* registry_path_split(const char* path, char* out)
{
    if (path == NULL || out == NULL) {
        return path; /* deviation: pinned by test_block.cpp — corpus strides blind */
    }
    /* Corpus shape: NUL the destination, seed the character, gate entry on
     * it, then a do-while whose back edge is the character's own test —
     * the source advances before the byte is stored. */
    *out = '\0';
    char c = *path;
    if (c != '\0') {
        do {
            if (c == '\\') {
                break;
            }
            ++path;
            *out = c;
            ++out;
            c = *path;
        } while (c != '\0');
    }
    if (*path == '\\') {
        ++path;
    }
    *out = '\0';
    return path;
}
