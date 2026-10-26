// SPDX-License-Identifier: LGPL-3.0-or-later
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gods98/resource.hpp"
#include "util.hpp"

static int util_isspace_c(int c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

const char* util_root_name(BlockFile* bf)
{
    static char root_buf[64];
    if (bf == NULL || block_record_count(bf) == 0) {
        return "Lego*";
    }
    BlockRec* r0 = block_record_at(bf, 0);
    const char* name = r0 ? block_name(r0) : NULL;
    snprintf(root_buf, sizeof(root_buf), "%s", name != NULL ? name : "Lego*");
    return root_buf;
}

const char* util_level_cfg_path(BlockFile* cfg, const char* level,
                                const char* suffix, char* out, size_t out_sz)
{
    if (cfg == NULL || level == NULL) {
        return NULL;
    }
    const char* root = util_root_name(cfg);
    snprintf(out, out_sz, "%s Levels %s %s", root, level,
             suffix != NULL ? suffix : "");
    if (block_get_strptr(cfg, out) != NULL) {
        return out;
    }
    snprintf(out, out_sz, "%s %s %s", root, level,
             suffix != NULL ? suffix : "");
    return block_get_strptr(cfg, out) != NULL ? out : NULL;
}

// FUNCTION: LEGORR 0x0048db70
long util_atoi(const char* s)
{
    return s != NULL ? strtol(s, NULL, 10) : 0;
}

// FUNCTION: LEGORR 0x0048d980
float util_atof(const char* s)
{
    return s != NULL ? (float)strtod(s, NULL) : 0.0f;
}

/* Split a NUL-terminated token stream produced by `src` into fields.
 * `|` is a field separator too (the config's pipe-delimited
 * `file.bmp|x|y` triples, finding 09 §"Config format"). Returns the number
 * of fields written (0..6). */
static int util_tokenize_fields(const char* src,
                                char out[UTIL_QUERY_MAX_FIELDS][256], size_t out_sz)
{
    int n = 0;
    const char* p = src;
    while (*p != '\0' && n < UTIL_QUERY_MAX_FIELDS) {
        while (*p != '\0' && (util_isspace_c((unsigned char)*p) || *p == '|')) {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        int i = 0;
        while (*p != '\0' && !util_isspace_c((unsigned char)*p) && *p != '|'
               && i < (int)out_sz - 1) {
            out[n][i++] = *p++;
        }
        out[n][i] = '\0';
        ++n;
    }
    return n;
}

static int util_status_from_count(int n)
{
    if (n <= 0) {
        return 0;
    }
    if (n == 3) {
        return 3;
    }
    if (n >= 6) {
        return 6;
    }
    return 2;
}

int util_query_fields(const char* path_or_value,
                      char out[UTIL_QUERY_MAX_FIELDS][256], size_t out_sz)
{
    for (int i = 0; i < UTIL_QUERY_MAX_FIELDS; ++i) {
        out[i][0] = '\0';
    }
    if (path_or_value == NULL || path_or_value[0] == '\0') {
        return 0;
    }

    /* Case (b): the value names a resource. Parse its content when it
     * opens; otherwise fall through and parse the value inline (case a). */
    Resource* r = resource_open(path_or_value, RESOURCE_AUTO);
    if (r != NULL) {
        int32_t sz = resource_read(r);
        if (sz > 0) {
            char* buf = (char*)malloc((size_t)sz + 1);
            if (buf != NULL) {
                memcpy(buf, r->data, (size_t)sz);
                buf[sz] = '\0';
                int n = util_tokenize_fields(buf, out, out_sz);
                free(buf);
                resource_free(r);
                return util_status_from_count(n);
            }
        }
        resource_free(r);
    }

    /* Case (a): inline token list (also the path-with-pipes shape some
     * config keys use). */
    int n = util_tokenize_fields(path_or_value, out, out_sz);
    return util_status_from_count(n);
}
