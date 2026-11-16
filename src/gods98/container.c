// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-002, Container.c (text 00472710-00476d50;
 * .rdata 0049e87c-0049e89c; .bss 00505400-00505800; commons
 * 0076ad80-0076cd98; findings 505 and 624).
 *
 * gods98's scene containers. A Container (0x2c bytes, from a pool of
 * blocks that double in size, Container_AddList) holds three Direct3DRM
 * frames: the master frame, which places it, the activity frame under it,
 * which carries its visual, and a hidden frame, where Container_Hide parks
 * the activity frame. Its type says what the visual is: a mesh made in
 * code or loaded from an `.x` file, a frame hierarchy, an animation set, a
 * set of activities read from an `.ae` file (each one an animation set or a
 * LightWave scene under its own "ActFrame" frame), a light, a reference to
 * a frame made elsewhere, a LightWave scene or a LightWave object. Every
 * frame the unit makes carries a Container_AppData record (its owner, the
 * animation clone and the activity's settings) as its app data.
 *
 * The unit also loads textures (Container_LoadTexture2, with a cache of
 * the textures a mesh file names, Container_TextureLoadCallback) and
 * keeps the scene's fog.
 *
 * The port's model of the unit stays in the port tail,
 * gods98/container_port.cpp, unannotated. In the matching build
 * container_link_names.h gives the unit's functions the port's C names
 * where their callers pass the original's arguments, so those callers reach
 * them; natively the port tail serves those names.
 *
 * The Direct3DRM and DirectDraw interfaces are miniwin's DirectX 6
 * interfaces, called with C++ syntax (ADR 0007 decision 3). */
#include "container_link_names.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "container_unit.h"
#include "main.h"
#include "utils.h"
#include "config.h"
#include "files.h"
#include "lws.h"
#include "animclone.h"
#include "mesh_unit.h"
#include "sound3d.h"
#include "g98_026.h"
#include "directdraw.h"
#include "container_imports.h"

#include <miniwin/d3drm.h>
#include <miniwin/ddraw.h>

#define CONTAINER_MAXLISTS 20
#define CONTAINER_MAXTEXTURES 1000
#define CONTAINER_MAXVISUALS 4
// STRING: LEGORR 0x004aab84
#define CONTAINER_ACTIVITYFRAMEPREFIX "ActFrame"
#define CONTAINER_ULONG_MAX 0xffffffff

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

typedef enum Container_SearchMode {
    Container_SearchMode_FirstMatch = 0,
    Container_SearchMode_MatchCount = 1,
    Container_SearchMode_NthMatch = 2
} Container_SearchMode;

#define CONTAINER_FLAG_INITIALISED 0x00000001
#define CONTAINER_FLAG_TRIGGERENABLED 0x00000040

#define CONTAINER_FLAG_DEADREFERENCE 0x00000001  /* unused */
#define CONTAINER_FLAG_ANIMATIONSKIPPED 0x00000002
#define CONTAINER_FLAG_MESHSWAPPED 0x00000004
#define CONTAINER_FLAG_HIDDEN 0x00000008
#define CONTAINER_FLAG_DEADREFERENCE2 0x00000010
#define CONTAINER_FLAG_ANIMATIONUPDATE 0x00000020
#define CONTAINER_FLAG_HIDDEN2 0x00000080

/* Images.c's BMP_Image (IndexedBitmapPaletteData), which
 * Container_LoadTextureSurface has the bitmap parsed into: the original
 * SDK's D3DRMIMAGE layout, which miniwin's D3DRMIMAGE does not keep. */
typedef struct CntImage {
    int width;
    int height;
    int aspectx;
    int aspecty;
    int depth;
    int rgb;
    int bytes_per_line;
    void* buffer1;
    void* buffer2;
    unsigned int red_mask;
    unsigned int green_mask;
    unsigned int blue_mask;
    unsigned int alpha_mask;
    int palette_size;
    void* palette;
} CntImage;

/* Main.c's lpD3DRM() (00505914) and lpDevice() (00505918). */
#define lpD3DRM() ((LPDIRECT3DRM3)s_main_d3drm)
#define lpDevice() ((LPDIRECT3DRMDEVICE3)s_main_device)
/* DirectDraw.c's lpDD() (0076ac84). */
#define lpDD() ((LPDIRECTDRAW4)CONTAINER_LPDD())

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

/* A Container_LoadTexture2 texture. */
struct Container_Texture {
    LPDIRECTDRAWSURFACE4 surface;        /* +0x00 */
    LPDIRECT3DRMTEXTURE3 texture;        /* +0x04 */
    int colourKey;              /* +0x08 */
};

/* A mesh container's group records (Container_MakeMesh2). Group 0 is the
 * mesh itself; group n is listSet[n - 1], a separate mesh. */
typedef struct Container_MeshAppData {
    LPDIRECT3DRMMESH* listSet;          /* +0x00 */
    unsigned long usedCount;    /* +0x04 */
    unsigned long listSize;     /* +0x08 */
    int groupZeroHidden;        /* +0x0c */
    int firstAddGroup;          /* +0x10 */
} Container_MeshAppData;

/* Every frame's app data (Container_Frame_SetAppData). */
typedef struct Container_AppData {
    struct Container* ownerContainer;  /* +0x00 */
    char* animSetFileName;             /* +0x04 */
    char* frameName;                   /* +0x08 */
    unsigned long frameCount;          /* +0x0c */
    float currTime;                    /* +0x10 */
    float transCo;                     /* +0x14 */
    char* activitySample;              /* +0x18 */
    struct AnimClone* animClone;       /* +0x1c */
    unsigned long trigger;             /* +0x20 */
    void* soundList;                   /* +0x24 */
} Container_AppData;

/* Container_SearchTree's walk state (Container_Frame_SearchCallback). */
typedef struct Container_SearchData {
    const char* string;         /* +0x00 */
    unsigned long stringLen;    /* +0x04 */
    int caseSensitive;          /* +0x08 */
    LPDIRECT3DRMFRAME3 resultFrame;      /* +0x0c */
    unsigned long count;        /* +0x10 */
    Container_SearchMode mode;  /* +0x14 */
    unsigned long matchNumber;  /* +0x18 */
} Container_SearchData;

/* A texture the mesh loader cached (Container_TextureLoadCallback). */
typedef struct Container_TextureRef {
    char* fileName;             /* +0x00 */
    LPDIRECT3DRMTEXTURE3 texture;        /* +0x04 */
} Container_TextureRef;

/* The unit's state: a common symbol at 0076ad80 (0x2018 bytes). */
typedef struct Container_Globs {
    struct Container* listSet[CONTAINER_MAXLISTS];     /* +0x0000 0076ad80 */
    struct Container* freeList;                        /* +0x0050 0076add0 */
    struct Container* rootContainer;                   /* +0x0054 0076add4 */
    const char* typeName[Container_TypeCount];         /* +0x0058 0076add8 */
    const char* extensionName[Container_TypeCount];    /* +0x007c 0076adfc */
    const char* gameName;                              /* +0x00a0 0076ae20 */
    IUnknown* visualArray[CONTAINER_MAXVISUALS];      /* +0x00a4 0076ae24 */
    Container_TextureRef textureSet[CONTAINER_MAXTEXTURES]; /* +0x00b4 0076ae34 */
    unsigned long textureCount;                        /* +0x1ff4 0076cd74 */
    ContainerSoundTriggerCallback soundTriggerCallback; /* +0x1ff8 0076cd78 */
    void* soundTriggerData;                            /* +0x1ffc 0076cd7c */
    ContainerTriggerFrameCallback triggerFrameCallback; /* +0x2000 0076cd80 */
    void* triggerFrameData;                            /* +0x2004 0076cd84 */
    char* sharedDir;                                   /* +0x2008 0076cd88 */
    unsigned long fogColour;                           /* +0x200c 0076cd8c */
    unsigned long listCount;                           /* +0x2010 0076cd90 */
    unsigned long flags;                               /* +0x2014 0076cd94 */
} Container_Globs;

/* d3drmdef.h's mapping flags and D3DRMGROUP_ALLGROUPS: static consts, so
 * every object that includes the header has a copy (Viewports.c's are at
 * 0049e89c). The original reads D3DRMMAP_PERSPCORRECT from memory; the
 * unit now passes miniwin's D3DRMMAP_PERSPCORRECT, and these copies stay
 * as a record of the original's .rdata. */
// GLOBAL: LEGORR 0x0049e87c
static const unsigned long cntMapWrapU = 1;
// GLOBAL: LEGORR 0x0049e880
static const unsigned long cntMapWrapV = 2;
// GLOBAL: LEGORR 0x0049e884
static const unsigned long cntMapPerspCorrect = 4;
// GLOBAL: LEGORR 0x0049e888
static const long cntGroupAllGroups = -1;

// GLOBAL: LEGORR 0x0076ad80
Container_Globs containerGlobs;

/* The unit's static callbacks, passed by address before their definitions. */
static int Container_SetPerspectiveCorrectionCallback(LPDIRECT3DRMFRAME3 frame,
                                                      void* data);
static void Container_Frame_ReferenceDestroyCallback(
                                                     LPDIRECT3DRMOBJECT obj, void* arg);
static HRESULT Container_TextureLoadCallback(char* name, void* data,
                                             LPDIRECT3DRMTEXTURE3* texture);
static int Container_TextureSetSort(const void* a, const void* b);
static void Container_TextureDestroyCallback(LPDIRECT3DRMOBJECT obj,
                                             void* arg);

// FUNCTION: LEGORR 0x00472710
Container* Container_Initialise(const char* gameName)
{
    Container* root;
    unsigned long loop;

    memset(&containerGlobs, 0, sizeof(containerGlobs));
    for (loop = 0; loop < CONTAINER_MAXLISTS; loop++) {
        containerGlobs.listSet[loop] = NULL;
    }

    // STRING: LEGORR 0x004aaac4
    containerGlobs.extensionName[Container_Mesh] = "x";
    containerGlobs.extensionName[Container_Frame] = "x";
    containerGlobs.extensionName[Container_Anim] = "x";

    containerGlobs.typeName[Container_Null] = "NULL";
    containerGlobs.typeName[Container_Mesh] = "MESH";
    // STRING: LEGORR 0x004aaabc
    containerGlobs.typeName[Container_Frame] = "FRAME";
    containerGlobs.typeName[Container_Anim] = "ANIM";
    containerGlobs.typeName[Container_LWS] = "LWS";
    containerGlobs.typeName[Container_LWO] = "LWO";
    containerGlobs.typeName[Container_FromActivity] = "ACT";
    containerGlobs.typeName[Container_Light] = NULL;
    containerGlobs.extensionName[Container_Null] = "";
    containerGlobs.extensionName[Container_FromActivity] = "ae";
    containerGlobs.extensionName[Container_Light] = "";

    containerGlobs.gameName = gameName;
    containerGlobs.freeList = NULL;
    containerGlobs.listCount = 0;
    containerGlobs.flags = CONTAINER_FLAG_INITIALISED;
    containerGlobs.textureCount = 0;
    containerGlobs.sharedDir = NULL;

    root = Container_Create(NULL);
    containerGlobs.rootContainer = root;
    root->masterFrame->SetSortMode(D3DRMSORT_NONE);

    return root;
}

// FUNCTION: LEGORR 0x00472800
void Container_Shutdown(void)
{
    unsigned long list, count, loop, removed = 0;
    Container* testCont;

    for (list = 0; list < CONTAINER_MAXLISTS; list++) {
        if (containerGlobs.listSet[list]) {
            count = 0x00000001 << list;
            for (loop = 0; loop < count; loop++) {
                if (testCont = &containerGlobs.listSet[list][loop]) {
                    if (testCont->nextFree == testCont) {
                        removed++;
                        Container_Remove2(testCont, 1);
                    }
                }
            }
            free(containerGlobs.listSet[list]);
        }
    }

    containerGlobs.freeList = NULL;
    containerGlobs.flags = 0x00000000;

    for (loop = 0; loop < containerGlobs.textureCount; loop++) {
        if (containerGlobs.textureSet[loop].fileName) {
            free(containerGlobs.textureSet[loop].fileName);
        }
    }

    if (containerGlobs.sharedDir) free(containerGlobs.sharedDir);
}

// FUNCTION: LEGORR 0x004728c0
void Container_SetSharedTextureDirectory(const char* path)
{
    containerGlobs.sharedDir = string_duplicate(path);
}

// FUNCTION: LEGORR 0x004728e0
void Container_EnableSoftwareRenderer(int on)
{
    if (on) {
        containerGlobs.flags |= CONTAINER_FLAG_TRIGGERENABLED;
    } else {
        containerGlobs.flags &= ~CONTAINER_FLAG_TRIGGERENABLED;
    }
}

// FUNCTION: LEGORR 0x00472900
void Container_SetTriggerFrameCallback(ContainerTriggerFrameCallback callback,
                                       void* data)
{
    containerGlobs.triggerFrameCallback = callback;
    containerGlobs.triggerFrameData = data;
}

// FUNCTION: LEGORR 0x00472920
void Container_SetSoundTriggerCallback(ContainerSoundTriggerCallback callback,
                                       void* data)
{
    containerGlobs.soundTriggerCallback = callback;
    containerGlobs.soundTriggerData = data;
    containerGlobs.flags |= CONTAINER_FLAG_TRIGGERENABLED;
}

// FUNCTION: LEGORR 0x00472940
Container* Container_GetRoot(void)
{
    return containerGlobs.rootContainer;
}

/* Main.c's mainGlobs fields under the port's names (finding 679). */
#include "gods98/main_globs.h"

// FUNCTION: LEGORR 0x00472950
Container* Container_Create(Container* parent)
{
    Container* newCont;
    LPDIRECT3DRMFRAME3 parentFrame = NULL;
    LPDIRECT3DRMFRAME3 hiddenParentFrame = NULL;

    if (parent) {
        parentFrame = parent->masterFrame;
        hiddenParentFrame = parent->hiddenFrame;
    }

    if (containerGlobs.freeList == NULL) Container_AddList();

    newCont = containerGlobs.freeList;
    containerGlobs.freeList = newCont->nextFree;
    memset(newCont, 0, sizeof(Container));
    newCont->nextFree = newCont;

    if (lpD3DRM()->CreateFrame(parentFrame,
                               &newCont->masterFrame) == D3DRM_OK) {
        if (lpD3DRM()->CreateFrame(newCont->masterFrame,
                                   &newCont->activityFrame) == D3DRM_OK) {
            if (lpD3DRM()->CreateFrame(hiddenParentFrame,
                                       &newCont->hiddenFrame) == D3DRM_OK) {
                Container_Frame_SetAppData(newCont->masterFrame, newCont, NULL,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL);
                Container_Frame_SetAppData(newCont->activityFrame, newCont,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL);
                Container_Frame_SetAppData(newCont->hiddenFrame, newCont, NULL,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL);
                return newCont;
            }
        }
    }

    Container_Remove(newCont);
    return NULL;
}

// FUNCTION: LEGORR 0x00472a40
void Container_Remove(Container* dead)
{
    Container_Remove2(dead, 0);
}

// FUNCTION: LEGORR 0x00472a50
void Container_Remove2(Container* dead, int kill)
{
    unsigned long count, loop;
    LPDIRECT3DRMFRAME3* frameList;
    struct AnimClone** cloneList;
    struct AnimClone* animClone;
    Container_MeshAppData* appData;
    LPDIRECT3DRMMESH mesh;
    LPDIRECT3DRMFRAME3 parent;

    if (dead->type != Container_Reference) Container_SetParent(dead, NULL);

    if ((dead->type == Container_FromActivity || dead->type == Container_Anim) &&
        dead->cloneData) {
        if (kill) {
            if (dead->cloneData->cloneTable) free(dead->cloneData->cloneTable);
            free(dead->cloneData);
        } else {
            dead->cloneData->used = 0;
            return;
        }
    }

    if (dead->type == Container_FromActivity) {
        count = Container_GetActivities(dead, NULL, NULL, NULL);
        cloneList = (struct AnimClone**)malloc(sizeof(struct AnimClone*) * count);
        frameList = (LPDIRECT3DRMFRAME3*)malloc(sizeof(LPDIRECT3DRMFRAME3) * count);
        Container_GetActivities(dead, frameList, cloneList, NULL);
        for (loop = 0; loop < count; loop++) {
            anim_clone_remove(cloneList[loop]);
            Container_Frame_FreeName(frameList[loop]);
            Container_Frame_RemoveAppData(frameList[loop]);
            frameList[loop]->Release();
        }
        free(cloneList);
        free(frameList);
    } else if (dead->type == Container_Mesh) {
        if (dead->typeData) {
            if (dead->typeData->transMesh) {
                mesh_remove(dead->typeData->transMesh, dead->activityFrame);
            } else {
                appData = (Container_MeshAppData*)dead->typeData->mesh->GetAppData();
                if (appData) {
                    for (loop = 0; loop < appData->usedCount; loop++) {
                        mesh = appData->listSet[loop];
                        dead->activityFrame->DeleteVisual(mesh);
                        mesh->Release();
                    }
                    free(appData->listSet);
                    free(appData);
                }
            }
        }
    } else if (dead->type == Container_Anim) {
        if (animClone = Container_Frame_GetAnimClone(dead->activityFrame)) {
            anim_clone_remove(animClone);
        }
    }

    Container_FreeTypeData(dead);

    if (dead->type != Container_Reference ||
        !(dead->flags & CONTAINER_FLAG_DEADREFERENCE2)) {
        Container_Frame_RemoveAppData(dead->masterFrame);
    }
    if (dead->type != Container_Reference) {
        Container_Frame_RemoveAppData(dead->activityFrame);
    }
    Container_Frame_RemoveAppData(dead->hiddenFrame);

    if (dead->type != Container_Reference) {
        dead->masterFrame->GetParent(&parent);
        if (parent) {
            parent->DeleteChild(dead->masterFrame);
            parent->Release();
        }
        dead->masterFrame->Release();
        dead->activityFrame->Release();

        dead->hiddenFrame->GetParent(&parent);
        if (parent) {
            parent->DeleteChild(dead->hiddenFrame);
            parent->Release();
        }
        dead->hiddenFrame->Release();
    } else {
        dead->hiddenFrame->Release();
    }

    dead->nextFree = containerGlobs.freeList;
    containerGlobs.freeList = dead;
}

// FUNCTION: LEGORR 0x00472cd0
Container* Container_Load(Container* parent, const char* filename,
                          const char* typestr, int looping)
{
    Container* cont = NULL;
    Container_Type type;
    int noTexture;
    char name[512];
    char baseName[512];
    char path[512];

    type = (Container_Type)Container_ParseTypeString(typestr, &noTexture);
    strcpy(name, filename);

    if (type == Container_FromActivity) {
        char* argv[20];
        unsigned long argc;
        BlockFile* config;

        strcpy(baseName, name);
        argc = sound_variant_split(name, argv, "\\");
        sprintf(path, "%s\\%s.%s", baseName, argv[argc - 1],
                containerGlobs.extensionName[Container_FromActivity]);
        if (config = block_open(path)) {
            BlockRec* arrayItem;
            float scale;

            sprintf(path,
                    "%s%s%s",
                    containerGlobs.gameName,
                    // STRING: LEGORR 0x004aab18
                    "::",
                    // STRING: LEGORR 0x004aab1c
                    "Activities");
            if (arrayItem = block_subgroup(config, path)) {
                cont = Container_Create(parent);
                cont->type = Container_FromActivity;
                do {
                    char tempString[512];
                    const char* fileName;
                    char* actname = arrayItem->key;

                    if (*actname == '!') {
                        if (s_engine_feature_word & 0x00400000) goto next;
                        actname++;
                    }

                    sprintf(tempString,
                            // STRING: LEGORR 0x004aab04
                            "%s%s%s%s%s",
                            containerGlobs.gameName, "::", arrayItem->value,
                            "::",
                            // STRING: LEGORR 0x004aab10
                            "FILE");
                    if (fileName = block_get_strptr(config, tempString)) {
                        float transCo;
                        unsigned long trigger;
                        int lws, loop;
                        char* sample;

                        sprintf(path, "%s\\%s", baseName, fileName);

                        sprintf(tempString, "%s%s%s%s%s", containerGlobs.gameName,
                                "::", arrayItem->value, "::",
                                // STRING: LEGORR 0x004aaaf8
                                "TRANSCOEF");
                        transCo = (float)atof(block_get_strptr(config, tempString)
                                                  ? block_get_strptr(config, tempString)
                                                  : "");

                        sprintf(tempString, "%s%s%s%s%s", containerGlobs.gameName,
                                "::", arrayItem->value, "::",
                                // STRING: LEGORR 0x004aaaf0
                                "TRIGGER");
                        trigger = atoi(block_get_strptr(config, tempString)
                                           ? block_get_strptr(config, tempString)
                                           : "");

                        sprintf(tempString, "%s%s%s%s%s", containerGlobs.gameName,
                                "::", arrayItem->value, "::",
                                // STRING: LEGORR 0x004aaae8
                                "LWSFILE");
                        lws = (block_get_bool_path(config, tempString) == 1);

                        sprintf(tempString, "%s%s%s%s%s", containerGlobs.gameName,
                                "::", arrayItem->value, "::",
                                // STRING: LEGORR 0x004aaae0
                                "LOOPING");
                        loop = (block_get_bool_path(config, tempString) != 0);

                        sprintf(tempString, "%s%s%s%s%s", containerGlobs.gameName,
                                "::", arrayItem->value, "::",
                                // STRING: LEGORR 0x004aaad8
                                "SAMPLE");
                        sample = block_get_str_copy(config, tempString);

                        if (Container_AddActivity2(cont, path, actname, transCo, trigger,
                                                   sample, NULL, lws, loop)) {
                            if (cont->typeData == NULL) Container_SetActivity(cont, actname);
                        }
                    }
                next:
                    ;
                } while (arrayItem = block_next_equal_key(arrayItem));
            }

            sprintf(path, "%s%s%s", containerGlobs.gameName, "::",
                    // STRING: LEGORR 0x004aaad0
                    "SCALE");
            scale = (float)atof(block_get_strptr(config, path)
                                    ? block_get_strptr(config, path)
                                    : "");
            if (0.0f != scale) {
                cont->activityFrame->AddScale(D3DRMCOMBINE_REPLACE,
                                              scale, scale, scale);
            }

            block_close(config);
            return cont;
        }
    } else if (type == Container_Frame) {
        cont = Container_Create(parent);
        cont->type = type;
        // STRING: LEGORR 0x004aaac8
        sprintf(path, "%s.%s", name, containerGlobs.extensionName[Container_Frame]);
        Container_FrameLoad(path, cont->activityFrame);
        return cont;
    } else if (type == Container_Mesh) {
        void* fileData;
        unsigned long fileSize;

        sprintf(path, "%s.%s", name, containerGlobs.extensionName[Container_Mesh]);
        if (fileData = file_load_binary(path, (unsigned int*)&fileSize)) {
            LPDIRECT3DRMMESH mesh;

            cont = Container_Create(parent);
            cont->type = type;
            if (mesh = Container_MeshLoad(fileData, fileSize, path, cont->activityFrame,
                                          noTexture)) {
                Container_SetTypeData(cont, NULL, NULL, mesh, NULL);
            }
            free(fileData);
            return cont;
        }
    } else if (type == Container_Anim || type == Container_LWS) {
        struct AnimClone* animClone;
        unsigned long frameCount;

        cont = Container_Create(parent);
        cont->type = Container_Anim;
        sprintf(path, "%s.%s", name, containerGlobs.extensionName[Container_Anim]);
        if (animClone = Container_LoadAnimSet(path, cont->activityFrame, &frameCount,
                                              type == Container_LWS, looping)) {
            Container_Frame_SetAppData(cont->activityFrame, cont, animClone, name,
                                       &frameCount, NULL, NULL, NULL, NULL, NULL, NULL);
            return cont;
        }
        Container_Remove(cont);
    } else if (type == Container_LWO) {
        void* transMesh;

        cont = Container_Create(parent);
        cont->type = type;
        if (transMesh = lwo_model_entry_load(name, cont->activityFrame,
                                             (void*)(long)noTexture)) {
            Container_SetTypeData(cont, NULL, NULL, NULL, transMesh);
            return cont;
        }
        Container_Remove(cont);
    }

    return NULL;
}

// FUNCTION: LEGORR 0x00473340
int Container_IsCurrentActivity(Container* cont, const char* actname)
{
    if (cont->type == Container_FromActivity) {
        if (Container_Frame_Find(cont, actname, 0)) return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00473370
int Container_SetActivity(Container* cont, const char* actname)
{
    char* freeName = NULL;
    char* currAnimName;
    LPDIRECT3DRMFRAME3 newFrame;
    LPDIRECT3DRMFRAME3 oldFrame;
    char* newName;
    int result = 0;

    if (cont->type == Container_FromActivity) {
        if (newFrame = Container_Frame_Find(cont, actname, 1)) {
            if (cont->typeData && (currAnimName = cont->typeData->name)) {
                if (oldFrame = Container_Frame_Find(cont, currAnimName, 0)) {
                    Container_Frame_SafeAddChild(cont->hiddenFrame, oldFrame);
                }
                freeName = currAnimName;
            }
            Container_Frame_SafeAddChild(cont->activityFrame, newFrame);
            newName = (char*)malloc(strlen(actname) + 1);
            strcpy(newName, actname);
            Container_SetTypeData(cont, newName, NULL, NULL, NULL);
            cont->flags |= CONTAINER_FLAG_ANIMATIONSKIPPED;
            result = 1;
            if (freeName) free(freeName);
        } else {
            result = 0;
        }

        if (cont->activityCallback) {
            cont->activityCallback(cont, cont->activityCallbackData);
        }
    }

    return result;
}

// FUNCTION: LEGORR 0x00473460
int Container_Light_SetSpotPenumbra(Container* spotlight, float angle)
{
    return spotlight->typeData->light->SetPenumbra(angle) == D3DRM_OK;
}

// FUNCTION: LEGORR 0x00473480
int Container_Light_SetSpotUmbra(Container* spotlight, float angle)
{
    return spotlight->typeData->light->SetUmbra(angle) == D3DRM_OK;
}

// FUNCTION: LEGORR 0x004734a0
int Container_Light_SetSpotRange(Container* spotlight, float dist)
{
    return spotlight->typeData->light->SetRange(dist) == D3DRM_OK;
}

// FUNCTION: LEGORR 0x004734c0
void Container_Light_SetEnableContainer(Container* light, Container* enable)
{
    LPDIRECT3DRMFRAME frame1;

    enable->masterFrame->QueryInterface(IID_IDirect3DRMFrame,
                                        (void**)&frame1);
    light->typeData->light->SetEnableFrame(frame1);
}

// FUNCTION: LEGORR 0x004734f0
Container* Container_MakeLight(Container* parent, unsigned long type, float r,
                               float g, float b)
{
    Container* cont = Container_Create(parent);
    LPDIRECT3DRMLIGHT light;

    if (cont) {
        cont->type = Container_Light;
        if (lpD3DRM()->CreateLightRGB((D3DRMLIGHTTYPE)type, r, g, b, &light) == D3DRM_OK) {
            cont->activityFrame->AddLight(light);
            Container_SetTypeData(cont, NULL, light, NULL, NULL);
        }
    }

    return cont;
}

// FUNCTION: LEGORR 0x00473560
Container* Container_MakeMesh2(Container* parent, int type)
{
    Container* cont = Container_Create(parent);
    LPDIRECT3DRMMESH mesh;
    Container_MeshAppData* appData;
    unsigned long flags;

    if (cont) {
        cont->type = Container_Mesh;
        if (type == 3 || type == 2 || type == 4) {
            flags = 0x2000;
            if (type == 3) flags = 0x3000;
            else if (type == 4) flags = 0x2800;
            Container_SetTypeData(cont, NULL, NULL, NULL,
                                  engine_task_callback_record_allocate_and_initialize(
                                      cont->activityFrame, NULL, flags, NULL, 0));
        } else if (lpD3DRM()->CreateMesh(&mesh) == D3DRM_OK) {
            cont->activityFrame->AddVisual(mesh);
            Container_SetTypeData(cont, NULL, NULL, mesh, NULL);
            if (type == 1) {
                appData = (Container_MeshAppData*)malloc(sizeof(Container_MeshAppData));
                appData->listSize = 20;
                appData->usedCount = 0;
                appData->listSet = (LPDIRECT3DRMMESH*)malloc(
                    sizeof(LPDIRECT3DRMMESH) * appData->listSize);
                appData->groupZeroHidden = 0;
                appData->firstAddGroup = 1;
                mesh->SetAppData(appData);
            } else {
                mesh->SetAppData(NULL);
            }
        }
    }

    return cont;
}

// FUNCTION: LEGORR 0x00473680
LPDIRECT3DRMFRAME3 Container_GetMasterFrame(Container* cont)
{
    return cont->masterFrame;
}

// FUNCTION: LEGORR 0x00473690
Container* Container_Clone(Container* orig)
{
    Container* cont;
    Container* unusedCont = NULL;
    unsigned long count = 0;
    unsigned long loop;
    LPDIRECT3DRMFRAME3 parent;
    float mat[4][4];

    if (orig->type == Container_Mesh) {
        orig->typeData->mesh->GetAppData();
    }

    if (orig->type == Container_FromActivity || orig->type == Container_Anim) {
        if (orig->cloneData && orig->cloneData->clonedFrom) orig = orig->cloneData->clonedFrom;

        if (orig->cloneData) {
            if (orig->cloneData->used == 0) {
                unusedCont = orig;
            } else {
                for (loop = 0; loop < orig->cloneData->cloneCount; loop++) {
                    Container* clone = orig->cloneData->cloneTable[loop];
                    if (clone && clone->cloneData->used == 0) {
                        unusedCont = clone;
                        break;
                    }
                }
            }

            if (unusedCont) {
                cont = unusedCont;
                unusedCont->cloneData->used = 1;
                Container_SetParent(unusedCont, Container_GetParent(orig));
                Container_SetAnimationTime(unusedCont, 0.0f);
            } else {
                cont = Container_Create(Container_GetParent(orig));
                count = orig->cloneData->cloneCount++;
                orig->cloneData->cloneTable = (Container**)realloc(
                    orig->cloneData->cloneTable,
                    sizeof(Container*) * orig->cloneData->cloneCount);
                orig->cloneData->cloneTable[count] = cont;
            }
        } else {
            cont = Container_Create(Container_GetParent(orig));
            orig->cloneData = (Container_CloneData*)malloc(sizeof(Container_CloneData));
            orig->cloneData->cloneCount = 1;
            orig->cloneData->cloneTable = (Container**)malloc(sizeof(Container*));
            count = 0;
            orig->cloneData->cloneTable[0] = cont;
            orig->cloneData->used = 1;
            orig->cloneData->clonedFrom = NULL;
            orig->cloneData->cloneNumber = 0;
        }
    } else {
        cont = Container_Create(Container_GetParent(orig));
    }

    orig->activityFrame->GetParent(&parent);
    orig->activityFrame->GetTransform(parent, mat);
    cont->activityFrame->AddTransform(D3DRMCOMBINE_REPLACE, mat);
    parent->Release();

    if (unusedCont == NULL) {
        cont->type = orig->type;
        if (orig->typeData) {
            cont->typeData = (Container_TypeData*)malloc(sizeof(Container_TypeData));
            *cont->typeData = *orig->typeData;
            if (orig->typeData->name) {
                cont->typeData->name = (char*)malloc(strlen(orig->typeData->name) + 1);
                strcpy(cont->typeData->name, orig->typeData->name);
            }
        } else {
            cont->typeData = NULL;
        }

        if (orig->type == Container_FromActivity || orig->type == Container_Anim) {
            cont->cloneData = (Container_CloneData*)malloc(sizeof(Container_CloneData));
            cont->cloneData->cloneTable = NULL;
            cont->cloneData->cloneCount = 0;
            cont->cloneData->clonedFrom = orig;
            cont->cloneData->cloneNumber = count;
            cont->cloneData->used = 1;
        }

        if (cont->type == Container_Mesh) {
            LPDIRECT3DRMMESH mesh = cont->typeData->mesh;
            cont->activityFrame->AddVisual(mesh);
            mesh->AddRef();
        } else if (cont->type == Container_FromActivity) {
            LPDIRECT3DRMFRAME3* frameList;
            char** nameList;
            unsigned long loop;

            count = Container_GetActivities(orig, NULL, NULL, NULL);
            frameList = (LPDIRECT3DRMFRAME3*)malloc(sizeof(LPDIRECT3DRMFRAME3) * count);
            nameList = (char**)malloc(sizeof(char*) * count);
            Container_GetActivities(orig, frameList, NULL, nameList);
            for (loop = 0; loop < count; loop++) {
                char* fname = Container_Frame_GetAnimSetFileName(frameList[loop]);
                float transCo = Container_Frame_GetTransCo(frameList[loop]);
                char* sample = Container_Frame_GetSample(frameList[loop]);
                struct AnimClone* animClone = Container_Frame_GetAnimClone(frameList[loop]);
                unsigned long trigger = Container_Frame_GetTrigger(frameList[loop]);

                Container_Frame_SetAppData(frameList[loop], NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL, NULL);
                Container_AddActivity2(cont, fname, &nameList[loop][9], transCo, trigger,
                                       sample, animClone, 0, 0);
                free(nameList[loop]);

                if (loop % 25 == 0) sound_source_spatial_refresh_all();
            }
            free(frameList);
            free(nameList);

            if (cont->typeData) Container_SetActivity(cont, cont->typeData->name);
        } else if (cont->type == Container_Anim) {
            unsigned long frameCount;
            char* fname = Container_Frame_GetAnimSetFileName(orig->activityFrame);
            struct AnimClone* animClone = anim_clone_make(
                Container_Frame_GetAnimClone(orig->activityFrame),
                cont->activityFrame, &frameCount);
            Container_Frame_SetAppData(cont->activityFrame, cont, animClone, fname,
                                       &frameCount, NULL, NULL, NULL, NULL, NULL, NULL);
        } else if (cont->type == Container_LWO) {
            cont->typeData->transMesh = mesh_clone(cont->typeData->transMesh,
                                                   cont->activityFrame);
        }
    }

    return cont;
}

// FUNCTION: LEGORR 0x00473b20
void Container_Hide2(Container* cont, int hide)
{
    if (hide) cont->flags |= CONTAINER_FLAG_HIDDEN2;
    else cont->flags &= ~CONTAINER_FLAG_HIDDEN2;
}

// FUNCTION: LEGORR 0x00473b40
void Container_Hide(Container* cont, int hide)
{
    unsigned long hidden = cont->flags & CONTAINER_FLAG_HIDDEN;

    if (hide && !hidden) {
        Container_Frame_SafeAddChild(cont->hiddenFrame, cont->activityFrame);
        cont->flags |= CONTAINER_FLAG_HIDDEN;
    } else if (!hide && hidden) {
        Container_Frame_SafeAddChild(cont->masterFrame, cont->activityFrame);
        cont->flags &= ~CONTAINER_FLAG_HIDDEN;
    }
}

/* No C form found gives the original's `and eax, 0ffh` before the shift:
 * `(flags & HIDDEN) ? 1 : 0`, `(flags & 0xff) >> 3 & 1`, a byte cast of
 * the flags or of the shifted flags, a byte local and `!= 0` forms all
 * compile without it (25% or 50%). The port's naked body (100%) stays. */
// FUNCTION: LEGORR 0x00473ba0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int Container_IsHidden(Container* cont)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax + 14h]
        and eax, 0ffh
        shr eax, 3
        and eax, 1
        ret
    }
}
#else
int Container_IsHidden(Container* cont)
{
    return (cont->flags & CONTAINER_FLAG_HIDDEN) ? 1 : 0;
}
#endif

// FUNCTION: LEGORR 0x00473bc0
Container* Container_SearchTree(Container* root, const char* name, int mode,
                                unsigned long* count)
{
    Container_SearchData search;

    search.string = name;
    search.stringLen = strlen(name);
    search.caseSensitive = 0;
    search.resultFrame = NULL;
    search.count = 0;
    search.mode = (Container_SearchMode)mode;
    if (count) search.matchNumber = *count;
    else search.matchNumber = 0;

    Container_Frame_WalkTree(root->masterFrame, 0, Container_Frame_SearchCallback, &search);

    if (mode == Container_SearchMode_FirstMatch || mode == Container_SearchMode_NthMatch) {
        if (search.resultFrame) return Container_Frame_GetContainer(search.resultFrame);
    } else if (mode == Container_SearchMode_MatchCount) {
        *count = search.count;
        return NULL;
    }

    return NULL;
}

/* Container_FormatPartName's result (00505400, the unit's .bss). */
// GLOBAL: LEGORR 0x00505400
static char containerPartName[1024];

// FUNCTION: LEGORR 0x00473c60
char* Container_FormatPartName(Container* cont, const char* partname,
                               unsigned long* instance)
{
    LPDIRECT3DRMFRAME3 frame;
    char name[1024];
    char* s;
    char* t;

    if (cont->type == Container_FromActivity) {
        frame = Container_Frame_Find(cont, cont->typeData->name, 0);
    } else if (cont->type == Container_Anim) {
        frame = cont->activityFrame;
    }

    if (anim_clone_is_lws(Container_Frame_GetAnimClone(frame))) {
        if (instance) {
            sprintf(containerPartName, "%s_%0.2i", partname, *instance);
        } else {
            // STRING: LEGORR 0x004aab64
            sprintf(containerPartName, "%s_??", partname);
        }
    } else {
        sprintf(name, "%s", Container_Frame_GetAnimSetFileName(frame));
        strlwr(name);
        for (s = t = name; *s != '\0'; s++) {
            if (*s == '\\') t = s + 1;
        }
        if (instance) {
            // STRING: LEGORR 0x004aab44
            sprintf(containerPartName, "xf_????????_%s_%s_%0.2d_DDc_00", partname, t, *instance);
        } else {
            // STRING: LEGORR 0x004aab28
            sprintf(containerPartName, "xf_????????_%s_%s_??_DDc_00", partname, t);
        }
    }

    return containerPartName;
}

// FUNCTION: LEGORR 0x00473da0
void Container_SetUserData(Container* cont, void* data)
{
    cont->userData = data;
}

// FUNCTION: LEGORR 0x00473db0
void* Container_GetUserData(Container* cont)
{
    return cont->userData;
}

// FUNCTION: LEGORR 0x00473dc0
void Container_EnableFog(int on)
{
    containerGlobs.rootContainer->masterFrame->SetSceneFogEnable(on);
    if (on) {
        containerGlobs.rootContainer->masterFrame->SetSceneBackground(containerGlobs.fogColour);
    } else {
        containerGlobs.rootContainer->masterFrame->SetSceneBackground(0);
    }
}

// FUNCTION: LEGORR 0x00473e10
void Container_SetFogColour(float r, float g, float b)
{
    containerGlobs.fogColour = (0xff << 24) |
                               ((unsigned long)(r * 255.0f) << 16) |
                               ((unsigned long)(g * 255.0f) << 8) |
                               (unsigned long)(b * 255.0f);
    containerGlobs.rootContainer->masterFrame->SetSceneFogColor(containerGlobs.fogColour);
}

// FUNCTION: LEGORR 0x00473e70
void Container_SetFogMode(unsigned long mode)
{
    containerGlobs.rootContainer->masterFrame->SetSceneFogMode((D3DRMFOGMODE)mode);
    containerGlobs.rootContainer->masterFrame->SetSceneFogMethod(s_scene_fog_mode);
}

// FUNCTION: LEGORR 0x00473ea0
void Container_SetFogParams(float start, float end, float density)
{
    containerGlobs.rootContainer->masterFrame->SetSceneFogParams(start, end, density);
}

/* Main.c's MAIN_FLAG_VIDEOTEXTURE (0x20) in mainGlobs.flags. */
#define CNT_MAIN_FLAG_VIDEOTEXTURE 0x00000020

// FUNCTION: LEGORR 0x00473ec0
void Container_SetPerspectiveCorrection(Container* cont, int on)
{
    if (cont->type == Container_FromActivity) {
        unsigned long count = Container_GetActivities(cont, NULL, NULL, NULL);
        LPDIRECT3DRMFRAME3* frameList = (LPDIRECT3DRMFRAME3*)malloc(
            sizeof(LPDIRECT3DRMFRAME3) * count);
        unsigned long loop;
        Container_GetActivities(cont, frameList, NULL, NULL);
        for (loop = 0; loop < count; loop++) {
            Container_Frame_WalkTree(frameList[loop], 0,
                                     Container_SetPerspectiveCorrectionCallback,
                                     &on);
        }
        free(frameList);
    } else if (cont->type == Container_Mesh) {
        unsigned long count = Container_Mesh_GetGroupCount(cont);
        unsigned long loop;
        for (loop = 0; loop < count; loop++) {
            Container_Mesh_SetPerspectiveCorrection(cont, loop, on);
        }
    }
}

// FUNCTION: LEGORR 0x00473f70
static int Container_SetPerspectiveCorrectionCallback(LPDIRECT3DRMFRAME3 frame,
                                                      void* data)
{
    int on = *(int*)data;
    DWORD count;
    unsigned long loop;

    frame->GetVisuals(&count, NULL);
    if (count) {
        frame->GetVisuals(&count, containerGlobs.visualArray);
        for (loop = 0; loop < count; loop++) {
            IUnknown* visual = containerGlobs.visualArray[loop];
            LPDIRECT3DRMMESHBUILDER2 mb;
            LPDIRECT3DRMMESH mesh;
            if (visual->QueryInterface(IID_IDirect3DRMMeshBuilder2,
                                       (void**)&mb) == D3DRM_OK) {
                mb->SetPerspective(on);
            } else if (visual->QueryInterface(IID_IDirect3DRMMesh,
                                              (void**)&mesh) == D3DRM_OK) {
                unsigned long groupCount = mesh->GetGroupCount();
                unsigned long group;
                for (group = 0; group < groupCount; group++) {
                    if (on) {
                        mesh->SetGroupMapping(group, D3DRMMAP_PERSPCORRECT);
                    } else {
                        mesh->SetGroupMapping(group, D3DRMMAP_NONE);
                    }
                }
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00474050
LPDIRECTDRAWSURFACE4 Container_LoadTextureSurface(const char* fname,
                                                  int managed,
                                                  unsigned long* width,
                                                  unsigned long* height,
                                                  int* trans)
{
    LPDIRECTDRAWSURFACE4 surface = NULL;
    int convert = 0;
    void* buffer;
    unsigned int size;
    unsigned long decalColour;
    LPDIRECTDRAWPALETTE palette;
    CntImage image;
    DDSURFACEDESC2 desc;
    DDSURFACEDESC2 origDesc;

    if ((buffer = file_load_binary(fname, &size)) != NULL) {
        indexed_bitmap_palette_data_prepare((const uint8_t*)buffer, size,
                                            (struct IndexedBitmapPaletteData*)&image);
        if (!image.rgb && image.depth == 8) {
            memset(&desc, 0, sizeof(DDSURFACEDESC2));
            desc.dwSize = sizeof(DDSURFACEDESC2);
            desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
            desc.dwWidth = image.width;
            desc.dwHeight = image.height;
            desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            if (!(s_engine_feature_word & CNT_MAIN_FLAG_VIDEOTEXTURE) && managed) {
                desc.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
            } else {
                desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
            }
            desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

            if (lpDevice()->FindPreferredTextureFormat(DDBD_8, D3DRMFPTF_PALETTIZED,
                    &desc.ddpfPixelFormat) != D3DRM_OK) {
                origDesc = desc;
                memset(&desc.ddpfPixelFormat, 0, sizeof(DDPIXELFORMAT));
                desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
                desc.ddpfPixelFormat.dwFlags =
                    DDPF_RGB | DDPF_PALETTEINDEXED8;
                desc.ddpfPixelFormat.dwRGBBitCount = 8;
                desc.ddsCaps.dwCaps &= ~(DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY);
                desc.ddsCaps.dwCaps2 &= ~DDSCAPS2_TEXTUREMANAGE;
                desc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY |
                                       DDSCAPS_OFFSCREENPLAIN;
                convert = 1;
            }

            if (lpDD()->CreateSurface(&desc, &surface, NULL) == DD_OK) {
                memset(&desc, 0, sizeof(DDSURFACEDESC2));
                desc.dwSize = sizeof(DDSURFACEDESC2);
                if (surface->Lock(NULL, &desc, DDLOCK_WAIT, NULL) == DD_OK) {
                    unsigned char* dst = (unsigned char*)desc.lpSurface;
                    unsigned char* src = (unsigned char*)image.buffer1;
                    int y;
                    for (y = 0; y < image.height; y++) {
                        memcpy(dst, src, image.bytes_per_line);
                        dst += desc.lPitch;
                        src += image.bytes_per_line;
                    }
                    surface->Unlock(NULL);

                    if (lpDD()->CreatePalette(
                            DDPCAPS_8BIT | DDPCAPS_INITIALIZE | DDPCAPS_ALLOW256,
                            (LPPALETTEENTRY)image.palette, &palette,
                            NULL) == DD_OK) {
                        if (surface->SetPalette(palette) == DD_OK) {
                            if (convert) {
                                memset(&origDesc.ddpfPixelFormat, 0,
                                       sizeof(DDPIXELFORMAT));
                                origDesc.ddpfPixelFormat.dwSize =
                                    sizeof(DDPIXELFORMAT);
                                if (lpDevice()->FindPreferredTextureFormat(DDBD_16, 0,
                                        &origDesc.ddpfPixelFormat) == D3DRM_OK) {
                                    LPDIRECTDRAWSURFACE4 tempSurface = surface;
                                    if (lpDD()->CreateSurface(&origDesc, &surface,
                                            NULL) == DD_OK) {
                                        if (origDesc.ddpfPixelFormat.dwRGBBitCount == 16) {
                                            directdraw_blt_8_to_16(
                                                (struct DirectDrawSurfaceIface*)surface,
                                                (struct DirectDrawSurfaceIface*)tempSurface,
                                                image.palette);
                                        }
                                        tempSurface->Release();
                                    }
                                }
                            }

                            if (trans) {
                                if (Container_GetDecalColour(fname, &decalColour)) {
                                    DDCOLORKEY colourKey;
                                    if (convert) {
                                        LPPALETTEENTRY pal =
                                            (LPPALETTEENTRY)image.palette;
                                        decalColour = directdraw_get_colour(
                                            (struct DirectDrawSurfaceIface*)surface,
                                            (pal[decalColour].peRed << 16) |
                                                (pal[decalColour].peGreen << 8) |
                                                pal[decalColour].peBlue);
                                    }
                                    colourKey.dwColorSpaceLowValue =
                                        colourKey.dwColorSpaceHighValue = decalColour;
                                    surface->SetColorKey(DDCKEY_SRCBLT, &colourKey);
                                    *trans = 1;
                                } else {
                                    *trans = 0;
                                }
                            }

                            if (width) *width = image.width;
                            if (height) *height = image.height;
                            surface->AddRef();
                        }
                        palette->Release();
                    }
                }
                if (surface->Release() == 0) surface = NULL;
            }
        }
        object_field_38_release_target(&image);
        free(buffer);
    }
    return surface;
}

// FUNCTION: LEGORR 0x00474410
int Container_GetDecalColour(const char* fname, unsigned long* colour)
{
    const char* s;
    const char* name = fname;

    for (s = fname; *s != '\0'; s++) {
        if (*s == '\\') name = s + 1;
    }
    if (*name == 'a' || *name == 'A') {
        if (isdigit((unsigned char)name[1]) && isdigit((unsigned char)name[2]) &&
            isdigit((unsigned char)name[3]) &&
            name[4] == '_') {
            *colour = atoi(&name[1]);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x004744f0
Container_Texture* Container_LoadTexture2(const char* fname, int immediate,
                                          unsigned long* width,
                                          unsigned long* height)
{
    LPDIRECT3DRMTEXTURE3 texture = NULL;
    const char* path;
    Container_Texture* text;
    unsigned long decalColour;

    path = file_verify_filename(fname);
    if ((text = (Container_Texture*)malloc(sizeof(Container_Texture))) != NULL) {
        if (immediate) {
            LPDIRECTDRAWSURFACE4 surface = Container_LoadTextureSurface(
                fname, immediate, width, height, NULL);
            if (surface) {
                LPDIRECTDRAWSURFACE surf1;
                if (surface->QueryInterface(IID_IDirectDrawSurface,
                        (void**)&surf1) == DD_OK) {
                    if (lpD3DRM()->CreateTextureFromSurface(surf1, &texture) == D3DRM_OK) {
                        surf1->Release();
                        if (Container_GetDecalColour(fname, &decalColour)) {
                            LPDIRECTDRAWPALETTE palette;
                            PALETTEENTRY entry;
                            surface->GetPalette(&palette);
                            palette->GetEntries(0, decalColour, 1, &entry);
                            palette->Release();
                            texture->SetDecalTransparency(1);
                            texture->SetDecalTransparentColor(
                                (entry.peRed << 16) | (entry.peGreen << 8) |
                                entry.peBlue);
                        }
                        text->texture = texture;
                        text->surface = surface;
                        texture->SetAppData(text);
                        return text;
                    }
                    surf1->Release();
                }
                surface->Release();
                free(text);
                return NULL;
            }
        } else {
            if (lpD3DRM()->LoadTexture(path, &texture) == D3DRM_OK) {
                D3DRMIMAGE* image = texture->GetImage();
                if (image) {
                    if (Container_GetDecalColour(fname, &decalColour)) {
                        D3DRMPALETTEENTRY* entry = &image->palette[decalColour];
                        unsigned long red = entry->red;
                        unsigned long green = entry->green;
                        unsigned long blue = entry->blue;
                        texture->SetDecalTransparency(1);
                        texture->SetDecalTransparentColor((red << 16) | (green << 8) | blue);
                    }
                    if (width) *width = image->width;
                    if (height) *height = image->height;
                }
                text->texture = texture;
                text->surface = NULL;
                texture->SetAppData(text);
                return text;
            }
        }
        free(text);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00474710
void Container_FreeTexture(Container_Texture* text)
{
    if (text) {
        if (text->surface) {
            while (text->surface->Release());
        }
        if (text->texture) {
            while (text->texture->Release());
        }
    }
}

/* Mesh.c's Mesh, as far as Container_Mesh_Swap reads it: its user visual
 * (+0x0c). */
typedef struct Container_MeshView {
    void* reserved[3];
    LPDIRECT3DRMUSERVISUAL uv;  /* +0x0c */
} Container_MeshView;

// FUNCTION: LEGORR 0x00474760
void Container_Mesh_Swap(Container* target, Container* origin, int restore)
{
    LPDIRECT3DRMFRAME3 frame;
    DWORD count, loop;

    if (target->type == Container_Reference) frame = target->masterFrame;
    else frame = target->activityFrame;

    if (!restore) {
        frame->GetVisuals(&count, NULL);
        if (count) {
            frame->GetVisuals(&count, containerGlobs.visualArray);
            for (loop = 0; loop < count; loop++) {
                IUnknown* visual = containerGlobs.visualArray[loop];
                target->hiddenFrame->AddVisual(visual);
                frame->DeleteVisual(visual);
            }
        }
        if (origin) {
            if (origin->typeData->mesh == NULL) {
                frame->AddVisual(((Container_MeshView*)origin->typeData->transMesh)->uv);
            } else {
                frame->AddVisual(origin->typeData->mesh);
            }
        }
        target->flags |= CONTAINER_FLAG_MESHSWAPPED;
    } else {
        frame->GetVisuals(&count, NULL);
        if (count) {
            frame->GetVisuals(&count, containerGlobs.visualArray);
            for (loop = 0; loop < count; loop++) {
                frame->DeleteVisual(containerGlobs.visualArray[loop]);
            }
        }
        target->hiddenFrame->GetVisuals(&count, NULL);
        if (count) {
            target->hiddenFrame->GetVisuals(&count, containerGlobs.visualArray);
            for (loop = 0; loop < count; loop++) {
                IUnknown* visual = containerGlobs.visualArray[loop];
                frame->AddVisual(visual);
                target->hiddenFrame->DeleteVisual(visual);
            }
        }
        target->flags &= ~CONTAINER_FLAG_MESHSWAPPED;
    }
}

// FUNCTION: LEGORR 0x004748f0
unsigned long Container_Mesh_AddGroup(Container* cont, unsigned long vertexCount,
                                      unsigned long faceCount, unsigned long vPerFace,
                                      unsigned long* faceData)
{
    if (cont->typeData->transMesh) {
        return mesh_record_allocate_and_fill((struct MeshRecordPool*)cont->typeData->transMesh,
                                             vertexCount, faceCount, vPerFace,
                                             (const uint16_t*)faceData);
    } else {
        LPDIRECT3DRMMESH mesh = cont->typeData->mesh;
        D3DRMGROUPINDEX groupID;
        Container_MeshAppData* appData = (Container_MeshAppData*)mesh->GetAppData();

        if (appData) {
            if (appData->firstAddGroup == 0) {
                LPDIRECT3DRMMESH* newList;
                if (appData->usedCount == appData->listSize) return CONTAINER_ULONG_MAX;
                if (lpD3DRM()->CreateMesh(&mesh) != D3DRM_OK) return CONTAINER_ULONG_MAX;
                appData->listSet[appData->usedCount++] = mesh;
                cont->activityFrame->AddVisual(mesh);
                /* A group mesh's app data is its group number and a visible
                 * bit, not a pointer. */
                mesh->SetAppData(
                    (LPD3DRM_APPDATA)(uintptr_t)(appData->usedCount | 0x80000000));
                if (appData->usedCount == appData->listSize) {
                    if (newList = (LPDIRECT3DRMMESH*)realloc(
                            appData->listSet,
                            sizeof(LPDIRECT3DRMMESH) * (appData->listSize + 20))) {
                        appData->listSize += 20;
                        appData->listSet = newList;
                    }
                }
            } else {
                appData->firstAddGroup = 0;
            }
        }

        /* faceData holds the original's 32-bit indices (selectplace.c). */
        if (mesh->AddGroup(vertexCount, faceCount, vPerFace,
                           (unsigned int*)faceData, &groupID) == D3DRM_OK) {
            if (appData) return appData->usedCount;
            return groupID;
        }
        return CONTAINER_ULONG_MAX;
    }
}

// FUNCTION: LEGORR 0x00474a20
unsigned long Container_Mesh_GetGroupCount(Container* cont)
{
    if (cont->typeData->transMesh) {
        return pointer_value_get(cont->typeData->transMesh);
    } else {
        Container_MeshAppData* appData = (Container_MeshAppData*)
            cont->typeData->mesh->GetAppData();
        if (appData) return appData->usedCount + 1;
        return cont->typeData->mesh->GetGroupCount();
    }
}

// FUNCTION: LEGORR 0x00474a60
void Container_Mesh_SetQuality(Container* cont, unsigned long groupID,
                               unsigned long quality)
{
    unsigned long renderQuality;
    LPDIRECT3DRMMESH mesh;

    if (cont->typeData->transMesh == NULL) {
        if (quality == 0) renderQuality = 0x40;
        if (quality == 1) renderQuality = 0x80;
        if (quality == 2) renderQuality = 0x88;
        if (quality == 3) renderQuality = 0x89;
        mesh = cont->typeData->mesh;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        mesh->SetGroupQuality(groupID, (D3DRMRENDERQUALITY)renderQuality);
    }
}

// FUNCTION: LEGORR 0x00474ae0
int Container_Mesh_IsGroupHidden(Container* cont, unsigned long group)
{
    if (cont->typeData->transMesh) {
        return container_mesh_is_group_hidden_adapter(cont->typeData->transMesh, group);
    } else {
        LPDIRECT3DRMMESH mesh = cont->typeData->mesh;
        Container_MeshAppData* appData = (Container_MeshAppData*)mesh->GetAppData();
        if (appData) {
            if (group) {
                mesh = appData->listSet[group - 1];
                return !((uintptr_t)mesh->GetAppData() & 0x80000000);
            }
            return appData->groupZeroHidden;
        }
        return (int)group;
    }
}

// FUNCTION: LEGORR 0x00474b30
void Container_Mesh_HideGroup(Container* cont, unsigned long group, int hide)
{
    if (cont->typeData->transMesh) {
        indexed_record_flag6c_bit0_set(cont->typeData->transMesh, (int)group, hide);
    } else {
        LPDIRECT3DRMMESH mesh = cont->typeData->mesh;
        Container_MeshAppData* appData = (Container_MeshAppData*)mesh->GetAppData();
        if (appData) {
            LPDIRECT3DRMMESH groupMesh;
            int hidden;
            if (group) {
                groupMesh = appData->listSet[group - 1];
                hidden = !((uintptr_t)groupMesh->GetAppData() & 0x80000000);
                mesh = groupMesh;
            } else {
                hidden = appData->groupZeroHidden;
                groupMesh = NULL;
            }

            if (hide && !hidden) {
                cont->activityFrame->DeleteVisual(mesh);
            } else if (!hide && hidden) {
                cont->activityFrame->AddVisual(mesh);
            }

            if (groupMesh) {
                uintptr_t data = (uintptr_t)groupMesh->GetAppData() & 0x7fffffff;
                if (hide) groupMesh->SetAppData((LPD3DRM_APPDATA)data);
                else groupMesh->SetAppData((LPD3DRM_APPDATA)(data | 0x80000000));
            } else {
                appData->groupZeroHidden = hide;
            }
        }
    }
}

// FUNCTION: LEGORR 0x00474c00
int Container_Mesh_HandleSeparateMeshGroups(LPDIRECT3DRMMESH* mesh, unsigned long* group)
{
    Container_MeshAppData* appData = (Container_MeshAppData*)(*mesh)->GetAppData();
    if (appData) {
        if (*group) *mesh = appData->listSet[*group - 1];
        *group = 0;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00474c40
int Container_Mesh_GetGroup(Container* cont, unsigned long groupID,
                            unsigned long* vertexCount, unsigned long* faceCount,
                            unsigned long* vPerFace, unsigned long* faceDataSize,
                            unsigned long* faceData)
{
    if (cont->typeData->transMesh) {
        return container_mesh_get_group_adapter(cont->typeData->transMesh, groupID,
                                                vertexCount, faceCount, vPerFace,
                                                faceDataSize, faceData);
    } else {
        LPDIRECT3DRMMESH mesh = cont->typeData->mesh;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        /* The outputs are the original's 32-bit counts and indices. */
        return mesh->GetGroup(groupID, (unsigned int*)vertexCount,
                              (unsigned int*)faceCount, (unsigned int*)vPerFace,
                              (DWORD*)faceDataSize,
                              (unsigned int*)faceData) == D3DRM_OK;
    }
}

// FUNCTION: LEGORR 0x00474cc0
unsigned long Container_Mesh_GetVertices(Container* cont, unsigned long groupID,
                                         unsigned long index, unsigned long count,
                                         void* retArray)
{
    if (cont->typeData->transMesh) {
        return container_mesh_get_vertices_adapter(cont->typeData->transMesh, groupID, index,
                                                   count, retArray);
    } else {
        LPDIRECT3DRMMESH mesh = cont->typeData->mesh;
        unsigned long result = 0;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        if (mesh->GetVertices(groupID, index, count,
                              (D3DRMVERTEX*)retArray) != D3DRM_OK) {
            result = CONTAINER_ULONG_MAX;
        }
        return result;
    }
}

// FUNCTION: LEGORR 0x00474d30
unsigned long Container_Mesh_SetVertices(Container* cont, unsigned long groupID,
                                         unsigned long index, unsigned long count,
                                         void* values)
{
    if (cont->typeData->transMesh) {
        return container_mesh_set_vertices_adapter(cont->typeData->transMesh, groupID, index,
                                                   count, values);
    } else {
        LPDIRECT3DRMMESH mesh = cont->typeData->mesh;
        unsigned long result = 0;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        if (mesh->SetVertices(groupID, index, count,
                              (D3DRMVERTEX*)values) != D3DRM_OK) {
            result = CONTAINER_ULONG_MAX;
        }
        return result;
    }
}

// FUNCTION: LEGORR 0x00474da0
void Container_Mesh_SetTexture(Container* cont, unsigned long groupID,
                               Container_Texture* itext)
{
    LPDIRECT3DRMTEXTURE3 texture;
    LPDIRECT3DRMTEXTURE texture1;

    if (itext) texture = itext->texture;
    else texture = NULL;

    if (cont->typeData->transMesh) {
        animated_texture_frame_surface_set(cont->typeData->transMesh, (int)groupID, itext);
    } else {
        LPDIRECT3DRMMESH mesh = cont->typeData->mesh;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        if (texture) {
            texture->QueryInterface(IID_IDirect3DRMTexture, (void**)&texture1);
        } else {
            texture1 = NULL;
        }
        mesh->SetGroupTexture(groupID, texture1);
        if (texture1) texture1->Release();
    }
}

// FUNCTION: LEGORR 0x00474e30
void Container_Mesh_SetPerspectiveCorrection(Container* cont, unsigned long groupID, int on)
{
    if (cont->typeData->transMesh == NULL) {
        LPDIRECT3DRMMESH mesh = cont->typeData->mesh;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        if (on) mesh->SetGroupMapping(groupID, D3DRMMAP_PERSPCORRECT);
        else mesh->SetGroupMapping(groupID, D3DRMMAP_NONE);
    }
}

// FUNCTION: LEGORR 0x00474e90
int Container_Mesh_Scale(Container* cont, float x, float y, float z)
{
    if (cont->typeData->transMesh) {
        container_mesh_scale_adapter(cont->typeData->transMesh, x, y, z);
    } else {
        LPDIRECT3DRMMESH mesh = cont->typeData->mesh;
        Container_MeshAppData* appData;
        unsigned long loop;

        mesh->Scale(x, y, z);
        if (appData = (Container_MeshAppData*)mesh->GetAppData()) {
            for (loop = 0; loop < appData->usedCount; loop++) {
                appData->listSet[loop]->Scale(x, y, z);
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00474f10
int Container_Mesh_GetBox(Container* cont, float* box)
{
    LPDIRECT3DRMMESH mesh = cont->typeData->mesh;
    D3DRMBOX* result = (D3DRMBOX*)box;
    Container_MeshAppData* appData;
    unsigned long loop;

    mesh->GetBox(result);
    if (appData = (Container_MeshAppData*)mesh->GetAppData()) {
        for (loop = 0; loop < appData->usedCount; loop++) {
            D3DRMBOX tempBox;
            appData->listSet[loop]->GetBox(&tempBox);
            if (tempBox.min.x < result->min.x) result->min.x = tempBox.min.x;
            if (tempBox.min.y < result->min.y) result->min.y = tempBox.min.y;
            if (tempBox.min.z < result->min.z) result->min.z = tempBox.min.z;
            if (tempBox.max.x > result->max.x) result->max.x = tempBox.max.x;
            if (tempBox.max.y > result->max.y) result->max.y = tempBox.max.y;
            if (tempBox.max.z > result->max.z) result->max.z = tempBox.max.z;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00474ff0
void Container_Mesh_SetEmissive(Container* cont, unsigned long groupID, float r, float g,
                                float b)
{
    if (cont->typeData->transMesh) {
        render_object_color_vector_set(cont->typeData->transMesh, (int)groupID, r, g, b, 3);
    }
}

// FUNCTION: LEGORR 0x00475020
void Container_Mesh_SetColourAlpha(Container* cont, unsigned long groupID, float r, float g,
                                   float b, float a)
{
    void* transMesh;

    if (transMesh = cont->typeData->transMesh) {
        render_object_color_vector_set(transMesh, (int)groupID, r, g, b, 0);
        mesh_group_material_values_set(transMesh, (int)groupID, a, 4);
    }
}

// FUNCTION: LEGORR 0x00475070
void Container_Transform(Container* cont, float* destVector, float* srcVector)
{
    cont->masterFrame->Transform((D3DVECTOR*)destVector,
                                 (D3DVECTOR*)srcVector);
}

// FUNCTION: LEGORR 0x00475090
void Container_InverseTransform(Container* cont, float* destVector, float* srcVector)
{
    cont->masterFrame->InverseTransform((D3DVECTOR*)destVector,
                                        (D3DVECTOR*)srcVector);
}

// FUNCTION: LEGORR 0x004750b0
void Container_SetColourAlpha(Container* cont, float r, float g, float b, float a)
{
    unsigned long colour = ((unsigned long)(a * 255.0f) << 24) |
                           ((unsigned long)(r * 255.0f) << 16) |
                           ((unsigned long)(g * 255.0f) << 8) |
                           (unsigned long)(b * 255.0f);

    if (cont->type == Container_Light) {
        cont->typeData->light->SetColor(colour);
    }
}

// FUNCTION: LEGORR 0x00475120
float Container_MoveAnimation(Container* cont, float delta)
{
    float time = Container_GetAnimationTime(cont);
    float newTime = time + delta;
    return Container_SetAnimationTime(cont, newTime);
}

// FUNCTION: LEGORR 0x00475140
float Container_SetAnimationTime(Container* cont, float time)
{
    struct AnimClone* animClone = NULL;
    float overrun = 0.0f;
    LPDIRECT3DRMFRAME3 frame;

    if (cont->type == Container_FromActivity) {
        if (frame = Container_Frame_Find(cont, cont->typeData->name, 0)) {
            animClone = Container_Frame_GetAnimClone(frame);
            if (cont->flags & CONTAINER_FLAG_ANIMATIONSKIPPED) {
                char* sample;
                if ((sample = Container_Frame_GetSample(frame)) &&
                    containerGlobs.soundTriggerCallback &&
                    (containerGlobs.flags & CONTAINER_FLAG_TRIGGERENABLED)) {
                    containerGlobs.soundTriggerCallback(sample, cont,
                                                        containerGlobs.soundTriggerData);
                }
                cont->flags &= ~CONTAINER_FLAG_ANIMATIONSKIPPED;
            }
        }
    } else if (cont->type == Container_Anim) {
        animClone = Container_Frame_GetAnimClone(cont->activityFrame);
        frame = cont->activityFrame;
    }

    if (animClone) {
        unsigned long hidden = cont->flags & CONTAINER_FLAG_HIDDEN;
        float oldTime = Container_GetAnimationTime(cont);
        unsigned long frameCount = Container_Frame_GetFrameCount(frame);

        Container_Frame_SetAppData(frame, NULL, NULL, NULL, NULL, NULL, &time, NULL, NULL,
                                   NULL, NULL);

        if (frameCount) {
            frameCount--;
            if (time > (float)frameCount) {
                overrun = time - (float)frameCount;
                hidden = 0;
            }
        }

        if (!hidden) {
            float triggerFrame = (float)Container_Frame_GetTrigger(frame);
            if (frameCount != 1) {
                anim_clone_set_time(animClone, time, NULL);
                if (containerGlobs.triggerFrameCallback && triggerFrame != 0.0f &&
                    oldTime < triggerFrame && time >= triggerFrame) {
                    containerGlobs.triggerFrameCallback(cont, containerGlobs.triggerFrameData);
                    return overrun;
                }
            }
        } else {
            cont->flags |= CONTAINER_FLAG_ANIMATIONUPDATE;
        }
    }

    return overrun;
}

// FUNCTION: LEGORR 0x00475300
void Container_ForceAnimationUpdate(Container* cont)
{
    struct AnimClone* animClone;
    float time;

    if (cont->flags & CONTAINER_FLAG_ANIMATIONUPDATE) {
        if (cont->type == Container_FromActivity) {
            LPDIRECT3DRMFRAME3 frame;
            if (frame = Container_Frame_Find(cont, cont->typeData->name, 0)) {
                animClone = Container_Frame_GetAnimClone(frame);
                time = Container_Frame_GetCurrTime(frame);
            }
        } else if (cont->type == Container_Anim) {
            animClone = Container_Frame_GetAnimClone(cont->activityFrame);
            time = Container_Frame_GetCurrTime(cont->activityFrame);
        }
        if (animClone) anim_clone_set_time(animClone, time, NULL);
    }
}

// FUNCTION: LEGORR 0x00475390
float Container_GetAnimationTime(Container* cont)
{
    float time = 0.0f;

    if (cont->type == Container_FromActivity) {
        LPDIRECT3DRMFRAME3 frame;
        if (frame = Container_Frame_Find(cont, cont->typeData->name, 0)) {
            time = Container_Frame_GetCurrTime(frame);
        }
    } else if (cont->type == Container_Anim) {
        time = Container_Frame_GetCurrTime(cont->activityFrame);
    }

    return time;
}

// FUNCTION: LEGORR 0x004753f0
unsigned long Container_GetAnimationFrames(Container* cont)
{
    LPDIRECT3DRMFRAME3 frame = NULL;

    if (cont->type == Container_FromActivity) {
        frame = Container_Frame_Find(cont, cont->typeData->name, 0);
    } else if (cont->type == Container_Anim) {
        frame = cont->activityFrame;
    }
    if (frame) return Container_Frame_GetFrameCount(frame);
    return 0;
}

// FUNCTION: LEGORR 0x00475430
void Container_SetPosition(Container* cont, Container* ref, float x, float y, float z)
{
    LPDIRECT3DRMFRAME3 contFrame;
    LPDIRECT3DRMFRAME3 refFrame;

    Container_GetFrames(cont, ref, &contFrame, &refFrame);
    contFrame->SetPosition(refFrame, x, y, z);
}

// FUNCTION: LEGORR 0x00475470
void Container_SetOrientation(Container* cont, Container* ref, float dirx, float diry,
                              float dirz, float upx, float upy, float upz)
{
    LPDIRECT3DRMFRAME3 contFrame;
    LPDIRECT3DRMFRAME3 refFrame;

    Container_GetFrames(cont, ref, &contFrame, &refFrame);
    contFrame->SetOrientation(refFrame, dirx, diry, dirz, upx, upy, upz);
}

// FUNCTION: LEGORR 0x004754c0
void Container_GetPosition(Container* cont, Container* ref, float* pos)
{
    LPDIRECT3DRMFRAME3 contFrame;
    LPDIRECT3DRMFRAME3 refFrame;

    Container_GetFrames(cont, ref, &contFrame, &refFrame);
    contFrame->GetPosition(refFrame, (D3DVECTOR*)pos);
}

// FUNCTION: LEGORR 0x00475500
void Container_GetOrientation(Container* cont, Container* ref, float* dir, float* up)
{
    LPDIRECT3DRMFRAME3 contFrame;
    LPDIRECT3DRMFRAME3 refFrame;
    D3DVECTOR tempDir, tempUp;

    Container_GetFrames(cont, ref, &contFrame, &refFrame);
    contFrame->GetOrientation(refFrame, &tempDir, &tempUp);
    if (dir) {
        dir[0] = tempDir.x;
        dir[1] = tempDir.y;
        dir[2] = tempDir.z;
    }
    if (up) {
        up[0] = tempUp.x;
        up[1] = tempUp.y;
        up[2] = tempUp.z;
    }
}

// FUNCTION: LEGORR 0x00475580
void Container_AddRotation(Container* cont, int combine, float x, float y, float z,
                           float angle)
{
    cont->masterFrame->AddRotation((D3DRMCOMBINETYPE)combine, x, y, z, angle);
}

// FUNCTION: LEGORR 0x004755b0
void Container_AddScale(Container* cont, int combine, float x, float y, float z)
{
    cont->masterFrame->AddScale((D3DRMCOMBINETYPE)combine, x, y, z);
}

// FUNCTION: LEGORR 0x004755e0
void Container_AddTranslation(Container* cont, int combine, float x, float y, float z)
{
    cont->masterFrame->AddTranslation((D3DRMCOMBINETYPE)combine, x, y, z);
}

// FUNCTION: LEGORR 0x00475610
void Container_ClearTransform(Container* cont)
{
    float mat[4][4];

    mat[0][0] = 1.0f; mat[1][0] = 0.0f; mat[2][0] = 0.0f; mat[3][0] = 0.0f;
    mat[0][1] = 0.0f; mat[1][1] = 1.0f; mat[2][1] = 0.0f; mat[3][1] = 0.0f;
    mat[0][2] = 0.0f; mat[1][2] = 0.0f; mat[2][2] = 1.0f; mat[3][2] = 0.0f;
    mat[0][3] = 0.0f; mat[1][3] = 0.0f; mat[2][3] = 0.0f; mat[3][3] = 1.0f;

    Container_AddTransform(cont, (int)D3DRMCOMBINE_REPLACE, mat);
}

// FUNCTION: LEGORR 0x004756b0
void Container_AddTransform(Container* cont, int combine, float (*matrix)[4])
{
    cont->masterFrame->AddTransform((D3DRMCOMBINETYPE)combine, matrix);
}

// FUNCTION: LEGORR 0x004756d0
float Container_GetZXRatio(Container* cont)
{
    LPDIRECT3DRMFRAME3 frame = cont->masterFrame;
    LPDIRECT3DRMFRAME3 parent;
    float mat[4][4];

    frame->GetParent(&parent);
    frame->GetTransform(parent, mat);
    parent->Release();

    return mat[2][2] / mat[0][0];
}

// FUNCTION: LEGORR 0x00475710
void Container_SetParent(Container* child, Container* parent)
{
    LPDIRECT3DRMFRAME3 frame = child->masterFrame;
    LPDIRECT3DRMFRAME3 hiddenFrame = child->hiddenFrame;
    LPDIRECT3DRMFRAME3 parentFrame;

    if (parent == NULL) {
        frame->GetParent(&parentFrame);
        if (parentFrame) {
            parentFrame->DeleteChild(frame);
            parentFrame->Release();
        }
        hiddenFrame->GetParent(&parentFrame);
        if (parentFrame) {
            parentFrame->DeleteChild(hiddenFrame);
            parentFrame->Release();
        }
    } else {
        Container_Frame_SafeAddChild(parentFrame = parent->masterFrame, frame);
        Container_Frame_SafeAddChild(parentFrame = parent->hiddenFrame, hiddenFrame);
    }
}

// FUNCTION: LEGORR 0x004757a0
Container* Container_GetParent(Container* child)
{
    LPDIRECT3DRMFRAME3 parentFrame;
    Container* parent;

    if (child == containerGlobs.rootContainer) return NULL;

    child->masterFrame->GetParent(&parentFrame);
    if (parentFrame) {
        parent = Container_Frame_GetContainer(parentFrame);
        parentFrame->Release();
    } else {
        parent = NULL;
    }

    return parent;
}

// FUNCTION: LEGORR 0x004757f0
float Container_GetTransCoef(Container* cont)
{
    if (cont->type == Container_FromActivity && cont->typeData && cont->typeData->name) {
        LPDIRECT3DRMFRAME3 frame;
        if (frame = Container_Frame_Find(cont, cont->typeData->name, 0)) {
            return Container_Frame_GetTransCo(frame);
        }
    }
    return 0.0f;
}

// FUNCTION: LEGORR 0x00475830
Container* Container_SearchOwner(LPDIRECT3DRMFRAME3 frame)
{
    Container* owner = NULL;
    Container* cont;
    LPDIRECT3DRMFRAME3 parent;

    while (owner == NULL && frame != NULL) {
        if ((cont = Container_Frame_GetOwner(frame)) && cont->type != Container_Reference) {
            owner = cont;
        }
        frame->GetParent(&parent);
        if (frame = parent) frame->Release();
        else frame = NULL;
    }

    return owner;
}

// FUNCTION: LEGORR 0x00475880
Container* Container_Frame_GetContainer(LPDIRECT3DRMFRAME3 frame)
{
    Container* cont = Container_Frame_GetOwner(frame);

    if (cont == NULL) {
        if (cont = Container_Create(NULL)) {
            cont->masterFrame->Release();
            cont->activityFrame->Release();
            cont->masterFrame = frame;
            cont->activityFrame = NULL;
            cont->type = Container_Reference;
            frame->AddDestroyCallback(Container_Frame_ReferenceDestroyCallback,
                                      NULL);
            Container_Frame_SetAppData(frame, cont, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL);
        }
    }

    return cont;
}

// FUNCTION: LEGORR 0x00475900
void Container_GetFrames(Container* cont, Container* ref, LPDIRECT3DRMFRAME3* contFrame,
                         LPDIRECT3DRMFRAME3* refFrame)
{
    *contFrame = cont->masterFrame;
    if (ref == NULL) *refFrame = containerGlobs.rootContainer->masterFrame;
    else *refFrame = ref->masterFrame;
}

// FUNCTION: LEGORR 0x00475930
int Container_ParseTypeString(const char* str, int* noTexture)
{
    char string[40];
    char* argv[10];
    unsigned long loop;

    if (str) {
        strcpy(string, str);
        if ((unsigned long)sound_variant_split(string, argv, ":") > 1 &&
            // STRING: LEGORR 0x004aab78
            stricmp(argv[1], "NOTEXTURE") == 0) {
            *noTexture = 1;
        } else {
            *noTexture = 0;
        }

        for (loop = 0; loop < Container_TypeCount; loop++) {
            if (containerGlobs.typeName[loop]) {
                if (stricmp(containerGlobs.typeName[loop], string) == 0) return (int)loop;
            }
        }
    }

    return Container_Invalid;
}

// FUNCTION: LEGORR 0x004759f0
void Container_AddList(void)
{
    Container* list;
    unsigned long loop, count;

    count = 0x00000001 << containerGlobs.listCount;
    containerGlobs.listSet[containerGlobs.listCount] =
        (Container*)malloc(sizeof(Container) * count);
    if (list = containerGlobs.listSet[containerGlobs.listCount]) {
        containerGlobs.listCount++;
        for (loop = 1; loop < count; loop++) {
            list[loop - 1].nextFree = &list[loop];
        }
        list[count - 1].nextFree = containerGlobs.freeList;
        containerGlobs.freeList = list;
    }
}

/* The texture loader's state (Container_TextureLoadCallback's data). */
typedef struct Container_TextureData {
    const char* xFileName;      /* +0x00 */
    unsigned long flags;        /* +0x04 */
} Container_TextureData;

#define CONTAINER_TEXTURE_NOLOAD 0x00000001

/* Main.c's MAIN_FLAG_MIPMAPENABLED (0x08) in mainGlobs.flags. */
#define CNT_MAIN_FLAG_MIPMAPENABLED 0x00000008

// FUNCTION: LEGORR 0x00475a70
unsigned long Container_GetActivities(Container* cont,
                                      LPDIRECT3DRMFRAME3* frameList,
                                      struct AnimClone** acList,
                                      char** nameList)
{
    LPDIRECT3DRMFRAME3 frame;
    DWORD len = 0;
    unsigned long count = 0;
    unsigned long loop;

    if (cont->type == Container_FromActivity) {
        for (loop = 0; loop < 2; loop++) {
            LPDIRECT3DRMFRAME3 root;
            LPDIRECT3DRMFRAMEARRAY children;
            if (loop == 0) root = cont->activityFrame;
            if (loop == 1) root = cont->hiddenFrame;
            if (root->GetChildren(&children) == D3DRM_OK) {
                unsigned long childCount = children->GetSize();
                unsigned long sub;
                for (sub = 0; sub < childCount; sub++) {
                    LPDIRECT3DRMFRAME frame1;
                    children->GetElement(sub, &frame1);
                    frame1->QueryInterface(IID_IDirect3DRMFrame3,
                                           (void**)&frame);
                    frame1->Release();
                    frame->GetName(&len, NULL);
                    if (len) {
                        char* name = (char*)malloc(len);
                        frame->GetName(&len, name);
                        if (strnicmp(name, CONTAINER_ACTIVITYFRAMEPREFIX,
                                      strlen(CONTAINER_ACTIVITYFRAMEPREFIX)) == 0) {
                            if (frameList) frameList[count] = frame;
                            if (acList) acList[count] = Container_Frame_GetAnimClone(frame);
                            if (nameList) nameList[count] = name;
                            count++;
                            if (!nameList) free(name);
                        } else {
                            free(name);
                        }
                    }
                    frame->Release();
                }
                children->Release();
            }
        }
    }
    return count;
}

// FUNCTION: LEGORR 0x00475c00
void Container_SetTypeData(Container* cont, char* name, void* light,
                           LPDIRECT3DRMMESH mesh, void* transMesh)
{
    if (cont->typeData) {
        if (name) cont->typeData->name = name;
        if (light) cont->typeData->light = (LPDIRECT3DRMLIGHT)light;
        if (mesh) cont->typeData->mesh = mesh;
        if (transMesh) cont->typeData->transMesh = transMesh;
    } else {
        cont->typeData = (Container_TypeData*)malloc(sizeof(Container_TypeData));
        cont->typeData->name = name;
        cont->typeData->light = (LPDIRECT3DRMLIGHT)light;
        cont->typeData->mesh = mesh;
        cont->typeData->transMesh = transMesh;
    }
}

// FUNCTION: LEGORR 0x00475c80
void Container_FreeTypeData(Container* cont)
{
    if (cont->typeData) {
        if (cont->type == Container_FromActivity) {
            free(cont->typeData->name);
        } else if (cont->type == Container_Light) {
            cont->typeData->light->Release();
        } else if (cont->type == Container_Mesh) {
            if (cont->typeData->mesh) {
                cont->activityFrame->DeleteVisual(cont->typeData->mesh);
                cont->typeData->mesh->Release();
            }
        } else if (cont->type == Container_LWO) {
            if (cont->typeData->transMesh) {
                mesh_remove(cont->typeData->transMesh, cont->activityFrame);
            }
        }
        free(cont->typeData);
        cont->typeData = NULL;
    }
}

// FUNCTION: LEGORR 0x00475d10
int Container_AddActivity2(Container* cont, const char* filename,
                           const char* actname, float transCo,
                           unsigned long trigger, const char* sample,
                           struct AnimClone* origClone, int lws,
                           int looping)
{
    LPDIRECT3DRMFRAME3 frame;
    unsigned long frameCount;
    char xFile[512];
    struct AnimClone* animClone;

    sprintf(xFile, "%s.%s", filename,
            containerGlobs.extensionName[Container_Anim]);
    if (lpD3DRM()->CreateFrame(cont->hiddenFrame,
                               &frame) == D3DRM_OK) {
        Container_Frame_FormatName(frame,
                                   // STRING: LEGORR 0x004aab90
                                   "%s_%s",
                                   CONTAINER_ACTIVITYFRAMEPREFIX, actname);
        if (origClone == NULL) {
            animClone = Container_LoadAnimSet(xFile, frame, &frameCount, lws,
                                              looping);
        } else {
            animClone = anim_clone_make(origClone, frame,
                                        &frameCount);
        }
        Container_Frame_SetAppData(frame, cont, animClone, filename,
                                   &frameCount, NULL, NULL, &transCo, sample,
                                   NULL, &trigger);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00475e10
static void Container_Frame_ReferenceDestroyCallback(
                                                     LPDIRECT3DRMOBJECT obj, void* arg)
{
    LPDIRECT3DRMFRAME3 frame = (LPDIRECT3DRMFRAME3)obj;
    Container* cont = Container_Frame_GetOwner(frame);
    (void)arg;
    if (cont) {
        cont->flags |= CONTAINER_FLAG_DEADREFERENCE2;
        Container_Frame_RemoveAppData(frame);
    }
}

// FUNCTION: LEGORR 0x00475e40
LPDIRECT3DRMFRAME3 Container_Frame_Find(Container* cont, const char* findName,
                                        int hidden)
{
    LPDIRECT3DRMFRAME3 parent;
    LPDIRECT3DRMFRAME3 frame;
    LPDIRECT3DRMFRAME3 foundFrame = NULL;
    LPDIRECT3DRMFRAMEARRAY children;
    LPDIRECT3DRMFRAME frame1;
    unsigned long count;
    unsigned long loop;
    DWORD len;
    char* name;
    char buffer[512];

    if (hidden) parent = cont->hiddenFrame;
    else parent = cont->activityFrame;
    if (parent->GetChildren(&children) == D3DRM_OK) {
        count = children->GetSize();
        for (loop = 0; loop < count; loop++) {
            children->GetElement(loop, &frame1);
            frame1->QueryInterface(IID_IDirect3DRMFrame3,
                                   (void**)&frame);
            frame1->Release();
            frame->GetName(&len, NULL);
            if (len) {
                name = (char*)malloc(len + 1);
                frame->GetName(&len, name);
                sprintf(buffer, "%s_%s", CONTAINER_ACTIVITYFRAMEPREFIX,
                        findName);
                if (stricmp(name, buffer) == 0) foundFrame = frame;
                free(name);
            }
            frame->Release();
            if (foundFrame) break;
        }
        children->Release();
    }
    return foundFrame;
}

// FUNCTION: LEGORR 0x00475f70
void Container_Frame_SetAppData(LPDIRECT3DRMFRAME3 frame, Container* owner,
                                struct AnimClone* animClone,
                                const char* asfname,
                                unsigned long* frameCount,
                                const char* frameName, float* currTime,
                                float* transCo, const char* actSample,
                                void* soundRecord, unsigned long* trigger)
{
    Container_AppData* appData =
        (Container_AppData*)frame->GetAppData();

    if (appData) {
        if (owner) appData->ownerContainer = owner;
        if (animClone) appData->animClone = animClone;
        if (asfname) {
            if (appData->animSetFileName) free(appData->animSetFileName);
        }
        if (frameCount) appData->frameCount = *frameCount;
        if (frameName) appData->frameName = (char*)frameName;
        if (currTime) appData->currTime = *currTime;
        if (transCo) appData->transCo = *transCo;
        if (actSample) appData->activitySample = (char*)actSample;
        if (soundRecord) appData->soundList = soundRecord;
        if (trigger) appData->trigger = *trigger;
    } else {
        appData = (Container_AppData*)malloc(sizeof(Container_AppData));
        appData->ownerContainer = owner;
        appData->animClone = animClone;
        appData->animSetFileName = NULL;
        if (frameCount) appData->frameCount = *frameCount;
        else appData->frameCount = 0;
        appData->frameName = (char*)frameName;
        if (currTime) appData->currTime = *currTime;
        else appData->currTime = 0.0f;
        if (transCo) appData->transCo = *transCo;
        else appData->transCo = 0.0f;
        if (trigger) appData->trigger = *trigger;
        else appData->trigger = 0;
        appData->activitySample = (char*)actSample;
        appData->soundList = soundRecord;
        frame->SetAppData(appData);
    }
    if (asfname) {
        appData->animSetFileName = (char*)malloc(strlen(asfname) + 1);
        strcpy(appData->animSetFileName, asfname);
    }
}

// FUNCTION: LEGORR 0x004760e0
void Container_Frame_RemoveAppData(LPDIRECT3DRMFRAME3 frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->GetAppData();
    if (appData) {
        if (appData->animSetFileName) free(appData->animSetFileName);
        free(appData);
    }
    frame->SetAppData(NULL);
}

// FUNCTION: LEGORR 0x00476120
Container* Container_Frame_GetOwner(LPDIRECT3DRMFRAME3 frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->GetAppData();
    if (appData) return appData->ownerContainer;
    return NULL;
}

// FUNCTION: LEGORR 0x00476140
char* Container_Frame_GetAnimSetFileName(LPDIRECT3DRMFRAME3 frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->GetAppData();
    if (appData) return appData->animSetFileName;
    return NULL;
}

// FUNCTION: LEGORR 0x00476160
struct AnimClone* Container_Frame_GetAnimClone(LPDIRECT3DRMFRAME3 frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->GetAppData();
    if (appData) return appData->animClone;
    return NULL;
}

// FUNCTION: LEGORR 0x00476180
unsigned long Container_Frame_GetFrameCount(LPDIRECT3DRMFRAME3 frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->GetAppData();
    if (appData) return appData->frameCount;
    return 0;
}

// FUNCTION: LEGORR 0x004761a0
float Container_Frame_GetCurrTime(LPDIRECT3DRMFRAME3 frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->GetAppData();
    if (appData) return appData->currTime;
    return 0.0f;
}

// FUNCTION: LEGORR 0x004761c0
float Container_Frame_GetTransCo(LPDIRECT3DRMFRAME3 frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->GetAppData();
    if (appData) return appData->transCo;
    return 0.0f;
}

// FUNCTION: LEGORR 0x004761e0
char* Container_Frame_GetSample(LPDIRECT3DRMFRAME3 frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->GetAppData();
    if (appData) return appData->activitySample;
    return NULL;
}

// FUNCTION: LEGORR 0x00476200
unsigned long Container_Frame_GetTrigger(LPDIRECT3DRMFRAME3 frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->GetAppData();
    if (appData) return appData->trigger;
    return 0;
}

// FUNCTION: LEGORR 0x00476220
void Container_Frame_SafeAddChild(LPDIRECT3DRMFRAME3 parent,
                                  LPDIRECT3DRMFRAME3 child)
{
    LPDIRECT3DRMFRAME3 oldParent;
    float matrix[4][4];

    child->GetParent(&oldParent);
    child->GetTransform(oldParent, matrix);
    if (oldParent) oldParent->Release();
    parent->AddChild(child);
    child->AddTransform(D3DRMCOMBINE_REPLACE, matrix);
}

// FUNCTION: LEGORR 0x00476270
void Container_Frame_FormatName(LPDIRECT3DRMFRAME3 frame, const char* msg, ...)
{
    char buffer[1024];
    char* name;
    unsigned long len;
    va_list args;

    va_start(args, msg);
    len = vsprintf(buffer, msg, args);
    name = (char*)malloc(len + 1);
    vsprintf(name, msg, args);
    va_end(args);

    frame->SetName(name);
    Container_Frame_SetAppData(frame, NULL, NULL, NULL, NULL, name, NULL,
                               NULL, NULL, NULL, NULL);
}

// FUNCTION: LEGORR 0x004762f0
void Container_Frame_FreeName(LPDIRECT3DRMFRAME3 frame)
{
    char* name = Container_Frame_GetName(frame);
    if (name) free(name);
}

// FUNCTION: LEGORR 0x00476310
char* Container_Frame_GetName(LPDIRECT3DRMFRAME3 frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->GetAppData();
    if (appData) return appData->frameName;
    return NULL;
}

// FUNCTION: LEGORR 0x00476330
int Container_Frame_WalkTree(LPDIRECT3DRMFRAME3 frame, unsigned long level,
                             ContainerWalkTreeCallback callback, void* data)
{
    LPDIRECT3DRMFRAMEARRAY children;
    LPDIRECT3DRMFRAME frame1;
    LPDIRECT3DRMFRAME3 child;
    unsigned long count;
    unsigned long loop;
    int finished = 0;

    if (callback(frame, data)) return 1;

    frame->GetChildren(&children);
    count = children->GetSize();
    for (loop = 0; loop < count; loop++) {
        children->GetElement(loop, &frame1);
        frame1->QueryInterface(IID_IDirect3DRMFrame3, (void**)&child);
        frame1->Release();
        if (Container_Frame_WalkTree(child, level + 1, callback, data)) {
            finished = 1;
            child->Release();
            break;
        }
        child->Release();
    }
    children->Release();
    return finished;
}

// FUNCTION: LEGORR 0x00476410
int Container_Frame_SearchCallback(LPDIRECT3DRMFRAME3 frame, void* data)
{
    Container_SearchData* search = (Container_SearchData*)data;
    char* name;
    DWORD len;
    unsigned long loop;

    frame->GetName(&len, NULL);
    if (len - 1 == search->stringLen) {
        name = (char*)malloc(len);
        name[0] = '\0';
        frame->GetName(&len, name);
        for (loop = 0; loop < len; loop++) {
            if (search->string[loop] == '?') name[loop] = '?';
        }

        if (search->mode == Container_SearchMode_FirstMatch) {
            search->resultFrame = NULL;
            if (search->caseSensitive) {
                if (strcmp(name, search->string) == 0) search->resultFrame = frame;
            } else {
                if (stricmp(name, search->string) == 0) search->resultFrame = frame;
            }
        } else if (search->mode == Container_SearchMode_MatchCount) {
            if (search->caseSensitive) {
                if (strcmp(name, search->string) == 0) search->count++;
            } else {
                if (stricmp(name, search->string) == 0) search->count++;
            }
        } else if (search->mode == Container_SearchMode_NthMatch) {
            search->resultFrame = NULL;
            if (search->caseSensitive) {
                if (strcmp(name, search->string) == 0) search->count++;
            } else {
                if (stricmp(name, search->string) == 0) search->count++;
            }
            if (search->count == search->matchNumber + 1) {
                search->resultFrame = frame;
            }
        }

        free(name);
        if (search->resultFrame) return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x004765c0
struct AnimClone* Container_LoadAnimSet(const char* fname,
                                        LPDIRECT3DRMFRAME3 frame,
                                        unsigned long* frameCount, int lws,
                                        int looping)
{
    LPDIRECT3DRMANIMATIONSET2 animSet = NULL;
    LPDIRECT3DRMFRAME3 animFrame;
    struct AnimClone* animClone = NULL;
    D3DRMLOADMEMORY buffer;
    Container_TextureData textureData;

    if (lws) {
        LwsInfo* scene;
        char path[260];
        strcpy(path, fname);
        path[strlen(path) - 2] = '\0';
        if ((scene = lws_parse(path, looping)) != NULL) {
            lws_meshes_load(scene, (struct LwsFrame*)frame);
            lws_time_set(scene, 0.0f);
            if (frameCount) *frameCount = resource_entry_id_get(scene);
            animClone = anim_clone_register_lws(
                scene, frame, *frameCount);
        }
    } else {
        if ((buffer.lpMemory = file_load_binary(fname, &buffer.dSize)) != NULL) {
            if (lpD3DRM()->CreateFrame(frame,
                                       &animFrame) == D3DRM_OK) {
                if (lpD3DRM()->CreateAnimationSet(&animSet) == D3DRM_OK) {
                    textureData.xFileName = fname;
                    textureData.flags = 0;
                    if (animSet->Load(&buffer, NULL,
                                      D3DRMLOAD_FROMMEMORY,
                                      Container_TextureLoadCallback,
                                      &textureData, animFrame) == D3DRM_OK) {
                        unsigned long count;
                        animSet->SetTime(0.0f);
                        count = Container_GetAnimFileFrameCount(
                            (const char*)buffer.lpMemory);
                        if (frameCount) *frameCount = count;
                        animClone = anim_clone_register(
                            animSet,
                            animFrame, count);
                    }
                    free(buffer.lpMemory);
                }
                animFrame->Release();
            }
        }
    }
    return animClone;
}

// FUNCTION: LEGORR 0x00476770
unsigned long Container_GetAnimFileFrameCount(const char* fileData)
{
    unsigned long animCount = 0;
    const char* s = strstr(fileData,
                           // STRING: LEGORR 0x004aab98
                           "DDiScene");

    if (s) {
        while (!isdigit((unsigned char)*s)) s++;
        animCount = atoi(s);
    }
    if (animCount == 0) animCount = 1;
    return animCount;
}

// FUNCTION: LEGORR 0x004767e0
int Container_FrameLoad(const char* fname, LPDIRECT3DRMFRAME3 frame)
{
    D3DRMLOADMEMORY buffer;
    Container_TextureData textureData;
    int result = 0;

    if ((buffer.lpMemory = file_load_binary(fname, &buffer.dSize)) != NULL) {
        textureData.xFileName = fname;
        textureData.flags = 0;
        if (frame->Load(&buffer, NULL, D3DRMLOAD_FROMMEMORY,
                        Container_TextureLoadCallback,
                        &textureData) == D3DRM_OK) {
            result = 1;
        }
        free(buffer.lpMemory);
    }
    return result;
}

// FUNCTION: LEGORR 0x00476850
LPDIRECT3DRMMESH Container_MeshLoad(void* fileData, unsigned long fileSize,
                                    const char* fname, LPDIRECT3DRMFRAME3 frame,
                                    int noTextures)
{
    D3DRMLOADMEMORY buffer;
    Container_TextureData textureData;
    LPDIRECT3DRMMESHBUILDER3 mb;
    LPDIRECT3DRMMESH mesh;

    buffer.lpMemory = fileData;
    buffer.dSize = fileSize;
    if (lpD3DRM()->CreateMeshBuilder(&mb) == D3DRM_OK) {
        textureData.xFileName = fname;
        textureData.flags = 0;
        if (noTextures) textureData.flags = CONTAINER_TEXTURE_NOLOAD;
        if (mb->Load(&buffer, NULL, D3DRMLOAD_FROMMEMORY,
                     Container_TextureLoadCallback,
                     &textureData) == D3DRM_OK) {
            mb->CreateMesh(&mesh);
            mb->Release();
            if (frame->AddVisual(mesh) == D3DRM_OK) {
                return mesh;
            }
        }
        mb->Release();
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00476900
static HRESULT Container_TextureLoadCallback(char* name, void* data,
                                             LPDIRECT3DRMTEXTURE3* texture)
{
    Container_TextureData* textureData = (Container_TextureData*)data;
    const char* s;
    char* t;
    char* lastSlash;
    int pass;
    unsigned long loop;
    Container_Texture* text;
    char path[1024];

    if (!(textureData->flags & CONTAINER_TEXTURE_NOLOAD)) {
        lastSlash = NULL;
        s = textureData->xFileName;
        t = path;
        for (pass = 0; pass < (containerGlobs.sharedDir ? 2 : 1); pass++) {
            if (pass == 0) {
                for (; *s != '\0'; s++, t++) {
                    if (*s == '\\') {
                        if (lastSlash) *lastSlash = *s;
                        *t = '\0';
                        lastSlash = t;
                    } else {
                        *t = *s;
                    }
                }
                if (lastSlash) {
                    strcat(path, "\\");
                    strcat(path, name);
                } else {
                    strcpy(path, name);
                }
            } else {
                sprintf(path, "%s\\%s", containerGlobs.sharedDir, name);
            }

            *texture = NULL;
            for (loop = 0; loop < containerGlobs.textureCount; loop++) {
                if (containerGlobs.textureSet[loop].fileName) {
                    if (strcmp(containerGlobs.textureSet[loop].fileName,
                               path) == 0) {
                        *texture = containerGlobs.textureSet[loop].texture;
                        (*texture)->AddRef();
                        return D3DRM_OK;
                    }
                }
            }

            if ((text = Container_LoadTexture2(path, 0, NULL, NULL)) != NULL) {
                *texture = text->texture;
                Container_YFlipTexture(*texture);
                if (s_engine_feature_word & CNT_MAIN_FLAG_MIPMAPENABLED) {
                    (*texture)->GenerateMIPMap(0);
                }
                if (containerGlobs.textureCount == CONTAINER_MAXTEXTURES) {
                    qsort(containerGlobs.textureSet, containerGlobs.textureCount,
                          sizeof(Container_TextureRef), Container_TextureSetSort);
                    while (1) {
                        if (containerGlobs.textureSet[containerGlobs.textureCount - 1]
                                .fileName) break;
                        containerGlobs.textureCount--;
                    }
                }
                if (containerGlobs.textureCount < CONTAINER_MAXTEXTURES) {
                    Container_TextureRef* ref =
                        &containerGlobs.textureSet[containerGlobs.textureCount];
                    (*texture)->AddDestroyCallback(Container_TextureDestroyCallback, ref);
                    ref->fileName = (char*)malloc(strlen(path) + 1);
                    strcpy(ref->fileName, path);
                    ref->texture = *texture;
                    containerGlobs.textureCount++;
                }
                return D3DRM_OK;
            }
        }
    }
    return D3DRMERR_NOTFOUND;
}

// FUNCTION: LEGORR 0x00476bf0
void Container_YFlipTexture(LPDIRECT3DRMTEXTURE3 texture)
{
    D3DRMIMAGE* image = texture->GetImage();
    unsigned long width;
    unsigned char* buffer;
    unsigned char* top;
    unsigned char* bottom;
    int loop;

    if (image) {
        width = (image->width * image->depth) / 8;
        if ((buffer = (unsigned char*)malloc(width)) != NULL) {
            top = (unsigned char*)image->buffer1;
            bottom = (unsigned char*)image->buffer1 +
                     (image->height - 1) * image->bytes_per_line;
            for (loop = 0; loop < image->height / 2; loop++) {
                memcpy(buffer, bottom, width);
                memcpy(bottom, top, width);
                memcpy(top, buffer, width);
                top += image->bytes_per_line;
                bottom -= image->bytes_per_line;
            }
            free(buffer);
        }
    }
}

// FUNCTION: LEGORR 0x00476ce0
static int Container_TextureSetSort(const void* a, const void* b)
{
    const Container_TextureRef* refA = (const Container_TextureRef*)a;
    const Container_TextureRef* refB = (const Container_TextureRef*)b;

    if (refA->fileName && !refB->fileName) return -1;
    else if (!refA->fileName && refB->fileName) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x00476d10
static void Container_TextureDestroyCallback(LPDIRECT3DRMOBJECT obj,
                                             void* arg)
{
    Container_TextureRef* ref = (Container_TextureRef*)arg;
    (void)obj;
    free(ref->texture->GetAppData());
    free(ref->fileName);
    ref->fileName = NULL;
    ref->texture = NULL;
}
