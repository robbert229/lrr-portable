// SPDX-License-Identifier: LGPL-3.0-or-later
/* Compile-time check that each C boundary header (ADR 0005) agrees with the
 * C++ header it mirrors. Both declare the same functions with C linkage, so a
 * C++ compiler rejects any difference in parameter or return types as a
 * conflicting declaration. Nothing runs: the test passes once it compiles. */
#include "gods98/block.hpp"
#include "gods98/config.h"
#include "gods98/resource.hpp"
#include "gods98/resource.h"
#include "gods98/sound.hpp"
#include "gods98/sound.h"
#include "gods98/string.hpp"
#include "gods98/utils.h"
#include "gods98/input.hpp"
#include "gods98/input.h"
#include "gods98/simtick.hpp"
#include "gods98/simtick.h"
#include "gods98/texture.hpp"
#include "gods98/view.hpp"
#include "gods98/maths.h"
#include "gods98/images.h"
#include "crt/crt.hpp"
#include "game/cmd.hpp"
#include "game/cmd.h"
#include "game/frontend.hpp"
#include "game/frontend.h"
#include "game/level.hpp"
#include "game/panel.h"
#include "game/priorities.h"
#include "game/util.hpp"
#include "gods98/dxerror.h"
#include "gods98/media.hpp"
#include "gods98/soundsys.h"
#include "gods98/audio.hpp"
#include "gods98/audio.h"
#include "platform/platform.h"
#include "platform/window.h"
#include "gods98/keys.h"
#include "gods98/g98_024.h"

int main(void)
{
    return 0;
}
