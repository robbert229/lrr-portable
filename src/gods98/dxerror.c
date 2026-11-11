// SPDX-License-Identifier: LGPL-3.0-or-later
/* DirectX error decoder — original translation unit G98-027 (text
 * 00489d90-0048b260, .data 004ab788-004af9e1, .bss 005488a8-005490c0;
 * findings 505 and 511). The original file name is not evidenced in the
 * binary, so the stem is descriptive.
 *
 * Rebuilt in the original's shape (issue #411, ADR 0005): one file for the
 * unit, its two functions in original order, its state in one globals
 * struct laid out as the original .bss, and the C-compatible subset so the
 * matching build compiles it as C. The native port compiles it as C++.
 *
 * gods98's DirectX wrappers pass a failed HRESULT here with the module that
 * raised it and the caller's __FILE__ and __LINE__. The decoder records them,
 * names the result from per-module tables written as switches, and formats
 * the report into a 2,048-byte buffer. It returns the result unchanged.
 *
 * The literals are the original's, typing errors included ("IUknown",
 * "DDERR_NOTLOADE"). The original compiles them as pooled COMDATs (/Gf),
 * which the link places in the object's order: the module-name table, its
 * literals in reverse, then function by function each one's literals in
 * reverse order of their first use in its code. A literal used more than
 * once is stored once; each use carries the same `STRING` marker. */
#include <stdio.h>

#include "dxerror.h"

#define DXERROR_MESSAGE_SIZE 2048

typedef struct DxerrorGlobs {
    int err;                             /* 005488a8: the last result */
    int line;                            /* 005488ac: its __LINE__ */
    int module;                          /* 005488b0: DXERROR_MODULE_*, 0..7 */
    unsigned int count;                  /* 005488b4: results reported */
    const char* file;                    /* 005488b8: its __FILE__ */
    int reserved;                        /* 005488bc: never referenced */
    char message[DXERROR_MESSAGE_SIZE];  /* 005488c0: the formatted report */
} DxerrorGlobs;

/* Module names, indexed by DXERROR_MODULE_*. Its literals follow it in
 * reverse order (004ab7a8-004ab824). */
// GLOBAL: LEGORR 0x004ab788
static const char* dxerror_module_names[8] = {
    // STRING: LEGORR 0x004ab824
    "Unknown module",
    // STRING: LEGORR 0x004ab80c
    "Direct3D Retained Mode",
    // STRING: LEGORR 0x004ab7fc
    "Direct Input",
    // STRING: LEGORR 0x004ab7f0
    "Direct Draw",
    // STRING: LEGORR 0x004ab7e0
    "Direct Sound",
    // STRING: LEGORR 0x004ab7d4
    "Direct Play",
    // STRING: LEGORR 0x004ab7bc
    "Direct3D Immediate mode",
    // STRING: LEGORR 0x004ab7a8
    "IUknown Interface"
};

// GLOBAL: LEGORR 0x005488a8
static DxerrorGlobs dxerrorGlobs;

// FUNCTION: LEGORR 0x00489d90
void directx_error_message_format(const char* name, const char* description)
{
    sprintf(dxerrorGlobs.message,
        // STRING: LEGORR 0x004ab834
        "A DirectX error was set in module %s at line no %i.\nError : %s (code %i)\nDX Module : %s\nError Description : %s\n",
        dxerrorGlobs.file, dxerrorGlobs.line, name, dxerrorGlobs.err,
        dxerror_module_names[dxerrorGlobs.module], description);
}

/* Each module's results form a switch on the HRESULT (the DirectX
 * headers' values, named by the literal below each case). Where the
 * original's case bodies keep their source order (each jump-table run, and
 * D3DRM_OK after the D3DRMERR_ codes) they are alphabetical, as the DirectX
 * documentation lists them, so every table here is alphabetical. MSVC
 * places the bodies its binary search reaches next to the tests itself.
 *
 * DirectSound, DirectPlay and Direct3D Immediate Mode have no table: each
 * reports that and falls through, with no break, to the Retained Mode table.
 * Every case ends in break and the function has one return, which MSVC
 * copies into each case as `mov eax, esi`. `return err` in a case would let
 * it return the case's constant instead. MSVC also merges the three
 * identical defaults into one block, as in the original. */
// FUNCTION: LEGORR 0x00489dd0
int directx_result_diagnostic_dispatch(int err, int module, const char* file,
                                       int line)
{
    dxerrorGlobs.err = err;
    if (module <= 0 || module > 7) dxerrorGlobs.module = 0;
    else dxerrorGlobs.module = module;
    dxerrorGlobs.file = file;
    dxerrorGlobs.line = line;
    dxerrorGlobs.count++;

    switch (module) {
    case DXERROR_MODULE_DSOUND:
        // STRING: LEGORR 0x004af9ac
        directx_error_message_format("Unknown",
            // STRING: LEGORR 0x004af9b4
            "Error handling is unsupported in this module");
    case DXERROR_MODULE_DPLAY:
        directx_error_message_format("Unknown",
            "Error handling is unsupported in this module");
    case DXERROR_MODULE_D3DIM:
        directx_error_message_format("Unknown",
            "Error handling is unsupported in this module");
    case DXERROR_MODULE_D3DRM:
        switch (err) {
        case (int)0x8876030f:
            // STRING: LEGORR 0x004af988
            directx_error_message_format("D3DRMERR_BADALLOC",
                // STRING: LEGORR 0x004af99c
                "Out of memory.");
            break;
        case (int)0x88760315:
            // STRING: LEGORR 0x004af94c
            directx_error_message_format("D3DRMERR_BADDEVICE",
                // STRING: LEGORR 0x004af960
                "Device is not compatible with renderer.");
            break;
        case (int)0x88760314:
            // STRING: LEGORR 0x004af920
            directx_error_message_format("D3DRMERR_BADFILE",
                // STRING: LEGORR 0x004af934
                "Data file is corrupt.");
            break;
        case (int)0x88760317:
            // STRING: LEGORR 0x004af8ec
            directx_error_message_format("D3DRMERR_BADMAJORVERSION",
                // STRING: LEGORR 0x004af908
                "Bad DLL major version.");
            break;
        case (int)0x88760318:
            // STRING: LEGORR 0x004af8b8
            directx_error_message_format("D3DRMERR_BADMINORVERSION",
                // STRING: LEGORR 0x004af8d4
                "Bad DLL minor version.");
            break;
        case (int)0x8876030d:
            // STRING: LEGORR 0x004af884
            directx_error_message_format("D3DRMERR_BADOBJECT",
                // STRING: LEGORR 0x004af898
                "Object expected in argument.");
            break;
        case (int)0x88760325:
            // STRING: LEGORR 0x004af7e4
            directx_error_message_format("D3DRMERR_BADPMDATA",
                // STRING: LEGORR 0x004af7f8
                "The data in the X File is corrupted.\nThe conversion to a progressive mesh succeeded but\nproduced an invalid progressive mesh in\nthe X File.");
            break;
        case (int)0x8876030e:
            // STRING: LEGORR 0x004af7b4
            directx_error_message_format("D3DRMERR_BADTYPE",
                // STRING: LEGORR 0x004af7c8
                "Bad argument type passed.");
            break;
        case (int)0x88760316:
            // STRING: LEGORR 0x004af784
            directx_error_message_format("D3DRMERR_BADVALUE",
                // STRING: LEGORR 0x004af798
                "Bad argument value passed.");
            break;
        case (int)0x88760324:
            // STRING: LEGORR 0x004af6e8
            directx_error_message_format("D3DRMERR_BOXNOTSET",
                // STRING: LEGORR 0x004af6fc
                "An attempt was made to access a bounding box\n(for example, with IDirect3DRMFrame2::GetBox)\nwhen no bounding box was set on the frame.");
            break;
        case (int)0x88760320:
            // STRING: LEGORR 0x004af690
            directx_error_message_format("D3DRMERR_CONNECTIONLOST",
                // STRING: LEGORR 0x004af6a8
                "Data connection was lost during a load,\nclone, or duplicate.");
            break;
        case (int)0x88760310:
            // STRING: LEGORR 0x004af65c
            directx_error_message_format("D3DRMERR_FACEUSED",
                // STRING: LEGORR 0x004af670
                "Face already used in a mesh.");
            break;
        case (int)0x88760313:
            // STRING: LEGORR 0x004af62c
            directx_error_message_format("D3DRMERR_FILENOTFOUND",
                // STRING: LEGORR 0x004af644
                "File cannot be opened.");
            break;
        case (int)0x88760312:
            // STRING: LEGORR 0x004af608
            directx_error_message_format("D3DRMERR_NOTDONEYET",
                // STRING: LEGORR 0x004af61c
                "Unimplemented.");
            break;
        case (int)0x8876031d:
            // STRING: LEGORR 0x004af5ac
            directx_error_message_format("D3DRMERR_NOTENOUGHDATA",
                // STRING: LEGORR 0x004af5c4
                "Not enough data has been loaded to\nperform the requested operation.");
            break;
        case (int)0x88760311:
            // STRING: LEGORR 0x004af570
            directx_error_message_format("D3DRMERR_NOTFOUND",
                // STRING: LEGORR 0x004af584
                "Object not found in specified place.");
            break;
        case (int)0x8876031c:
            // STRING: LEGORR 0x004af50c
            directx_error_message_format("D3DRMERR_PENDING",
                // STRING: LEGORR 0x004af520
                "The data required to supply the requested\ninformation has not finished loading.");
            break;
        case (int)0x8876031e:
            // STRING: LEGORR 0x004af488
            directx_error_message_format("D3DRMERR_REQUESTTOOLARGE",
                // STRING: LEGORR 0x004af4a4
                "An attempt was made to set a level\nof detail in a progressive mesh greater than the maximum available.");
            break;
        case (int)0x8876031f:
            // STRING: LEGORR 0x004af3d8
            directx_error_message_format("D3DRMERR_REQUESTTOOSMALL",
                // STRING: LEGORR 0x004af3f4
                "An attempt was made to set the minimum\nrendering detail of a progressive mesh smaller than\nthe detail in the base mesh\n(the minimum for rendering).");
            break;
        case (int)0x88760319:
            // STRING: LEGORR 0x004af39c
            directx_error_message_format("D3DRMERR_UNABLETOEXECUTE",
                // STRING: LEGORR 0x004af3b8
                "Unable to carry out procedure.");
            break;
        case 0x00000000:
            // STRING: LEGORR 0x004af384
            directx_error_message_format("D3DRM_OK",
                // STRING: LEGORR 0x004af390
                "No error.");
            break;
        default:
            // STRING: LEGORR 0x004ab9fc
            directx_error_message_format("UNKNOWN ERROR",
                // STRING: LEGORR 0x004aba0c
                "Invalid error code specified");
            break;
        }
        break;
    case DXERROR_MODULE_DINPUT:
        switch (err) {
        case (int)0x800700aa:
            // STRING: LEGORR 0x004ae934
            directx_error_message_format("DIERR_ACQUIRED",
                // STRING: LEGORR 0x004ae944
                "The operation cannot be performed while\nthe device is acquired.");
            break;
        case (int)0x800704df:
            // STRING: LEGORR 0x004ae828
            directx_error_message_format("DIERR_ALREADYINITIALIZED",
                // STRING: LEGORR 0x004ae844
                "This object is already initialized");
            break;
        case (int)0x80070077:
            // STRING: LEGORR 0x004ae984
            directx_error_message_format("DIERR_BADDRIVERVER",
                // STRING: LEGORR 0x004ae998
                "The object could not be created due to an\nincompatible driver version or mismatched or\nincomplete driver components.");
            break;
        case (int)0x80070481:
            // STRING: LEGORR 0x004ae868
            directx_error_message_format("DIERR_BETADIRECTINPUTVERSION",
                // STRING: LEGORR 0x004ae888
                "The application was written for an unsupported\nprerelease version of DirectInput.");
            break;
        case (int)0x80040201:
            // STRING: LEGORR 0x004af0ac
            directx_error_message_format("DIERR_DEVICEFULL",
                // STRING: LEGORR 0x004af0c0
                "The device is full.");
            break;
        case (int)0x80040154:
            // STRING: LEGORR 0x004af0d4
            directx_error_message_format("DIERR_DEVICENOTREG",
                // STRING: LEGORR 0x004af0e8
                "The device or device instance is not registered with\nDirectInput. This value is equal to the\nREGDB_E_CLASSNOTREG standard COM return value.");
            break;
        case (int)0x80040208:
            // STRING: LEGORR 0x004aeff8
            directx_error_message_format("DIERR_EFFECTPLAYING",
                // STRING: LEGORR 0x004af00c
                "The parameters were updated in memory but were not\ndownloaded to the device because the device does not\nsupport updating an effect while it is still playing.");
            break;
        case (int)0x80004005:
            // STRING: LEGORR 0x004af1b4
            directx_error_message_format("DIERR_GENERIC",
                // STRING: LEGORR 0x004af1c4
                "An undetermined error occurred inside the DirectInput subsystem.\nThis value is equal to the E_FAIL standard\nCOM return value.");
            break;
        case (int)0x80070005:
            // STRING: LEGORR 0x004aec88
            directx_error_message_format("DIERR_HANDLEEXISTS",
                // STRING: LEGORR 0x004aec9c
                "The device already has an event notification associated\nwith it. This value is equal to the E_ACCESSDENIED\nstandard COM return value.");
            break;
        case (int)0x80040204:
            // STRING: LEGORR 0x004aef90
            directx_error_message_format("DIERR_HASEFFECTS",
                // STRING: LEGORR 0x004aefa4
                "The device cannot be reinitialized because there\nare still effects attached to it");
            break;
        case (int)0x80040206:
            // STRING: LEGORR 0x004aeebc
            directx_error_message_format("DIERR_INCOMPLETEEFFECT",
                // STRING: LEGORR 0x004aeed4
                "The effect could not be downloaded because essential\ninformation is missing. For example, no axes have\nbeen associated with the effect, or no type-specific\ninformation has been supplied");
            break;
        case (int)0x8007001e:
            // STRING: LEGORR 0x004aeaf0
            directx_error_message_format("DIERR_INPUTLOST",
                // STRING: LEGORR 0x004aeb00
                "Access to the input device has been lost. It must be reacquired.");
            break;
        case (int)0x80070057:
            // STRING: LEGORR 0x004aea10
            directx_error_message_format("DIERR_INVALIDPARAM",
                // STRING: LEGORR 0x004aea24
                "An invalid parameter was passed to the returning function, or the object was not in a state that permitted the function to be called. This value is equal to the E_INVALIDARG standard COM return value.");
            break;
        case (int)0x80040202:
            // STRING: LEGORR 0x004aee70
            directx_error_message_format("DIERR_MOREDATA",
                // STRING: LEGORR 0x004aee80
                "Not all the requested information fitted into the buffer");
            break;
        case (int)0x80040110:
            // STRING: LEGORR 0x004af174
            directx_error_message_format("DIERR_NOAGGREGATION",
                // STRING: LEGORR 0x004af188
                "This object does not support aggregation.");
            break;
        case (int)0x80004002:
            // STRING: LEGORR 0x004af244
            directx_error_message_format("DIERR_NOINTERFACE",
                // STRING: LEGORR 0x004af258
                "The specified interface is not supported by\nthe object. This value is equal to the E_NOINTERFACE\nstandard COM return value.");
            break;
        case (int)0x8007000c:
            // STRING: LEGORR 0x004aec30
            directx_error_message_format("DIERR_NOTACQUIRED",
                // STRING: LEGORR 0x004aec44
                "The operation cannot be performed unless\nthe device is acquired.");
            break;
        case (int)0x80040207:
            // STRING: LEGORR 0x004aee08
            directx_error_message_format("DIERR_NOTBUFFERED",
                // STRING: LEGORR 0x004aee1c
                "The device is not buffered.\nSet the DIPROP_BUFFERSIZE property to enable buffering");
            break;
        case (int)0x80040203:
            // STRING: LEGORR 0x004aedd4
            directx_error_message_format("DIERR_NOTDOWNLOADED",
                // STRING: LEGORR 0x004aede8
                "The effect is not downloaded");
            break;
        case (int)0x80040205:
            // STRING: LEGORR 0x004aed5c
            directx_error_message_format("DIERR_NOTEXCLUSIVEACQUIRED",
                // STRING: LEGORR 0x004aed78
                "The operation cannot be performed unless the device\nis acquired in DISCL_EXCLUSIVE mode.");
            break;
        case (int)0x80070002:
            // STRING: LEGORR 0x004aed24
            directx_error_message_format("DIERR_NOTFOUND",
                // STRING: LEGORR 0x004aed34
                "The requested object does not exist.");
            break;
        case (int)0x80070015:
            // STRING: LEGORR 0x004aeb44
            directx_error_message_format("DIERR_NOTINITIALIZED",
                // STRING: LEGORR 0x004aeb5c
                "This object has not been initialized.");
            break;
        case (int)0x8007047e:
            // STRING: LEGORR 0x004ae8dc
            directx_error_message_format("DIERR_OLDDIRECTINPUTVERSION",
                // STRING: LEGORR 0x004ae8f8
                "The application requires a newer version of DirectInput.");
            break;
        case (int)0x8007000e:
            // STRING: LEGORR 0x004aeb84
            directx_error_message_format("DIERR_OUTOFMEMORY",
                // STRING: LEGORR 0x004aeb98
                "The DirectInput subsystem couldn't allocate\nsufficient memory to complete the call.\nThis value is equal to the E_OUTOFMEMORY\nstandard COM return value.");
            break;
        case (int)0x80004001:
            // STRING: LEGORR 0x004af2d4
            directx_error_message_format("DIERR_UNSUPPORTED",
                // STRING: LEGORR 0x004af2e8
                "The function called is not supported at this time.\nThis value is equal to the E_NOTIMPL\nstandard COM return value.");
            break;
        case 0x00000001:
            // STRING: LEGORR 0x004ae734
            directx_error_message_format("DI_BUFFEROVERFLOW",
                // STRING: LEGORR 0x004ae748
                "The device buffer overflowed and some input was lost.\nThis value is equal to the S_FALSE\nstandard COM return value.");
            break;
        case 0x00000003:
            // STRING: LEGORR 0x004ae67c
            directx_error_message_format("DI_DOWNLOADSKIPPED",
                // STRING: LEGORR 0x004ae690
                "The parameters of the effect were successfully updated,\nbut the effect could not be downloaded because the associated\ndevice was not acquired in exclusive mode.");
            break;
        case 0x00000004:
            // STRING: LEGORR 0x004ae614
            directx_error_message_format("DI_EFFECTRESTARTED",
                // STRING: LEGORR 0x004ae628
                "The effect was stopped,\nthe parameters were updated,\nand the effect was restarted.");
            break;
        case 0x00000000:
            // STRING: LEGORR 0x004ae7bc
            directx_error_message_format("DI_OK",
                // STRING: LEGORR 0x004ae7c4
                "The operation completed successfully.\nThis value is equal to the S_OK standard\nCOM return value.");
            break;
        case 0x00000002:
            // STRING: LEGORR 0x004ae548
            directx_error_message_format("DI_POLLEDDEVICE",
                // STRING: LEGORR 0x004ae558
                "The device is a polled device.\nAs a result, device buffering will not collect\nany data and event notifications will not be signaled\nuntil the IDirectInputDevice2::Poll method is called.");
            break;
        case 0x00000008:
            // STRING: LEGORR 0x004ae490
            directx_error_message_format("DI_TRUNCATED",
                // STRING: LEGORR 0x004ae4a0
                "The parameters of the effect were successfully updated,\nbut some of them were beyond the capabilities of the\ndevice and were truncated to the nearest supported value.");
            break;
        case 0x0000000c:
            // STRING: LEGORR 0x004ae448
            directx_error_message_format("DI_TRUNCATEDANDRESTARTED",
                // STRING: LEGORR 0x004ae464
                "Equal to DI_EFFECTRESTARTED | DI_TRUNCATED");
            break;
        case (int)0x8000000a:
            // STRING: LEGORR 0x004af35c
            directx_error_message_format("E_PENDING",
                // STRING: LEGORR 0x004af368
                "Data is not yet availabe.");
            break;
        default:
            directx_error_message_format("UNKNOWN ERROR",
                "Invalid error code specified");
            break;
        }
        break;
    case DXERROR_MODULE_DDRAW:
        switch (err) {
        case (int)0x88760005:
            // STRING: LEGORR 0x004ae244
            directx_error_message_format("DDERR_ALREADYINITIALIZED",
                // STRING: LEGORR 0x004ae260
                "The object has already been initialized.");
            break;
        case (int)0x8876023e:
            // STRING: LEGORR 0x004acacc
            directx_error_message_format("DDERR_BLTFASTCANTCLIP",
                // STRING: LEGORR 0x004acae4
                "A DirectDrawClipper object is attached to a source\nsurface that has passed into a call to the\nIDirectDrawSurface3::BltFast method.");
            break;
        case (int)0x8876000a:
            // STRING: LEGORR 0x004ae1ec
            directx_error_message_format("DDERR_CANNOTATTACHSURFACE",
                // STRING: LEGORR 0x004ae208
                "A surface cannot be attached to\nanother requested surface.");
            break;
        case (int)0x88760014:
            // STRING: LEGORR 0x004ae190
            directx_error_message_format("DDERR_CANNOTDETACHSURFACE",
                // STRING: LEGORR 0x004ae1ac
                "A surface cannot be detached from another\nrequested surface.");
            break;
        case (int)0x88760249:
            // STRING: LEGORR 0x004aca80
            directx_error_message_format("DDERR_CANTCREATEDC",
                // STRING: LEGORR 0x004aca94
                "Windows cannot create any more device\ncontexts (DCs).");
            break;
        case (int)0x88760247:
            // STRING: LEGORR 0x004aca10
            directx_error_message_format("DDERR_CANTDUPLICATE",
                // STRING: LEGORR 0x004aca24
                "Primary and 3-D surfaces, or surfaces that\nare implicitly created, cannot be duplicated.");
            break;
        case (int)0x887601b3:
            // STRING: LEGORR 0x004acf54
            directx_error_message_format("DDERR_CANTLOCKSURFACE",
                // STRING: LEGORR 0x004acf6c
                "Access to this surface is refused because an\nattempt was made to lock the primary\nsurface without DCI support.");
            break;
        case (int)0x88760280:
            // STRING: LEGORR 0x004abb80
            directx_error_message_format("DDERR_CANTPAGELOCK",
                // STRING: LEGORR 0x004abb94
                "An attempt to page lock a surface failed.\nPage lock will not work on a display-memory surface or\nan emulated primary surface.");
            break;
        case (int)0x88760294:
            // STRING: LEGORR 0x004abae4
            directx_error_message_format("DDERR_CANTPAGEUNLOCK",
                // STRING: LEGORR 0x004abafc
                "An attempt to page unlock a surface failed.\nPage unlock will not work on a display-memory\nsurface or an emulated primary surface.");
            break;
        case (int)0x88760237:
            // STRING: LEGORR 0x004ac980
            directx_error_message_format("DDERR_CLIPPERISUSINGHWND",
                // STRING: LEGORR 0x004ac99c
                "An attempt was made to set a clip list for a\nDirectDrawClipper object that is already monitoring\na window handle.");
            break;
        case (int)0x88760190:
            // STRING: LEGORR 0x004ad148
            directx_error_message_format("DDERR_COLORKEYNOTSET",
                // STRING: LEGORR 0x004ad160
                "No source color key is specified for this operation.");
            break;
        case (int)0x88760028:
            // STRING: LEGORR 0x004ae154
            directx_error_message_format("DDERR_CURRENTLYNOTAVAIL",
                // STRING: LEGORR 0x004ae16c
                "No support is currently available.");
            break;
        case (int)0x8876026c:
            // STRING: LEGORR 0x004ac8f4
            directx_error_message_format("DDERR_DCALREADYCREATED",
                // STRING: LEGORR 0x004ac90c
                "A device context (DC) has already been returned\nfor this surface. Only one DC can be retrieved for each surface.");
            break;
        case (int)0x887602bb:
            // STRING: LEGORR 0x004ab950
            directx_error_message_format("DDERR_DEVICEDOESNTOWNSURFAC",
                // STRING: LEGORR 0x004ab96c
                "Surfaces created by one direct draw device cannot\nbe used directly by another direct draw device");
            break;
        case (int)0x88760232:
            // STRING: LEGORR 0x004ac87c
            directx_error_message_format("DDERR_DIRECTDRAWALREADYCREATED",
                // STRING: LEGORR 0x004ac89c
                "A DirectDraw object representing this driver\nhas already been created for this process.");
            break;
        case (int)0x88760037:
            // STRING: LEGORR 0x004ae0fc
            directx_error_message_format("DDERR_EXCEPTION",
                // STRING: LEGORR 0x004ae10c
                "An exception was encountered while performing the requested operation.");
            break;
        case (int)0x88760245:
            // STRING: LEGORR 0x004ac804
            directx_error_message_format("DDERR_EXCLUSIVEMODEALREADYSET",
                // STRING: LEGORR 0x004ac824
                "An attempt was made to set the cooperative level\nwhen it was already set to exclusive.");
            break;
        case (int)0x80004005:
            // STRING: LEGORR 0x004ae3dc
            directx_error_message_format("DDERR_GENERIC",
                // STRING: LEGORR 0x004ae3ec
                "There is an undefined error condition.");
            break;
        case (int)0x8876005a:
            // STRING: LEGORR 0x004ae094
            directx_error_message_format("DDERR_HEIGHTALIGN",
                // STRING: LEGORR 0x004ae0a8
                "The height of the provided rectangle is not\na multiple of the required alignment.");
            break;
        case (int)0x8876023b:
            // STRING: LEGORR 0x004ac760
            directx_error_message_format("DDERR_HWNDALREADYSET",
                // STRING: LEGORR 0x004ac778
                "The DirectDraw cooperative level window handle\nhas already been set. It cannot be reset while the process\nhas surfaces or palettes created.");
            break;
        case (int)0x8876023a:
            // STRING: LEGORR 0x004ac6cc
            directx_error_message_format("DDERR_HWNDSUBCLASSED",
                // STRING: LEGORR 0x004ac6e4
                "DirectDraw is prevented from restoring state\nbecause the DirectDraw cooperative level window\nhandle has been subclassed.");
            break;
        case (int)0x8876024c:
            // STRING: LEGORR 0x004ac668
            directx_error_message_format("DDERR_IMPLICITLYCREATED",
                // STRING: LEGORR 0x004ac680
                "The surface cannot be restored because\nit is an implicitly created surface.");
            break;
        case (int)0x8876005f:
            // STRING: LEGORR 0x004ae020
            directx_error_message_format("DDERR_INCOMPATIBLEPRIMARY",
                // STRING: LEGORR 0x004ae03c
                "The primary surface creation request does\nnot match with the existing primary surface.");
            break;
        case (int)0x88760064:
            // STRING: LEGORR 0x004adfb8
            directx_error_message_format("DDERR_INVALIDCAPS",
                // STRING: LEGORR 0x004adfcc
                "One or more of the capability bits passed to\nthe callback function are incorrect.");
            break;
        case (int)0x8876006e:
            // STRING: LEGORR 0x004adf6c
            directx_error_message_format("DDERR_INVALIDCLIPLIST",
                // STRING: LEGORR 0x004adf84
                "DirectDraw does not support the provided clip list.");
            break;
        case (int)0x88760231:
            // STRING: LEGORR 0x004ac5d0
            directx_error_message_format("DDERR_INVALIDDIRECTDRAWGUID",
                // STRING: LEGORR 0x004ac5ec
                "The globally unique identifier (GUID) passed to the\nDirectDrawCreate function is not a valid DirectDraw\ndriver identifier.");
            break;
        case (int)0x88760078:
            // STRING: LEGORR 0x004adf28
            directx_error_message_format("DDERR_INVALIDMODE",
                // STRING: LEGORR 0x004adf3c
                "DirectDraw does not support the requested mode.");
            break;
        case (int)0x88760082:
            // STRING: LEGORR 0x004adecc
            directx_error_message_format("DDERR_INVALIDOBJECT",
                // STRING: LEGORR 0x004adee0
                "DirectDraw received a pointer that was an\ninvalid DirectDraw object.");
            break;
        case (int)0x80070057:
            // STRING: LEGORR 0x004ae28c
            directx_error_message_format("DDERR_INVALIDPARAMS",
                // STRING: LEGORR 0x004ae2a0
                "One or more of the parameters passed to the\nmethod are incorrect.");
            break;
        case (int)0x88760091:
            // STRING: LEGORR 0x004ade84
            directx_error_message_format("DDERR_INVALIDPIXELFORMAT",
                // STRING: LEGORR 0x004adea0
                "The pixel format was invalid as specified.");
            break;
        case (int)0x88760243:
            // STRING: LEGORR 0x004ac574
            directx_error_message_format("DDERR_INVALIDPOSITION",
                // STRING: LEGORR 0x004ac58c
                "The position of the overlay on the destination is no longer legal.");
            break;
        case (int)0x88760096:
            // STRING: LEGORR 0x004ade4c
            directx_error_message_format("DDERR_INVALIDRECT",
                // STRING: LEGORR 0x004ade60
                "The provided rectangle was invalid.");
            break;
        case (int)0x88760250:
            // STRING: LEGORR 0x004ac4fc
            directx_error_message_format("DDERR_INVALIDSURFACETYPE",
                // STRING: LEGORR 0x004ac518
                "The requested operation could not be performed\nbecause the surface was of the wrong type.");
            break;
        case (int)0x887600a0:
            // STRING: LEGORR 0x004adde0
            directx_error_message_format("DDERR_LOCKEDSURFACES",
                // STRING: LEGORR 0x004addf8
                "One or more surfaces are locked, causing the failure of\nthe requested operation.");
            break;
        case (int)0x887602b2:
            // STRING: LEGORR 0x004aba2c
            directx_error_message_format("DDERR_MOREDAT",
                // STRING: LEGORR 0x004aba3c
                "There is more data available than the\nspecified buffer size can hold");
            break;
        case (int)0x887600aa:
            // STRING: LEGORR 0x004adda8
            directx_error_message_format("DDERR_NO3D",
                // STRING: LEGORR 0x004addb4
                "No 3-D hardware or emulation is present.");
            break;
        case (int)0x887600b4:
            // STRING: LEGORR 0x004add30
            directx_error_message_format("DDERR_NOALPHAHW",
                // STRING: LEGORR 0x004add40
                "No alpha acceleration hardware is present or\navailable, causing the failure of\nthe requested operation.");
            break;
        case (int)0x8876023f:
            // STRING: LEGORR 0x004ac4cc
            directx_error_message_format("DDERR_NOBLTHW",
                // STRING: LEGORR 0x004ac4dc
                "No blitter hardware is present.");
            break;
        case (int)0x887600cd:
            // STRING: LEGORR 0x004add00
            directx_error_message_format("DDERR_NOCLIPLIST",
                // STRING: LEGORR 0x004add14
                "No clip list is available.");
            break;
        case (int)0x88760238:
            // STRING: LEGORR 0x004ac474
            directx_error_message_format("DDERR_NOCLIPPERATTACHED",
                // STRING: LEGORR 0x004ac48c
                "No DirectDrawClipper object is attached\nto the surface object.");
            break;
        case (int)0x887600d2:
            // STRING: LEGORR 0x004adbc4
            directx_error_message_format("DDERR_NOCOLORCONVHW",
                // STRING: LEGORR 0x004adbd8
                "The operation cannot be carried out because no\ncolor-conversion hardware is present or available.");
            break;
        case (int)0x887600d7:
            // STRING: LEGORR 0x004adcb8
            directx_error_message_format("DDERR_NOCOLORKEY",
                // STRING: LEGORR 0x004adccc
                "The surface does not currently have a color key.");
            break;
        case (int)0x887600dc:
            // STRING: LEGORR 0x004adb48
            directx_error_message_format("DDERR_NOCOLORKEYHW",
                // STRING: LEGORR 0x004adb5c
                "The operation cannot be carried out because there is\nno hardware support for the destination color key.");
            break;
        case (int)0x887600d4:
            // STRING: LEGORR 0x004adc3c
            directx_error_message_format("DDERR_NOCOOPERATIVELEVELSET",
                // STRING: LEGORR 0x004adc58
                "A create function is called without the\nIDirectDraw2::SetCooperativeLevel method being called.");
            break;
        case (int)0x8876024a:
            // STRING: LEGORR 0x004ac438
            directx_error_message_format("DDERR_NODC",
                // STRING: LEGORR 0x004ac444
                "No DC has ever been created for this surface.");
            break;
        case (int)0x88760240:
            // STRING: LEGORR 0x004ac3e8
            directx_error_message_format("DDERR_NODDROPSHW",
                // STRING: LEGORR 0x004ac3fc
                "No DirectDraw raster operation (ROP) hardware is available.");
            break;
        case (int)0x88760233:
            // STRING: LEGORR 0x004ac36c
            directx_error_message_format("DDERR_NODIRECTDRAWHW",
                // STRING: LEGORR 0x004ac384
                "Hardware-only DirectDraw object creation is not possible;\nthe driver does not support any hardware.");
            break;
        case (int)0x887600de:
            // STRING: LEGORR 0x004adae8
            directx_error_message_format("DDERR_NODIRECTDRAWSUPPORT",
                // STRING: LEGORR 0x004adb04
                "DirectDraw support is not possible\nwith the current display driver.");
            break;
        case (int)0x88760235:
            // STRING: LEGORR 0x004ac330
            directx_error_message_format("DDERR_NOEMULATION",
                // STRING: LEGORR 0x004ac344
                "Software emulation is not available.");
            break;
        case (int)0x887600e1:
            // STRING: LEGORR 0x004ada5c
            directx_error_message_format("DDERR_NOEXCLUSIVEMODE",
                // STRING: LEGORR 0x004ada74
                "The operation requires the application to have exclusive mode,\nbut the application does not have exclusive mode.");
            break;
        case (int)0x887600e6:
            // STRING: LEGORR 0x004ada20
            directx_error_message_format("DDERR_NOFLIPHW",
                // STRING: LEGORR 0x004ada30
                "Flipping visible surfaces is not supported.");
            break;
        case (int)0x887600f0:
            // STRING: LEGORR 0x004ada00
            directx_error_message_format("DDERR_NOGDI",
                // STRING: LEGORR 0x004ada0c
                "No GDI is present.");
            break;
        case (int)0x88760239:
            // STRING: LEGORR 0x004ac29c
            directx_error_message_format("DDERR_NOHWND",
                // STRING: LEGORR 0x004ac2ac
                "Clipper notification requires a window handle,\nor no window handle has been previously set as\nthe cooperative level window handle.");
            break;
        case (int)0x8876024f:
            // STRING: LEGORR 0x004ac220
            directx_error_message_format("DDERR_NOMIPMAPHW",
                // STRING: LEGORR 0x004ac234
                "The operation cannot be carried out because no mipmap\ntexture mapping hardware is present or available.");
            break;
        case (int)0x887600fa:
            // STRING: LEGORR 0x004ad990
            directx_error_message_format("DDERR_NOMIRRORHW",
                // STRING: LEGORR 0x004ad9a4
                "The operation cannot be carried out because no mirroring\nhardware is present or available.");
            break;
        case (int)0x88760276:
            // STRING: LEGORR 0x004abc14
            directx_error_message_format("DDERR_NONONLOCALVIDME",
                // STRING: LEGORR 0x004abc2c
                "An attempt was made to allocate non-local video\nmemory from a device that does not support non-local\nvideo memory");
            break;
        case (int)0x88760258:
            // STRING: LEGORR 0x004ac1dc
            directx_error_message_format("DDERR_NOOPTIMIZEH",
                // STRING: LEGORR 0x004ac1f0
                "The device does not support optimized surfaces");
            break;
        case (int)0x88760242:
            // STRING: LEGORR 0x004ac114
            directx_error_message_format("DDERR_NOOVERLAYDEST",
                // STRING: LEGORR 0x004ac128
                "The IDirectDrawSurface3::GetOverlayPosition method is called\non an overlay that the IDirectDrawSurface3::UpdateOverlay method\nhas not been called on to establish a destination.");
            break;
        case (int)0x88760104:
            // STRING: LEGORR 0x004ad8ec
            directx_error_message_format("DDERR_NOOVERLAYHW",
                // STRING: LEGORR 0x004ad900
                "The operation cannot be carried out because no\noverlay hardware is present or available.");
            break;
        case (int)0x8876023c:
            // STRING: LEGORR 0x004ac0cc
            directx_error_message_format("DDERR_NOPALETTEATTACHED",
                // STRING: LEGORR 0x004ac0e4
                "No palette object is attached to this surface.");
            break;
        case (int)0x8876023d:
            // STRING: LEGORR 0x004ac07c
            directx_error_message_format("DDERR_NOPALETTEHW",
                // STRING: LEGORR 0x004ac090
                "There is no hardware support for 16- or 256-color palettes.");
            break;
        case (int)0x88760118:
            // STRING: LEGORR 0x004ad868
            directx_error_message_format("DDERR_NORASTEROPHW",
                // STRING: LEGORR 0x004ad87c
                "The operation cannot be carried out because no appropriate\nraster operation hardware is present or available.");
            break;
        case (int)0x88760122:
            // STRING: LEGORR 0x004ad7f8
            directx_error_message_format("DDERR_NOROTATIONHW",
                // STRING: LEGORR 0x004ad80c
                "The operation cannot be carried out because\nno rotation hardware is present or available.");
            break;
        case (int)0x88760136:
            // STRING: LEGORR 0x004ad788
            directx_error_message_format("DDERR_NOSTRETCHHW",
                // STRING: LEGORR 0x004ad79c
                "The operation cannot be carried out because\nthere is no hardware support for stretching.");
            break;
        case (int)0x8876013c:
            // STRING: LEGORR 0x004ad5c4
            directx_error_message_format("DDERR_NOT4BITCOLOR",
                // STRING: LEGORR 0x004ad5d8
                "The DirectDrawSurface object is not using a 4-bit color palette\nand the requested operation requires a 4-bit color palette.");
            break;
        case (int)0x8876013d:
            // STRING: LEGORR 0x004ad654
            directx_error_message_format("DDERR_NOT4BITCOLORINDEX",
                // STRING: LEGORR 0x004ad66c
                "The DirectDrawSurface object is not using a 4-bit color index\npalette and the requested operation requires a\n4-bit color index palette.");
            break;
        case (int)0x88760140:
            // STRING: LEGORR 0x004ad6f4
            directx_error_message_format("DDERR_NOT8BITCOLOR",
                // STRING: LEGORR 0x004ad708
                "The DirectDrawSurface object is not using an 8-bit color\npalette and the requested operation requires\nan 8-bit color palette.");
            break;
        case (int)0x88760244:
            // STRING: LEGORR 0x004ac024
            directx_error_message_format("DDERR_NOTAOVERLAYSURFACE",
                // STRING: LEGORR 0x004ac040
                "An overlay component is called for a non-overlay surface.");
            break;
        case (int)0x8876014a:
            // STRING: LEGORR 0x004ad54c
            directx_error_message_format("DDERR_NOTEXTUREHW",
                // STRING: LEGORR 0x004ad560
                "The operation cannot be carried out because no\ntexture-mapping hardware is present or available.");
            break;
        case (int)0x88760246:
            // STRING: LEGORR 0x004abfcc
            directx_error_message_format("DDERR_NOTFLIPPABLE",
                // STRING: LEGORR 0x004abfe0
                "An attempt has been made to flip\na surface that cannot be flipped.");
            break;
        case (int)0x887600ff:
            // STRING: LEGORR 0x004ad95c
            directx_error_message_format("DDERR_NOTFOUND",
                // STRING: LEGORR 0x004ad96c
                "The requested item was not found.");
            break;
        case (int)0x800401f0:
            // STRING: LEGORR 0x004ae33c
            directx_error_message_format("DDERR_NOTINITIALIZED",
                // STRING: LEGORR 0x004ae354
                "An attempt was made to call an interface method\nof a DirectDraw object created by CoCreateInstance before\nthe object was initialized.");
            break;
        case (int)0x88760259:
            // STRING: LEGORR 0x004abf68
            directx_error_message_format("DDERR_NOTLOADE",
                // STRING: LEGORR 0x004abf78
                "The surface is an optimized surface,\nbut it has not yet been allocated any memory");
            break;
        case (int)0x88760248:
            // STRING: LEGORR 0x004abf1c
            directx_error_message_format("DDERR_NOTLOCKED",
                // STRING: LEGORR 0x004abf2c
                "An attempt is made to unlock a\nsurface that was not locked.");
            break;
        case (int)0x887602a8:
            // STRING: LEGORR 0x004aba84
            directx_error_message_format("DDERR_NOTPAGELOCKED",
                // STRING: LEGORR 0x004aba98
                "An attempt is made to page unlock a\nsurface with no outstanding page locks.");
            break;
        case (int)0x8876024d:
            // STRING: LEGORR 0x004abed0
            directx_error_message_format("DDERR_NOTPALETTIZED",
                // STRING: LEGORR 0x004abee4
                "The surface being used is not a\npalette-based surface.");
            break;
        case (int)0x8876014f:
            // STRING: LEGORR 0x004ad4c4
            directx_error_message_format("DDERR_NOVSYNCHW",
                // STRING: LEGORR 0x004ad4d4
                "The operation cannot be carried out because\nthere is no hardware support for vertical\nblank synchronized operations.");
            break;
        case (int)0x88760154:
            // STRING: LEGORR 0x004ad408
            directx_error_message_format("DDERR_NOZBUFFERHW",
                // STRING: LEGORR 0x004ad41c
                "The operation to create a z-buffer in display memory\nor to perform a blit using a z-buffer cannot be\ncarried out because there is no hardware support\nfor z-buffers.");
            break;
        case (int)0x8876015e:
            // STRING: LEGORR 0x004ad378
            directx_error_message_format("DDERR_NOZOVERLAYHW",
                // STRING: LEGORR 0x004ad38c
                "The overlay surfaces cannot be z-layered based on the\nz-order because the hardware does not support\nz-ordering of overlays.");
            break;
        case (int)0x88760168:
            // STRING: LEGORR 0x004ad31c
            directx_error_message_format("DDERR_OUTOFCAPS",
                // STRING: LEGORR 0x004ad32c
                "The hardware needed for the requested\noperation has already been allocated.");
            break;
        case (int)0x8007000e:
            // STRING: LEGORR 0x004ae2e4
            directx_error_message_format("DDERR_OUTOFMEMORY",
                // STRING: LEGORR 0x004ae2f8
                "DirectDraw does not have enough\nmemory to perform the operation.");
            break;
        case (int)0x8876017c:
            // STRING: LEGORR 0x004ad2b8
            directx_error_message_format("DDERR_OUTOFVIDEOMEMORY",
                // STRING: LEGORR 0x004ad2d0
                "DirectDraw does not have enough display\nmemory to perform the operation.");
            break;
        case (int)0x8876017e:
            // STRING: LEGORR 0x004ad270
            directx_error_message_format("DDERR_OVERLAYCANTCLIP",
                // STRING: LEGORR 0x004ad288
                "The hardware does not support clipped overlays.");
            break;
        case (int)0x88760180:
            // STRING: LEGORR 0x004ad200
            directx_error_message_format("DDERR_OVERLAYCOLORKEYONLYONEACTIVE",
                // STRING: LEGORR 0x004ad224
                "An attempt was made to have more than one\ncolor key active on an overlay.");
            break;
        case (int)0x88760241:
            // STRING: LEGORR 0x004abe64
            directx_error_message_format("DDERR_OVERLAYNOTVISIBLE",
                // STRING: LEGORR 0x004abe7c
                "The IDirectDrawSurface3::GetOverlayPosition method\nis called on a hidden overlay.");
            break;
        case (int)0x88760183:
            // STRING: LEGORR 0x004ad198
            directx_error_message_format("DDERR_PALETTEBUSY",
                // STRING: LEGORR 0x004ad1ac
                "Access to this palette is refused because\nthe palette is locked by another thread.");
            break;
        case (int)0x88760234:
            // STRING: LEGORR 0x004abe0c
            directx_error_message_format("DDERR_PRIMARYSURFACEALREADYEXISTS",
                // STRING: LEGORR 0x004abe30
                "This process has already created a primary surface.");
            break;
        case (int)0x88760236:
            // STRING: LEGORR 0x004abda4
            directx_error_message_format("DDERR_REGIONTOOSMALL",
                // STRING: LEGORR 0x004abdbc
                "The region passed to the\nIDirectDrawClipper::GetClipList\nmethod is too small.");
            break;
        case (int)0x8876019a:
            // STRING: LEGORR 0x004ad0cc
            directx_error_message_format("DDERR_SURFACEALREADYATTACHED",
                // STRING: LEGORR 0x004ad0ec
                "An attempt was made to attach a surface to another\nsurface to which it is already attached.");
            break;
        case (int)0x887601a4:
            // STRING: LEGORR 0x004ad044
            directx_error_message_format("DDERR_SURFACEALREADYDEPENDENT",
                // STRING: LEGORR 0x004ad064
                "An attempt was made to make a surface a dependency\nof another surface to which it is\nalready dependent.");
            break;
        case (int)0x887601ae:
            // STRING: LEGORR 0x004acfdc
            directx_error_message_format("DDERR_SURFACEBUSY",
                // STRING: LEGORR 0x004acff0
                "Access to the surface is refused because\nthe surface is locked by another thread.");
            break;
        case (int)0x887601b8:
            // STRING: LEGORR 0x004acef8
            directx_error_message_format("DDERR_SURFACEISOBSCURED",
                // STRING: LEGORR 0x004acf10
                "Access to the surface is refused\nbecause the surface is obscured.");
            break;
        case (int)0x887601c2:
            // STRING: LEGORR 0x004ace24
            directx_error_message_format("DDERR_SURFACELOST",
                // STRING: LEGORR 0x004ace38
                "Access to the surface is refused because the surface\nmemory is gone. The DirectDrawSurface object representing this\nsurface should have the IDirectDrawSurface3::Restore\nmethod called on it.");
            break;
        case (int)0x887601cc:
            // STRING: LEGORR 0x004acde0
            directx_error_message_format("DDERR_SURFACENOTATTACHED",
                // STRING: LEGORR 0x004acdfc
                "The requested surface is not attached.");
            break;
        case (int)0x887601d6:
            // STRING: LEGORR 0x004acd98
            directx_error_message_format("DDERR_TOOBIGHEIGHT",
                // STRING: LEGORR 0x004acdac
                "The height requested by DirectDraw is too large.");
            break;
        case (int)0x887601e0:
            // STRING: LEGORR 0x004acd24
            directx_error_message_format("DDERR_TOOBIGSIZE",
                // STRING: LEGORR 0x004acd38
                "The size requested by DirectDraw is too large.\nHowever, the individual height and\nwidth are OK.");
            break;
        case (int)0x887601ea:
            // STRING: LEGORR 0x004acce0
            directx_error_message_format("DDERR_TOOBIGWIDTH",
                // STRING: LEGORR 0x004accf4
                "The width requested by DirectDraw\nis too large.");
            break;
        case (int)0x80004001:
            // STRING: LEGORR 0x004ae414
            directx_error_message_format("DDERR_UNSUPPORTED",
                // STRING: LEGORR 0x004ae428
                "The operation is not supported.");
            break;
        case (int)0x887601fe:
            // STRING: LEGORR 0x004acc8c
            directx_error_message_format("DDERR_UNSUPPORTEDFORMAT",
                // STRING: LEGORR 0x004acca4
                "The FourCC format requested is\nnot supported by DirectDraw.");
            break;
        case (int)0x88760208:
            // STRING: LEGORR 0x004acc28
            directx_error_message_format("DDERR_UNSUPPORTEDMASK",
                // STRING: LEGORR 0x004acc40
                "The bitmask in the pixel format requested\nis not supported by DirectDraw.");
            break;
        case (int)0x8876024e:
            // STRING: LEGORR 0x004abd58
            directx_error_message_format("DDERR_UNSUPPORTEDMODE",
                // STRING: LEGORR 0x004abd70
                "The display is currently in\nan unsupported mode.");
            break;
        case (int)0x88760219:
            // STRING: LEGORR 0x004acbe4
            directx_error_message_format("DDERR_VERTICALBLANKINPROGRESS",
                // STRING: LEGORR 0x004acc04
                "A vertical blank is in progress.");
            break;
        case (int)0x887602b7:
            // STRING: LEGORR 0x004ab91c
            directx_error_message_format("DDERR_VIDEONOTACTIV",
                // STRING: LEGORR 0x004ab930
                "The video port is not active");
            break;
        case (int)0x8876021c:
            // STRING: LEGORR 0x004acb68
            directx_error_message_format("DDERR_WASSTILLDRAWING",
                // STRING: LEGORR 0x004acb80
                "The previous blit operation that is transferring\ninformation to or from this\nsurface is incomplete.");
            break;
        case (int)0x8876024b:
            // STRING: LEGORR 0x004abcfc
            directx_error_message_format("DDERR_WRONGMODE",
                // STRING: LEGORR 0x004abd0c
                "This surface cannot be restored because it\nwas created in a different mode.");
            break;
        case (int)0x88760230:
            // STRING: LEGORR 0x004abca0
            directx_error_message_format("DDERR_XALIGN",
                // STRING: LEGORR 0x004abcb0
                "The provided rectangle was not horizontally\naligned on a required boundary.");
            break;
        case 0x00000000:
            // STRING: LEGORR 0x004ab9d0
            directx_error_message_format("DD_OK",
                // STRING: LEGORR 0x004ab9d8
                "The request completed successfully.");
            break;
        default:
            directx_error_message_format("UNKNOWN ERROR",
                "Invalid error code specified");
            break;
        }
        break;
    case DXERROR_MODULE_IUNKNOWN:
        // STRING: LEGORR 0x004ab8dc
        directx_error_message_format("QUERY_INTERFACE",
            // STRING: LEGORR 0x004ab8ec
            "The requested interface could not be obtained");
        break;
    default:
        directx_error_message_format("Unknown",
            // STRING: LEGORR 0x004ab8a4
            "An error in an unknown DirectX module/object was raised");
        break;
    }
    return err;
}
