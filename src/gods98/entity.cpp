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
#include "string.hpp"

/* --- kind dispatch (FUN_00475930) ---------------------------------------------- */

/* --- .ae assembly (FUN_00472cd0, ACT branch) ---------------------------------- */

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
    const char* backslash = strrchr(name, '\\');
    const char* slash = strrchr(name, '/');
    if (backslash != NULL && (slash == NULL || backslash > slash)) {
        base = backslash + 1;
    } else if (slash != NULL) {
        base = slash + 1;
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

/* Container_Load's signature (ADR 0005 step 6) for units rebuilt as C,
 * declared in gods98/entity.h. The port's 0x00472cd0 is the .ae assembly
 * parser above, with a different contract, and the port has no container
 * loader yet: this adapter loads nothing. */
extern "C" void* container_load(void* parent, const char* filename,
                                const char* typeString, int looseLoad)
{
    (void)parent;
    (void)filename;
    (void)typeString;
    (void)looseLoad;
    return NULL;
}
