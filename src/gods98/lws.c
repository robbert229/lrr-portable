// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-020, Lws.c (text 004869f0-00488b50; .rdata
 * 0049ea48-0049ea58; literals 004ab5f8-004ab6c8; .bss 00540838-00544864;
 * finding 505): the LightWave scene loader.
 *
 * Lws_Parse reads an "LWSC" version 1 .lws file into a node list: each
 * LoadObject or AddNullObject line starts a node, and the lines after it
 * give the node's motion keys, dissolve keys, parent, pivot and flags. Null
 * objects named "SFX,<sample>,<frames>..." become sound triggers.
 * Lws_LoadMeshes and Lws_Clone build a Direct3DRM frame per node and attach
 * its mesh (cached by path, with a shared directory as fallback), and
 * Lws_SetTime interpolates the keys, the dissolve levels, the mesh texture
 * time and the triggers.
 *
 * The native port never calls this code: it loads scenes through
 * gods98/lwsc_scene.cpp. Its own earlier models of these functions, which
 * port code still calls, are in the port tail, gods98/lws_port.cpp,
 * unannotated. lws.h lists which functions keep a port name and contract.
 *
 * C-compatible (ADR 0005): C in the matching build, C++ elsewhere. The
 * frames, user visuals and IDirect3DRM3 are called through their COM
 * method tables, declared here with the slots the unit uses. */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#define LEGORRR_LWS_UNIT 1
#include "lws.h"
#include "utils.h"
#include "files_open.h"
#include "mesh_unit.h"
#include "main.h"
#include "sound3d.h"
#include "crt/crt.h"

#if defined(_WIN32)
#define LWS_STDCALL __stdcall
#else
#define LWS_STDCALL
#endif

#define LWS_MAXLINELEN 1024
#define LWS_MAXARGS 20
#define LWS_MAXTRIGARGS 256
#define LWS_NODELISTBLOCKSIZE 10
#define LWS_MAXMESHFILES 1000
#define LWS_MAXSTATICDISSOLVES 100
#define LWS_MAXTRIGKEYS 25

#define LWSNODE_FLAG_NULL 0x01
#define LWSNODE_FLAG_SOUNDTRIGGER 0x02
#define LWSNODE_FLAG_FACECAMERA 0x04

#define LWS_FLAG_LOOPING 0x00000001

/* Main.c's MAIN_FLAG_REDUCESAMPLES? The flag that keeps a trigger whose
 * sample was not found from playing. */
#define LWS_MAINFLAG_SAMPLESOFF 0x00100000

/* Mesh.c's MESH_FLAG_FACECAMERA, in the mesh's flags at +0x2c. */
#define LWS_MESH_FLAG_FACECAMERA 0x01000000

/* Mesh.c's colour type for Mesh_SetGroupMaterialValues: the alpha. */
#define LWS_MESH_COLOUR_ALPHA 4

typedef struct LwsIID {
    unsigned long data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} LwsIID;

typedef struct LwsVector3 {
    float x;
    float y;
    float z;
} LwsVector3;

typedef struct LwsFrame LwsFrame;            /* IDirect3DRMFrame3 */
typedef struct LwsUnknown LwsUnknown;        /* IUnknown */
typedef struct LwsUserVisual LwsUserVisual;  /* IDirect3DRMUserVisual */
typedef struct LwsD3DRM LwsD3DRM;            /* IDirect3DRM3 */

typedef struct LwsFrameVtbl {
    long (LWS_STDCALL* QueryInterface)(LwsFrame*, const LwsIID*, void**);
    unsigned long (LWS_STDCALL* AddRef)(LwsFrame*);
    unsigned long (LWS_STDCALL* Release)(LwsFrame*);        /* +0x08 */
    void* slots_0c[5];
    long (LWS_STDCALL* SetName)(LwsFrame*, const char*);     /* +0x20 */
    void* slots_24[5];
    long (LWS_STDCALL* AddTransform)(LwsFrame*, int,
                                     float (*)[4]);          /* +0x38 */
    void* slots_3c[8];
    long (LWS_STDCALL* GetParent)(LwsFrame*, LwsFrame**);    /* +0x5c */
    void* slots_60[5];
    long (LWS_STDCALL* GetTransform)(LwsFrame*, LwsFrame*,
                                     float (*)[4]);          /* +0x74 */
    void* slots_78[2];
    long (LWS_STDCALL* GetVisuals)(LwsFrame*, unsigned long*,
                                   LwsUnknown**);            /* +0x80 */
} LwsFrameVtbl;

struct LwsFrame {
    const LwsFrameVtbl* lpVtbl;
};

typedef struct LwsUnknownVtbl {
    long (LWS_STDCALL* QueryInterface)(LwsUnknown*, const LwsIID*, void**);
    unsigned long (LWS_STDCALL* AddRef)(LwsUnknown*);
    unsigned long (LWS_STDCALL* Release)(LwsUnknown*);      /* +0x08 */
} LwsUnknownVtbl;

struct LwsUnknown {
    const LwsUnknownVtbl* lpVtbl;
};

typedef struct LwsUserVisualVtbl {
    long (LWS_STDCALL* QueryInterface)(LwsUserVisual*, const LwsIID*,
                                       void**);
    unsigned long (LWS_STDCALL* AddRef)(LwsUserVisual*);
    unsigned long (LWS_STDCALL* Release)(LwsUserVisual*);   /* +0x08 */
    void* slots_0c[4];
    unsigned long (LWS_STDCALL* GetAppData)(LwsUserVisual*); /* +0x1c */
} LwsUserVisualVtbl;

struct LwsUserVisual {
    const LwsUserVisualVtbl* lpVtbl;
};

typedef struct LwsD3DRMVtbl {
    long (LWS_STDCALL* QueryInterface)(LwsD3DRM*, const LwsIID*, void**);
    unsigned long (LWS_STDCALL* AddRef)(LwsD3DRM*);
    unsigned long (LWS_STDCALL* Release)(LwsD3DRM*);
    void* slots_0c[1];
    long (LWS_STDCALL* CreateFrame)(LwsD3DRM*, LwsFrame*,
                                    LwsFrame**);             /* +0x10 */
} LwsD3DRMVtbl;

struct LwsD3DRM {
    const LwsD3DRMVtbl* lpVtbl;
};

/* D3DRMCOMBINE_REPLACE. */
#define LWS_COMBINE_REPLACE 0

typedef struct LwsSoundTrigger {
    unsigned long sfxID;                                 /* +0x00 */
    unsigned short frameStartList[LWS_MAXTRIGKEYS];      /* +0x04 */
    unsigned short frameEndList[LWS_MAXTRIGKEYS];        /* +0x36 */
    unsigned long loopUID[LWS_MAXTRIGKEYS];              /* +0x68 */
    unsigned short count;                                /* +0xcc */
} LwsSoundTrigger;

typedef struct LwsKeyInfo {
    LwsVector3 position;                                 /* +0x00 */
    LwsVector3 hpb;                                      /* +0x0c */
    LwsVector3 scale;                                    /* +0x18 */
    unsigned short frame;                                /* +0x24 */
} LwsKeyInfo;

struct LwsNode {
    char* name;                                          /* +0x00 */
    unsigned short reference;                            /* +0x04 */
    unsigned short frameIndex;                           /* +0x06 */
    unsigned char triggerIndex;                          /* +0x08 */
    LwsVector3 pivotVector;                              /* +0x0c */
    LwsKeyInfo* keyList;                                 /* +0x18 */
    float* dissolveLevel;                                /* +0x1c */
    unsigned short* dissolveFrame;                       /* +0x20 */
    unsigned short keyCount;                             /* +0x24 */
    unsigned short dissolveCount;                        /* +0x26 */
    unsigned char flags;                                 /* +0x28 */
    struct LwsNode* childList;                           /* +0x2c */
    struct LwsNode* next;                                /* +0x30 */
};

struct LwsInfo {
    unsigned short firstFrame;                           /* +0x00 */
    unsigned short lastFrame;                            /* +0x02 */
    float fps;                                           /* +0x04 */
    char* filePath;                                      /* +0x08 */
    float lastTime;                                      /* +0x0c */
    float time;                                          /* +0x10 */
    LwsFrame** frameList;                                /* +0x14 */
    LwsSoundTrigger* triggerList;                        /* +0x18 */
    LwsNode* masterNode;                                 /* +0x1c */
    LwsNode* nodeList;                                   /* +0x20 */
    unsigned short nodeCount;                            /* +0x24 */
    unsigned short nodeListSize;                         /* +0x26 */
    unsigned short triggerCount;                         /* +0x28 */
    struct LwsInfo* clonedFrom;                          /* +0x2c */
    unsigned long referenceCount;                        /* +0x30 */
    unsigned char flags;                                 /* +0x34 */
};

typedef struct LwsMeshPath {
    char* path;
    void* mesh;
} LwsMeshPath;

typedef struct LwsGlobs {
    LwsMeshPath meshPathList[LWS_MAXMESHFILES];          /* 00540838 */
    LwsMeshPath meshPathListShared[LWS_MAXMESHFILES];    /* 00542778 */
    char* sharedDir;                                     /* 005446b8 */
    unsigned int meshPathCount;                          /* 005446bc */
    unsigned int meshPathCountShared;                    /* 005446c0 */
    float staticDissolveLevel[LWS_MAXSTATICDISSOLVES];   /* 005446c4 */
    unsigned long staticDissolveCount;                   /* 00544854 */
    LwsFindSFXIDFunc FindSFXIDFunc;                      /* 00544858 */
    LwsSoundEnabledFunc SoundEnabledFunc;                /* 0054485c */
    LwsPlaySample3DFunc PlaySample3DFunc;                /* 00544860 */
} LwsGlobs;


/* The functions lws.h declares with the port's contracts, with the
 * original's argument types. */
#ifdef __cplusplus
extern "C" {
#endif
unsigned long resource_entry_id_get(LwsInfo* scene);
float animation_track_find_keyframe_and_fraction(LwsNode* node, float time,
                                                 unsigned short* prev);
int shared_object_sample_time_matches(LwsInfo* scene, unsigned long key);
float keyframe_time_segment_fraction(LwsNode* node, float time,
                                     unsigned short* prev);
void* pair_table_find(LwsMeshPath* list, unsigned long count,
                      const char* path);
void pair_table_append(LwsMeshPath* list, unsigned int* count,
                       const char* path, void* mesh);
#ifdef __cplusplus
}
#endif

/* IID_IDirect3DRMUserVisual. The two units that use it, this one and
 * G98-025 (animclone.c), share the copy at 0049fb48, so it has external
 * linkage. */
#ifdef __cplusplus
extern "C" const LwsIID s_iid_user_visual;
#endif
// GLOBAL: LEGORR 0x0049fb48
const LwsIID s_iid_user_visual = {
    0x59163de0, 0x6d43, 0x11cf,
    { 0xac, 0x4a, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1 }
};

/* The degree-to-radian factors. Lws_Parse multiplies by each in turn; as
 * literals MSVC folds them into one constant. */
// GLOBAL: LEGORR 0x0049ea4c
static const float s_lws_pi = 3.14159265358979323846f;
// GLOBAL: LEGORR 0x0049ea48
static const float s_lws_one_over_180 = 1.0f / 180.0f;

// GLOBAL: LEGORR 0x00540838
static LwsGlobs lwsGlobs;

// FUNCTION: LEGORR 0x004869f0
LwsInfo* lws_parse(const char* fname, int looping)
{
    LwsInfo* scene = NULL;
    char path[260];
    struct GameFileHandle* file;

    sprintf(path, "%s.%s", fname,
            // STRING: LEGORR 0x004ab6c4
            "lws");
    if ((file = game_file_open_dispatch(path, "r")) != NULL) {
        char line[LWS_MAXLINELEN];
        char lineSplit[LWS_MAXLINELEN];
        unsigned short len;

        game_file_read_crlf_line(line, sizeof(line), file);
        len = (unsigned short)(strlen(line) + 1);
        game_file_read_crlf_line(&line[len], sizeof(line) - len, file);
        // STRING: LEGORR 0x004ab6bc
        if (0 == strcmp(line, "LWSC")) {
            // STRING: LEGORR 0x004ab6b8
            if (0 == strcmp(&line[len], "1")) {
                LwsNode* node = NULL;
                char* argv[LWS_MAXARGS];
                unsigned short* parentList;
                long keyLine = 0;
                long dissolveLine = 0;
                long loop;
                char* s;
                char* t;

                for (s = t = path; '\0' != *s; s++) {
                    if ('\\' == *s) t = s;
                }
                t[t != path] = '\0';

                scene = (LwsInfo*)malloc(sizeof(LwsInfo));
                memset(scene, 0, sizeof(LwsInfo));
                if ('\0' != path[0]) scene->filePath = string_duplicate(path);
                else scene->filePath = NULL;
                scene->nodeListSize = LWS_NODELISTBLOCKSIZE;
                scene->referenceCount = 1;
                if (looping) scene->flags |= LWS_FLAG_LOOPING;
                scene->nodeList = (LwsNode*)malloc(sizeof(LwsNode) * LWS_NODELISTBLOCKSIZE);
                parentList = (unsigned short*)malloc(sizeof(unsigned short) * scene->nodeListSize);

                while (game_file_read_crlf_line(line, sizeof(line), file)) {
                    strcpy(lineSplit, line);
                    if (whitespace_tokenize(lineSplit, argv)) {
                        if (NULL == node) {
                            // STRING: LEGORR 0x004ab6ac
                            if (0 == strcmp("FirstFrame", argv[0])) scene->firstFrame = (unsigned short)atoi(argv[1]);
                            // STRING: LEGORR 0x004ab6a0
                            if (0 == strcmp("LastFrame", argv[0])) scene->lastFrame = (unsigned short)atoi(argv[1]);
                            if (0 == scene->lastFrame) scene->lastFrame = 1;
                            // STRING: LEGORR 0x004ab690
                            if (0 == strcmp("FramesPerSecond", argv[0])) scene->fps = (float)atof(argv[1]);
                            // STRING: LEGORR 0x004ab680
                            if (0 == strcmp("AddNullObject", argv[0]) ||
                                // STRING: LEGORR 0x004ab674
                                0 == strcmp("LoadObject", argv[0])) {

                                if (scene->nodeCount == scene->nodeListSize) {
                                    scene->nodeListSize += LWS_NODELISTBLOCKSIZE;
                                    scene->nodeList = (LwsNode*)realloc(scene->nodeList, sizeof(LwsNode) * scene->nodeListSize);
                                    parentList = (unsigned short*)realloc(parentList, sizeof(unsigned short) * scene->nodeListSize);
                                }
                                parentList[scene->nodeCount] = 0;
                                node = &scene->nodeList[scene->nodeCount];
                                memset(node, 0, sizeof(LwsNode));

                                if (0 == strcmp("AddNullObject", argv[0])) {
                                    node->flags |= LWSNODE_FLAG_NULL;
                                    node->name = string_duplicate(&line[14]);
                                    // STRING: LEGORR 0x004ab670
                                    if (0 == _strnicmp(node->name, "SFX", 3) &&
                                        0 == _strnicmp(&node->name[3], ",", 1)) {
                                        node->flags |= LWSNODE_FLAG_SOUNDTRIGGER;
                                        scene->triggerCount++;
                                    }
                                } else {
                                    int argc;
                                    strcpy(lineSplit, line);
                                    argc = sound_variant_split(line, argv, "\\");
                                    // STRING: LEGORR 0x004ab668
                                    if ((s = (char*)case_insensitive_substring_find(argv[argc - 1], ".lwo")) != NULL) *s = '\0';
                                    node->name = string_duplicate(argv[argc - 1]);
                                }

                                for (loop = scene->nodeCount - 1; loop >= 0; loop--) {
                                    if (((scene->nodeList[loop].flags & LWSNODE_FLAG_NULL) && (node->flags & LWSNODE_FLAG_NULL)) ||
                                        (!(scene->nodeList[loop].flags & LWSNODE_FLAG_NULL) && !(node->flags & LWSNODE_FLAG_NULL))) {
                                        if (0 == strcmp(node->name, scene->nodeList[loop].name)) {
                                            node->reference = scene->nodeList[loop].reference + 1;
                                            break;
                                        }
                                    }
                                }
                            }
                        } else if (keyLine) {
                            if (1 != keyLine) {
                                if (2 == keyLine) {
                                    node->keyCount = (unsigned short)atoi(argv[1]);
                                    node->keyList = (LwsKeyInfo*)malloc(sizeof(LwsKeyInfo) * (node->keyCount + 1));
                                } else {
                                    unsigned short index = (unsigned short)((keyLine - 3) / 2);
                                    if (index < node->keyCount) {
                                        if (keyLine % 2) {
                                            node->keyList[index].position.x = (float)atof(argv[1]);
                                            node->keyList[index].position.y = (float)atof(argv[2]);
                                            node->keyList[index].position.z = (float)atof(argv[3]);
                                            node->keyList[index].hpb.x = (float)atof(argv[4]) * s_lws_one_over_180 * s_lws_pi;
                                            node->keyList[index].hpb.y = (float)atof(argv[5]) * s_lws_one_over_180 * s_lws_pi;
                                            node->keyList[index].hpb.z = (float)atof(argv[6]) * s_lws_one_over_180 * s_lws_pi;
                                            node->keyList[index].scale.x = (float)atof(argv[7]);
                                            node->keyList[index].scale.y = (float)atof(argv[8]);
                                            node->keyList[index].scale.z = (float)atof(argv[9]);
                                        } else {
                                            node->keyList[index].frame = (unsigned short)atoi(argv[1]);
                                        }
                                    }
                                }
                            }
                            keyLine++;
                            // STRING: LEGORR 0x004ab65c
                            if (0 == strcmp(argv[0], "EndBehavior")) keyLine = 0;
                        } else if (dissolveLine) {
                            if (1 != dissolveLine) {
                                if (2 == dissolveLine) {
                                    node->dissolveCount = (unsigned short)atoi(argv[1]);
                                    node->dissolveLevel = (float*)malloc(sizeof(float) * (node->dissolveCount + 1));
                                    node->dissolveFrame = (unsigned short*)malloc(sizeof(unsigned short) * (node->dissolveCount + 1));
                                } else {
                                    unsigned short index = (unsigned short)((dissolveLine - 3) / 2);
                                    if (index < node->dissolveCount) {
                                        if (dissolveLine % 2) {
                                            node->dissolveLevel[index] = (float)atof(argv[1]);
                                            if (node->dissolveLevel[index] < 0.0f) node->dissolveLevel[index] = 0.0f;
                                            if (node->dissolveLevel[index] > 1.0f) node->dissolveLevel[index] = 1.0f;
                                            node->dissolveLevel[index] = 1.0f - node->dissolveLevel[index];
                                        } else {
                                            node->dissolveFrame[index] = (unsigned short)atoi(argv[1]);
                                        }
                                    }
                                }
                            }
                            dissolveLine++;
                            if (0 == strcmp(argv[0], "EndBehavior")) dissolveLine = 0;
                        // STRING: LEGORR 0x004ab64c
                        } else if (0 == strcmp(argv[0], "ObjectMotion")) {
                            keyLine = 1;
                        // STRING: LEGORR 0x004ab640
                        } else if (0 == strcmp(argv[0], "ObjDissolve")) {
                            // STRING: LEGORR 0x004ab634
                            if (0 == strcmp(argv[1], "(envelope)")) {
                                dissolveLine = 1;
                            } else {
                                lwsGlobs.staticDissolveLevel[lwsGlobs.staticDissolveCount] = (float)atof(argv[1]);
                                node->dissolveLevel = &lwsGlobs.staticDissolveLevel[lwsGlobs.staticDissolveCount];
                                lwsGlobs.staticDissolveCount++;
                                node->dissolveLevel = NULL;
                            }
                        // STRING: LEGORR 0x004ab624
                        } else if (0 == strcmp(argv[0], "ParentObject")) {
                            parentList[scene->nodeCount] = (unsigned short)atoi(argv[1]);
                        // STRING: LEGORR 0x004ab618
                        } else if (0 == strcmp(argv[0], "PivotPoint")) {
                            node->pivotVector.x = -(float)atof(argv[1]);
                            node->pivotVector.y = -(float)atof(argv[2]);
                            node->pivotVector.z = -(float)atof(argv[3]);
                        // STRING: LEGORR 0x004ab608
                        } else if (0 == strcmp(argv[0], "UnseenByCamera") && 1 == atoi(argv[1])) {
                            node->flags |= LWSNODE_FLAG_FACECAMERA;
                        // STRING: LEGORR 0x004ab5f8
                        } else if (0 == strcmp(argv[0], "ShadowOptions")) {
                            node = NULL;
                            scene->nodeCount++;
                        }
                    }
                }

                for (loop = 0; loop < scene->nodeCount; loop++) {
                    node = &scene->nodeList[loop];
                    if (1 != node->keyCount) {
                        if (node->keyList[node->keyCount - 1].frame < scene->lastFrame) {
                            node->keyList[node->keyCount].frame = scene->lastFrame;
                            node->keyList[node->keyCount].position = node->keyList[node->keyCount - 1].position;
                            node->keyList[node->keyCount].hpb = node->keyList[node->keyCount - 1].hpb;
                            node->keyList[node->keyCount].scale = node->keyList[node->keyCount - 1].scale;
                            node->keyCount++;
                        }
                    }
                }

                for (loop = 0; loop < scene->nodeCount; loop++) {
                    node = &scene->nodeList[loop];
                    if (node->dissolveCount) {
                        if (node->dissolveFrame[node->dissolveCount - 1] < scene->lastFrame) {
                            node->dissolveFrame[node->dissolveCount] = scene->lastFrame;
                            node->dissolveLevel[node->dissolveCount] = node->dissolveLevel[node->dissolveCount - 1];
                            node->dissolveCount++;
                        }
                    }
                }

                for (loop = scene->nodeCount - 1; loop >= 0; loop--) {
                    node = &scene->nodeList[loop];
                    if (parentList[loop]) {
                        LwsNode* parent = &scene->nodeList[parentList[loop] - 1];
                        node->next = parent->childList;
                        parent->childList = node;
                    } else {
                        node->next = scene->masterNode;
                        scene->masterNode = node;
                    }
                }

                free(parentList);
                lws_sound_triggers_setup(scene);
            }
        }
        game_file_close(file);
    }

    return scene;
}

// FUNCTION: LEGORR 0x004876c0
void lws_initialise(const char* sharedDir, LwsFindSFXIDFunc FindSFXIDFunc,
                    LwsPlaySample3DFunc PlaySample3DFunc,
                    LwsSoundEnabledFunc SoundEnabledFunc)
{
    if (sharedDir) {
        unsigned short len = (unsigned short)strlen(sharedDir);
        if ('\\' != sharedDir[len - 1]) len++;
        lwsGlobs.sharedDir = (char*)malloc(len + 1);
        strcpy(lwsGlobs.sharedDir, sharedDir);
        lwsGlobs.sharedDir[len - 1] = '\\';
        lwsGlobs.sharedDir[len] = '\0';
    }

    lwsGlobs.FindSFXIDFunc = FindSFXIDFunc;
    lwsGlobs.PlaySample3DFunc = PlaySample3DFunc;
    lwsGlobs.SoundEnabledFunc = SoundEnabledFunc;
}

// FUNCTION: LEGORR 0x00487760
void lws_shutdown(void)
{
    unsigned short loop;

    for (loop = 0; loop < lwsGlobs.meshPathCount; loop++) free(lwsGlobs.meshPathList[loop].path);
    for (loop = 0; loop < lwsGlobs.meshPathCountShared; loop++) free(lwsGlobs.meshPathListShared[loop].path);
}

// FUNCTION: LEGORR 0x004877d0
unsigned long resource_entry_id_get(LwsInfo* scene)
{
    return scene->lastFrame;
}

// FUNCTION: LEGORR 0x004877e0
void lws_sound_triggers_setup(LwsInfo* scene)
{
    if (lwsGlobs.FindSFXIDFunc) {
        unsigned char triggerIndex = 0;
        if (scene->triggerCount) {
            unsigned short loop;
            scene->triggerList = (LwsSoundTrigger*)malloc(sizeof(LwsSoundTrigger) * scene->triggerCount);
            for (loop = 0; loop < scene->nodeCount; loop++) {
                LwsNode* node = &scene->nodeList[loop];
                if (node->flags & LWSNODE_FLAG_SOUNDTRIGGER) {
                    LwsSoundTrigger* trigger = &scene->triggerList[triggerIndex];
                    char string[LWS_MAXLINELEN];
                    char* argv[LWS_MAXTRIGARGS];
                    unsigned long sub;
                    int found;
                    unsigned long argc;

                    node->triggerIndex = triggerIndex++;
                    strcpy(string, node->name);
                    argc = sound_variant_split(string, argv, ",");
                    trigger->count = (unsigned short)(argc - 2);
                    found = lwsGlobs.FindSFXIDFunc(argv[1], &trigger->sfxID);
                    if ((s_engine_feature_word & LWS_MAINFLAG_SAMPLESOFF) && !found) trigger->sfxID = 0;
                    for (sub = 0; sub < trigger->count; sub++) {
                        // STRING: LEGORR 0x004ab6c8
                        char* s = strstr(argv[2 + sub], "-");
                        trigger->frameStartList[sub] = (unsigned short)atoi(argv[2 + sub]);
                        if (s) trigger->frameEndList[sub] = (unsigned short)atoi(s + 1);
                        else trigger->frameEndList[sub] = trigger->frameStartList[sub];
                    }
                }
            }
        }
    } else {
        scene->triggerCount = 0;
    }
}

// FUNCTION: LEGORR 0x00487990
void lws_meshes_load(LwsInfo* scene, LwsFrame* parent)
{
    unsigned short frameCount = 0;
    LwsNode* node;

    scene->frameList = (LwsFrame**)malloc(sizeof(LwsFrame*) * scene->nodeCount);
    for (node = scene->masterNode; node; node = node->next) {
        lws_frames_create(scene, node, parent, &frameCount);
        lws_nodes_load(scene, node);
    }
    lws_time_set(scene, 0.0f);
}

// FUNCTION: LEGORR 0x00487a00
LwsInfo* lws_clone(LwsInfo* scene, LwsFrame* parent)
{
    LwsInfo* newScene = (LwsInfo*)malloc(sizeof(LwsInfo));
    unsigned short frameCount = 0;
    unsigned short loop;
    LwsNode* node;
    LwsFrame* parentFrame;

    if (scene->clonedFrom) scene = scene->clonedFrom;

    memset(newScene, 0, sizeof(LwsInfo));
    memcpy(newScene, scene, sizeof(LwsInfo));
    newScene->referenceCount = 1;
    newScene->frameList = (LwsFrame**)malloc(sizeof(LwsFrame*) * newScene->nodeCount);
    for (node = newScene->masterNode; node; node = node->next) {
        lws_frames_create(newScene, node, parent, &frameCount);
    }

    for (loop = 0; loop < newScene->nodeCount; loop++) {
        float mat[4][4];
        node = &newScene->nodeList[loop];

        scene->frameList[loop]->lpVtbl->GetParent(scene->frameList[loop], &parentFrame);
        scene->frameList[loop]->lpVtbl->GetTransform(scene->frameList[loop], parentFrame, mat);
        parentFrame->lpVtbl->Release(parentFrame);
        newScene->frameList[loop]->lpVtbl->AddTransform(newScene->frameList[loop], LWS_COMBINE_REPLACE, mat);

        if (!(node->flags & LWSNODE_FLAG_NULL)) {
            LwsFrame* frame = scene->frameList[node->frameIndex];
            LwsUnknown* iunknown;
            LwsUserVisual* userVisual;
            unsigned long count;
            void* mesh;

            frame->lpVtbl->GetVisuals(frame, &count, NULL);
            frame->lpVtbl->GetVisuals(frame, &count, &iunknown);
            iunknown->lpVtbl->QueryInterface(iunknown, &s_iid_user_visual, (void**)&userVisual);
            iunknown->lpVtbl->Release(iunknown);
            mesh = (void*)userVisual->lpVtbl->GetAppData(userVisual);
            userVisual->lpVtbl->Release(userVisual);

            mesh_clone(mesh, newScene->frameList[node->frameIndex]);
        }
    }

    newScene->clonedFrom = scene;
    scene->referenceCount++;

    lws_time_set(newScene, 0.0f);

    return newScene;
}

// FUNCTION: LEGORR 0x00487ba0
void lws_time_set(LwsInfo* scene, float time)
{
    unsigned short loop;
    unsigned short prev;
    float delta;

    scene->lastTime = scene->time;
    if (scene->flags & LWS_FLAG_LOOPING) {
        scene->time = (float)fmod(time, scene->lastFrame);
    } else {
        if (time > scene->lastFrame) scene->time = scene->lastFrame;
        else scene->time = time;
    }

    for (loop = 0; loop < scene->nodeCount; loop++) {
        LwsNode* node = &scene->nodeList[loop];
        if (node->keyCount > 1) {
            delta = animation_track_find_keyframe_and_fraction(node, scene->time, &prev);
            lws_interpolate(scene, node, prev, delta);
        }
        if (node->dissolveCount > 1) {
            delta = keyframe_time_segment_fraction(node, scene->time, &prev);
            lws_dissolve_interpolate(scene, node, prev, delta);
        }
        lws_textures_animate(scene, node);
        lws_trigger_handle(scene, node);
    }
}

// FUNCTION: LEGORR 0x00487cb0
float animation_track_find_keyframe_and_fraction(LwsNode* node, float time,
                                                 unsigned short* prev)
{
    unsigned short key;
    unsigned short upper = node->keyCount;
    unsigned short lower = 0;
    unsigned short frame;

    while (1) {
        key = (unsigned short)((lower + upper) / 2);
        frame = node->keyList[key].frame;
        if (key == lower) break;
        if (frame > time) upper = key;
        else lower = key;
    }
    *prev = key;

    return (time - frame) / ((float)node->keyList[key + 1].frame - frame);
}

// FUNCTION: LEGORR 0x00487d50
void lws_textures_animate(LwsInfo* scene, LwsNode* node)
{
    if (!(node->flags & LWSNODE_FLAG_NULL)) {
        LwsFrame* frame = scene->frameList[node->frameIndex];
        LwsUnknown* iunknown;
        LwsUserVisual* userVisual;
        unsigned long count;
        void* mesh;

        frame->lpVtbl->GetVisuals(frame, &count, NULL);
        frame->lpVtbl->GetVisuals(frame, &count, &iunknown);
        iunknown->lpVtbl->QueryInterface(iunknown, &s_iid_user_visual, (void**)&userVisual);
        iunknown->lpVtbl->Release(iunknown);
        mesh = (void*)userVisual->lpVtbl->GetAppData(userVisual);
        userVisual->lpVtbl->Release(userVisual);

        mesh_texture_time_set(mesh, scene->time);
    }
}

// FUNCTION: LEGORR 0x00487de0
void lws_trigger_handle(LwsInfo* scene, LwsNode* node)
{
    if (node->flags & LWSNODE_FLAG_SOUNDTRIGGER) {
        LwsFrame* frame = scene->frameList[node->frameIndex];
        LwsSoundTrigger* trigger = &scene->triggerList[node->triggerIndex];
        unsigned short loop;

        for (loop = 0; loop < trigger->count; loop++) {
            int loopSound = (trigger->frameStartList[loop] != trigger->frameEndList[loop]);
            if (shared_object_sample_time_matches(scene, trigger->frameStartList[loop]) &&
                lwsGlobs.SoundEnabledFunc()) {
                trigger->loopUID[loop] = lwsGlobs.PlaySample3DFunc(frame, trigger->sfxID, loopSound, 1, NULL);
            }
            if (loopSound && shared_object_sample_time_matches(scene, trigger->frameEndList[loop])) {
                sound_buffer_stop_handle((int)trigger->loopUID[loop]);
            }
        }
    }
}

// FUNCTION: LEGORR 0x00487ed0
int shared_object_sample_time_matches(LwsInfo* scene, unsigned long key)
{
    float lastTime = scene->lastTime;
    float currTime = scene->time;
    float keyTime = (float)key;
    float frames = (float)resource_entry_id_get(scene);
    float max;
    float min;

    if (lastTime > currTime) max = lastTime;
    else max = currTime;
    if (lastTime < currTime) min = lastTime;
    else min = currTime;

    if ((max - min) / frames < 0.5f) {
        if (keyTime <= max && keyTime >= min) return 1;
    } else {
        if (keyTime >= max || keyTime <= min) return 1;
    }

    return 0;
}

// FUNCTION: LEGORR 0x00487fc0
float keyframe_time_segment_fraction(LwsNode* node, float time,
                                     unsigned short* prev)
{
    unsigned short loop;

    for (loop = 1; loop < node->dissolveCount; loop++) {
        unsigned short frame = node->dissolveFrame[loop];
        if (frame >= time) {
            *prev = loop - 1;
            return (time - node->dissolveFrame[*prev]) / ((float)frame - node->dissolveFrame[*prev]);
        }
    }

    *prev = node->dissolveCount - 1;

    return 0.0f;
}

// FUNCTION: LEGORR 0x00488070
void lws_dissolve_interpolate(LwsInfo* scene, LwsNode* node,
                              unsigned short prev, float delta)
{
    float level;

    if (prev == node->dissolveCount - 1) level = node->dissolveLevel[prev];
    else level = node->dissolveLevel[prev] + ((node->dissolveLevel[prev + 1] - node->dissolveLevel[prev]) * delta);
    lws_dissolve_level_set(scene, node, level);
}

// FUNCTION: LEGORR 0x004880d0
void lws_dissolve_level_set(LwsInfo* scene, LwsNode* node, float level)
{
    LwsFrame* frame = scene->frameList[node->frameIndex];
    LwsUnknown* iunknown;
    LwsUserVisual* userVisual;
    unsigned long count;
    void* mesh;
    unsigned long groupCount;
    unsigned long group;

    frame->lpVtbl->GetVisuals(frame, &count, NULL);
    frame->lpVtbl->GetVisuals(frame, &count, &iunknown);
    iunknown->lpVtbl->QueryInterface(iunknown, &s_iid_user_visual, (void**)&userVisual);
    iunknown->lpVtbl->Release(iunknown);
    mesh = (void*)userVisual->lpVtbl->GetAppData(userVisual);
    userVisual->lpVtbl->Release(userVisual);

    groupCount = pointer_value_get(mesh);
    for (group = 0; group < groupCount; group++) {
        mesh_group_material_values_set(mesh, (int)group, level, LWS_MESH_COLOUR_ALPHA);
    }
}

// FUNCTION: LEGORR 0x00488170
void lws_interpolate(LwsInfo* scene, LwsNode* node, unsigned short key,
                     float delta)
{
    LwsVector3 pos;
    LwsVector3 hpb;
    LwsVector3 scale;
    LwsKeyInfo* curr = &node->keyList[key];
    LwsKeyInfo* next = &node->keyList[key + 1];

    pos.x = next->position.x - curr->position.x;
    pos.y = next->position.y - curr->position.y;
    pos.z = next->position.z - curr->position.z;
    pos.x = pos.x * delta;
    pos.y = pos.y * delta;
    pos.z = pos.z * delta;
    pos.x = curr->position.x + pos.x;
    pos.y = curr->position.y + pos.y;
    pos.z = curr->position.z + pos.z;
    scale.x = next->scale.x - curr->scale.x;
    scale.y = next->scale.y - curr->scale.y;
    scale.z = next->scale.z - curr->scale.z;
    scale.x = scale.x * delta;
    scale.y = scale.y * delta;
    scale.z = scale.z * delta;
    scale.x = curr->scale.x + scale.x;
    scale.y = curr->scale.y + scale.y;
    scale.z = curr->scale.z + scale.z;
    hpb.x = next->hpb.x - curr->hpb.x;
    hpb.y = next->hpb.y - curr->hpb.y;
    hpb.z = next->hpb.z - curr->hpb.z;
    hpb.x = hpb.x * delta;
    hpb.y = hpb.y * delta;
    hpb.z = hpb.z * delta;
    hpb.x = curr->hpb.x + hpb.x;
    hpb.y = curr->hpb.y + hpb.y;
    hpb.z = curr->hpb.z + hpb.z;

    lws_absolute_key_set(scene, node, &pos, &hpb, &scale);
}

// FUNCTION: LEGORR 0x004882e0
void lws_absolute_key_set(LwsInfo* scene, LwsNode* node, LwsVector3* pos,
                          LwsVector3* hpb, LwsVector3* scale)
{
    LwsFrame* frame = scene->frameList[node->frameIndex];
    float mat[4][4];

    if (node->flags & LWSNODE_FLAG_FACECAMERA) {
        LwsFrame* parent;

        frame->lpVtbl->GetParent(frame, &parent);
        frame->lpVtbl->GetTransform(frame, parent, mat);
        parent->lpVtbl->Release(parent);
        parent = NULL;

        mat[0][0] *= scale->x;
        mat[1][1] *= scale->y;
        mat[2][2] *= scale->z;
        mat[3][0] = pos->x;
        mat[3][1] = pos->y;
        mat[3][2] = pos->z;
    } else {
        float sinB = (float)sin(hpb->z);
        float sinP = (float)sin(hpb->y);
        float cosB = (float)cos(hpb->z);
        float sinH = (float)sin(hpb->x);
        float px = node->pivotVector.x;
        float cosH = (float)cos(hpb->x);
        float cosP = (float)cos(hpb->y);
        float py = node->pivotVector.y;
        float pz = node->pivotVector.z;
        float a, b, c, d, e, f, g;

        a = scale->x * sinB;
        b = scale->x * cosB;
        d = scale->y * cosB;
        c = sinP * a;
        mat[0][1] = cosP * a;
        e = sinP * d;
        f = -scale->y * sinB;
        mat[0][0] = cosH * b + sinH * c;
        mat[0][2] = -sinH * b + cosH * c;
        g = scale->z * cosP;
        mat[2][1] = -scale->z * sinP;
        mat[2][0] = sinH * g;
        mat[2][2] = cosH * g;
        mat[1][1] = cosP * d;
        mat[1][0] = cosH * f + sinH * e;
        mat[1][2] = cosH * e + -sinH * f;
        mat[0][3] = 0.0f;
        mat[1][3] = 0.0f;
        mat[2][3] = 0.0f;
        mat[3][0] = px * mat[0][0] + py * mat[1][0] + pz * mat[2][0] + pos->x;
        mat[3][1] = px * mat[0][1] + py * mat[1][1] + pz * mat[2][1] + pos->y;
        mat[3][2] = px * mat[0][2] + py * mat[1][2] + pz * mat[2][2] + pos->z;
        mat[3][3] = 1.0f;
    }

    frame->lpVtbl->AddTransform(frame, LWS_COMBINE_REPLACE, mat);
}

// FUNCTION: LEGORR 0x004885c0
void* lws_mesh_load(const char* baseDir, const char* fname, LwsFrame* frame,
                    int noTextures)
{
    char path[260];
    void* mesh;

    if (baseDir) sprintf(path, "%s%s", baseDir, fname);
    else strcpy(path, fname);

    if ((mesh = pair_table_find(lwsGlobs.meshPathList, lwsGlobs.meshPathCount, path)) != NULL) {
        mesh_clone(mesh, frame);
        return mesh;
    }
    if ((mesh = lwo_model_entry_load(path, frame, (void*)(long)noTextures)) != NULL) {
        pair_table_append(lwsGlobs.meshPathList, &lwsGlobs.meshPathCount, path, mesh);
        return mesh;
    }

    if (lwsGlobs.sharedDir) {
        sprintf(path, "%s%s", lwsGlobs.sharedDir, fname);
        if ((mesh = pair_table_find(lwsGlobs.meshPathListShared, lwsGlobs.meshPathCountShared, fname)) != NULL) {
            mesh_clone(mesh, frame);
            return mesh;
        }
        if ((mesh = lwo_model_entry_load(path, frame, (void*)(long)noTextures)) != NULL) {
            pair_table_append(lwsGlobs.meshPathListShared, &lwsGlobs.meshPathCountShared, fname, mesh);
            return mesh;
        }
    }

    return NULL;
}

// FUNCTION: LEGORR 0x00488730
void* pair_table_find(LwsMeshPath* list, unsigned long count,
                      const char* path)
{
    unsigned short loop;

    for (loop = 0; loop < count; loop++) {
        if (0 == _strcmpi(path, list[loop].path)) return list[loop].mesh;
    }

    return NULL;
}

// FUNCTION: LEGORR 0x00488790
void pair_table_append(LwsMeshPath* list, unsigned int* count,
                       const char* path, void* mesh)
{
    list[*count].path = string_duplicate(path);
    list[*count].mesh = mesh;
    (*count)++;
}

// FUNCTION: LEGORR 0x004887c0
void lws_frames_create(LwsInfo* scene, LwsNode* node, LwsFrame* parent,
                       unsigned short* frameCount)
{
    LwsD3DRM* d3drm = (LwsD3DRM*)s_main_d3drm;
    LwsFrame* frame;
    unsigned char name[1024];
    unsigned char* s;
    LwsNode* child;

    d3drm->lpVtbl->CreateFrame(d3drm, parent, &frame);

    if (node->flags & LWSNODE_FLAG_FACECAMERA) {
        float mat[4][4];
        memset(mat, 0, sizeof(mat));
        mat[3][0] = mat[3][1] = mat[3][2] = 0.0f;
        mat[0][0] = mat[1][1] = mat[2][2] = mat[3][3] = 1.0f;
        frame->lpVtbl->AddTransform(frame, LWS_COMBINE_REPLACE, mat);
    }

    scene->frameList[*frameCount] = frame;
    node->frameIndex = *frameCount;
    (*frameCount)++;

    // STRING: LEGORR 0x004aab6c
    sprintf((char*)name, "%s_%0.2i", node->name, node->reference);
    for (s = name; '\0' != *s; s++) {
        if (!isprint(*s)) *s = '_';
    }
    frame->lpVtbl->SetName(frame, (const char*)name);

    for (child = node->childList; child; child = child->next) {
        lws_frames_create(scene, child, frame, frameCount);
    }
}

// FUNCTION: LEGORR 0x00488900
void lws_nodes_load(LwsInfo* scene, LwsNode* node)
{
    LwsFrame* frame = scene->frameList[node->frameIndex];
    void* mesh;
    LwsNode* child;

    if (1 == node->keyCount) lws_node_transform_setup(scene, node, 0);

    if (!(node->flags & LWSNODE_FLAG_NULL)) {
        if ((mesh = lws_mesh_load(scene->filePath, node->name, frame, 0)) != NULL) {
            if (node->flags & LWSNODE_FLAG_FACECAMERA) {
                *(unsigned long*)((char*)mesh + 0x2c) |= LWS_MESH_FLAG_FACECAMERA;
            }
            if (node->dissolveLevel && 0 == node->dissolveCount) {
                lws_dissolve_level_set(scene, node, *node->dissolveLevel);
                node->dissolveLevel = NULL;
            }
        }
    }

    for (child = node->childList; child; child = child->next) {
        lws_nodes_load(scene, child);
    }
}

// FUNCTION: LEGORR 0x004889a0
void lws_node_transform_setup(LwsInfo* scene, LwsNode* node,
                              unsigned short key)
{
    LwsKeyInfo* keyInfo = &node->keyList[key];

    lws_absolute_key_set(scene, node, &keyInfo->position, &keyInfo->hpb, &keyInfo->scale);
}

// FUNCTION: LEGORR 0x004889d0
void lws_free(LwsInfo* scene)
{
    LwsInfo* clonedFrom = scene->clonedFrom;
    unsigned short loop;

    scene->referenceCount--;
    if (0 == scene->referenceCount) {
        for (loop = 0; loop < scene->nodeCount; loop++) {
            lws_node_free(scene, &scene->nodeList[loop]);
        }
        if (NULL == scene->clonedFrom) {
            free(scene->nodeList);
            free(scene->filePath);
            if (scene->triggerCount) free(scene->triggerList);
        }
        free(scene->frameList);
        free(scene);
    }

    if (clonedFrom) lws_free(clonedFrom);
}

// FUNCTION: LEGORR 0x00488a70
void lws_node_free(LwsInfo* scene, LwsNode* node)
{
    if (!(node->flags & LWSNODE_FLAG_NULL)) {
        LwsFrame* frame = scene->frameList[node->frameIndex];
        LwsUnknown* iunknown;
        LwsUserVisual* userVisual;
        unsigned long count;
        void* mesh;

        frame->lpVtbl->GetVisuals(frame, &count, NULL);
        frame->lpVtbl->GetVisuals(frame, &count, &iunknown);
        iunknown->lpVtbl->QueryInterface(iunknown, &s_iid_user_visual, (void**)&userVisual);
        iunknown->lpVtbl->Release(iunknown);
        mesh = (void*)userVisual->lpVtbl->GetAppData(userVisual);
        userVisual->lpVtbl->Release(userVisual);

        mesh_remove(mesh, scene->frameList[node->frameIndex]);
    }

    if (NULL == scene->clonedFrom && 0 == scene->referenceCount) {
        free(node->name);
        free(node->keyList);
        if (node->dissolveCount) {
            free(node->dissolveLevel);
            free(node->dissolveFrame);
        }
    }
}
