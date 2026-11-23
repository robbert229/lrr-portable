// SPDX-License-Identifier: LGPL-3.0-or-later
/* config.c — parse block text in memory, walk nested records, resolve an
 * exact key before a wildcard, and read typed values. */
#include <cstdlib>

#include "gods98/config.h"
#include "test/lrr_test.h"

int main()
{
    const char text[] =
        "Level* {\n Name generic\n}\n"
        "LevelOne {\r\n"
        "  Name\tfirst ; comment words and a brace }\r\n"
        "  Camera {\n Tilt 90\n }\n"
        "  Enabled oN\n Tint 255:128:0\n BadTint 1:2\n"
        "}\n";

    block_text_token_pool_init();
    BlockFile* config = block_parse(text, sizeof(text) - 1);
    if (CHECK(config != nullptr, "parse config text")) {
        CHECK_STREQ(block_get_strptr(config, "levelone::name"), "first");
        CHECK_STREQ(block_get_strptr(config, "LevelTwo::Name"), "generic");
        CHECK_EQ(block_get_strptr(config, "LevelOne::Absent"), nullptr);

        const char* path = string_path_join("LevelOne", "Camera", "Tilt",
                                            static_cast<const char*>(nullptr));
        CHECK_STREQ(path, "LevelOne::Camera::Tilt");
        CHECK_NEAR(config_angle_radians_get(config, path), 1.57079633f, 1e-6);
        CHECK_EQ(block_get_bool_path(config, "LevelOne::Enabled"), 1);
        CHECK_EQ(block_get_bool_path(config, "LevelOne::Absent"), 2);

        BlockRec* camera = block_subgroup(config, "LevelOne::Camera");
        if (CHECK(camera != nullptr, "camera subgroup has a child")) {
            CHECK_STREQ(camera->key, "Tilt");
            CHECK_EQ(camera->depth, 2u);
            CHECK_EQ(block_next_equal_key(camera), nullptr);
        }
        BlockRec* first = block_find(config, "LevelOne::Name");
        if (CHECK(first != nullptr, "find the exact level Name record")) {
            CHECK_STREQ(first->key, "Name");
            BlockRec* entry = block_next_equal_key(first);
            if (CHECK(entry != nullptr, "camera follows Name")) {
                CHECK_STREQ(entry->key, "Camera");
                entry = block_next_equal_key(entry);
                if (CHECK(entry != nullptr, "next peer skips the nested Tilt"))
                    CHECK_STREQ(entry->key, "Enabled");
            }
        }

        float red = -1.0f, green = -1.0f, blue = -1.0f;
        CHECK_EQ(config_rgb_vector_read(config, "LevelOne::Tint", &red, &green, &blue), 1);
        CHECK_NEAR(red, 1.0f, 1e-6);
        CHECK_NEAR(green, 128.0f / 255.0f, 1e-6);
        CHECK_NEAR(blue, 0.0f, 1e-6);
        CHECK_EQ(config_rgb_vector_read(config, "LevelOne::BadTint", &red, &green, &blue), 0);
        CHECK_NEAR(red, 1.0f, 1e-6);

        char* copy = block_get_str_copy(config, "LevelOne::Name");
        block_close(config);
        CHECK_STREQ(copy, "first");
        std::free(copy);
        CHECK(block_text_token_pool_head_peek() != nullptr, "close returns records to the pool");
    }
    block_text_token_pool_destroy();
    CHECK_EQ(block_text_token_pool_resident_peek(), 0);
    return lrr_test_result();
}
