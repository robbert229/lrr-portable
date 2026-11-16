// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-004's animation object record, shared with the units that allocate
 * and read it (Lego.c, LegoObject.c). The +0x... comments are the Win32
 * offsets; the pointer fields hold real pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GAME_ANIMATION_OBJECT_RECORD_H
#define LEGORRR_GAME_ANIMATION_OBJECT_RECORD_H

#include "gods98/model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The most camera nulls an object caches (+0x54..+0x60). */
#define ANIMATION_OBJECT_MAX_CAMERAS 4

/* Bit 0 of flags: the object owns its null names (a loaded object, not a
 * clone). */
#define ANIMATION_OBJECT_FLAG_OWNS_NAMES 0x1

/* One animation object (0x74 bytes on Win32): the creature model
 * (CreatureModel) of a mini-figure or rock monster type, and each such
 * object's clone (LegoObject +0x010, +0x014). */
typedef struct AnimationObject {
    unsigned int objectId;              /* +0x00 */
    void* container;                    /* +0x04 the activity container */
    unsigned int reserved[6];           /* +0x08 never touched by the unit */
    char* cameraNullName;               /* +0x20 */
    unsigned int cameraNullFrames;      /* +0x24 */
    int cameraFlipDir;                  /* +0x28 */
    char* drillNullName;                /* +0x2c */
    char* footStepNullName;             /* +0x30 */
    char* carryNullName;                /* +0x34 */
    char* throwNullName;                /* +0x38 */
    char* depositNullName;              /* +0x3c */
    void* drillNull;                    /* +0x40 */
    void* footStepNull;                 /* +0x44 */
    void* carryNull;                    /* +0x48 */
    void* throwNull;                    /* +0x4c */
    void* depositNull;                  /* +0x50 */
    void* cameraNulls[ANIMATION_OBJECT_MAX_CAMERAS]; /* +0x54 */
    ModelEntryRecord* polyMedium;       /* +0x64 */
    ModelEntryRecord* polyHigh;         /* +0x68 */
    ModelEntryRecord* polyFP;           /* +0x6c first-person meshes */
    unsigned int flags;                 /* +0x70 */
} AnimationObject;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ANIMATION_OBJECT_RECORD_H */
