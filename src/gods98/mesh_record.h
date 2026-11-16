// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-015 Mesh.c's mesh record, shared with Lws.c, which sets a mesh's flags.
 * The +0x... comments are the Win32 offsets. */
#ifndef LEGORRR_GODS98_MESH_RECORD_H
#define LEGORRR_GODS98_MESH_RECORD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Mesh Mesh;
typedef struct Mesh_Group Mesh_Group;
typedef struct Mesh_LightWave_Surface Mesh_LightWave_Surface;
typedef struct Mesh_Viewport Mesh_Viewport;
typedef void (*Mesh_RenderCallbackFunc)(Mesh* mesh, void* data,
                                        Mesh_Viewport* vp);

struct IDirect3DRMUserVisual;
struct IDirect3DRMFrame3;

/* A mesh (0x34 bytes on Win32). Lws.c sets its flags. The pointer fields
 * hold real pointers (ADR 0007 decision 4). */
struct Mesh {
    unsigned long groupCount;              /* +0x00 */
    unsigned long listSize;                /* +0x04 */
    Mesh_Group* groupList;                 /* +0x08 */
    struct IDirect3DRMUserVisual* uv; /* +0x0c */
    Mesh_RenderCallbackFunc renderCallback; /* +0x10 */
    void* renderCallbackData;              /* +0x14 */
    unsigned long renderFlags;             /* +0x18 */
    Mesh_LightWave_Surface* lightWaveSurf; /* +0x1c */
    unsigned long numOfRefs;               /* +0x20 */
    Mesh* clonedFrom;                      /* +0x24 */
    struct IDirect3DRMFrame3* frameCreatedOn; /* +0x28 */
    unsigned long flags;                   /* +0x2c */
    Mesh* nextFree;                        /* +0x30 */
};

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MESH_RECORD_H */
