// SPDX-License-Identifier: LGPL-3.0-or-later
/* dxerror.c — independent HRESULT matrix, source metadata, module fallback
 * and truncation of the fixed report buffer. */
#include <string>

#include <miniwin/ddraw.h>
#include <miniwin/d3drm.h>

#include "gods98/dxerror.h"
#include "test/lrr_test.h"

/* This report record is private to dxerror.c. */
struct DxerrorGlobs {
    int err;
    int line;
    int module;
    unsigned int count;
    const char* file;
    int reserved;
    char message[2048];
};
extern DxerrorGlobs dxerrorGlobs;

namespace {
struct Result {
    int code;
    const char* symbol;
};

// These expected names come from the headers, not from the decoder.
#define NAMED(hr) {static_cast<int>(hr), #hr}
#define LEGACY(code, symbol) {static_cast<int>(code), symbol}

const Result draw[] = {
    NAMED(DD_OK),
    NAMED(DDERR_ALREADYINITIALIZED),
    NAMED(DDERR_CANNOTATTACHSURFACE),
    NAMED(DDERR_CANNOTDETACHSURFACE),
    NAMED(DDERR_CURRENTLYNOTAVAIL),
    NAMED(DDERR_EXCEPTION),
    NAMED(DDERR_GENERIC),
    NAMED(DDERR_HEIGHTALIGN),
    NAMED(DDERR_INCOMPATIBLEPRIMARY),
    NAMED(DDERR_INVALIDCAPS),
    NAMED(DDERR_INVALIDCLIPLIST),
    NAMED(DDERR_INVALIDMODE),
    NAMED(DDERR_INVALIDOBJECT),
    NAMED(DDERR_INVALIDPARAMS),
    NAMED(DDERR_INVALIDPIXELFORMAT),
    NAMED(DDERR_INVALIDRECT),
    NAMED(DDERR_LOCKEDSURFACES),
    NAMED(DDERR_NO3D),
    NAMED(DDERR_NOALPHAHW),
    NAMED(DDERR_NOCLIPLIST),
    NAMED(DDERR_NOCOLORCONVHW),
    NAMED(DDERR_NOCOOPERATIVELEVELSET),
    NAMED(DDERR_NOCOLORKEY),
    NAMED(DDERR_NOCOLORKEYHW),
    NAMED(DDERR_NOEXCLUSIVEMODE),
    NAMED(DDERR_NOFLIPHW),
    NAMED(DDERR_NOGDI),
    NAMED(DDERR_NOMIRRORHW),
    NAMED(DDERR_NOTFOUND),
    NAMED(DDERR_NOOVERLAYHW),
    NAMED(DDERR_NORASTEROPHW),
    NAMED(DDERR_NOROTATIONHW),
    NAMED(DDERR_NOSTRETCHHW),
    NAMED(DDERR_NOT4BITCOLOR),
    NAMED(DDERR_NOT4BITCOLORINDEX),
    NAMED(DDERR_NOT8BITCOLOR),
    NAMED(DDERR_NOTEXTUREHW),
    NAMED(DDERR_NOVSYNCHW),
    NAMED(DDERR_NOZBUFFERHW),
    NAMED(DDERR_NOZOVERLAYHW),
    NAMED(DDERR_OUTOFCAPS),
    NAMED(DDERR_OUTOFMEMORY),
    NAMED(DDERR_OUTOFVIDEOMEMORY),
    NAMED(DDERR_OVERLAYCANTCLIP),
    NAMED(DDERR_OVERLAYCOLORKEYONLYONEACTIVE),
    NAMED(DDERR_PALETTEBUSY),
    NAMED(DDERR_COLORKEYNOTSET),
    NAMED(DDERR_SURFACEALREADYATTACHED),
    NAMED(DDERR_SURFACEALREADYDEPENDENT),
    NAMED(DDERR_SURFACEBUSY),
    NAMED(DDERR_SURFACEISOBSCURED),
    NAMED(DDERR_SURFACELOST),
    NAMED(DDERR_SURFACENOTATTACHED),
    NAMED(DDERR_TOOBIGHEIGHT),
    NAMED(DDERR_TOOBIGSIZE),
    NAMED(DDERR_TOOBIGWIDTH),
    NAMED(DDERR_UNSUPPORTED),
    NAMED(DDERR_UNSUPPORTEDFORMAT),
    NAMED(DDERR_UNSUPPORTEDMASK),
    NAMED(DDERR_VERTICALBLANKINPROGRESS),
    NAMED(DDERR_WASSTILLDRAWING),
    NAMED(DDERR_XALIGN),
    NAMED(DDERR_INVALIDDIRECTDRAWGUID),
    NAMED(DDERR_DIRECTDRAWALREADYCREATED),
    NAMED(DDERR_NODIRECTDRAWHW),
    NAMED(DDERR_PRIMARYSURFACEALREADYEXISTS),
    NAMED(DDERR_NOEMULATION),
    NAMED(DDERR_REGIONTOOSMALL),
    NAMED(DDERR_CLIPPERISUSINGHWND),
    NAMED(DDERR_NOCLIPPERATTACHED),
    NAMED(DDERR_NOHWND),
    NAMED(DDERR_HWNDSUBCLASSED),
    NAMED(DDERR_HWNDALREADYSET),
    NAMED(DDERR_NOPALETTEATTACHED),
    NAMED(DDERR_NOPALETTEHW),
    NAMED(DDERR_BLTFASTCANTCLIP),
    NAMED(DDERR_NOBLTHW),
    NAMED(DDERR_NODDROPSHW),
    NAMED(DDERR_OVERLAYNOTVISIBLE),
    NAMED(DDERR_NOOVERLAYDEST),
    NAMED(DDERR_INVALIDPOSITION),
    NAMED(DDERR_NOTAOVERLAYSURFACE),
    NAMED(DDERR_EXCLUSIVEMODEALREADYSET),
    NAMED(DDERR_NOTFLIPPABLE),
    NAMED(DDERR_CANTDUPLICATE),
    NAMED(DDERR_NOTLOCKED),
    NAMED(DDERR_CANTCREATEDC),
    NAMED(DDERR_NODC),
    NAMED(DDERR_WRONGMODE),
    NAMED(DDERR_IMPLICITLYCREATED),
    NAMED(DDERR_NOTPALETTIZED),
    // Additional DirectDraw SDK result codes absent from miniwin's header.
    LEGACY(MAKE_DDHRESULT(222), "DDERR_NODIRECTDRAWSUPPORT"),
    LEGACY(MAKE_DDHRESULT(435), "DDERR_CANTLOCKSURFACE"),
    LEGACY(MAKE_DDHRESULT(590), "DDERR_UNSUPPORTEDMODE"),
    LEGACY(MAKE_DDHRESULT(591), "DDERR_NOMIPMAPHW"),
    LEGACY(MAKE_DDHRESULT(592), "DDERR_INVALIDSURFACETYPE"),
    LEGACY(MAKE_DDHRESULT(620), "DDERR_DCALREADYCREATED"),
    LEGACY(MAKE_DDHRESULT(640), "DDERR_CANTPAGELOCK"),
    LEGACY(MAKE_DDHRESULT(660), "DDERR_CANTPAGEUNLOCK"),
    LEGACY(MAKE_DDHRESULT(680), "DDERR_NOTPAGELOCKED"),
    LEGACY(0x800401f0u, "DDERR_NOTINITIALIZED"), // CO_E_NOTINITIALIZED
    // The diagnostic contract retains these historical truncated spellings.
    LEGACY(MAKE_DDHRESULT(600), "DDERR_NOOPTIMIZEH"),
    LEGACY(MAKE_DDHRESULT(601), "DDERR_NOTLOADE"),
    LEGACY(MAKE_DDHRESULT(630), "DDERR_NONONLOCALVIDME"),
    LEGACY(MAKE_DDHRESULT(690), "DDERR_MOREDAT"),
    LEGACY(MAKE_DDHRESULT(695), "DDERR_VIDEONOTACTIV"),
    LEGACY(MAKE_DDHRESULT(699), "DDERR_DEVICEDOESNTOWNSURFAC"),
};

// DirectInput SDK results not exposed by miniwin: COM HRESULTs and
// HRESULT_FROM_WIN32 values, plus the documented force-feedback results.
constexpr unsigned win32_error(unsigned code) { return 0x80070000u | code; }
const Result input[] = {
    LEGACY(S_OK, "DI_OK"),
    LEGACY(1, "DI_BUFFEROVERFLOW"),
    LEGACY(2, "DI_POLLEDDEVICE"),
    LEGACY(3, "DI_DOWNLOADSKIPPED"),
    LEGACY(4, "DI_EFFECTRESTARTED"),
    LEGACY(8, "DI_TRUNCATED"),
    LEGACY(4 | 8, "DI_TRUNCATEDANDRESTARTED"),
    LEGACY(win32_error(170), "DIERR_ACQUIRED"), // ERROR_BUSY
    LEGACY(win32_error(1247), "DIERR_ALREADYINITIALIZED"),
    LEGACY(win32_error(119), "DIERR_BADDRIVERVER"),
    LEGACY(win32_error(1153), "DIERR_BETADIRECTINPUTVERSION"),
    LEGACY(0x80040201u, "DIERR_DEVICEFULL"),
    LEGACY(0x80040154u, "DIERR_DEVICENOTREG"),
    LEGACY(0x80040208u, "DIERR_EFFECTPLAYING"),
    LEGACY(DDERR_GENERIC, "DIERR_GENERIC"),
    LEGACY(win32_error(5), "DIERR_HANDLEEXISTS"),
    LEGACY(0x80040204u, "DIERR_HASEFFECTS"),
    LEGACY(0x80040206u, "DIERR_INCOMPLETEEFFECT"),
    LEGACY(win32_error(30), "DIERR_INPUTLOST"),
    LEGACY(DDERR_INVALIDPARAMS, "DIERR_INVALIDPARAM"),
    LEGACY(0x80040202u, "DIERR_MOREDATA"),
    LEGACY(0x80040110u, "DIERR_NOAGGREGATION"),
    LEGACY(E_NOINTERFACE, "DIERR_NOINTERFACE"),
    LEGACY(win32_error(12), "DIERR_NOTACQUIRED"),
    LEGACY(0x80040207u, "DIERR_NOTBUFFERED"),
    LEGACY(0x80040203u, "DIERR_NOTDOWNLOADED"),
    LEGACY(0x80040205u, "DIERR_NOTEXCLUSIVEACQUIRED"),
    LEGACY(win32_error(2), "DIERR_NOTFOUND"),
    LEGACY(win32_error(21), "DIERR_NOTINITIALIZED"),
    LEGACY(win32_error(1150), "DIERR_OLDDIRECTINPUTVERSION"),
    LEGACY(DDERR_OUTOFMEMORY, "DIERR_OUTOFMEMORY"),
    LEGACY(E_NOTIMPL, "DIERR_UNSUPPORTED"),
    LEGACY(0x8000000au, "E_PENDING"),
};

// The retained-mode facility uses the same MAKE_DDHRESULT encoding.
// Only NOTFOUND and OK are exposed by miniwin; the other indices are
// the Direct3DRM SDK values.
const Result retained[] = {
    NAMED(D3DRM_OK),
    NAMED(D3DRMERR_NOTFOUND),
    LEGACY(MAKE_DDHRESULT(781), "D3DRMERR_BADOBJECT"),
    LEGACY(MAKE_DDHRESULT(782), "D3DRMERR_BADTYPE"),
    LEGACY(MAKE_DDHRESULT(783), "D3DRMERR_BADALLOC"),
    LEGACY(MAKE_DDHRESULT(784), "D3DRMERR_FACEUSED"),
    LEGACY(MAKE_DDHRESULT(786), "D3DRMERR_NOTDONEYET"),
    LEGACY(MAKE_DDHRESULT(787), "D3DRMERR_FILENOTFOUND"),
    LEGACY(MAKE_DDHRESULT(788), "D3DRMERR_BADFILE"),
    LEGACY(MAKE_DDHRESULT(789), "D3DRMERR_BADDEVICE"),
    LEGACY(MAKE_DDHRESULT(790), "D3DRMERR_BADVALUE"),
    LEGACY(MAKE_DDHRESULT(791), "D3DRMERR_BADMAJORVERSION"),
    LEGACY(MAKE_DDHRESULT(792), "D3DRMERR_BADMINORVERSION"),
    LEGACY(MAKE_DDHRESULT(793), "D3DRMERR_UNABLETOEXECUTE"),
    LEGACY(MAKE_DDHRESULT(796), "D3DRMERR_PENDING"),
    LEGACY(MAKE_DDHRESULT(797), "D3DRMERR_NOTENOUGHDATA"),
    LEGACY(MAKE_DDHRESULT(798), "D3DRMERR_REQUESTTOOLARGE"),
    LEGACY(MAKE_DDHRESULT(799), "D3DRMERR_REQUESTTOOSMALL"),
    LEGACY(MAKE_DDHRESULT(800), "D3DRMERR_CONNECTIONLOST"),
    LEGACY(MAKE_DDHRESULT(804), "D3DRMERR_BOXNOTSET"),
    LEGACY(MAKE_DDHRESULT(805), "D3DRMERR_BADPMDATA"),
};
#undef NAMED
#undef LEGACY

const char filename[] = "test_dxerror_fixture.cpp";
const char alternateFilename[] = "test_other_fixture.cpp";
unsigned int reports;

void check_report(Result result, int module, int recordedModule,
                  const char* moduleName, const char* guidance = nullptr)
{
    const char* file = (reports % 2 == 0) ? filename : alternateFilename;
    const int line = 42 + static_cast<int>(reports++);
    const unsigned int oldCount = dxerrorGlobs.count;
    CHECK_EQ(directx_result_diagnostic_dispatch(result.code, module, file, line), result.code);
    CHECK_EQ(dxerrorGlobs.err, result.code);
    CHECK_EQ(dxerrorGlobs.module, recordedModule);
    CHECK_EQ(dxerrorGlobs.line, line);
    CHECK_EQ(dxerrorGlobs.file, file);
    CHECK_EQ(dxerrorGlobs.count, oldCount + 1);
    CHECK_EQ(dxerrorGlobs.message[sizeof(dxerrorGlobs.message) - 1], '\0');

    const std::string report = dxerrorGlobs.message;
    const std::string prefix = std::string("A DirectX error was set in module ") + file +
        " at line no " + std::to_string(line) + ".\nError : " + result.symbol +
        " (code " + std::to_string(result.code) + ")\nDX Module : " + moduleName +
        "\nError Description : ";
    // Comparing the exact prefix also ensures stale error/module/location text
    // never survives a subsequent report.
    CHECK_EQ(report.substr(0, prefix.size()), prefix);
    CHECK(report.size() > prefix.size() + 1, "nonempty description");
    if (guidance) CHECK(report.find(guidance) != std::string::npos, "expected diagnostic guidance");
}
} // namespace

int main()
{
    const unsigned int count = dxerrorGlobs.count;
    for (const auto& result : draw)
        check_report(result, DXERROR_MODULE_DDRAW, DXERROR_MODULE_DDRAW, "Direct Draw");
    for (const auto& result : input)
        check_report(result, DXERROR_MODULE_DINPUT, DXERROR_MODULE_DINPUT, "Direct Input");
    for (const auto& result : retained)
        check_report(result, DXERROR_MODULE_D3DRM, DXERROR_MODULE_D3DRM, "Direct3D Retained Mode");

    check_report({static_cast<int>(DDERR_SURFACELOST), "DDERR_SURFACELOST"},
                 DXERROR_MODULE_DDRAW, DXERROR_MODULE_DDRAW, "Direct Draw", "IDirectDrawSurface3::Restore");
    // A common HRESULT must be decoded differently by each module.
    check_report({static_cast<int>(E_NOINTERFACE), "QUERY_INTERFACE"},
                 DXERROR_MODULE_IUNKNOWN, DXERROR_MODULE_IUNKNOWN, "IUknown Interface",
                 "requested interface could not be obtained");

    for (int module : {DXERROR_MODULE_DDRAW, DXERROR_MODULE_DINPUT, DXERROR_MODULE_D3DRM}) {
        const char* name = module == DXERROR_MODULE_DDRAW ? "Direct Draw" :
                           module == DXERROR_MODULE_DINPUT ? "Direct Input" : "Direct3D Retained Mode";
        check_report({125, "UNKNOWN ERROR"}, module, module, name, "Invalid error code specified");
    }
    // D3DRMERR_LIBRARYNOTFOUND is a valid SDK result not in this decoder.
    check_report({static_cast<int>(MAKE_DDHRESULT(794)), "UNKNOWN ERROR"},
                 DXERROR_MODULE_D3DRM, DXERROR_MODULE_D3DRM, "Direct3D Retained Mode",
                 "Invalid error code specified");
    // Unsupported module tables intentionally fall through to retained mode,
    // but retain the original module metadata in the report.
    struct Fallback { int module; const char* name; };
    const Fallback fallback[] = {
        {DXERROR_MODULE_DSOUND, "Direct Sound"},
        {DXERROR_MODULE_DPLAY, "Direct Play"},
        {DXERROR_MODULE_D3DIM, "Direct3D Immediate mode"}
    };
    for (const auto& entry : fallback) {
        check_report({static_cast<int>(D3DRMERR_NOTFOUND), "D3DRMERR_NOTFOUND"},
                     entry.module, entry.module, entry.name);
        check_report({125, "UNKNOWN ERROR"}, entry.module, entry.module, entry.name,
                     "Invalid error code specified");
    }
    for (int module : {-1, DXERROR_MODULE_UNKNOWN, 8, 12})
        check_report({125, "Unknown"}, module, DXERROR_MODULE_UNKNOWN, "Unknown module",
                     "unknown DirectX module/object");
    CHECK_EQ(dxerrorGlobs.count, count + reports);

    const std::string longDescription(3000, 'x');
    directx_error_message_format("CUSTOM", longDescription.c_str());
    CHECK(std::strstr(dxerrorGlobs.message, "Error : CUSTOM (code 125)") != nullptr,
          "custom report retains recorded result");
    CHECK_EQ(dxerrorGlobs.message[sizeof(dxerrorGlobs.message) - 1], '\0');
    CHECK_EQ(std::strlen(dxerrorGlobs.message), sizeof(dxerrorGlobs.message) - 1);
    CHECK_EQ(dxerrorGlobs.count, count + reports);
    return lrr_test_result();
}
