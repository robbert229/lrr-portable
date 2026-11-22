// SPDX-License-Identifier: LGPL-3.0-or-later
/* dxerror.c — selected HRESULT reports, source metadata, unknown module
 * fallback, and truncation of the fixed report buffer. */
#include <string>

#include <miniwin/ddraw.h>

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

int main()
{
    const char filename[] = "test_dxerror_fixture.cpp";
    const int lost = DDERR_SURFACELOST;
    const unsigned int count = dxerrorGlobs.count;
    CHECK_EQ(directx_result_diagnostic_dispatch(lost, DXERROR_MODULE_DDRAW, filename, 42), lost);
    CHECK_EQ(dxerrorGlobs.err, lost);
    CHECK_EQ(dxerrorGlobs.module, DXERROR_MODULE_DDRAW);
    CHECK_EQ(dxerrorGlobs.line, 42);
    CHECK_EQ(dxerrorGlobs.file, filename + 0);
    CHECK_EQ(dxerrorGlobs.count, count + 1);
    std::string report = dxerrorGlobs.message;
    CHECK(report.find("test_dxerror_fixture.cpp at line no 42") != std::string::npos,
          "report includes the source location");
    CHECK(report.find("DDERR_SURFACELOST (code " + std::to_string(lost) + ")") != std::string::npos,
          "report includes the symbolic and numeric error");
    CHECK(report.find("DX Module : Direct Draw") != std::string::npos,
          "report names the DirectDraw module");
    CHECK(report.find("IDirectDrawSurface3::Restore") != std::string::npos,
          "report includes the surface restoration guidance");

    CHECK_EQ(directx_result_diagnostic_dispatch(0, DXERROR_MODULE_DDRAW, filename, 9), 0);
    CHECK(std::strstr(dxerrorGlobs.message, "DD_OK") != nullptr, "success has its own report");

    CHECK_EQ(directx_result_diagnostic_dispatch(125, 12, filename, 23), 125);
    CHECK_EQ(dxerrorGlobs.module, DXERROR_MODULE_UNKNOWN);
    CHECK_EQ(dxerrorGlobs.count, count + 3);
    CHECK(std::strstr(dxerrorGlobs.message, "DX Module : Unknown module") != nullptr,
          "unknown modules get the fallback report");
    CHECK(std::strstr(dxerrorGlobs.message, "DD_OK") == nullptr, "the previous report is replaced");

    const std::string longDescription(3000, 'x');
    directx_error_message_format("CUSTOM", longDescription.c_str());
    CHECK_EQ(dxerrorGlobs.message[sizeof(dxerrorGlobs.message) - 1], '\0');
    CHECK_EQ(std::strlen(dxerrorGlobs.message), sizeof(dxerrorGlobs.message) - 1);
    CHECK_EQ(dxerrorGlobs.count, count + 3);
    return lrr_test_result();
}
