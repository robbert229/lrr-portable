// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.hpp"
#include "resource.hpp"

/* ------------------------------------------------------------------ *
 *  Small helpers (ASCII only — the corpus is 8-bit, no locale).
 * ------------------------------------------------------------------ */

static int cfg_islower(int c) { return c >= 'a' && c <= 'z'; }

static int cfg_isspace(int c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int cfg_strcasecmp(const char* a, const char* b)
{
    for (;;) {
        int ca = (unsigned char)*a++;
        int cb = (unsigned char)*b++;
        if (cfg_islower(ca)) ca -= 0x20;
        if (cfg_islower(cb)) cb -= 0x20;
        if (ca != cb) {
            return ca - cb;
        }
        if (ca == 0) {
            return 0;
        }
    }
}

static char* cfg_strdup(const char* s)
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

/* Sentinel: the string the corpus passes as the fallback value everywhere
 * (DAT_004b8a14) — a parseable "0" so atoi/atof defaults agree. */
#define CFG_DEFAULT_STR "0"

/* ------------------------------------------------------------------ *
 *  Node tree.
 * ------------------------------------------------------------------ */

struct ConfigNode {
    char*         token;   /* record name */
    char*         value;   /* next same-depth token, or NULL */
    ConfigNode**  kids;    /* children, ordered */
    int           n_kids;
};

struct ConfigDb {
    ConfigNode** roots;    /* query-root records (wrapper-past) */
    int          n_roots;
};

static void cfg_node_free(ConfigNode* n)
{
    if (n == NULL) {
        return;
    }
    for (int i = 0; i < n->n_kids; ++i) {
        cfg_node_free(n->kids[i]);
    }
    free(n->kids);
    free(n->token);
    free(n->value);
    free(n);
}

static ConfigNode* cfg_node_new(const char* token)
{
    ConfigNode* n = (ConfigNode*)calloc(1, sizeof(ConfigNode));
    if (n == NULL) {
        return NULL;
    }
    n->token = cfg_strdup(token);
    if (n->token == NULL) {
        free(n);
        return NULL;
    }
    return n;
}

static int cfg_node_add_child(ConfigNode* n, ConfigNode* kid)
{
    ConfigNode** kids = (ConfigNode**)realloc(n->kids,
                                              (size_t)(n->n_kids + 1) * sizeof(kid));
    if (kids == NULL) {
        return 0;
    }
    n->kids = kids;
    n->kids[n->n_kids++] = kid;
    return 1;
}

/* ------------------------------------------------------------------ *
 *  Tokenizer + recursive descent.
 *
 *  Comments (`;` to end of line) are blanked before tokenization, exactly
 *  as the corpus's pass 1 does (finding 12 §2).
 * ------------------------------------------------------------------ */

/* Tokenize into a malloc'd array of NUL-terminated strings. Whitespace and
 * `;`-comments were already blanked by the caller; braces become their own
 * single-char tokens so the descent sees the structure (finding 12). */
static char** cfg_tokenize(const char* buf, int* out_n)
{
    const char* p = buf;
    /* Count first. */
    int n = 0;
    while (*p != '\0') {
        while (*p != '\0' && cfg_isspace((unsigned char)*p)) {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        /* Braces are one-token; anything else runs to ws/brace. */
        if (*p == '{' || *p == '}') {
            ++n;
            ++p;
        } else {
            while (*p != '\0' && !cfg_isspace((unsigned char)*p)
                   && *p != '{' && *p != '}') {
                ++p;
            }
            ++n;
        }
    }
    *out_n = n;
    if (n == 0) {
        return NULL;
    }
    char** toks = (char**)calloc((size_t)n, sizeof(char*));
    if (toks == NULL) {
        return NULL;
    }
    p = buf;
    int i = 0;
    while (*p != '\0') {
        while (*p != '\0' && cfg_isspace((unsigned char)*p)) {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        const char* start = p;
        if (*p == '{' || *p == '}') {
            ++p;
        } else {
            while (*p != '\0' && !cfg_isspace((unsigned char)*p)
                   && *p != '{' && *p != '}') {
                ++p;
            }
        }
        size_t len = (size_t)(p - start);
        char* t = (char*)malloc(len + 1);
        if (t == NULL) {
            for (int k = 0; k < i; ++k) free(toks[k]);
            free(toks);
            return NULL;
        }
        memcpy(t, start, len);
        t[len] = '\0';
        toks[i++] = t;
    }
    return toks;
}

/* Recursive descent over the token stream. `parent` (when non-NULL) receives
 * every node parsed at this level as a child; when NULL the nodes are parsed
 * and counted but not retained. Returns the number of tokens consumed (the
 * terminating `}` included), or -1 on allocation failure. `*node_count` is
 * incremented for every node retained under `parent`. */
static int cfg_parse_ex(char** toks, int from, int n, int* node_count,
                        ConfigNode* parent)
{
    *node_count = 0;
    int i = from;
    while (i < n) {
        if (strcmp(toks[i], "}") == 0) {
            ++i;
            break;
        }
        if (strcmp(toks[i], "{") == 0) {
            ++i;
            continue;
        }
        ConfigNode* node = cfg_node_new(toks[i]);
        if (node == NULL) {
            return -1;
        }
        ++i;
        if (i < n && strcmp(toks[i], "{") == 0) {
            ++i;
            int kids = 0;
            int sub = cfg_parse_ex(toks, i, n, &kids, node);
            if (sub < 0) {
                cfg_node_free(node);
                return -1;
            }
            i += sub;
            *node_count += kids;
        } else if (i < n && strcmp(toks[i], "}") != 0 && strcmp(toks[i], "{") != 0) {
            node->value = cfg_strdup(toks[i]);
            ++i;
        }
        if (parent != NULL) {
            if (!cfg_node_add_child(parent, node)) {
                cfg_node_free(node);
                return -1;
            }
        }
        ++*node_count;
    }
    return i - from;
}

/* Wrapper: not needed after the unwrap step is done inline in config_open. */

ConfigDb* config_open(const char* resource_name)
{
    if (resource_name == NULL || resource_name[0] == '\0') {
        return NULL;
    }
    Resource* r = resource_open(resource_name, RESOURCE_AUTO);
    if (r == NULL) {
        return NULL;
    }
    if (resource_read(r) <= 0) {
        resource_free(r);
        return NULL;
    }

    /* Working buffer: strip `;` line comments and NUL-separate whitespace. */
    char* buf = (char*)malloc((size_t)r->size + 1);
    if (buf == NULL) {
        resource_free(r);
        return NULL;
    }
    memcpy(buf, r->data, (size_t)r->size);
    buf[r->size] = '\0';
    resource_free(r);

    int in_comment = 0;
    for (char* p = buf; *p != '\0'; ++p) {
        if (in_comment) {
            if (*p == '\n') {
                in_comment = 0;
            }
            *p = ' ';
        } else if (*p == ';') {
            in_comment = 1;
            *p = ' ';
        }
    }

    int ntok = 0;
    char** toks = cfg_tokenize(buf, &ntok);
    if (toks == NULL) {
        free(buf);
        return NULL;
    }

    /* cfg_parse_ex preserves nodes only under a parent; parse the stream
     * into a synthetic root, then unwrap the single top-level record. */
    ConfigNode root; /* stack parent, never freed */
    memset(&root, 0, sizeof(root));
    int count = 0;
    int consumed = cfg_parse_ex(toks, 0, ntok, &count, &root);
    for (int i = 0; i < ntok; ++i) {
        free(toks[i]);
    }
    free(toks);
    free(buf);
    if (consumed < 0 || count == 0) {
        return NULL;
    }

    ConfigDb* db = (ConfigDb*)calloc(1, sizeof(ConfigDb));
    if (db == NULL) {
        return NULL;
    }
    if (root.n_kids == 1) {
        /* One top-level `name {…}` wrapper (Lego.cfg). Unwrap it: its
         * children become the query roots. The wrapper node itself has no
         * value — free its shell only, the children live on. */
        ConfigNode* wrap = root.kids[0];
        db->roots = wrap->kids;
        db->n_roots = wrap->n_kids;
        free(wrap->token);
        free(wrap->value);
        free(wrap);
        free(root.kids);
    } else {
        db->roots = root.kids;
        db->n_roots = root.n_kids;
    }
    return db;
}

void config_close(ConfigDb* db)
{
    if (db == NULL) {
        return;
    }
    for (int i = 0; i < db->n_roots; ++i) {
        cfg_node_free(db->roots[i]);
    }
    free(db->roots);
    free(db);
}

int config_roots(const ConfigDb* db)
{
    return db != NULL ? db->n_roots : 0;
}

const ConfigNode* config_root(const ConfigDb* db, int i)
{
    if (db == NULL || i < 0 || i >= db->n_roots) {
        return NULL;
    }
    return db->roots[i];
}

const char* config_node_name(const ConfigNode* n)
{
    return n != NULL ? n->token : NULL;
}

const char* config_node_value(const ConfigNode* n)
{
    return n != NULL ? n->value : NULL;
}

int config_node_child_count(const ConfigNode* n)
{
    return n != NULL ? n->n_kids : 0;
}

const ConfigNode* config_node_child(const ConfigNode* n, int i)
{
    if (n == NULL || i < 0 || i >= n->n_kids) {
        return NULL;
    }
    return n->kids[i];
}

/* Resolve a single path segment against a set of candidate children. */
static const ConfigNode* cfg_find_child(const ConfigNode* parent, const char* seg)
{
    if (parent == NULL) {
        return NULL;
    }
    for (int i = 0; i < parent->n_kids; ++i) {
        if (cfg_strcasecmp(parent->kids[i]->token, seg) == 0) {
            return parent->kids[i];
        }
    }
    return NULL;
}

const ConfigNode* config_find(ConfigDb* db, const char* path)
{
    if (db == NULL || path == NULL || path[0] == '\0') {
        return NULL;
    }

    /* Split the path into whitespace-separated segments (corpus builds
     * these with "%s %s %s" — keys are space-wrapped tokens, finding 12). */
    char segs[8][256];
    int  nsegs = 0;
    const char* p = path;
    while (*p != '\0' && nsegs < 8) {
        while (*p == ' ') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        int i = 0;
        while (*p != '\0' && *p != ' ' && i < 255) {
            segs[nsegs][i++] = *p++;
        }
        segs[nsegs][i] = '\0';
        ++nsegs;
    }
    if (nsegs == 0) {
        return NULL;
    }

    /* First segment: try query roots directly, then (Lego.cfg shape) the
     * first root's children — the wrapper-resolving step. */
    const ConfigNode* cur = NULL;
    for (int k = 0; k < db->n_roots; ++k) {
        if (cfg_strcasecmp(db->roots[k]->token, segs[0]) == 0) {
            cur = db->roots[k];
            break;
        }
    }
    if (cur == NULL && db->n_roots == 1) {
        cur = cfg_find_child(db->roots[0], segs[0]);
    }
    if (cur == NULL) {
        return NULL;
    }

    /* Remaining segments descend children. */
    for (int s = 1; s < nsegs; ++s) {
        cur = cfg_find_child(cur, segs[s]);
        if (cur == NULL) {
            return NULL;
        }
    }
    return cur;
}

const char* config_value(ConfigDb* db, const char* path)
{
    const ConfigNode* n = config_find(db, path);
    return n != NULL ? n->value : NULL;
}

long config_int(ConfigDb* db, const char* path, long dflt)
{
    const char* v = config_value(db, path);
    if (v == NULL) {
        return dflt;
    }
    /* The corpus's typed getter maps TRUE/FALSE; atoi for the rest. */
    if (cfg_strcasecmp(v, "TRUE") == 0) {
        return 1;
    }
    if (cfg_strcasecmp(v, "FALSE") == 0) {
        return 0;
    }
    return strtol(v, NULL, 10);
}

float config_float(ConfigDb* db, const char* path, float dflt)
{
    const char* v = config_value(db, path);
    if (v == NULL) {
        return dflt;
    }
    return (float)strtod(v, NULL);
}
