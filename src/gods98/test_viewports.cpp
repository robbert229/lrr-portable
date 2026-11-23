// SPDX-License-Identifier: LGPL-3.0-or-later
/* viewports.c — fractional sizing, bounds, camera, clip and field settings
 * on a dummy-driver software device. No scene is rendered. */
#include <miniwin/d3drm.h>
#include <SDL3/SDL.h>

#include "gods98/container_link_names.h"
#include "gods98/viewports_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/directdraw.h"
#include "gods98/main_globs.h"
#include "gods98/viewports.h"
#include "test/lrr_test.h"

extern "C" int main_init_app(HINSTANCE instance);

static void check_viewports()
{
    Container* root = Container_Initialise("ViewportTest");
    Container* camera = Container_Create(root);
    if (CHECK(camera != nullptr, "create camera container")) {
        viewport_initialise();
        CHECK_EQ(viewport_create_pixel(310, 0, 20, 10, camera), nullptr);
        CHECK_EQ(viewport_create_pixel(0, 190, 10, 20, camera), nullptr);

        Viewport* left = viewport_create(0.0f, 0.0f, 0.5f, 0.25f, camera);
        Viewport* corner = viewport_create_pixel(-20, -10, 20, 10, camera);
        if (CHECK(left != nullptr, "create fractional viewport")) {
            unsigned long width = 0, height = 0;
            viewport_get_size(left, &width, &height);
            CHECK_EQ(width, 160ul);
            CHECK_EQ(height, 50ul);
            CHECK_EQ(viewport_get_camera(left), camera);
            viewport_set_camera(left, root);
            CHECK_EQ(viewport_get_camera(left), root);

            const float front = viewport_get_front_clip(left);
            viewport_set_back_clip(left, 80.0f);
            CHECK_NEAR(viewport_get_back_clip(left), 80.0f, 1e-6);
            CHECK_NEAR(viewport_get_front_clip(left), front, 1e-6);
            viewport_set_field(left, 0.75f);
            CHECK_NEAR(viewport_get_field(left), 0.75f, 1e-6);
        }
        if (CHECK(corner != nullptr, "negative origins are relative to the device edges")) {
            unsigned long width = 0, height = 0;
            viewport_get_size(corner, &width, nullptr);
            viewport_get_size(corner, nullptr, &height);
            CHECK_EQ(width, 20ul);
            CHECK_EQ(height, 10ul);
            viewport_remove(corner);
        }
        if (left) viewport_remove(left);
        viewport_shutdown();
        Container_Remove(camera);
    }
    Container_Shutdown();
}

int main()
{
    if (!CHECK(SDL_Init(SDL_INIT_VIDEO), SDL_GetError())) return lrr_test_result();
    std::strcpy(mainGlobs.programName, "ViewportTest");
    mainGlobs.className = mainGlobs.programName;
    if (CHECK(main_init_app(nullptr), "create dummy-driver window")) {
        directdraw_initialise(mainGlobs.hWnd);
        DirectDraw_Driver drivers[16] = {};
        unsigned int driverCount = 0;
        DirectDraw_Device devices[32] = {};
        unsigned int count = 0;
        const DirectDraw_Device* software = nullptr;
        if (CHECK(directdraw_enum_drivers(drivers, &driverCount) && driverCount > 0,
                  "enumerate drivers") &&
            CHECK(directdraw_enum_devices(&drivers[0], devices, &count), "enumerate devices")) {
            for (unsigned int i = 0; i < count; ++i) {
                if (std::strstr(devices[i].desc, "Miniwin Emulation") != nullptr) software = &devices[i];
            }
        }
        if (CHECK(software != nullptr, "find software device") &&
            CHECK(directdraw_setup(FALSE, nullptr, software, nullptr, 0, 0, 320, 200),
                  "set up software device")) {
            check_viewports();
            directdraw_shutdown();
        }
    }
    SDL_Quit();
    return lrr_test_result();
}
