// SPDX-License-Identifier: LGPL-3.0-or-later
/* Container.c (original translation unit G98-002, finding 624): gods98's
 * scene containers. A Container wraps a Direct3DRM frame (its master
 * frame), an activity frame under it and a hidden frame, with a type:
 * a mesh, a frame, an animation set, a set of activities (an `.ae` file),
 * a light, a reference to a frame made elsewhere, a LightWave scene or a
 * LightWave mesh. The unit also loads textures (Container_LoadTexture2)
 * and keeps the scene's fog.
 *
 * C-compatible header (ADR 0005). container.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * includes it.
 *
 * In the matching build container_link_names.h, which container.c includes
 * first, gives the unit's functions the port's C names where their callers
 * pass the original's arguments, so those callers reach the unit. Natively
 * the port tail (container_port.cpp and the adapters in entity.cpp,
 * surface.cpp and interface_port.cpp) serves those names and nothing calls
 * the unit. The functions keep the gods98 names (OpenLRR's, inferred): the
 * port already uses several of their snake-case spellings
 * (container_load, container_make_mesh2) for adapters. */
#ifndef LEGORRR_GODS98_CONTAINER_UNIT_H
#define LEGORRR_GODS98_CONTAINER_UNIT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Container Container;
typedef struct Container_Texture Container_Texture;
struct CntFrame;        /* IDirect3DRMFrame3 */
struct CntMesh;         /* IDirect3DRMMesh */
struct CntTexture;      /* IDirect3DRMTexture3 */
struct CntSurface;      /* IDirectDrawSurface4 */
struct CntObject;       /* IDirect3DRMObject */
struct AnimClone;

typedef void (*ContainerTriggerFrameCallback)(Container* cont, void* data);
typedef void (*ContainerSoundTriggerCallback)(const char* sample,
                                              Container* cont, void* data);
typedef void (*ContainerActivityCallback)(Container* cont, void* data);
typedef int (*ContainerWalkTreeCallback)(struct CntFrame* frame, void* data);

Container* Container_Initialise(const char* gameName);               /* 00472710 */
void Container_Shutdown(void);                                       /* 00472800 */
void Container_SetSharedTextureDirectory(const char* path);       /* 004728c0 */
void Container_EnableSoftwareRenderer(int on);                     /* 004728e0 */
void Container_SetTriggerFrameCallback(
    ContainerTriggerFrameCallback callback, void* data);             /* 00472900 */
void Container_SetSoundTriggerCallback(
    ContainerSoundTriggerCallback callback, void* data);             /* 00472920 */
Container* Container_GetRoot(void);                                 /* 00472940 */
Container* Container_Create(Container* parent);                      /* 00472950 */
void Container_Remove(Container* dead);                              /* 00472a40 */
void Container_Remove2(Container* dead, int kill);                   /* 00472a50 */
Container* Container_Load(Container* parent, const char* filename,
                          const char* typestr, int looping);         /* 00472cd0 */
int Container_IsCurrentActivity(Container* cont,
                                  const char* actname);              /* 00473340 */
int Container_SetActivity(Container* cont, const char* actname);    /* 00473370 */
int Container_Light_SetSpotPenumbra(Container* spotlight,
                                      float angle);                  /* 00473460 */
int Container_Light_SetSpotUmbra(Container* spotlight,
                                   float angle);                     /* 00473480 */
int Container_Light_SetSpotRange(Container* spotlight,
                                   float dist);                      /* 004734a0 */
void Container_Light_SetEnableContainer(Container* light,
                                          Container* enable);        /* 004734c0 */
Container* Container_MakeLight(Container* parent, unsigned long type,
                                float r, float g, float b);          /* 004734f0 */
Container* Container_MakeMesh2(Container* parent, int type);        /* 00473560 */
struct CntFrame* Container_GetMasterFrame(Container* cont);        /* 00473680 */
Container* Container_Clone(Container* orig);                         /* 00473690 */
void Container_Hide2(Container* cont, int hide);                     /* 00473b20 */
void Container_Hide(Container* cont, int hide);                      /* 00473b40 */
int Container_IsHidden(Container* cont);                            /* 00473ba0 */
Container* Container_SearchTree(Container* root, const char* name,
                                 int mode, unsigned long* count);    /* 00473bc0 */
char* Container_FormatPartName(Container* cont, const char* partname,
                                 unsigned long* instance);           /* 00473c60 */
void Container_SetUserData(Container* cont, void* data);           /* 00473da0 */
void* Container_GetUserData(Container* cont);                      /* 00473db0 */
void Container_EnableFog(int on);                                   /* 00473dc0 */
void Container_SetFogColour(float r, float g, float b);            /* 00473e10 */
void Container_SetFogMode(unsigned long mode);                     /* 00473e70 */
void Container_SetFogParams(float start, float end,
                              float density);                        /* 00473ea0 */
void Container_SetPerspectiveCorrection(Container* cont, int on);  /* 00473ec0 */
struct CntSurface* Container_LoadTextureSurface(
    const char* fname, int managed, unsigned long* width,
    unsigned long* height, int* trans);                              /* 00474050 */
int Container_GetDecalColour(const char* fname,
                               unsigned long* colour);               /* 00474410 */
Container_Texture* Container_LoadTexture2(const char* fname,
                                           int immediate,
                                           unsigned long* width,
                                           unsigned long* height);   /* 004744f0 */
void Container_FreeTexture(Container_Texture* text);                /* 00474710 */
void Container_Mesh_Swap(Container* target, Container* origin,
                         int restore);                               /* 00474760 */
unsigned long Container_Mesh_AddGroup(Container* cont,
                                       unsigned long vertexCount,
                                       unsigned long faceCount,
                                       unsigned long vPerFace,
                                       unsigned long* faceData);     /* 004748f0 */
unsigned long Container_Mesh_GetGroupCount(Container* cont);       /* 00474a20 */
void Container_Mesh_SetQuality(Container* cont, unsigned long groupID,
                                unsigned long quality);              /* 00474a60 */
int Container_Mesh_IsGroupHidden(Container* cont,
                                   unsigned long group);             /* 00474ae0 */
void Container_Mesh_HideGroup(Container* cont, unsigned long group,
                               int hide);                            /* 00474b30 */
int Container_Mesh_HandleSeparateMeshGroups(struct CntMesh** mesh,
                                               unsigned long* group); /* 00474c00 */
int Container_Mesh_GetGroup(Container* cont, unsigned long groupID,
                             unsigned long* vertexCount,
                             unsigned long* faceCount,
                             unsigned long* vPerFace,
                             unsigned long* faceDataSize,
                             unsigned long* faceData);               /* 00474c40 */
unsigned long Container_Mesh_GetVertices(Container* cont,
                                          unsigned long groupID,
                                          unsigned long index,
                                          unsigned long count,
                                          void* retArray);           /* 00474cc0 */
unsigned long Container_Mesh_SetVertices(Container* cont,
                                          unsigned long groupID,
                                          unsigned long index,
                                          unsigned long count,
                                          void* values);             /* 00474d30 */
void Container_Mesh_SetTexture(Container* cont, unsigned long groupID,
                                Container_Texture* itext);           /* 00474da0 */
void Container_Mesh_SetPerspectiveCorrection(Container* cont,
                                               unsigned long groupID,
                                               int on);              /* 00474e30 */
int Container_Mesh_Scale(Container* cont, float x, float y, float z); /* 00474e90 */
int Container_Mesh_GetBox(Container* cont, float* box);             /* 00474f10 */
void Container_Mesh_SetEmissive(Container* cont, unsigned long groupID,
                                 float r, float g, float b);         /* 00474ff0 */
void Container_Mesh_SetColourAlpha(Container* cont,
                                     unsigned long groupID, float r,
                                     float g, float b, float a);     /* 00475020 */
void Container_Transform(Container* cont, float* destVector,
                         float* srcVector);                          /* 00475070 */
void Container_InverseTransform(Container* cont, float* destVector,
                                 float* srcVector);                  /* 00475090 */
void Container_SetColourAlpha(Container* cont, float r, float g,
                                float b, float a);                   /* 004750b0 */
float Container_MoveAnimation(Container* cont, float delta);        /* 00475120 */
float Container_SetAnimationTime(Container* cont, float time);     /* 00475140 */
void Container_ForceAnimationUpdate(Container* cont);              /* 00475300 */
float Container_GetAnimationTime(Container* cont);                 /* 00475390 */
unsigned long Container_GetAnimationFrames(Container* cont);       /* 004753f0 */
void Container_SetPosition(Container* cont, Container* ref, float x,
                            float y, float z);                       /* 00475430 */
void Container_SetOrientation(Container* cont, Container* ref,
                               float dirx, float diry, float dirz,
                               float upx, float upy, float upz);     /* 00475470 */
void Container_GetPosition(Container* cont, Container* ref,
                            float* pos);                             /* 004754c0 */
void Container_GetOrientation(Container* cont, Container* ref,
                               float* dir, float* up);               /* 00475500 */
void Container_AddRotation(Container* cont, int combine, float x,
                            float y, float z, float angle);          /* 00475580 */
void Container_AddScale(Container* cont, int combine, float x,
                         float y, float z);                          /* 004755b0 */
void Container_AddTranslation(Container* cont, int combine, float x,
                               float y, float z);                    /* 004755e0 */
void Container_ClearTransform(Container* cont);                     /* 00475610 */
void Container_AddTransform(Container* cont, int combine,
                             float (*matrix)[4]);                    /* 004756b0 */
float Container_GetZXRatio(Container* cont);                       /* 004756d0 */
void Container_SetParent(Container* child, Container* parent);      /* 00475710 */
Container* Container_GetParent(Container* child);                   /* 004757a0 */
float Container_GetTransCoef(Container* cont);                     /* 004757f0 */
Container* Container_SearchOwner(struct CntFrame* frame);           /* 00475830 */
Container* Container_Frame_GetContainer(struct CntFrame* frame);    /* 00475880 */
void Container_GetFrames(Container* cont, Container* ref,
                          struct CntFrame** contFrame,
                          struct CntFrame** refFrame);               /* 00475900 */
int Container_ParseTypeString(const char* str, int* noTexture);    /* 00475930 */
void Container_AddList(void);                                       /* 004759f0 */
unsigned long Container_GetActivities(Container* cont,
                                       struct CntFrame** frameList,
                                       struct AnimClone** acList,
                                       char** nameList);             /* 00475a70 */
void Container_SetTypeData(Container* cont, char* name,
                             void* light, struct CntMesh* mesh,
                             void* transMesh);                       /* 00475c00 */
void Container_FreeTypeData(Container* cont);                      /* 00475c80 */
int Container_AddActivity2(Container* cont, const char* filename,
                            const char* actname, float transCo,
                            unsigned long trigger, const char* sample,
                            struct AnimClone* origClone, int lws,
                            int looping);                            /* 00475d10 */
struct CntFrame* Container_Frame_Find(Container* cont,
                                      const char* findName,
                                      int hidden);                   /* 00475e40 */
void Container_Frame_SetAppData(struct CntFrame* frame,
                                  Container* owner,
                                  struct AnimClone* animClone,
                                  const char* asfname,
                                  unsigned long* frameCount,
                                  const char* frameName,
                                  float* currTime, float* transCo,
                                  const char* actSample,
                                  void* soundRecord,
                                  unsigned long* trigger);           /* 00475f70 */
void Container_Frame_RemoveAppData(struct CntFrame* frame);        /* 004760e0 */
Container* Container_Frame_GetOwner(struct CntFrame* frame);        /* 00476120 */
char* Container_Frame_GetAnimSetFileName(struct CntFrame* frame); /* 00476140 */
struct AnimClone* Container_Frame_GetAnimClone(struct CntFrame* frame); /* 00476160 */
unsigned long Container_Frame_GetFrameCount(struct CntFrame* frame); /* 00476180 */
float Container_Frame_GetCurrTime(struct CntFrame* frame);         /* 004761a0 */
float Container_Frame_GetTransCo(struct CntFrame* frame);          /* 004761c0 */
char* Container_Frame_GetSample(struct CntFrame* frame);            /* 004761e0 */
unsigned long Container_Frame_GetTrigger(struct CntFrame* frame);   /* 00476200 */
void Container_Frame_SafeAddChild(struct CntFrame* parent,
                                    struct CntFrame* child);         /* 00476220 */
void Container_Frame_FormatName(struct CntFrame* frame,
                                 const char* msg, ...);              /* 00476270 */
void Container_Frame_FreeName(struct CntFrame* frame);              /* 004762f0 */
char* Container_Frame_GetName(struct CntFrame* frame);              /* 00476310 */
int Container_Frame_WalkTree(struct CntFrame* frame,
                              unsigned long level,
                              ContainerWalkTreeCallback callback,
                              void* data);                           /* 00476330 */
int Container_Frame_SearchCallback(struct CntFrame* frame,
                                    void* data);                     /* 00476410 */
struct AnimClone* Container_LoadAnimSet(const char* fname,
                                          struct CntFrame* frame,
                                          unsigned long* frameCount,
                                          int lws, int looping);     /* 004765c0 */
unsigned long Container_GetAnimFileFrameCount(const char* fileData); /* 00476770 */
int Container_FrameLoad(const char* fname, struct CntFrame* frame); /* 004767e0 */
struct CntMesh* Container_MeshLoad(void* fileData,
                                    unsigned long fileSize,
                                    const char* fname,
                                    struct CntFrame* frame,
                                    int noTextures);                 /* 00476850 */
void Container_YFlipTexture(struct CntTexture* texture);           /* 00476bf0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_CONTAINER_UNIT_H */
