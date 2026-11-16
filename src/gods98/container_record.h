// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-002 Container.c's record, shared with the units that read a
 * container's type (Vehicle.c). The +0x... comments are the Win32 offsets;
 * the pointer fields hold real pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GODS98_CONTAINER_RECORD_H
#define LEGORRR_GODS98_CONTAINER_RECORD_H

#include <miniwin/d3drm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Container Container;
typedef struct Container_Texture Container_Texture;
struct AnimClone;

typedef void (*ContainerTriggerFrameCallback)(Container* cont, void* data);
typedef void (*ContainerSoundTriggerCallback)(const char* sample,
                                              Container* cont, void* data);
typedef void (*ContainerActivityCallback)(Container* cont, void* data);
typedef int (*ContainerWalkTreeCallback)(LPDIRECT3DRMFRAME3 frame, void* data);

typedef enum Container_Type {
    Container_Invalid = -1,
    Container_Null = 0,
    Container_Mesh = 1,
    Container_Frame = 2,
    Container_Anim = 3,
    Container_FromActivity = 4,
    Container_Light = 5,
    Container_Reference = 6,
    Container_LWS = 7,
    Container_LWO = 8,
    Container_TypeCount = 9
} Container_Type;

/* A container's type data (Container_SetTypeData). */
typedef struct Container_TypeData {
    char* name;                 /* +0x00 the current activity */
    LPDIRECT3DRMLIGHT light;            /* +0x04 */
    LPDIRECT3DRMMESH mesh;              /* +0x08 */
    void* transMesh;            /* +0x0c Mesh.c's mesh */
} Container_TypeData;

/* The clones of an activity or animation container (Container_Clone). */
typedef struct Container_CloneData {
    struct Container* clonedFrom;  /* +0x00 */
    struct Container** cloneTable; /* +0x04 */
    unsigned long cloneCount;      /* +0x08 */
    unsigned long cloneNumber;     /* +0x0c */
    int used;                      /* +0x10 */
} Container_CloneData;

/* A container (0x2c bytes on Win32). */
struct Container {
    LPDIRECT3DRMFRAME3 masterFrame;                       /* +0x00 */
    LPDIRECT3DRMFRAME3 activityFrame;                     /* +0x04 */
    LPDIRECT3DRMFRAME3 hiddenFrame;                       /* +0x08 */
    Container_TypeData* typeData;                /* +0x0c */
    Container_Type type;                         /* +0x10 */
    unsigned long flags;                         /* +0x14 */
    ContainerActivityCallback activityCallback;  /* +0x18 */
    void* activityCallbackData;                  /* +0x1c */
    void* userData;                              /* +0x20 */
    Container_CloneData* cloneData;              /* +0x24 */
    struct Container* nextFree;                  /* +0x28 */
};

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_CONTAINER_RECORD_H */
