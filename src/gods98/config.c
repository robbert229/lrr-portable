// SPDX-License-Identifier: LGPL-3.0-or-later
/* Config.c — original translation unit G98-006 (text 00478df0-00479500,
 * .rdata 0049e8f8-0049e908, .bss 00506098-00506524; finding 505).
 *
 * Rebuilt in the original's shape (issue #394, ADR 0005): one file for the
 * unit, its functions in original order, its state in one typed globals
 * struct laid out as the original .bss, and the C-compatible subset so the
 * matching build compiles it as C. The native port compiles it as C++.
 *
 * A loaded file is a flat list of 32-byte records taken from a pool of
 * doubling blocks. Each record holds one `key value` pair and its brace
 * depth (finding 12); lookups walk the list. */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "resource.h"
#include "utils.h"
#include "crt/vc5names.h"

#if defined(_MSC_VER) && _MSC_VER == 1100
#include <windows.h>
#include <d3drm.h>
#endif

#define CONFIG_MAXLISTS 32
#define CONFIG_MAXTOKENS 100
#define CONFIG_JOINPATH_SIZE 1024

#define CONFIG_FLAG_INITIALISED 0x1

typedef struct ConfigGlobs {
    char join_path[CONFIG_JOINPATH_SIZE];  /* 00506098: string_path_join */
    BlockRec* lists[CONFIG_MAXLISTS];      /* 00506498: pool blocks */
    BlockRec* free_list;                   /* 00506518 */
    unsigned int list_count;               /* 0050651c */
    unsigned int flags;                    /* 00506520 */
} ConfigGlobs;

// GLOBAL: LEGORR 0x00506098
static ConfigGlobs configGlobs;

/* Config_GetAngle's and Config_GetRGBValue's constants (.rdata). */
// GLOBAL: LEGORR 0x0049e8f8
static const float s_config_angle_zero = 0.0f;
// GLOBAL: LEGORR 0x0049e8fc
static const float s_config_angle_degrees_to_turn = 1.0f / 360.0f;
// GLOBAL: LEGORR 0x0049e900
static const float s_config_angle_turn_radians = 6.2831855f;
// GLOBAL: LEGORR 0x0049e904
static const float s_config_rgb_scale = 1.0f / 255.0f;

/* Literals the unit shares with earlier objects. The original's "" for a
 * missing angle resolves to a zero byte in another object's .bss. */
// GLOBAL: LEGORR 0x004a1cd4
static const char s_config_rgb_separator[] = ":";
// GLOBAL: LEGORR 0x004aab18
static const char s_config_separator[] = "::";
// GLOBAL: LEGORR 0x004b8a14
static const char s_config_angle_empty[4] = { 0, 0, 0, 0 };

// FUNCTION: LEGORR 0x00478df0
void block_text_token_pool_init(void)
{
    memset(configGlobs.lists, 0, sizeof(configGlobs.lists));
    configGlobs.free_list = NULL;
    configGlobs.list_count = 0;
    configGlobs.flags = CONFIG_FLAG_INITIALISED;
}

// FUNCTION: LEGORR 0x00478e20
void block_text_token_pool_destroy(void)
{
    unsigned int loop;

    for (loop = 0; loop < CONFIG_MAXLISTS; loop++) {
        if (configGlobs.lists[loop]) free(configGlobs.lists[loop]);
    }
    configGlobs.free_list = NULL;
    configGlobs.flags = 0;
}

// FUNCTION: LEGORR 0x00478e60
BlockFile* block_open(const char* path)
{
    int size;
    char* s;
    BlockRec* root = NULL;
    BlockRec* conf;
    int comment = 0;
    unsigned int loop;

    if ((s = (char*)read_whole_file(path, &size)) != NULL) {
        conf = root = block_record_create(NULL);
        root->file_text = s;

        for (loop = 0; loop < (unsigned int)size; loop++) {
            if (*s == ';') comment = 1;
            else if (*s == '\n') comment = 0;
            if (comment || *s == '\t' || *s == '\n' || *s == '\r' || *s == ' ') {
                *s = '\0';
            }
            s++;
        }

        for (s = root->file_text, loop = 0; loop < (unsigned int)size; loop++) {
            if (*s != '\0') {
                if (*s == '}' && s[1] == '\0') conf->depth--;
                else if (conf->key == NULL) conf->key = s;
                else {
                    conf->value = s;
                    conf = block_record_create(conf);
                    if (*s == '{' && s[1] == '\0') conf->depth++;
                }
                while (loop < (unsigned int)size && *s++ != '\0') loop++;
            }
            else s++;
        }
    }

    return root;
}

// FUNCTION: LEGORR 0x00478f50
const char* string_path_join(const char* root, ...)
{
    va_list args;
    const char* part;

    strcpy(configGlobs.join_path, root);
    va_start(args, root);
    while ((part = va_arg(args, const char*)) != NULL) {
        strcat(configGlobs.join_path, s_config_separator);
        strcat(configGlobs.join_path, part);
    }
    va_end(args);

    return configGlobs.join_path;
}

// FUNCTION: LEGORR 0x00478ff0
BlockRec* block_subgroup(BlockFile* root, const char* path)
{
    BlockRec* conf;

    if ((conf = block_find(root, path)) != NULL) {
        if (conf->depth < conf->next->depth) return conf->next;
    }

    return NULL;
}

// FUNCTION: LEGORR 0x00479020
BlockRec* block_next_equal_key(const BlockRec* start)
{
    unsigned int depth = start->depth;
    const BlockRec* conf = start;

    while (conf && conf->depth >= depth) {
        conf = conf->next;
        if (conf->depth == depth) return (BlockRec*)conf;
    }

    return NULL;
}

// FUNCTION: LEGORR 0x00479050
char* block_get_str_copy(BlockFile* root, const char* path)
{
    BlockRec* conf;
    char* str;

    if ((conf = block_find(root, path)) != NULL && conf->value != NULL) {
        str = (char*)malloc(strlen(conf->value) + 1);
        strcpy(str, conf->value);
        return str;
    }

    return NULL;
}

// FUNCTION: LEGORR 0x004790b0
const char* block_get_strptr(BlockFile* root, const char* path)
{
    BlockRec* conf;

    if ((conf = block_find(root, path)) != NULL) return conf->value;

    return NULL;
}

// FUNCTION: LEGORR 0x004790d0
int block_get_bool_path(BlockFile* root, const char* path)
{
    char* str;
    int res = 2;

    if ((str = block_get_str_copy(root, path)) != NULL) {
        res = block_get_bool(str);
        free(str);
    }

    return res;
}

// FUNCTION: LEGORR 0x00479110
float config_angle_radians_get(BlockFile* root, const char* path)
{
    float deg = (float)atof(block_get_strptr(root, path)
                            ? block_get_strptr(root, path)
                            : s_config_angle_empty);

    if (deg != s_config_angle_zero) {
        return (deg * s_config_angle_degrees_to_turn) * s_config_angle_turn_radians;
    }

    return s_config_angle_zero;
}

// FUNCTION: LEGORR 0x00479170
int config_rgb_vector_read(BlockFile* root, const char* path,
                           float* r, float* g, float* b)
{
    char* argv[CONFIG_MAXTOKENS];
    char* str;
    int res = 0;

    if ((str = block_get_str_copy(root, path)) != NULL) {
        if (sound_variant_split(str, argv, s_config_rgb_separator) == 3) {
            *r = (float)atoi(argv[0]) * s_config_rgb_scale;
            *g = (float)atoi(argv[1]) * s_config_rgb_scale;
            *b = (float)atoi(argv[2]) * s_config_rgb_scale;
            res = 1;
        }
        free(str);
    }

    return res;
}

// FUNCTION: LEGORR 0x00479240
void block_close(BlockFile* root)
{
    BlockRec* temp;

    free(root->file_text);
    while (root) {
        temp = root->next;
        block_record_remove(root);
        root = temp;
    }
}

// FUNCTION: LEGORR 0x00479270
BlockRec* block_record_create(BlockRec* prev)
{
    BlockRec* conf;

    if (configGlobs.free_list == NULL) block_text_token_pool_grow();

    conf = configGlobs.free_list;
    configGlobs.free_list = configGlobs.free_list->next_free;
    conf->next_free = conf;

    conf->key = NULL;
    conf->value = NULL;
    conf->next = NULL;

    if (prev) {
        prev->next = conf;
        conf->prev = prev;
        conf->file_text = NULL;
        conf->depth = prev->depth;
    }
    else {
        conf->depth = 0;
        conf->prev = NULL;
    }

    return conf;
}

// FUNCTION: LEGORR 0x004792c0
void block_record_remove(BlockRec* dead)
{
    dead->next_free = configGlobs.free_list;
    configGlobs.free_list = dead;
}

// FUNCTION: LEGORR 0x004792e0
BlockRec* block_find(BlockFile* conf, const char* path)
{
    BlockRec* found = NULL;
    BlockRec* back;
    char* temp;
    char* argv[CONFIG_MAXTOKENS];
    unsigned int count;
    unsigned int level;
    unsigned int length;
    int wildcard;
    char* s;

    temp = (char*)malloc(strlen(path) + 1);
    strcpy(temp, path);
    count = sound_variant_split(temp, argv, s_config_separator);

    while (conf) {
        if (conf->depth == count - 1) {
            wildcard = 0;
            if (count == 1) {
                for (s = conf->key, length = 0; *s != '\0' && *s != '*'; s++) length++;
                if (*s == '*') wildcard = (_strnicmp(argv[0], conf->key, length) == 0);
            }
            if (wildcard || _strcmpi(argv[count - 1], conf->key) == 0) {
                wildcard = 0;
                level = count - 1;
                for (back = conf; back; back = back->prev) {
                    if (back->depth == level - 1) {
                        if (level == 1) {
                            for (s = back->key, length = 0; *s != '\0' && *s != '*'; s++) length++;
                            if (*s == '*') wildcard = (_strnicmp(argv[0], back->key, length) == 0);
                        }
                        if (!wildcard && _strcmpi(argv[level - 1], back->key) != 0) break;
                        level--;
                    }
                }
                if (level == 0) {
                    found = conf;
                    if (!wildcard) break;
                }
            }
        }
        conf = conf->next;
    }

    free(temp);

    return found;
}

// FUNCTION: LEGORR 0x00479490
void block_text_token_pool_grow(void)
{
    BlockRec* list;
    unsigned int count;
    unsigned int loop;

    count = 1 << configGlobs.list_count;
    if ((list = configGlobs.lists[configGlobs.list_count] =
             (BlockRec*)malloc(sizeof(BlockRec) * count)) != NULL) {
        configGlobs.list_count++;
        for (loop = 1; loop < count; loop++) list[loop - 1].next_free = &list[loop];
        list[count - 1].next_free = configGlobs.free_list;
        configGlobs.free_list = list;
    }
}

/* Port-only additions (not in the original unit). */

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
/* block_open's tokeniser over a heap copy of `text`. */
BlockFile* block_parse(const char* text, size_t length)
{
    char* s;
    BlockRec* root;
    BlockRec* conf;
    int comment = 0;
    size_t loop;

    if ((s = (char*)malloc(length + 1)) == NULL) return NULL;
    memcpy(s, text, length);
    s[length] = '\0';
    conf = root = block_record_create(NULL);
    root->file_text = s;

    for (loop = 0; loop < length; loop++) {
        if (*s == ';') comment = 1;
        else if (*s == '\n') comment = 0;
        if (comment || *s == '\t' || *s == '\n' || *s == '\r' || *s == ' ') {
            *s = '\0';
        }
        s++;
    }

    for (s = root->file_text, loop = 0; loop < length; loop++) {
        if (*s != '\0') {
            if (*s == '}' && s[1] == '\0') conf->depth--;
            else if (conf->key == NULL) conf->key = s;
            else {
                conf->value = s;
                conf = block_record_create(conf);
                if (*s == '{' && s[1] == '\0') conf->depth++;
            }
            while (loop < length && *s++ != '\0') loop++;
        }
        else s++;
    }

    return root;
}

void* block_text_token_pool_slot_peek(int slot)
{
    return (slot >= 0 && slot < CONFIG_MAXLISTS) ? configGlobs.lists[slot] : NULL;
}

void* block_text_token_pool_head_peek(void)
{
    return configGlobs.free_list;
}

int block_text_token_pool_resident_peek(void)
{
    return (int)configGlobs.flags;
}

int block_text_token_pool_watermark_peek(void)
{
    return (int)configGlobs.list_count;
}
#endif /* !LEGORRR_BACKEND_WINAPI */
