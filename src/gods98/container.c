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
 * C-compatible (ADR 0005): C in the matching build, C++ elsewhere. The
 * Direct3DRM and DirectDraw interfaces are called through their COM method
 * tables, declared here with the slots the unit uses. */
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

#if defined(_WIN32)
#define CNT_STDCALL __stdcall
#define CNT_CDECL __cdecl
#else
#define CNT_STDCALL
#define CNT_CDECL
#endif

#define CONTAINER_MAXLISTS 20
#define CONTAINER_MAXTEXTURES 1000
#define CONTAINER_MAXVISUALS 4
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

/* D3DRMCOMBINE_REPLACE, D3DRMLOAD_FROMFILE, D3DRMLOAD_FROMMEMORY,
 * D3DRMLOAD_FIRST and D3DRMSORT_FRONTTOBACK. */
#define CNT_COMBINE_REPLACE 0
#define CNT_LOAD_FROMFILE 0x00
#define CNT_LOAD_FROMMEMORY 0x02
#define CNT_LOAD_FIRST 0x00
#define CNT_SORT_FRONTTOBACK 1

typedef struct CntIID {
    unsigned long data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} CntIID;

typedef struct CntVector3 {
    float x;
    float y;
    float z;
} CntVector3;

typedef struct CntBox {
    CntVector3 min;
    CntVector3 max;
} CntBox;

/* D3DRMIMAGE. */
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
    unsigned long red_mask;
    unsigned long green_mask;
    unsigned long blue_mask;
    unsigned long alpha_mask;
    int palette_size;
    void* palette;
} CntImage;

typedef struct CntFrame CntFrame;            /* IDirect3DRMFrame3 */
typedef struct CntFrame1 CntFrame1;          /* IDirect3DRMFrame */
typedef struct CntFrameArray CntFrameArray;  /* IDirect3DRMFrameArray */
typedef struct CntMesh CntMesh;              /* IDirect3DRMMesh */
typedef struct CntMeshBuilder CntMeshBuilder; /* IDirect3DRMMeshBuilder3 */
typedef struct CntLight CntLight;            /* IDirect3DRMLight */
typedef struct CntTexture CntTexture;        /* IDirect3DRMTexture3 */
typedef struct CntAnimSet CntAnimSet;        /* IDirect3DRMAnimationSet2 */
typedef struct CntObject CntObject;          /* IDirect3DRMObject, IUnknown */
typedef struct CntD3DRM CntD3DRM;            /* IDirect3DRM3 */
typedef struct CntDevice CntDevice;          /* IDirect3DRMDevice3 */
typedef struct CntDDraw CntDDraw;            /* IDirectDraw4 */
typedef struct CntSurface CntSurface;        /* IDirectDrawSurface4 */
typedef struct CntPalette CntPalette;        /* IDirectDrawPalette */

typedef void (CNT_CDECL* CntObjectCallback)(CntObject* obj, void* arg);
typedef long (CNT_CDECL* CntLoadTextureCallback)(char* name, void* arg,
                                                 CntTexture** texture);

typedef struct CntFrameVtbl {
    long (CNT_STDCALL* QueryInterface)(CntFrame*, const CntIID*, void**);
    unsigned long (CNT_STDCALL* AddRef)(CntFrame*);                    /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntFrame*);                   /* +0x08 */
    void* Clone;                                                       /* +0x0c */
    long (CNT_STDCALL* AddDestroyCallback)(CntFrame*, CntObjectCallback,
                                           void*);                     /* +0x10 */
    void* DeleteDestroyCallback;                                       /* +0x14 */
    long (CNT_STDCALL* SetAppData)(CntFrame*, void*);                  /* +0x18 */
    void* (CNT_STDCALL* GetAppData)(CntFrame*);                        /* +0x1c */
    long (CNT_STDCALL* SetName)(CntFrame*, const char*);               /* +0x20 */
    long (CNT_STDCALL* GetName)(CntFrame*, unsigned long*, char*);     /* +0x24 */
    void* GetClassName;                                                /* +0x28 */
    long (CNT_STDCALL* AddChild)(CntFrame*, CntFrame*);                /* +0x2c */
    long (CNT_STDCALL* AddLight)(CntFrame*, CntLight*);                /* +0x30 */
    void* AddMoveCallback;                                             /* +0x34 */
    long (CNT_STDCALL* AddTransform)(CntFrame*, int, float (*)[4]);    /* +0x38 */
    long (CNT_STDCALL* AddTranslation)(CntFrame*, int, float, float,
                                       float);                         /* +0x3c */
    long (CNT_STDCALL* AddScale)(CntFrame*, int, float, float, float); /* +0x40 */
    long (CNT_STDCALL* AddRotation)(CntFrame*, int, float, float, float,
                                    float);                            /* +0x44 */
    long (CNT_STDCALL* AddVisual)(CntFrame*, CntObject*);              /* +0x48 */
    long (CNT_STDCALL* GetChildren)(CntFrame*, CntFrameArray**);       /* +0x4c */
    void* GetColor;                                                    /* +0x50 */
    void* GetLights;                                                   /* +0x54 */
    void* GetMaterialMode;                                             /* +0x58 */
    long (CNT_STDCALL* GetParent)(CntFrame*, CntFrame**);              /* +0x5c */
    long (CNT_STDCALL* GetPosition)(CntFrame*, CntFrame*, CntVector3*); /* +0x60 */
    void* GetRotation;                                                 /* +0x64 */
    void* GetScene;                                                    /* +0x68 */
    void* GetSortMode;                                                 /* +0x6c */
    void* GetTexture;                                                  /* +0x70 */
    long (CNT_STDCALL* GetTransform)(CntFrame*, CntFrame*,
                                     float (*)[4]);                    /* +0x74 */
    void* GetVelocity;                                                 /* +0x78 */
    long (CNT_STDCALL* GetOrientation)(CntFrame*, CntFrame*, CntVector3*,
                                       CntVector3*);                   /* +0x7c */
    long (CNT_STDCALL* GetVisuals)(CntFrame*, unsigned long*,
                                   CntObject**);                       /* +0x80 */
    long (CNT_STDCALL* InverseTransform)(CntFrame*, CntVector3*,
                                         CntVector3*);                 /* +0x84 */
    long (CNT_STDCALL* Load)(CntFrame*, void*, void*, unsigned long,
                             CntLoadTextureCallback, void*);           /* +0x88 */
    void* LookAt;                                                      /* +0x8c */
    void* Move;                                                        /* +0x90 */
    long (CNT_STDCALL* DeleteChild)(CntFrame*, CntFrame*);             /* +0x94 */
    void* DeleteLight;                                                 /* +0x98 */
    void* DeleteMoveCallback;                                          /* +0x9c */
    long (CNT_STDCALL* DeleteVisual)(CntFrame*, CntObject*);           /* +0xa0 */
    void* GetSceneBackground;                                          /* +0xa4 */
    void* GetSceneBackgroundDepth;                                     /* +0xa8 */
    void* GetSceneFogColor;                                            /* +0xac */
    void* GetSceneFogEnable;                                           /* +0xb0 */
    void* GetSceneFogMode;                                             /* +0xb4 */
    void* GetSceneFogParams;                                           /* +0xb8 */
    long (CNT_STDCALL* SetSceneBackground)(CntFrame*, unsigned long);  /* +0xbc */
    void* SetSceneBackgroundRGB;                                       /* +0xc0 */
    void* SetSceneBackgroundDepth;                                     /* +0xc4 */
    void* SetSceneBackgroundImage;                                     /* +0xc8 */
    long (CNT_STDCALL* SetSceneFogEnable)(CntFrame*, int);             /* +0xcc */
    long (CNT_STDCALL* SetSceneFogColor)(CntFrame*, unsigned long);    /* +0xd0 */
    long (CNT_STDCALL* SetSceneFogMode)(CntFrame*, unsigned long);     /* +0xd4 */
    long (CNT_STDCALL* SetSceneFogParams)(CntFrame*, float, float,
                                          float);                      /* +0xd8 */
    void* SetColor;                                                    /* +0xdc */
    void* SetColorRGB;                                                 /* +0xe0 */
    void* GetZbufferMode;                                              /* +0xe4 */
    void* SetMaterialMode;                                             /* +0xe8 */
    long (CNT_STDCALL* SetOrientation)(CntFrame*, CntFrame*, float, float,
                                       float, float, float, float);    /* +0xec */
    long (CNT_STDCALL* SetPosition)(CntFrame*, CntFrame*, float, float,
                                    float);                            /* +0xf0 */
    void* SetRotation;                                                 /* +0xf4 */
    long (CNT_STDCALL* SetSortMode)(CntFrame*, int);                   /* +0xf8 */
    void* SetTexture;                                                  /* +0xfc */
    void* SetVelocity;                                                 /* +0x100 */
    void* SetZbufferMode;                                              /* +0x104 */
    long (CNT_STDCALL* Transform)(CntFrame*, CntVector3*, CntVector3*); /* +0x108 */
    void* slots_10c[18];
    long (CNT_STDCALL* SetSceneFogMethod)(CntFrame*, unsigned long);   /* +0x154 */
} CntFrameVtbl;
struct CntFrame {
    const CntFrameVtbl* lpVtbl;
};

typedef struct CntFrame1Vtbl {
    long (CNT_STDCALL* QueryInterface)(CntFrame1*, const CntIID*, void**);
    unsigned long (CNT_STDCALL* AddRef)(CntFrame1*);                   /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntFrame1*);                  /* +0x08 */
} CntFrame1Vtbl;
struct CntFrame1 {
    const CntFrame1Vtbl* lpVtbl;
};

typedef struct CntFrameArrayVtbl {
    long (CNT_STDCALL* QueryInterface)(CntFrameArray*, const CntIID*,
                                       void**);
    unsigned long (CNT_STDCALL* AddRef)(CntFrameArray*);               /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntFrameArray*);              /* +0x08 */
    unsigned long (CNT_STDCALL* GetSize)(CntFrameArray*);              /* +0x0c */
    long (CNT_STDCALL* GetElement)(CntFrameArray*, unsigned long,
                                   CntFrame1**);                       /* +0x10 */
} CntFrameArrayVtbl;
struct CntFrameArray {
    const CntFrameArrayVtbl* lpVtbl;
};

typedef struct CntMeshVtbl {
    long (CNT_STDCALL* QueryInterface)(CntMesh*, const CntIID*, void**);
    unsigned long (CNT_STDCALL* AddRef)(CntMesh*);                     /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntMesh*);                    /* +0x08 */
    void* Clone;                                                       /* +0x0c */
    void* AddDestroyCallback;                                          /* +0x10 */
    void* DeleteDestroyCallback;                                       /* +0x14 */
    long (CNT_STDCALL* SetAppData)(CntMesh*, unsigned long);           /* +0x18 */
    void* (CNT_STDCALL* GetAppData)(CntMesh*);                         /* +0x1c */
    void* SetName;                                                     /* +0x20 */
    void* GetName;                                                     /* +0x24 */
    void* GetClassName;                                                /* +0x28 */
    long (CNT_STDCALL* Scale)(CntMesh*, float, float, float);          /* +0x2c */
    void* Translate;                                                   /* +0x30 */
    long (CNT_STDCALL* GetBox)(CntMesh*, CntBox*);                     /* +0x34 */
    long (CNT_STDCALL* AddGroup)(CntMesh*, unsigned long, unsigned long,
                                 unsigned long, unsigned long*,
                                 unsigned long*);                      /* +0x38 */
    long (CNT_STDCALL* SetVertices)(CntMesh*, unsigned long, unsigned long,
                                    unsigned long, void*);             /* +0x3c */
    void* SetGroupColor;                                               /* +0x40 */
    void* SetGroupColorRGB;                                            /* +0x44 */
    long (CNT_STDCALL* SetGroupMapping)(CntMesh*, unsigned long,
                                        unsigned long);                /* +0x48 */
    long (CNT_STDCALL* SetGroupQuality)(CntMesh*, unsigned long,
                                        unsigned long);                /* +0x4c */
    void* SetGroupMaterial;                                            /* +0x50 */
    long (CNT_STDCALL* SetGroupTexture)(CntMesh*, unsigned long,
                                        CntTexture*);                  /* +0x54 */
    unsigned long (CNT_STDCALL* GetGroupCount)(CntMesh*);              /* +0x58 */
    long (CNT_STDCALL* GetGroup)(CntMesh*, unsigned long, unsigned long*,
                                 unsigned long*, unsigned long*,
                                 unsigned long*, unsigned long*);      /* +0x5c */
    long (CNT_STDCALL* GetVertices)(CntMesh*, unsigned long, unsigned long,
                                    unsigned long, void*);             /* +0x60 */
} CntMeshVtbl;
struct CntMesh {
    const CntMeshVtbl* lpVtbl;
};

typedef struct CntMeshBuilderVtbl {
    long (CNT_STDCALL* QueryInterface)(CntMeshBuilder*, const CntIID*,
                                       void**);
    unsigned long (CNT_STDCALL* AddRef)(CntMeshBuilder*);              /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntMeshBuilder*);             /* +0x08 */
    void* slots_0c[8];
    long (CNT_STDCALL* Load)(CntMeshBuilder*, void*, void*, unsigned long,
                             CntLoadTextureCallback, void*);           /* +0x2c */
    void* slots_30[19];
    long (CNT_STDCALL* SetPerspective)(CntMeshBuilder*, int);          /* +0x7c */
    void* slots_80[16];
    long (CNT_STDCALL* CreateMesh)(CntMeshBuilder*, CntMesh**);        /* +0xc0 */
} CntMeshBuilderVtbl;
struct CntMeshBuilder {
    const CntMeshBuilderVtbl* lpVtbl;
};

typedef struct CntLightVtbl {
    long (CNT_STDCALL* QueryInterface)(CntLight*, const CntIID*, void**);
    unsigned long (CNT_STDCALL* AddRef)(CntLight*);                    /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntLight*);                   /* +0x08 */
    void* slots_0c[9];
    long (CNT_STDCALL* SetColor)(CntLight*, unsigned long);            /* +0x30 */
    void* SetColorRGB;                                                 /* +0x34 */
    long (CNT_STDCALL* SetRange)(CntLight*, float);                    /* +0x38 */
    long (CNT_STDCALL* SetUmbra)(CntLight*, float);                    /* +0x3c */
    long (CNT_STDCALL* SetPenumbra)(CntLight*, float);                 /* +0x40 */
    void* slots_44[11];
    long (CNT_STDCALL* SetEnableFrame)(CntLight*, CntFrame1*);         /* +0x70 */
} CntLightVtbl;
struct CntLight {
    const CntLightVtbl* lpVtbl;
};

typedef struct CntTextureVtbl {
    long (CNT_STDCALL* QueryInterface)(CntTexture*, const CntIID*, void**);
    unsigned long (CNT_STDCALL* AddRef)(CntTexture*);                  /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntTexture*);                 /* +0x08 */
    void* Clone;                                                       /* +0x0c */
    long (CNT_STDCALL* AddDestroyCallback)(CntTexture*, CntObjectCallback,
                                           void*);                     /* +0x10 */
    void* DeleteDestroyCallback;                                       /* +0x14 */
    long (CNT_STDCALL* SetAppData)(CntTexture*, void*);                /* +0x18 */
    void* (CNT_STDCALL* GetAppData)(CntTexture*);                      /* +0x1c */
    void* slots_20[12];
    long (CNT_STDCALL* SetDecalTransparency)(CntTexture*, int);        /* +0x50 */
    long (CNT_STDCALL* SetDecalTransparentColor)(CntTexture*,
                                                 unsigned long);       /* +0x54 */
    void* slots_58[2];
    CntImage* (CNT_STDCALL* GetImage)(CntTexture*);                    /* +0x60 */
    void* slots_64[7];
    long (CNT_STDCALL* GenerateMIPMap)(CntTexture*, unsigned long);    /* +0x80 */
} CntTextureVtbl;
struct CntTexture {
    const CntTextureVtbl* lpVtbl;
};

typedef struct CntAnimSetVtbl {
    long (CNT_STDCALL* QueryInterface)(CntAnimSet*, const CntIID*, void**);
    unsigned long (CNT_STDCALL* AddRef)(CntAnimSet*);                  /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntAnimSet*);                 /* +0x08 */
    void* slots_0c[9];
    long (CNT_STDCALL* Load)(CntAnimSet*, void*, void*, unsigned long,
                             CntLoadTextureCallback, void*, CntFrame*); /* +0x30 */
    void* DeleteAnimation;                                             /* +0x34 */
    long (CNT_STDCALL* SetTime)(CntAnimSet*, float);                   /* +0x38 */
} CntAnimSetVtbl;
struct CntAnimSet {
    const CntAnimSetVtbl* lpVtbl;
};

typedef struct CntObjectVtbl {
    long (CNT_STDCALL* QueryInterface)(CntObject*, const CntIID*, void**);
    unsigned long (CNT_STDCALL* AddRef)(CntObject*);                   /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntObject*);                  /* +0x08 */
    void* Clone;                                                       /* +0x0c */
    void* AddDestroyCallback;                                          /* +0x10 */
    void* DeleteDestroyCallback;                                       /* +0x14 */
    long (CNT_STDCALL* SetAppData)(CntObject*, void*);                 /* +0x18 */
    void* (CNT_STDCALL* GetAppData)(CntObject*);                       /* +0x1c */
} CntObjectVtbl;
struct CntObject {
    const CntObjectVtbl* lpVtbl;
};

typedef struct CntD3DRMVtbl {
    long (CNT_STDCALL* QueryInterface)(CntD3DRM*, const CntIID*, void**);
    unsigned long (CNT_STDCALL* AddRef)(CntD3DRM*);                    /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntD3DRM*);                   /* +0x08 */
    void* CreateObject;                                                /* +0x0c */
    long (CNT_STDCALL* CreateFrame)(CntD3DRM*, CntFrame*, CntFrame**); /* +0x10 */
    long (CNT_STDCALL* CreateMesh)(CntD3DRM*, CntMesh**);              /* +0x14 */
    long (CNT_STDCALL* CreateMeshBuilder)(CntD3DRM*, CntMeshBuilder**); /* +0x18 */
    void* CreateFace;                                                  /* +0x1c */
    void* CreateAnimation;                                             /* +0x20 */
    long (CNT_STDCALL* CreateAnimationSet)(CntD3DRM*, CntAnimSet**);   /* +0x24 */
    void* CreateTexture;                                               /* +0x28 */
    void* CreateLight;                                                 /* +0x2c */
    long (CNT_STDCALL* CreateLightRGB)(CntD3DRM*, unsigned long, float,
                                       float, float, CntLight**);      /* +0x30 */
    void* slots_34[5];
    long (CNT_STDCALL* CreateTextureFromSurface)(CntD3DRM*, CntSurface*,
                                                 CntTexture**);        /* +0x48 */
    void* slots_4c[4];
    long (CNT_STDCALL* LoadTexture)(CntD3DRM*, const char*,
                                    CntTexture**);                     /* +0x5c */
} CntD3DRMVtbl;
struct CntD3DRM {
    const CntD3DRMVtbl* lpVtbl;
};

typedef struct CntDeviceVtbl {
    void* slots_00[39];
    long (CNT_STDCALL* FindPreferredTextureFormat)(CntDevice*,
                                                   unsigned long,
                                                   unsigned long,
                                                   void*);             /* +0x9c */
} CntDeviceVtbl;
struct CntDevice {
    const CntDeviceVtbl* lpVtbl;
};

typedef struct CntDDrawVtbl {
    long (CNT_STDCALL* QueryInterface)(CntDDraw*, const CntIID*, void**);
    unsigned long (CNT_STDCALL* AddRef)(CntDDraw*);                    /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntDDraw*);                   /* +0x08 */
    void* slots_0c[2];
    long (CNT_STDCALL* CreatePalette)(CntDDraw*, unsigned long, void*,
                                      CntPalette**, void*);            /* +0x14 */
    long (CNT_STDCALL* CreateSurface)(CntDDraw*, void*, CntSurface**,
                                      void*);                          /* +0x18 */
} CntDDrawVtbl;
struct CntDDraw {
    const CntDDrawVtbl* lpVtbl;
};

typedef struct CntSurfaceVtbl {
    long (CNT_STDCALL* QueryInterface)(CntSurface*, const CntIID*, void**);
    unsigned long (CNT_STDCALL* AddRef)(CntSurface*);                  /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntSurface*);                 /* +0x08 */
    void* slots_0c[17];
    long (CNT_STDCALL* GetPalette)(CntSurface*, CntPalette**);         /* +0x50 */
    void* slots_54[4];
    long (CNT_STDCALL* Lock)(CntSurface*, void*, void*, unsigned long,
                             void*);                                   /* +0x64 */
    void* slots_68[3];
    long (CNT_STDCALL* SetColorKey)(CntSurface*, unsigned long, void*); /* +0x74 */
    void* SetOverlayPosition;                                          /* +0x78 */
    long (CNT_STDCALL* SetPalette)(CntSurface*, CntPalette*);          /* +0x7c */
    long (CNT_STDCALL* Unlock)(CntSurface*, void*);                    /* +0x80 */
} CntSurfaceVtbl;
struct CntSurface {
    const CntSurfaceVtbl* lpVtbl;
};

typedef struct CntPaletteVtbl {
    long (CNT_STDCALL* QueryInterface)(CntPalette*, const CntIID*, void**);
    unsigned long (CNT_STDCALL* AddRef)(CntPalette*);                  /* +0x04 */
    unsigned long (CNT_STDCALL* Release)(CntPalette*);                 /* +0x08 */
    void* GetCaps;                                                     /* +0x0c */
    long (CNT_STDCALL* GetEntries)(CntPalette*, unsigned long,
                                   unsigned long, unsigned long,
                                   void*);                             /* +0x10 */
} CntPaletteVtbl;
struct CntPalette {
    const CntPaletteVtbl* lpVtbl;
};

/* Main.c's lpD3DRM() (00505914) and lpDevice() (00505918). */
#define lpD3DRM() ((CntD3DRM*)s_main_d3drm)
#define lpDevice() ((CntDevice*)s_main_device)
/* DirectDraw.c's lpDD() (0076ac84). */
#define lpDD() ((CntDDraw*)CONTAINER_LPDD())

/* A container's type data (Container_SetTypeData). */
typedef struct Container_TypeData {
    char* name;                 /* +0x00 the current activity */
    CntLight* light;            /* +0x04 */
    CntMesh* mesh;              /* +0x08 */
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
    CntFrame* masterFrame;                       /* +0x00 */
    CntFrame* activityFrame;                     /* +0x04 */
    CntFrame* hiddenFrame;                       /* +0x08 */
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
    CntSurface* surface;        /* +0x00 */
    CntTexture* texture;        /* +0x04 */
    int colourKey;              /* +0x08 */
};

/* A mesh container's group records (Container_MakeMesh2). Group 0 is the
 * mesh itself; group n is listSet[n - 1], a separate mesh. */
typedef struct Container_MeshAppData {
    CntMesh** listSet;          /* +0x00 */
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
    CntFrame* resultFrame;      /* +0x0c */
    unsigned long count;        /* +0x10 */
    Container_SearchMode mode;  /* +0x14 */
    unsigned long matchNumber;  /* +0x18 */
} Container_SearchData;

/* A texture the mesh loader cached (Container_TextureLoadCallback). */
typedef struct Container_TextureRef {
    char* fileName;             /* +0x00 */
    CntTexture* texture;        /* +0x04 */
} Container_TextureRef;

/* The unit's state: a common symbol at 0076ad80 (0x2018 bytes). */
typedef struct Container_Globs {
    struct Container* listSet[CONTAINER_MAXLISTS];     /* +0x0000 0076ad80 */
    struct Container* freeList;                        /* +0x0050 0076add0 */
    struct Container* rootContainer;                   /* +0x0054 0076add4 */
    const char* typeName[Container_TypeCount];         /* +0x0058 0076add8 */
    const char* extensionName[Container_TypeCount];    /* +0x007c 0076adfc */
    const char* gameName;                              /* +0x00a0 0076ae20 */
    CntObject* visualArray[CONTAINER_MAXVISUALS];      /* +0x00a4 0076ae24 */
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
 * 0049e89c). The unit reads D3DRMMAP_PERSPCORRECT from memory. */
// GLOBAL: LEGORR 0x0049e87c
static const unsigned long cntMapWrapU = 1;
// GLOBAL: LEGORR 0x0049e880
static const unsigned long cntMapWrapV = 2;
// GLOBAL: LEGORR 0x0049e884
static const unsigned long cntMapPerspCorrect = 4;
// GLOBAL: LEGORR 0x0049e888
static const long cntGroupAllGroups = -1;

/* The interface IDs the unit asks for: library data in the GUID block
 * (finding 618). The unit's own copies, as movie_unit.cpp and
 * directdraw.c keep theirs; IID_IDirect3DRMFrame3 (0049f988) is
 * resource.cpp's s_resource_tree_child_iid, which animclone.c reads too. */
// GLOBAL: LEGORR 0x0049f338
static const CntIID IID_CntDirectDrawSurface = {
    0x6c14db81, 0xa733, 0x11ce,
    {0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60}};
// GLOBAL: LEGORR 0x0049f968
static const CntIID IID_CntFrame = {
    0xeb16cb03, 0xd271, 0x11ce,
    {0xac, 0x48, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1}};
// GLOBAL: LEGORR 0x0049f9a8
static const CntIID IID_CntMesh = {
    0xa3a80d01, 0x6e12, 0x11cf,
    {0xac, 0x4a, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1}};
// GLOBAL: LEGORR 0x0049f9c8
static const CntIID IID_CntMeshBuilder3 = {
    0x4516ec77, 0x8f20, 0x11d0,
    {0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3}};
// GLOBAL: LEGORR 0x0049fa18
static const CntIID IID_CntTexture = {
    0xeb16cb09, 0xd271, 0x11ce,
    {0xac, 0x48, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1}};
extern const unsigned char s_resource_tree_child_iid[16];          /* 0049f988 */
#define IID_CntFrame3 (*(const CntIID*)s_resource_tree_child_iid)

/* Config.c's ":" (004a1cd4), "::" (004aab18) and "" (004b8a14): the
 * original pools these literals across units (findings 569 and 581). */
extern const char s_config_rgb_separator[];
extern const char s_config_separator[];
extern const char s_config_angle_empty[4];

// GLOBAL: LEGORR 0x0076ad80
Container_Globs containerGlobs;

/* The unit's static callbacks, passed by address before their definitions. */
static int Container_SetPerspectiveCorrectionCallback(CntFrame* frame,
                                                      void* data);
static void CNT_CDECL Container_Frame_ReferenceDestroyCallback(
    CntObject* obj, void* arg);
static long CNT_CDECL Container_TextureLoadCallback(char* name, void* data,
                                                    CntTexture** texture);
static int Container_TextureSetSort(const void* a, const void* b);
static void CNT_CDECL Container_TextureDestroyCallback(CntObject* obj,
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
    containerGlobs.extensionName[Container_Null] = s_config_angle_empty;
    containerGlobs.extensionName[Container_FromActivity] = "ae";
    containerGlobs.extensionName[Container_Light] = s_config_angle_empty;

    containerGlobs.gameName = gameName;
    containerGlobs.freeList = NULL;
    containerGlobs.listCount = 0;
    containerGlobs.flags = CONTAINER_FLAG_INITIALISED;
    containerGlobs.textureCount = 0;
    containerGlobs.sharedDir = NULL;

    root = Container_Create(NULL);
    containerGlobs.rootContainer = root;
    root->masterFrame->lpVtbl->SetSortMode(root->masterFrame,
                                           CNT_SORT_FRONTTOBACK);

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

// FUNCTION: LEGORR 0x00472950
Container* Container_Create(Container* parent)
{
    Container* newCont;
    CntFrame* parentFrame = NULL;
    CntFrame* hiddenParentFrame = NULL;

    if (parent) {
        parentFrame = parent->masterFrame;
        hiddenParentFrame = parent->hiddenFrame;
    }

    if (containerGlobs.freeList == NULL) Container_AddList();

    newCont = containerGlobs.freeList;
    containerGlobs.freeList = newCont->nextFree;
    memset(newCont, 0, sizeof(Container));
    newCont->nextFree = newCont;

    if (lpD3DRM()->lpVtbl->CreateFrame(lpD3DRM(), parentFrame,
                                       &newCont->masterFrame) == 0) {
        if (lpD3DRM()->lpVtbl->CreateFrame(lpD3DRM(), newCont->masterFrame,
                                           &newCont->activityFrame) == 0) {
            if (lpD3DRM()->lpVtbl->CreateFrame(lpD3DRM(), hiddenParentFrame,
                                               &newCont->hiddenFrame) == 0) {
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
    CntFrame** frameList;
    struct AnimClone** cloneList;
    struct AnimClone* animClone;
    Container_MeshAppData* appData;
    CntMesh* mesh;
    CntFrame* parent;

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
        frameList = (CntFrame**)malloc(sizeof(CntFrame*) * count);
        Container_GetActivities(dead, frameList, cloneList, NULL);
        for (loop = 0; loop < count; loop++) {
            anim_clone_remove(cloneList[loop]);
            Container_Frame_FreeName(frameList[loop]);
            Container_Frame_RemoveAppData(frameList[loop]);
            frameList[loop]->lpVtbl->Release(frameList[loop]);
        }
        free(cloneList);
        free(frameList);
    } else if (dead->type == Container_Mesh) {
        if (dead->typeData) {
            if (dead->typeData->transMesh) {
                mesh_remove(dead->typeData->transMesh, dead->activityFrame);
            } else {
                appData = (Container_MeshAppData*)dead->typeData->mesh->lpVtbl->GetAppData(dead->typeData->mesh);
                if (appData) {
                    for (loop = 0; loop < appData->usedCount; loop++) {
                        mesh = appData->listSet[loop];
                        dead->activityFrame->lpVtbl->DeleteVisual(dead->activityFrame, (CntObject*)mesh);
                        mesh->lpVtbl->Release(mesh);
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
        dead->masterFrame->lpVtbl->GetParent(dead->masterFrame, &parent);
        if (parent) {
            parent->lpVtbl->DeleteChild(parent, dead->masterFrame);
            parent->lpVtbl->Release(parent);
        }
        dead->masterFrame->lpVtbl->Release(dead->masterFrame);
        dead->activityFrame->lpVtbl->Release(dead->activityFrame);

        dead->hiddenFrame->lpVtbl->GetParent(dead->hiddenFrame, &parent);
        if (parent) {
            parent->lpVtbl->DeleteChild(parent, dead->hiddenFrame);
            parent->lpVtbl->Release(parent);
        }
        dead->hiddenFrame->lpVtbl->Release(dead->hiddenFrame);
    } else {
        dead->hiddenFrame->lpVtbl->Release(dead->hiddenFrame);
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
                    containerGlobs.gameName, s_config_separator,
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
                            containerGlobs.gameName, s_config_separator, arrayItem->value,
                            s_config_separator,
                            // STRING: LEGORR 0x004aab10
                            "FILE");
                    if (fileName = block_get_strptr(config, tempString)) {
                        float transCo;
                        unsigned long trigger;
                        int lws, loop;
                        char* sample;

                        sprintf(path, "%s\\%s", baseName, fileName);

                        sprintf(tempString, "%s%s%s%s%s", containerGlobs.gameName,
                                s_config_separator, arrayItem->value, s_config_separator,
                                // STRING: LEGORR 0x004aaaf8
                                "TRANSCOEF");
                        transCo = (float)atof(block_get_strptr(config, tempString)
                                                  ? block_get_strptr(config, tempString)
                                                  : s_config_angle_empty);

                        sprintf(tempString, "%s%s%s%s%s", containerGlobs.gameName,
                                s_config_separator, arrayItem->value, s_config_separator,
                                // STRING: LEGORR 0x004aaaf0
                                "TRIGGER");
                        trigger = atoi(block_get_strptr(config, tempString)
                                           ? block_get_strptr(config, tempString)
                                           : s_config_angle_empty);

                        sprintf(tempString, "%s%s%s%s%s", containerGlobs.gameName,
                                s_config_separator, arrayItem->value, s_config_separator,
                                // STRING: LEGORR 0x004aaae8
                                "LWSFILE");
                        lws = (block_get_bool_path(config, tempString) == 1);

                        sprintf(tempString, "%s%s%s%s%s", containerGlobs.gameName,
                                s_config_separator, arrayItem->value, s_config_separator,
                                // STRING: LEGORR 0x004aaae0
                                "LOOPING");
                        loop = (block_get_bool_path(config, tempString) != 0);

                        sprintf(tempString, "%s%s%s%s%s", containerGlobs.gameName,
                                s_config_separator, arrayItem->value, s_config_separator,
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

            sprintf(path, "%s%s%s", containerGlobs.gameName, s_config_separator,
                    // STRING: LEGORR 0x004aaad0
                    "SCALE");
            scale = (float)atof(block_get_strptr(config, path)
                                    ? block_get_strptr(config, path)
                                    : s_config_angle_empty);
            if (scale != 0.0f) {
                cont->activityFrame->lpVtbl->AddScale(cont->activityFrame, CNT_COMBINE_REPLACE,
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
            CntMesh* mesh;

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
    CntFrame* newFrame;
    CntFrame* oldFrame;
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
    return spotlight->typeData->light->lpVtbl->SetPenumbra(spotlight->typeData->light, angle) == 0;
}

// FUNCTION: LEGORR 0x00473480
int Container_Light_SetSpotUmbra(Container* spotlight, float angle)
{
    return spotlight->typeData->light->lpVtbl->SetUmbra(spotlight->typeData->light, angle) == 0;
}

// FUNCTION: LEGORR 0x004734a0
int Container_Light_SetSpotRange(Container* spotlight, float dist)
{
    return spotlight->typeData->light->lpVtbl->SetRange(spotlight->typeData->light, dist) == 0;
}

// FUNCTION: LEGORR 0x004734c0
void Container_Light_SetEnableContainer(Container* light, Container* enable)
{
    CntFrame1* frame1;

    enable->masterFrame->lpVtbl->QueryInterface(enable->masterFrame, &IID_CntFrame,
                                                (void**)&frame1);
    light->typeData->light->lpVtbl->SetEnableFrame(light->typeData->light, frame1);
}

// FUNCTION: LEGORR 0x004734f0
Container* Container_MakeLight(Container* parent, unsigned long type, float r,
                               float g, float b)
{
    Container* cont = Container_Create(parent);
    CntLight* light;

    if (cont) {
        cont->type = Container_Light;
        if (lpD3DRM()->lpVtbl->CreateLightRGB(lpD3DRM(), type, r, g, b, &light) == 0) {
            cont->activityFrame->lpVtbl->AddLight(cont->activityFrame, light);
            Container_SetTypeData(cont, NULL, light, NULL, NULL);
        }
    }

    return cont;
}

// FUNCTION: LEGORR 0x00473560
Container* Container_MakeMesh2(Container* parent, int type)
{
    Container* cont = Container_Create(parent);
    CntMesh* mesh;
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
        } else if (lpD3DRM()->lpVtbl->CreateMesh(lpD3DRM(), &mesh) == 0) {
            cont->activityFrame->lpVtbl->AddVisual(cont->activityFrame, (CntObject*)mesh);
            Container_SetTypeData(cont, NULL, NULL, mesh, NULL);
            if (type == 1) {
                appData = (Container_MeshAppData*)malloc(sizeof(Container_MeshAppData));
                appData->listSize = 20;
                appData->usedCount = 0;
                appData->listSet = (CntMesh**)malloc(sizeof(CntMesh*) * appData->listSize);
                appData->groupZeroHidden = 0;
                appData->firstAddGroup = 1;
                mesh->lpVtbl->SetAppData(mesh, (unsigned long)appData);
            } else {
                mesh->lpVtbl->SetAppData(mesh, 0);
            }
        }
    }

    return cont;
}

// FUNCTION: LEGORR 0x00473680
CntFrame* Container_GetMasterFrame(Container* cont)
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
    CntFrame* parent;
    float mat[4][4];

    if (orig->type == Container_Mesh) {
        orig->typeData->mesh->lpVtbl->GetAppData(orig->typeData->mesh);
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
            orig->cloneData->clonedFrom = NULL;
            orig->cloneData->cloneNumber = 0;
            orig->cloneData->used = 1;
        }
    } else {
        cont = Container_Create(Container_GetParent(orig));
    }

    orig->activityFrame->lpVtbl->GetParent(orig->activityFrame, &parent);
    orig->activityFrame->lpVtbl->GetTransform(orig->activityFrame, parent, mat);
    cont->activityFrame->lpVtbl->AddTransform(cont->activityFrame, CNT_COMBINE_REPLACE, mat);
    parent->lpVtbl->Release(parent);

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
            CntMesh* mesh = cont->typeData->mesh;
            cont->activityFrame->lpVtbl->AddVisual(cont->activityFrame, (CntObject*)mesh);
            mesh->lpVtbl->AddRef(mesh);
        } else if (cont->type == Container_FromActivity) {
            CntFrame** frameList;
            char** nameList;
            unsigned long loop;

            count = Container_GetActivities(orig, NULL, NULL, NULL);
            frameList = (CntFrame**)malloc(sizeof(CntFrame*) * count);
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
                (struct AnimCloneFrame*)cont->activityFrame, &frameCount);
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
    CntFrame* frame;
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
    containerGlobs.rootContainer->masterFrame->lpVtbl->SetSceneFogEnable(
        containerGlobs.rootContainer->masterFrame, on);
    if (on) {
        containerGlobs.rootContainer->masterFrame->lpVtbl->SetSceneBackground(
            containerGlobs.rootContainer->masterFrame, containerGlobs.fogColour);
    } else {
        containerGlobs.rootContainer->masterFrame->lpVtbl->SetSceneBackground(
            containerGlobs.rootContainer->masterFrame, 0);
    }
}

// FUNCTION: LEGORR 0x00473e10
void Container_SetFogColour(float r, float g, float b)
{
    containerGlobs.fogColour = (0xff << 24) |
                               ((unsigned long)(r * 255.0f) << 16) |
                               ((unsigned long)(g * 255.0f) << 8) |
                               (unsigned long)(b * 255.0f);
    containerGlobs.rootContainer->masterFrame->lpVtbl->SetSceneFogColor(
        containerGlobs.rootContainer->masterFrame, containerGlobs.fogColour);
}

// FUNCTION: LEGORR 0x00473e70
void Container_SetFogMode(unsigned long mode)
{
    containerGlobs.rootContainer->masterFrame->lpVtbl->SetSceneFogMode(
        containerGlobs.rootContainer->masterFrame, mode);
    containerGlobs.rootContainer->masterFrame->lpVtbl->SetSceneFogMethod(
        containerGlobs.rootContainer->masterFrame, s_scene_fog_mode);
}

// FUNCTION: LEGORR 0x00473ea0
void Container_SetFogParams(float start, float end, float density)
{
    containerGlobs.rootContainer->masterFrame->lpVtbl->SetSceneFogParams(
        containerGlobs.rootContainer->masterFrame, start, end, density);
}

/* DDPIXELFORMAT, DDSURFACEDESC2, DDCOLORKEY and PALETTEENTRY. */
typedef struct CntPixelFormat {
    unsigned long dwSize;          /* +0x00 */
    unsigned long dwFlags;         /* +0x04 */
    unsigned long dwFourCC;        /* +0x08 */
    unsigned long dwRGBBitCount;   /* +0x0c */
    unsigned long dwRBitMask;      /* +0x10 */
    unsigned long dwGBitMask;      /* +0x14 */
    unsigned long dwBBitMask;      /* +0x18 */
    unsigned long dwAlphaBitMask;  /* +0x1c */
} CntPixelFormat;

typedef struct CntColourKey {
    unsigned long low;   /* +0x00 */
    unsigned long high;  /* +0x04 */
} CntColourKey;

typedef struct CntSurfaceDesc {
    unsigned long dwSize;              /* +0x00 */
    unsigned long dwFlags;             /* +0x04 */
    unsigned long dwHeight;            /* +0x08 */
    unsigned long dwWidth;             /* +0x0c */
    long lPitch;                       /* +0x10 */
    unsigned long dwBackBufferCount;   /* +0x14 */
    unsigned long dwMipMapCount;       /* +0x18 */
    unsigned long dwAlphaBitDepth;     /* +0x1c */
    unsigned long dwReserved;          /* +0x20 */
    void* lpSurface;                   /* +0x24 */
    CntColourKey ddckCKDestOverlay;    /* +0x28 */
    CntColourKey ddckCKDestBlt;        /* +0x30 */
    CntColourKey ddckCKSrcOverlay;     /* +0x38 */
    CntColourKey ddckCKSrcBlt;         /* +0x40 */
    CntPixelFormat ddpfPixelFormat;    /* +0x48 */
    unsigned long dwCaps;              /* +0x68 */
    unsigned long dwCaps2;             /* +0x6c */
    unsigned long dwCaps3;             /* +0x70 */
    unsigned long dwCaps4;             /* +0x74 */
    unsigned long dwTextureStage;      /* +0x78 */
} CntSurfaceDesc;

typedef struct CntPaletteEntry {
    unsigned char red;    /* +0x00 */
    unsigned char green;  /* +0x01 */
    unsigned char blue;   /* +0x02 */
    unsigned char flags;  /* +0x03 */
} CntPaletteEntry;

/* DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, the DDSCAPS
 * and DDPF bits, DDBD_8 and DDBD_16, D3DRMFPTF_PALETTIZED, DDLOCK_WAIT,
 * DDPCAPS_8BIT | DDPCAPS_INITIALIZE | DDPCAPS_ALLOW256 and
 * DDCKEY_SRCBLT. */
#define CNT_DDSD_TEXTUREFLAGS 0x00001007
#define CNT_DDSCAPS_OFFSCREENPLAIN 0x00000040
#define CNT_DDSCAPS_SYSTEMMEMORY 0x00000800
#define CNT_DDSCAPS_TEXTURE 0x00001000
#define CNT_DDSCAPS_VIDEOMEMORY 0x00004000
#define CNT_DDSCAPS2_TEXTUREMANAGE 0x00000010
#define CNT_DDPF_PALETTEINDEXED8 0x00000020
#define CNT_DDPF_RGB 0x00000040
#define CNT_DDBD_8 0x00000800
#define CNT_DDBD_16 0x00000400
#define CNT_FPTF_PALETTIZED 0x00000004
#define CNT_DDLOCK_WAIT 0x00000001
#define CNT_DDPCAPS_8BIT256 0x0000004c
#define CNT_DDCKEY_SRCBLT 0x00000008
/* Main.c's MAIN_FLAG_VIDEOTEXTURE (0x20) in mainGlobs.flags. */
#define CNT_MAIN_FLAG_VIDEOTEXTURE 0x00000020

// FUNCTION: LEGORR 0x00473ec0
void Container_SetPerspectiveCorrection(Container* cont, int on)
{
    if (cont->type == Container_FromActivity) {
        unsigned long count = Container_GetActivities(cont, NULL, NULL, NULL);
        CntFrame** frameList = (CntFrame**)malloc(sizeof(CntFrame*) * count);
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
static int Container_SetPerspectiveCorrectionCallback(CntFrame* frame,
                                                      void* data)
{
    int on = *(int*)data;
    unsigned long count;
    unsigned long loop;

    frame->lpVtbl->GetVisuals(frame, &count, NULL);
    if (count) {
        frame->lpVtbl->GetVisuals(frame, &count, containerGlobs.visualArray);
        for (loop = 0; loop < count; loop++) {
            CntObject* visual = containerGlobs.visualArray[loop];
            CntMeshBuilder* mb;
            CntMesh* mesh;
            if (visual->lpVtbl->QueryInterface(visual, &IID_CntMeshBuilder3,
                                               (void**)&mb) == 0) {
                mb->lpVtbl->SetPerspective(mb, on);
            } else if (visual->lpVtbl->QueryInterface(visual, &IID_CntMesh,
                                                      (void**)&mesh) == 0) {
                unsigned long groupCount = mesh->lpVtbl->GetGroupCount(mesh);
                unsigned long group;
                for (group = 0; group < groupCount; group++) {
                    if (on) {
                        mesh->lpVtbl->SetGroupMapping(mesh, group,
                                                      cntMapPerspCorrect);
                    } else {
                        mesh->lpVtbl->SetGroupMapping(mesh, group, 0);
                    }
                }
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00474050
struct CntSurface* Container_LoadTextureSurface(const char* fname,
                                                int managed,
                                                unsigned long* width,
                                                unsigned long* height,
                                                int* trans)
{
    CntSurface* surface = NULL;
    int convert = 0;
    void* buffer;
    unsigned int size;
    unsigned long decalColour;
    CntPalette* palette;
    CntImage image;
    CntSurfaceDesc desc;
    CntSurfaceDesc origDesc;

    if ((buffer = file_load_binary(fname, &size)) != NULL) {
        indexed_bitmap_palette_data_prepare((const uint8_t*)buffer, size,
                                            (struct IndexedBitmapPaletteData*)&image);
        if (!image.rgb && image.depth == 8) {
            memset(&desc, 0, sizeof(CntSurfaceDesc));
            desc.dwSize = sizeof(CntSurfaceDesc);
            desc.dwFlags = CNT_DDSD_TEXTUREFLAGS;
            desc.dwWidth = image.width;
            desc.dwHeight = image.height;
            desc.dwCaps = CNT_DDSCAPS_TEXTURE;
            if (!(s_engine_feature_word & CNT_MAIN_FLAG_VIDEOTEXTURE) && managed) {
                desc.dwCaps2 = CNT_DDSCAPS2_TEXTUREMANAGE;
            } else {
                desc.dwCaps = CNT_DDSCAPS_TEXTURE | CNT_DDSCAPS_VIDEOMEMORY;
            }
            desc.ddpfPixelFormat.dwSize = sizeof(CntPixelFormat);

            if (lpDevice()->lpVtbl->FindPreferredTextureFormat(
                    lpDevice(), CNT_DDBD_8, CNT_FPTF_PALETTIZED,
                    &desc.ddpfPixelFormat) != 0) {
                origDesc = desc;
                memset(&desc.ddpfPixelFormat, 0, sizeof(CntPixelFormat));
                desc.ddpfPixelFormat.dwSize = sizeof(CntPixelFormat);
                desc.ddpfPixelFormat.dwFlags =
                    CNT_DDPF_RGB | CNT_DDPF_PALETTEINDEXED8;
                desc.ddpfPixelFormat.dwRGBBitCount = 8;
                desc.dwCaps &= ~(CNT_DDSCAPS_TEXTURE | CNT_DDSCAPS_VIDEOMEMORY);
                desc.dwCaps2 &= ~CNT_DDSCAPS2_TEXTUREMANAGE;
                desc.dwCaps |= CNT_DDSCAPS_SYSTEMMEMORY |
                               CNT_DDSCAPS_OFFSCREENPLAIN;
                convert = 1;
            }

            if (lpDD()->lpVtbl->CreateSurface(lpDD(), &desc, &surface,
                                              NULL) == 0) {
                memset(&desc, 0, sizeof(CntSurfaceDesc));
                desc.dwSize = sizeof(CntSurfaceDesc);
                if (surface->lpVtbl->Lock(surface, NULL, &desc,
                                          CNT_DDLOCK_WAIT, NULL) == 0) {
                    unsigned char* dst = (unsigned char*)desc.lpSurface;
                    unsigned char* src = (unsigned char*)image.buffer1;
                    int y;
                    for (y = 0; y < image.height; y++) {
                        memcpy(dst, src, image.bytes_per_line);
                        dst += desc.lPitch;
                        src += image.bytes_per_line;
                    }
                    surface->lpVtbl->Unlock(surface, NULL);

                    if (lpDD()->lpVtbl->CreatePalette(lpDD(), CNT_DDPCAPS_8BIT256,
                                                      image.palette, &palette,
                                                      NULL) == 0) {
                        if (surface->lpVtbl->SetPalette(surface, palette) == 0) {
                            if (convert) {
                                memset(&origDesc.ddpfPixelFormat, 0,
                                       sizeof(CntPixelFormat));
                                origDesc.ddpfPixelFormat.dwSize =
                                    sizeof(CntPixelFormat);
                                if (lpDevice()->lpVtbl->FindPreferredTextureFormat(
                                        lpDevice(), CNT_DDBD_16, 0,
                                        &origDesc.ddpfPixelFormat) == 0) {
                                    CntSurface* tempSurface = surface;
                                    if (lpDD()->lpVtbl->CreateSurface(
                                            lpDD(), &origDesc, &surface,
                                            NULL) == 0) {
                                        if (origDesc.ddpfPixelFormat.dwRGBBitCount == 16) {
                                            directdraw_blt_8_to_16(
                                                (struct DirectDrawSurfaceIface*)surface,
                                                (struct DirectDrawSurfaceIface*)tempSurface,
                                                image.palette);
                                        }
                                        tempSurface->lpVtbl->Release(tempSurface);
                                    }
                                }
                            }

                            if (trans) {
                                if (Container_GetDecalColour(fname, &decalColour)) {
                                    CntColourKey colourKey;
                                    if (convert) {
                                        CntPaletteEntry* pal =
                                            (CntPaletteEntry*)image.palette;
                                        decalColour = directdraw_get_colour(
                                            (struct DirectDrawSurfaceIface*)surface,
                                            (pal[decalColour].red << 16) |
                                                (pal[decalColour].green << 8) |
                                                pal[decalColour].blue);
                                    }
                                    colourKey.low = colourKey.high = decalColour;
                                    surface->lpVtbl->SetColorKey(
                                        surface, CNT_DDCKEY_SRCBLT, &colourKey);
                                    *trans = 1;
                                } else {
                                    *trans = 0;
                                }
                            }

                            if (width) *width = image.width;
                            if (height) *height = image.height;
                            surface->lpVtbl->AddRef(surface);
                        }
                        palette->lpVtbl->Release(palette);
                    }
                }
                if (surface->lpVtbl->Release(surface) == 0) surface = NULL;
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
    CntTexture* texture = NULL;
    const char* path;
    Container_Texture* text;
    unsigned long decalColour;

    path = file_verify_filename(fname);
    if ((text = (Container_Texture*)malloc(sizeof(Container_Texture))) != NULL) {
        if (immediate) {
            CntSurface* surface = Container_LoadTextureSurface(
                fname, immediate, width, height, NULL);
            if (surface) {
                CntSurface* surf1;
                if (surface->lpVtbl->QueryInterface(
                        surface, &IID_CntDirectDrawSurface,
                        (void**)&surf1) == 0) {
                    if (lpD3DRM()->lpVtbl->CreateTextureFromSurface(
                            lpD3DRM(), surf1, &texture) == 0) {
                        surf1->lpVtbl->Release(surf1);
                        if (Container_GetDecalColour(fname, &decalColour)) {
                            CntPalette* palette;
                            CntPaletteEntry entry;
                            surface->lpVtbl->GetPalette(surface, &palette);
                            palette->lpVtbl->GetEntries(palette, 0, decalColour,
                                                        1, &entry);
                            palette->lpVtbl->Release(palette);
                            texture->lpVtbl->SetDecalTransparency(texture, 1);
                            texture->lpVtbl->SetDecalTransparentColor(
                                texture, (entry.red << 16) | (entry.green << 8) |
                                             entry.blue);
                        }
                        text->texture = texture;
                        text->surface = surface;
                        texture->lpVtbl->SetAppData(texture, text);
                        return text;
                    }
                    surf1->lpVtbl->Release(surf1);
                }
                surface->lpVtbl->Release(surface);
                free(text);
                return NULL;
            }
        } else {
            if (lpD3DRM()->lpVtbl->LoadTexture(lpD3DRM(), path, &texture) == 0) {
                CntImage* image = texture->lpVtbl->GetImage(texture);
                if (image) {
                    if (Container_GetDecalColour(fname, &decalColour)) {
                        CntPaletteEntry* entry =
                            &((CntPaletteEntry*)image->palette)[decalColour];
                        unsigned long red = entry->red;
                        unsigned long green = entry->green;
                        unsigned long blue = entry->blue;
                        texture->lpVtbl->SetDecalTransparency(texture, 1);
                        texture->lpVtbl->SetDecalTransparentColor(
                            texture, (red << 16) | (green << 8) | blue);
                    }
                    if (width) *width = image->width;
                    if (height) *height = image->height;
                }
                text->texture = texture;
                text->surface = NULL;
                texture->lpVtbl->SetAppData(texture, text);
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
            while (text->surface->lpVtbl->Release(text->surface));
        }
        if (text->texture) {
            while (text->texture->lpVtbl->Release(text->texture));
        }
    }
}

/* Mesh.c's Mesh, as far as Container_Mesh_Swap reads it: its user visual
 * (+0x0c). */
typedef struct Container_MeshView {
    void* reserved[3];
    CntObject* uv;              /* +0x0c */
} Container_MeshView;

// FUNCTION: LEGORR 0x00474760
void Container_Mesh_Swap(Container* target, Container* origin, int restore)
{
    CntFrame* frame;
    unsigned long count, loop;

    if (target->type == Container_Reference) frame = target->masterFrame;
    else frame = target->activityFrame;

    if (!restore) {
        frame->lpVtbl->GetVisuals(frame, &count, NULL);
        if (count) {
            frame->lpVtbl->GetVisuals(frame, &count, containerGlobs.visualArray);
            for (loop = 0; loop < count; loop++) {
                CntObject* visual = containerGlobs.visualArray[loop];
                target->hiddenFrame->lpVtbl->AddVisual(target->hiddenFrame, visual);
                frame->lpVtbl->DeleteVisual(frame, visual);
            }
        }
        if (origin) {
            if (origin->typeData->mesh == NULL) {
                frame->lpVtbl->AddVisual(
                    frame, ((Container_MeshView*)origin->typeData->transMesh)->uv);
            } else {
                frame->lpVtbl->AddVisual(frame, (CntObject*)origin->typeData->mesh);
            }
        }
        target->flags |= CONTAINER_FLAG_MESHSWAPPED;
    } else {
        frame->lpVtbl->GetVisuals(frame, &count, NULL);
        if (count) {
            frame->lpVtbl->GetVisuals(frame, &count, containerGlobs.visualArray);
            for (loop = 0; loop < count; loop++) {
                frame->lpVtbl->DeleteVisual(frame, containerGlobs.visualArray[loop]);
            }
        }
        target->hiddenFrame->lpVtbl->GetVisuals(target->hiddenFrame, &count, NULL);
        if (count) {
            target->hiddenFrame->lpVtbl->GetVisuals(target->hiddenFrame, &count,
                                                    containerGlobs.visualArray);
            for (loop = 0; loop < count; loop++) {
                CntObject* visual = containerGlobs.visualArray[loop];
                frame->lpVtbl->AddVisual(frame, visual);
                target->hiddenFrame->lpVtbl->DeleteVisual(target->hiddenFrame, visual);
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
        CntMesh* mesh = cont->typeData->mesh;
        unsigned long groupID;
        Container_MeshAppData* appData = (Container_MeshAppData*)mesh->lpVtbl->GetAppData(mesh);

        if (appData) {
            if (appData->firstAddGroup == 0) {
                CntMesh** newList;
                if (appData->usedCount == appData->listSize) return CONTAINER_ULONG_MAX;
                if (lpD3DRM()->lpVtbl->CreateMesh(lpD3DRM(), &mesh) != 0) return CONTAINER_ULONG_MAX;
                appData->listSet[appData->usedCount++] = mesh;
                cont->activityFrame->lpVtbl->AddVisual(cont->activityFrame, (CntObject*)mesh);
                mesh->lpVtbl->SetAppData(mesh, appData->usedCount | 0x80000000);
                if (appData->usedCount == appData->listSize) {
                    if (newList = (CntMesh**)realloc(appData->listSet,
                                                     sizeof(CntMesh*) * (appData->listSize + 20))) {
                        appData->listSize += 20;
                        appData->listSet = newList;
                    }
                }
            } else {
                appData->firstAddGroup = 0;
            }
        }

        if (mesh->lpVtbl->AddGroup(mesh, vertexCount, faceCount, vPerFace, faceData,
                                   &groupID) == 0) {
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
            cont->typeData->mesh->lpVtbl->GetAppData(cont->typeData->mesh);
        if (appData) return appData->usedCount + 1;
        return cont->typeData->mesh->lpVtbl->GetGroupCount(cont->typeData->mesh);
    }
}

// FUNCTION: LEGORR 0x00474a60
void Container_Mesh_SetQuality(Container* cont, unsigned long groupID,
                               unsigned long quality)
{
    unsigned long renderQuality;
    CntMesh* mesh;

    if (cont->typeData->transMesh == NULL) {
        if (quality == 0) renderQuality = 0x40;
        if (quality == 1) renderQuality = 0x80;
        if (quality == 2) renderQuality = 0x88;
        if (quality == 3) renderQuality = 0x89;
        mesh = cont->typeData->mesh;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        cont->typeData->mesh->lpVtbl->SetGroupQuality(mesh, groupID, renderQuality);
    }
}

// FUNCTION: LEGORR 0x00474ae0
int Container_Mesh_IsGroupHidden(Container* cont, unsigned long group)
{
    if (cont->typeData->transMesh) {
        return container_mesh_is_group_hidden_adapter(cont->typeData->transMesh, group);
    } else {
        CntMesh* mesh = cont->typeData->mesh;
        Container_MeshAppData* appData = (Container_MeshAppData*)mesh->lpVtbl->GetAppData(mesh);
        if (appData) {
            if (group) {
                mesh = appData->listSet[group - 1];
                return !((unsigned long)mesh->lpVtbl->GetAppData(mesh) & 0x80000000);
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
        CntMesh* mesh = cont->typeData->mesh;
        Container_MeshAppData* appData = (Container_MeshAppData*)mesh->lpVtbl->GetAppData(mesh);
        if (appData) {
            CntMesh* groupMesh;
            int hidden;
            if (group) {
                groupMesh = appData->listSet[group - 1];
                hidden = !((unsigned long)groupMesh->lpVtbl->GetAppData(groupMesh) & 0x80000000);
                mesh = groupMesh;
            } else {
                hidden = appData->groupZeroHidden;
                groupMesh = NULL;
            }

            if (hide && !hidden) {
                cont->activityFrame->lpVtbl->DeleteVisual(cont->activityFrame, (CntObject*)mesh);
            } else if (!hide && hidden) {
                cont->activityFrame->lpVtbl->AddVisual(cont->activityFrame, (CntObject*)mesh);
            }

            if (groupMesh) {
                unsigned long data = (unsigned long)groupMesh->lpVtbl->GetAppData(groupMesh) & 0x7fffffff;
                if (hide) groupMesh->lpVtbl->SetAppData(groupMesh, data);
                else groupMesh->lpVtbl->SetAppData(groupMesh, data | 0x80000000);
            } else {
                appData->groupZeroHidden = hide;
            }
        }
    }
}

// FUNCTION: LEGORR 0x00474c00
int Container_Mesh_HandleSeparateMeshGroups(CntMesh** mesh, unsigned long* group)
{
    Container_MeshAppData* appData = (Container_MeshAppData*)(*mesh)->lpVtbl->GetAppData(*mesh);
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
        CntMesh* mesh = cont->typeData->mesh;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        return mesh->lpVtbl->GetGroup(mesh, groupID, vertexCount, faceCount, vPerFace,
                                      faceDataSize, faceData) == 0;
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
        CntMesh* mesh = cont->typeData->mesh;
        unsigned long result;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        result = mesh->lpVtbl->GetVertices(mesh, groupID, index, count, retArray);
        if (result) result = CONTAINER_ULONG_MAX;
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
        CntMesh* mesh = cont->typeData->mesh;
        unsigned long result;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        result = mesh->lpVtbl->SetVertices(mesh, groupID, index, count, values);
        if (result) result = CONTAINER_ULONG_MAX;
        return result;
    }
}

// FUNCTION: LEGORR 0x00474da0
void Container_Mesh_SetTexture(Container* cont, unsigned long groupID,
                               Container_Texture* itext)
{
    CntTexture* texture;
    CntTexture* texture1;

    if (itext) texture = itext->texture;
    else texture = NULL;

    if (cont->typeData->transMesh) {
        animated_texture_frame_surface_set(cont->typeData->transMesh, (int)groupID, itext);
    } else {
        CntMesh* mesh = cont->typeData->mesh;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        if (texture) {
            texture->lpVtbl->QueryInterface(texture, &IID_CntTexture, (void**)&texture1);
        } else {
            texture1 = NULL;
        }
        mesh->lpVtbl->SetGroupTexture(mesh, groupID, texture1);
        if (texture1) texture1->lpVtbl->Release(texture1);
    }
}

// FUNCTION: LEGORR 0x00474e30
void Container_Mesh_SetPerspectiveCorrection(Container* cont, unsigned long groupID, int on)
{
    if (cont->typeData->transMesh == NULL) {
        CntMesh* mesh = cont->typeData->mesh;
        Container_Mesh_HandleSeparateMeshGroups(&mesh, &groupID);
        if (on) mesh->lpVtbl->SetGroupMapping(mesh, groupID, cntMapPerspCorrect);
        else mesh->lpVtbl->SetGroupMapping(mesh, groupID, 0);
    }
}

// FUNCTION: LEGORR 0x00474e90
int Container_Mesh_Scale(Container* cont, float x, float y, float z)
{
    if (cont->typeData->transMesh) {
        container_mesh_scale_adapter(cont->typeData->transMesh, x, y, z);
    } else {
        CntMesh* mesh = cont->typeData->mesh;
        Container_MeshAppData* appData;
        unsigned long loop;

        mesh->lpVtbl->Scale(mesh, x, y, z);
        if (appData = (Container_MeshAppData*)mesh->lpVtbl->GetAppData(mesh)) {
            for (loop = 0; loop < appData->usedCount; loop++) {
                appData->listSet[loop]->lpVtbl->Scale(appData->listSet[loop], x, y, z);
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00474f10
int Container_Mesh_GetBox(Container* cont, float* box)
{
    CntMesh* mesh = cont->typeData->mesh;
    CntBox* result = (CntBox*)box;
    Container_MeshAppData* appData;
    unsigned long loop;

    mesh->lpVtbl->GetBox(mesh, result);
    if (appData = (Container_MeshAppData*)mesh->lpVtbl->GetAppData(mesh)) {
        for (loop = 0; loop < appData->usedCount; loop++) {
            CntBox tempBox;
            appData->listSet[loop]->lpVtbl->GetBox(appData->listSet[loop], &tempBox);
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
    cont->masterFrame->lpVtbl->Transform(cont->masterFrame, (CntVector3*)destVector,
                                         (CntVector3*)srcVector);
}

// FUNCTION: LEGORR 0x00475090
void Container_InverseTransform(Container* cont, float* destVector, float* srcVector)
{
    cont->masterFrame->lpVtbl->InverseTransform(cont->masterFrame, (CntVector3*)destVector,
                                                (CntVector3*)srcVector);
}

// FUNCTION: LEGORR 0x004750b0
void Container_SetColourAlpha(Container* cont, float r, float g, float b, float a)
{
    unsigned long colour = ((unsigned long)(a * 255.0f) << 24) |
                           ((unsigned long)(r * 255.0f) << 16) |
                           ((unsigned long)(g * 255.0f) << 8) |
                           (unsigned long)(b * 255.0f);

    if (cont->type == Container_Light) {
        cont->typeData->light->lpVtbl->SetColor(cont->typeData->light, colour);
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
    CntFrame* frame;

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
            CntFrame* frame;
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
        CntFrame* frame;
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
    CntFrame* frame = NULL;

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
    CntFrame* contFrame;
    CntFrame* refFrame;

    Container_GetFrames(cont, ref, &contFrame, &refFrame);
    contFrame->lpVtbl->SetPosition(contFrame, refFrame, x, y, z);
}

// FUNCTION: LEGORR 0x00475470
void Container_SetOrientation(Container* cont, Container* ref, float dirx, float diry,
                              float dirz, float upx, float upy, float upz)
{
    CntFrame* contFrame;
    CntFrame* refFrame;

    Container_GetFrames(cont, ref, &contFrame, &refFrame);
    contFrame->lpVtbl->SetOrientation(contFrame, refFrame, dirx, diry, dirz, upx, upy, upz);
}

// FUNCTION: LEGORR 0x004754c0
void Container_GetPosition(Container* cont, Container* ref, float* pos)
{
    CntFrame* contFrame;
    CntFrame* refFrame;

    Container_GetFrames(cont, ref, &contFrame, &refFrame);
    contFrame->lpVtbl->GetPosition(contFrame, refFrame, (CntVector3*)pos);
}

// FUNCTION: LEGORR 0x00475500
void Container_GetOrientation(Container* cont, Container* ref, float* dir, float* up)
{
    CntFrame* contFrame;
    CntFrame* refFrame;
    CntVector3 tempDir, tempUp;

    Container_GetFrames(cont, ref, &contFrame, &refFrame);
    contFrame->lpVtbl->GetOrientation(contFrame, refFrame, &tempDir, &tempUp);
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
    cont->masterFrame->lpVtbl->AddRotation(cont->masterFrame, combine, x, y, z, angle);
}

// FUNCTION: LEGORR 0x004755b0
void Container_AddScale(Container* cont, int combine, float x, float y, float z)
{
    cont->masterFrame->lpVtbl->AddScale(cont->masterFrame, combine, x, y, z);
}

// FUNCTION: LEGORR 0x004755e0
void Container_AddTranslation(Container* cont, int combine, float x, float y, float z)
{
    cont->masterFrame->lpVtbl->AddTranslation(cont->masterFrame, combine, x, y, z);
}

// FUNCTION: LEGORR 0x00475610
void Container_ClearTransform(Container* cont)
{
    float mat[4][4];

    mat[0][0] = 1.0f; mat[1][0] = 0.0f; mat[2][0] = 0.0f; mat[3][0] = 0.0f;
    mat[0][1] = 0.0f; mat[1][1] = 1.0f; mat[2][1] = 0.0f; mat[3][1] = 0.0f;
    mat[0][2] = 0.0f; mat[1][2] = 0.0f; mat[2][2] = 1.0f; mat[3][2] = 0.0f;
    mat[0][3] = 0.0f; mat[1][3] = 0.0f; mat[2][3] = 0.0f; mat[3][3] = 1.0f;

    Container_AddTransform(cont, CNT_COMBINE_REPLACE, mat);
}

// FUNCTION: LEGORR 0x004756b0
void Container_AddTransform(Container* cont, int combine, float (*matrix)[4])
{
    cont->masterFrame->lpVtbl->AddTransform(cont->masterFrame, combine, matrix);
}

// FUNCTION: LEGORR 0x004756d0
float Container_GetZXRatio(Container* cont)
{
    CntFrame* frame = cont->masterFrame;
    CntFrame* parent;
    float mat[4][4];

    frame->lpVtbl->GetParent(frame, &parent);
    frame->lpVtbl->GetTransform(frame, parent, mat);
    parent->lpVtbl->Release(parent);

    return mat[2][2] / mat[0][0];
}

// FUNCTION: LEGORR 0x00475710
void Container_SetParent(Container* child, Container* parent)
{
    CntFrame* frame = child->masterFrame;
    CntFrame* hiddenFrame = child->hiddenFrame;
    CntFrame* parentFrame;

    if (parent == NULL) {
        frame->lpVtbl->GetParent(frame, &parentFrame);
        if (parentFrame) {
            parentFrame->lpVtbl->DeleteChild(parentFrame, frame);
            parentFrame->lpVtbl->Release(parentFrame);
        }
        frame->lpVtbl->GetParent(hiddenFrame, &parentFrame);
        if (parentFrame) {
            parentFrame->lpVtbl->DeleteChild(parentFrame, hiddenFrame);
            parentFrame->lpVtbl->Release(parentFrame);
        }
    } else {
        Container_Frame_SafeAddChild(parentFrame = parent->masterFrame, frame);
        Container_Frame_SafeAddChild(parentFrame = parent->hiddenFrame, hiddenFrame);
    }
}

// FUNCTION: LEGORR 0x004757a0
Container* Container_GetParent(Container* child)
{
    CntFrame* parentFrame;
    Container* parent;

    if (child == containerGlobs.rootContainer) return NULL;

    child->masterFrame->lpVtbl->GetParent(child->masterFrame, &parentFrame);
    if (parentFrame) {
        parent = Container_Frame_GetContainer(parentFrame);
        parentFrame->lpVtbl->Release(parentFrame);
    } else {
        parent = NULL;
    }

    return parent;
}

// FUNCTION: LEGORR 0x004757f0
float Container_GetTransCoef(Container* cont)
{
    if (cont->type == Container_FromActivity && cont->typeData && cont->typeData->name) {
        CntFrame* frame;
        if (frame = Container_Frame_Find(cont, cont->typeData->name, 0)) {
            return Container_Frame_GetTransCo(frame);
        }
    }
    return 0.0f;
}

// FUNCTION: LEGORR 0x00475830
Container* Container_SearchOwner(CntFrame* frame)
{
    Container* owner = NULL;
    Container* cont;
    CntFrame* parent;

    do {
        if (frame == NULL) break;
        if ((cont = Container_Frame_GetOwner(frame)) && cont->type != Container_Reference) {
            owner = cont;
        }
        frame->lpVtbl->GetParent(frame, &parent);
        if (frame = parent) frame->lpVtbl->Release(frame);
        else frame = NULL;
    } while (owner == NULL);

    return owner;
}

// FUNCTION: LEGORR 0x00475880
Container* Container_Frame_GetContainer(CntFrame* frame)
{
    Container* cont = Container_Frame_GetOwner(frame);

    if (cont == NULL) {
        if (cont = Container_Create(NULL)) {
            cont->masterFrame->lpVtbl->Release(cont->masterFrame);
            cont->activityFrame->lpVtbl->Release(cont->activityFrame);
            cont->masterFrame = frame;
            cont->activityFrame = NULL;
            cont->type = Container_Reference;
            frame->lpVtbl->AddDestroyCallback(frame, Container_Frame_ReferenceDestroyCallback,
                                              NULL);
            Container_Frame_SetAppData(frame, cont, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL);
        }
    }

    return cont;
}

// FUNCTION: LEGORR 0x00475900
void Container_GetFrames(Container* cont, Container* ref, CntFrame** contFrame,
                         CntFrame** refFrame)
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
        if ((unsigned long)sound_variant_split(string, argv, s_config_rgb_separator) > 1 &&
            // STRING: LEGORR 0x004aab78
            _strcmpi(argv[1], "NOTEXTURE") == 0) {
            *noTexture = 1;
        } else {
            *noTexture = 0;
        }

        for (loop = 0; loop < Container_TypeCount; loop++) {
            if (containerGlobs.typeName[loop]) {
                if (_strcmpi(containerGlobs.typeName[loop], string) == 0) return (int)loop;
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

/* D3DRMLOADMEMORY. */
typedef struct CntLoadMemory {
    void* lpMemory;             /* +0x00 */
    unsigned int dSize;         /* +0x04 */
} CntLoadMemory;

/* D3DRMERR_FILENOTFOUND. */
#define CNT_D3DRMERR_FILENOTFOUND ((long)0x88760311)
/* Main.c's MAIN_FLAG_MIPMAPENABLED (0x08) in mainGlobs.flags. */
#define CNT_MAIN_FLAG_MIPMAPENABLED 0x00000008

// FUNCTION: LEGORR 0x00475a70
unsigned long Container_GetActivities(Container* cont,
                                      struct CntFrame** frameList,
                                      struct AnimClone** acList,
                                      char** nameList)
{
    CntFrame* frame;
    unsigned long len = 0;
    unsigned long count = 0;
    unsigned long loop;

    if (cont->type == Container_FromActivity) {
        for (loop = 0; loop < 2; loop++) {
            CntFrame* root;
            CntFrameArray* children;
            if (loop == 0) root = cont->activityFrame;
            if (loop == 1) root = cont->hiddenFrame;
            if (root->lpVtbl->GetChildren(root, &children) == 0) {
                unsigned long childCount = children->lpVtbl->GetSize(children);
                unsigned long sub;
                for (sub = 0; sub < childCount; sub++) {
                    CntFrame1* frame1;
                    children->lpVtbl->GetElement(children, sub, &frame1);
                    frame1->lpVtbl->QueryInterface(frame1, &IID_CntFrame3,
                                                   (void**)&frame);
                    frame1->lpVtbl->Release(frame1);
                    frame->lpVtbl->GetName(frame, &len, NULL);
                    if (len) {
                        char* name = (char*)malloc(len);
                        frame->lpVtbl->GetName(frame, &len, name);
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
                    frame->lpVtbl->Release(frame);
                }
                children->lpVtbl->Release(children);
            }
        }
    }
    return count;
}

// FUNCTION: LEGORR 0x00475c00
void Container_SetTypeData(Container* cont, char* name, void* light,
                           struct CntMesh* mesh, void* transMesh)
{
    if (cont->typeData) {
        if (name) cont->typeData->name = name;
        if (light) cont->typeData->light = (CntLight*)light;
        if (mesh) cont->typeData->mesh = mesh;
        if (transMesh) cont->typeData->transMesh = transMesh;
    } else {
        cont->typeData = (Container_TypeData*)malloc(sizeof(Container_TypeData));
        cont->typeData->name = name;
        cont->typeData->light = (CntLight*)light;
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
            cont->typeData->light->lpVtbl->Release(cont->typeData->light);
        } else if (cont->type == Container_Mesh) {
            if (cont->typeData->mesh) {
                cont->activityFrame->lpVtbl->DeleteVisual(
                    cont->activityFrame, (CntObject*)cont->typeData->mesh);
                cont->typeData->mesh->lpVtbl->Release(cont->typeData->mesh);
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
    CntFrame* frame;
    unsigned long frameCount;
    char xFile[512];
    struct AnimClone* animClone;

    sprintf(xFile, "%s.%s", filename,
            containerGlobs.extensionName[Container_Anim]);
    if (lpD3DRM()->lpVtbl->CreateFrame(lpD3DRM(), cont->hiddenFrame,
                                       &frame) == 0) {
        Container_Frame_FormatName(frame, "%s_%s",
                                   CONTAINER_ACTIVITYFRAMEPREFIX, actname);
        if (origClone == NULL) {
            animClone = Container_LoadAnimSet(xFile, frame, &frameCount, lws,
                                              looping);
        } else {
            animClone = anim_clone_make(origClone, (struct AnimCloneFrame*)frame,
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
static void CNT_CDECL Container_Frame_ReferenceDestroyCallback(
    CntObject* obj, void* arg)
{
    CntFrame* frame = (CntFrame*)obj;
    Container* cont = Container_Frame_GetOwner(frame);
    (void)arg;
    if (cont) {
        cont->flags |= CONTAINER_FLAG_DEADREFERENCE2;
        Container_Frame_RemoveAppData(frame);
    }
}

// FUNCTION: LEGORR 0x00475e40
struct CntFrame* Container_Frame_Find(Container* cont, const char* findName,
                                      int hidden)
{
    CntFrame* parent;
    CntFrame* frame;
    CntFrame* foundFrame = NULL;
    CntFrameArray* children;
    CntFrame1* frame1;
    unsigned long count;
    unsigned long loop;
    unsigned long len;
    char* name;
    char buffer[512];

    if (hidden) parent = cont->hiddenFrame;
    else parent = cont->activityFrame;
    if (parent->lpVtbl->GetChildren(parent, &children) == 0) {
        count = children->lpVtbl->GetSize(children);
        for (loop = 0; loop < count; loop++) {
            children->lpVtbl->GetElement(children, loop, &frame1);
            frame1->lpVtbl->QueryInterface(frame1, &IID_CntFrame3,
                                           (void**)&frame);
            frame1->lpVtbl->Release(frame1);
            frame->lpVtbl->GetName(frame, &len, NULL);
            if (len) {
                name = (char*)malloc(len + 1);
                frame->lpVtbl->GetName(frame, &len, name);
                sprintf(buffer, "%s_%s", CONTAINER_ACTIVITYFRAMEPREFIX,
                        findName);
                if (_strcmpi(name, buffer) == 0) foundFrame = frame;
                free(name);
            }
            frame->lpVtbl->Release(frame);
            if (foundFrame) break;
        }
        children->lpVtbl->Release(children);
    }
    return foundFrame;
}

// FUNCTION: LEGORR 0x00475f70
void Container_Frame_SetAppData(struct CntFrame* frame, Container* owner,
                                struct AnimClone* animClone,
                                const char* asfname,
                                unsigned long* frameCount,
                                const char* frameName, float* currTime,
                                float* transCo, const char* actSample,
                                void* soundRecord, unsigned long* trigger)
{
    Container_AppData* appData =
        (Container_AppData*)frame->lpVtbl->GetAppData(frame);

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
        frame->lpVtbl->SetAppData(frame, appData);
    }
    if (asfname) {
        appData->animSetFileName = (char*)malloc(strlen(asfname) + 1);
        strcpy(appData->animSetFileName, asfname);
    }
}

// FUNCTION: LEGORR 0x004760e0
void Container_Frame_RemoveAppData(struct CntFrame* frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->lpVtbl->GetAppData(frame);
    if (appData) {
        if (appData->animSetFileName) free(appData->animSetFileName);
        free(appData);
    }
    frame->lpVtbl->SetAppData(frame, NULL);
}

// FUNCTION: LEGORR 0x00476120
Container* Container_Frame_GetOwner(struct CntFrame* frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->lpVtbl->GetAppData(frame);
    if (appData) return appData->ownerContainer;
    return NULL;
}

// FUNCTION: LEGORR 0x00476140
char* Container_Frame_GetAnimSetFileName(struct CntFrame* frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->lpVtbl->GetAppData(frame);
    if (appData) return appData->animSetFileName;
    return NULL;
}

// FUNCTION: LEGORR 0x00476160
struct AnimClone* Container_Frame_GetAnimClone(struct CntFrame* frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->lpVtbl->GetAppData(frame);
    if (appData) return appData->animClone;
    return NULL;
}

// FUNCTION: LEGORR 0x00476180
unsigned long Container_Frame_GetFrameCount(struct CntFrame* frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->lpVtbl->GetAppData(frame);
    if (appData) return appData->frameCount;
    return 0;
}

// FUNCTION: LEGORR 0x004761a0
float Container_Frame_GetCurrTime(struct CntFrame* frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->lpVtbl->GetAppData(frame);
    if (appData) return appData->currTime;
    return 0.0f;
}

// FUNCTION: LEGORR 0x004761c0
float Container_Frame_GetTransCo(struct CntFrame* frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->lpVtbl->GetAppData(frame);
    if (appData) return appData->transCo;
    return 0.0f;
}

// FUNCTION: LEGORR 0x004761e0
char* Container_Frame_GetSample(struct CntFrame* frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->lpVtbl->GetAppData(frame);
    if (appData) return appData->activitySample;
    return NULL;
}

// FUNCTION: LEGORR 0x00476200
unsigned long Container_Frame_GetTrigger(struct CntFrame* frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->lpVtbl->GetAppData(frame);
    if (appData) return appData->trigger;
    return 0;
}

// FUNCTION: LEGORR 0x00476220
void Container_Frame_SafeAddChild(struct CntFrame* parent,
                                  struct CntFrame* child)
{
    CntFrame* oldParent;
    float matrix[4][4];

    child->lpVtbl->GetParent(child, &oldParent);
    child->lpVtbl->GetTransform(child, oldParent, matrix);
    if (oldParent) oldParent->lpVtbl->Release(oldParent);
    parent->lpVtbl->AddChild(parent, child);
    child->lpVtbl->AddTransform(child, CNT_COMBINE_REPLACE, matrix);
}

// FUNCTION: LEGORR 0x00476270
void Container_Frame_FormatName(struct CntFrame* frame, const char* msg, ...)
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

    frame->lpVtbl->SetName(frame, name);
    Container_Frame_SetAppData(frame, NULL, NULL, NULL, NULL, name, NULL,
                               NULL, NULL, NULL, NULL);
}

// FUNCTION: LEGORR 0x004762f0
void Container_Frame_FreeName(struct CntFrame* frame)
{
    char* name = Container_Frame_GetName(frame);
    if (name) free(name);
}

// FUNCTION: LEGORR 0x00476310
char* Container_Frame_GetName(struct CntFrame* frame)
{
    Container_AppData* appData =
        (Container_AppData*)frame->lpVtbl->GetAppData(frame);
    if (appData) return appData->frameName;
    return NULL;
}

// FUNCTION: LEGORR 0x00476330
int Container_Frame_WalkTree(struct CntFrame* frame, unsigned long level,
                             ContainerWalkTreeCallback callback, void* data)
{
    CntFrameArray* children;
    CntFrame1* frame1;
    CntFrame* child;
    unsigned long count;
    unsigned long loop;
    int finished = 0;

    if (callback(frame, data)) return 1;

    frame->lpVtbl->GetChildren(frame, &children);
    count = children->lpVtbl->GetSize(children);
    for (loop = 0; loop < count; loop++) {
        children->lpVtbl->GetElement(children, loop, &frame1);
        frame1->lpVtbl->QueryInterface(frame1, &IID_CntFrame3, (void**)&child);
        frame1->lpVtbl->Release(frame1);
        if (Container_Frame_WalkTree(child, level + 1, callback, data)) {
            finished = 1;
            child->lpVtbl->Release(child);
            break;
        }
        child->lpVtbl->Release(child);
    }
    children->lpVtbl->Release(children);
    return finished;
}

// FUNCTION: LEGORR 0x00476410
int Container_Frame_SearchCallback(struct CntFrame* frame, void* data)
{
    Container_SearchData* search = (Container_SearchData*)data;
    char* name;
    unsigned long len;
    unsigned long loop;

    frame->lpVtbl->GetName(frame, &len, NULL);
    if (len - 1 == search->stringLen) {
        name = (char*)malloc(len);
        name[0] = '\0';
        frame->lpVtbl->GetName(frame, &len, name);
        for (loop = 0; loop < len; loop++) {
            if (search->string[loop] == '?') name[loop] = '?';
        }

        if (search->mode == Container_SearchMode_FirstMatch) {
            search->resultFrame = NULL;
            if (search->caseSensitive) {
                if (strcmp(name, search->string) == 0) search->resultFrame = frame;
            } else {
                if (_strcmpi(name, search->string) == 0) search->resultFrame = frame;
            }
        } else if (search->mode == Container_SearchMode_MatchCount) {
            if (search->caseSensitive) {
                if (strcmp(name, search->string) == 0) search->count++;
            } else {
                if (_strcmpi(name, search->string) == 0) search->count++;
            }
        } else if (search->mode == Container_SearchMode_NthMatch) {
            search->resultFrame = NULL;
            if (search->caseSensitive) {
                if (strcmp(name, search->string) == 0) search->count++;
            } else {
                if (_strcmpi(name, search->string) == 0) search->count++;
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
                                        struct CntFrame* frame,
                                        unsigned long* frameCount, int lws,
                                        int looping)
{
    CntAnimSet* animSet = NULL;
    CntFrame* animFrame;
    struct AnimClone* animClone = NULL;
    CntLoadMemory buffer;
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
                scene, (struct AnimCloneFrame*)frame, *frameCount);
        }
    } else {
        if ((buffer.lpMemory = file_load_binary(fname, &buffer.dSize)) != NULL) {
            if (lpD3DRM()->lpVtbl->CreateFrame(lpD3DRM(), frame,
                                               &animFrame) == 0) {
                if (lpD3DRM()->lpVtbl->CreateAnimationSet(lpD3DRM(),
                                                          &animSet) == 0) {
                    textureData.xFileName = fname;
                    textureData.flags = 0;
                    if (animSet->lpVtbl->Load(animSet, &buffer, NULL,
                                              CNT_LOAD_FROMMEMORY,
                                              Container_TextureLoadCallback,
                                              &textureData, animFrame) == 0) {
                        unsigned long count;
                        animSet->lpVtbl->SetTime(animSet, 0.0f);
                        count = Container_GetAnimFileFrameCount(
                            (const char*)buffer.lpMemory);
                        if (frameCount) *frameCount = count;
                        animClone = anim_clone_register(
                            (struct AnimCloneAnimSet*)animSet,
                            (struct AnimCloneFrame*)animFrame, count);
                    }
                    free(buffer.lpMemory);
                }
                animFrame->lpVtbl->Release(animFrame);
            }
        }
    }
    return animClone;
}

// FUNCTION: LEGORR 0x00476770
unsigned long Container_GetAnimFileFrameCount(const char* fileData)
{
    unsigned long animCount = 0;
    const char* s = strstr(fileData, "DDiScene");

    if (s) {
        while (!isdigit((unsigned char)*s)) s++;
        animCount = atoi(s);
    }
    if (animCount == 0) animCount = 1;
    return animCount;
}

// FUNCTION: LEGORR 0x004767e0
int Container_FrameLoad(const char* fname, struct CntFrame* frame)
{
    CntLoadMemory buffer;
    Container_TextureData textureData;
    int result = 0;

    if ((buffer.lpMemory = file_load_binary(fname, &buffer.dSize)) != NULL) {
        textureData.xFileName = fname;
        textureData.flags = 0;
        if (frame->lpVtbl->Load(frame, &buffer, NULL, CNT_LOAD_FROMMEMORY,
                                Container_TextureLoadCallback,
                                &textureData) == 0) {
            result = 1;
        }
        free(buffer.lpMemory);
    }
    return result;
}

// FUNCTION: LEGORR 0x00476850
struct CntMesh* Container_MeshLoad(void* fileData, unsigned long fileSize,
                                   const char* fname, struct CntFrame* frame,
                                   int noTextures)
{
    CntLoadMemory buffer;
    Container_TextureData textureData;
    CntMeshBuilder* mb;
    CntMesh* mesh;

    buffer.lpMemory = fileData;
    buffer.dSize = fileSize;
    if (lpD3DRM()->lpVtbl->CreateMeshBuilder(lpD3DRM(), &mb) == 0) {
        textureData.xFileName = fname;
        textureData.flags = 0;
        if (noTextures) textureData.flags = CONTAINER_TEXTURE_NOLOAD;
        if (mb->lpVtbl->Load(mb, &buffer, NULL, CNT_LOAD_FROMMEMORY,
                             Container_TextureLoadCallback,
                             &textureData) == 0) {
            mb->lpVtbl->CreateMesh(mb, &mesh);
            mb->lpVtbl->Release(mb);
            if (frame->lpVtbl->AddVisual(frame, (CntObject*)mesh) == 0) {
                return mesh;
            }
        }
        mb->lpVtbl->Release(mb);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00476900
static long CNT_CDECL Container_TextureLoadCallback(char* name, void* data,
                                                    CntTexture** texture)
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
                        (*texture)->lpVtbl->AddRef(*texture);
                        return 0;
                    }
                }
            }

            if ((text = Container_LoadTexture2(path, 0, NULL, NULL)) != NULL) {
                *texture = text->texture;
                Container_YFlipTexture(*texture);
                if (s_engine_feature_word & CNT_MAIN_FLAG_MIPMAPENABLED) {
                    (*texture)->lpVtbl->GenerateMIPMap(*texture, 0);
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
                    (*texture)->lpVtbl->AddDestroyCallback(
                        *texture, Container_TextureDestroyCallback, ref);
                    ref->fileName = (char*)malloc(strlen(path) + 1);
                    strcpy(ref->fileName, path);
                    ref->texture = *texture;
                    containerGlobs.textureCount++;
                }
                return 0;
            }
        }
    }
    return CNT_D3DRMERR_FILENOTFOUND;
}

// FUNCTION: LEGORR 0x00476bf0
void Container_YFlipTexture(struct CntTexture* texture)
{
    CntImage* image = texture->lpVtbl->GetImage(texture);
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
static void CNT_CDECL Container_TextureDestroyCallback(CntObject* obj,
                                                       void* arg)
{
    Container_TextureRef* ref = (Container_TextureRef*)arg;
    (void)obj;
    free(ref->texture->lpVtbl->GetAppData(ref->texture));
    free(ref->fileName);
    ref->fileName = NULL;
    ref->texture = NULL;
}
