// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — `.ae` entity assembly (see entity.hpp).
 *
 * Implements the ACT (kind 4) branch of FUN_00472cd0 and the kind dispatch
 * FUN_00475930, on top of the block-text API (block.c*). The scene pipeline
 * the corpus calls after building the table (FUN_00475d10 / FUN_00473370 /
 * the scale application through the record vtable) is out of scope here.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entity.hpp"
#include "resource.hpp"

/* --- kind dispatch (FUN_00475930) ---------------------------------------------- */

/* The 9-slot kind table initialized by FUN_00472710 (orders match the
 * corpus: index 0 "NULL", 1 "MESH", 2 "FRAME", 3 "ANIM", 4 "ACT", then LWS/
 * LWO at 7/8; slots 5/6 stay zero and never match). */
static const char* const s_kinds[9] = {
    "NULL", "MESH", "FRAME", "ANIM", "ACT", NULL, NULL, "LWS", "LWO",
};

static int s_strcasecmp(const char* a, const char* b)
{
    for (;; a++, b++) {
        int ca = (unsigned char)*a;
        int cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb) return ca - cb;
        if (ca == 0) return 0;
    }
}

int entity_kind_lookup(const char* kind, int* notexture)
{
    if (notexture != NULL) {
        *notexture = 0;
    }
    if (kind == NULL || *kind == '\0') {
        return ENTITY_KIND_UNKNOWN;
    }

    /* Tokenize the kind on ':' — the second component is the optional
     * NOTEXTURE flag (s_NOTEXTURE at 0x4aab78), compared case-insensitively
     * once the caller splits with DAT_004a1cd4 ("::" surrogate in corpus;
     * here a single ':' delimiter). */
    char buf[64];
    snprintf(buf, sizeof buf, "%s", kind);
    char* colon = strchr(buf, ':');
    if (colon != NULL) {
        *colon = '\0';
        if (s_strcasecmp(colon + 1, "NOTEXTURE") == 0 && notexture != NULL) {
            *notexture = 1;
        }
    }

    for (int i = 0; i < 9; i++) {
        if (s_kinds[i] != NULL && s_strcasecmp(s_kinds[i], buf) == 0) {
            return i;
        }
    }
    return ENTITY_KIND_UNKNOWN;
}

/* --- .ae assembly (FUN_00472cd0, ACT branch) ---------------------------------- */

EntityAssembly* entity_assembly_parse(BlockFile* bf, const char* root,
                                     uint32_t feature)
{
    if (bf == NULL || root == NULL || *root == '\0') {
        return NULL;
    }

    char path[512]; /* longest query: root::<name>::KEY */
    snprintf(path, sizeof path, "%s Activities", root);
    BlockRec* acts = block_subgroup(bf, path);
    if (acts == NULL) {
        /* corpus: no Activities subgroup => no assembly record */
        return NULL;
    }

    EntityAssembly* ea = (EntityAssembly*)calloc(1, sizeof *ea);
    if (ea == NULL) {
        return NULL;
    }

    /* Root SCALE float (read <root>::Scale). The corpus applies a nonzero
     * scale through the record vtable; the value is kept for the caller. */
    snprintf(path, sizeof path, "%s SCALE", root);
    ea->scale = block_get_float(bf, path, 0.0f);

    size_t cap = 0;
    for (BlockRec* r = block_first_child(acts); r != NULL;
         r = block_next_sibling(r)) {
        const char* key = block_name(r);   /* "Activity_<State>", '!' possible */
        const char* grp = block_value(r);  /* target animation group */
        if (key == NULL || grp == NULL) {
            continue;
        }
        int was_flagged = 0;
        if (key[0] == '!') {
            if (feature & ENTITY_FEATURE_REDUCE_ANIM) {
                /* reduce-animation: activity omitted entirely */
                continue;
            }
            was_flagged = 1;
            ++key; /* strip '!' — processed under the default feature */
        }

        snprintf(path, sizeof path, "%s %s FILE", root, grp);
        const char* file = block_get_strptr(bf, path);
        if (file == NULL) {
            /* no FILE => the corpus makes no scene for this activity */
            continue;
        }

        if (ea->count >= cap) {
            size_t ncap = cap ? cap * 2 : 8;
            EntityActivity* na = (EntityActivity*)realloc(ea->acts,
                                                          ncap * sizeof *na);
            if (na == NULL) {
                break;
            }
            ea->acts = na;
            cap = ncap;
        }

        EntityActivity* a = &ea->acts[ea->count++];
        memset(a, 0, sizeof *a);
        snprintf(a->state, sizeof a->state, "%s", key);
        snprintf(a->name, sizeof a->name, "%s", grp);
        snprintf(a->file, sizeof a->file, "%s", file);
        a->reduce_anim = was_flagged;

        snprintf(path, sizeof path, "%s %s TRANSCOEF", root, grp);
        a->transcoef = block_get_float(bf, path, 0.0f);
        snprintf(path, sizeof path, "%s %s TRIGGER", root, grp);
        a->trigger = block_get_int(bf, path, 0);
        snprintf(path, sizeof path, "%s %s LWSFILE", root, grp);
        a->lwsfile = block_get_bool(bf, path);
        snprintf(path, sizeof path, "%s %s LOOPING", root, grp);
        a->looping = block_get_bool(bf, path);
        snprintf(path, sizeof path, "%s %s SAMPLE", root, grp);
        const char* sample = block_get_strptr(bf, path);
        if (sample != NULL) {
            snprintf(a->sample, sizeof a->sample, "%s", sample);
        }
    }
    return ea;
}

EntityAssembly* entity_assembly_open(const char* name, const char* root,
                                     uint32_t feature)
{
    if (name == NULL || *name == '\0') {
        return NULL;
    }

    /* corpus path build (FUN_00472cd0 kind-4): split name on '\', take the
     * last component, join as "%s\%s.%s" with the "ae" extension (string at
     * 0x4a19c4, slot DAT_0076ae0c). */
    const char* base = name;
    for (const char* p = name; *p != '\0'; p++) {
        if (*p == '\\' || *p == '/') {
            base = p + 1;
        }
    }
    char path[520];
    snprintf(path, sizeof path, "%s\\%s.ae", name, base);

    BlockFile* bf = block_open(path);
    if (bf == NULL) {
        return NULL;
    }
    EntityAssembly* ea = entity_assembly_parse(bf, root, feature);
    block_close(bf);
    return ea;
}

void entity_assembly_free(EntityAssembly* ea)
{
    if (ea == NULL) {
        return;
    }
    free(ea->acts);
    free(ea);
}
