// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_text — message name lookup and escape conversion, timed message
 * replacement, and resetting the NERPs line and both text windows. */
#include "game/text_link_names.h"
#include "game/text.h"
#include "game/text_record.h"
#include "test/lrr_test.h"

#include <cstdlib>

extern "C" Text_Globs textGlobs;
extern "C" int nerpfunc_set_tutorial_flags(unsigned int* stack);

int main(void)
{
    char textBuffer[] = "text";
    char imageBuffer[] = "image";
    TextWindow only = {};
    only.windowBuf = textBuffer;
    only.windowBufLen = sizeof(textBuffer);
    TextWindow withImage = {};
    withImage.windowBuf = imageBuffer;
    withImage.windowBufLen = sizeof(imageBuffer);
    text_load(&only, &withImage, 4, 7, 0.0f);
    CHECK_EQ(textGlobs.pauseTime, 100.0f);
    CHECK_EQ(textGlobs.currType, Text_Type_Count);

    int type = -1;
    CHECK_EQ(text_get_type_from_name("text_crystalFOUND", &type), 1);
    CHECK_EQ(type, static_cast<int>(Text_CrystalFound));
    CHECK_EQ(text_get_type_from_name("unlisted", &type), 0);
    CHECK_EQ(type, static_cast<int>(Text_CrystalFound));
    text_set_message(type, "Crystal_found\\nWaiting\\t");
    CHECK_STREQ(textGlobs.messages[type], "Crystal found\nWaiting\\t");
    text_set_message(type, "Collected");
    CHECK_STREQ(textGlobs.messages[type], "Collected");

    unsigned int flags = 1; // A tutorial suppresses message sounds.
    nerpfunc_set_tutorial_flags(&flags);
    text_display_message(Text_CrystalFound, 1, 1);
    CHECK_EQ(textGlobs.currType, Text_CrystalFound);
    CHECK_EQ(textGlobs.timer, 100.0f);
    CHECK_EQ(textGlobs.flags, 4u); // Flash
    text_display_message(Text_OreFound, 0, 0);
    CHECK_EQ(textGlobs.currType, Text_CrystalFound);
    CHECK_EQ(textGlobs.timer, 100.0f);
    textGlobs.timer = 0.0f;
    text_display_message(Text_OreFound, 0, 0);
    CHECK_EQ(textGlobs.currType, Text_OreFound);

    char tutorial[] = "Synthetic tutorial";
    text_set_nerps_message(tutorial, 3);
    CHECK_EQ(textGlobs.currText, &tutorial[0]);
    CHECK_EQ(textGlobs.counter, 0x10003u);
    text_set_nerps_message(tutorial, 0);
    CHECK_EQ(textGlobs.counter, 0x18000u);
    text_restart();
    CHECK_EQ(textGlobs.currType, Text_Type_Count);
    CHECK_EQ(textGlobs.currText, nullptr);
    CHECK_EQ(textGlobs.counter, 0u);
    CHECK_EQ(textGlobs.flags, 0u);
    CHECK_STREQ(textBuffer, "");
    CHECK_STREQ(imageBuffer, "");

    std::free(textGlobs.messages[type]);
    textGlobs.messages[type] = nullptr;
    return lrr_test_result();
}
