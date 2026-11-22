// SPDX-License-Identifier: LGPL-3.0-or-later
/* keys.c — names resolve back to their DirectInput slots, with a
 * case-insensitive distinction between Return and keypad Enter. */
#include "gods98/keys.h"
#include "test/lrr_test.h"

int main()
{
    keyboard_key_names_init();
    CHECK_EQ(key_names[0], nullptr);
    CHECK_EQ(key_names[255], nullptr);
    CHECK_STREQ(key_names[0x01], "KEY_ESCAPE");
    CHECK_STREQ(key_names[0x1e], "KEY_A");
    CHECK_STREQ(key_names[0x1c], "KEY_RETURN");
    CHECK_STREQ(key_names[0x9c], "KEYPAD_ENTER");

    unsigned int named = 0;
    for (unsigned int index = 0; index < KEY_NAMES_COUNT; ++index) {
        if (!key_names[index]) continue;
        ++named;
        unsigned char encoded = 0;
        CHECK_EQ(named_option_index_find_case_insensitive(key_names[index], &encoded), 1);
        CHECK_EQ(encoded, index);
    }
    CHECK_EQ(named, 101u);

    unsigned char index = 0;
    CHECK_EQ(named_option_index_find_case_insensitive("KeY_ReTuRn", &index), 1);
    CHECK_EQ(index, 0x1cu);
    CHECK_EQ(named_option_index_find_case_insensitive("keypad_enter", &index), 1);
    CHECK_EQ(index, 0x9cu);
    CHECK_EQ(named_option_index_find_case_insensitive("keypad_ente", &index), 0);
    CHECK_EQ(index, 0x9cu);
    return lrr_test_result();
}
