// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_message_unit — script names, queued move payloads, cancellation
 * before dispatch and deferring a move posted during dispatch. */
#include "game/legoobject_record.h"
#include "game/message_unit_link_names.h"
#include "game/message_unit.h"
#include "test/lrr_test.h"

#include <vector>

static Lego_Object deferredObject;

struct Move {
    Lego_Object* object;
    int x;
    int y;
    int param;
};

static std::vector<Move> moves;

extern "C" void __wrap_ptl_translate_event(uint32_t*) {}

extern "C" void __wrap_aitask_cmd_create_move(Lego_Object* object, const int* block,
                                             int param)
{
    moves.push_back({ object, block[0], block[1], param });
    if (param == 7) message_post_event(MESSAGE_GOTO, &deferredObject, 8, nullptr);
}

int main(void)
{
    message_initialise();
    CHECK_EQ(message_lookup_physical_name("rOcKmOnStErGoTo"), MESSAGE_ROCKMONSTERGOTO);
    CHECK_EQ(message_lookup_physical_name("Select"), MESSAGE_SELECT);
    CHECK_EQ(message_lookup_physical_name("Unlisted"), -1);

    Lego_Object cancelled = {};
    Lego_Object remaining = {};
    int block[] = { 3, 5 };
    message_post_event(MESSAGE_GOTO, &cancelled, 6, block);
    message_post_event(MESSAGE_ROCKMONSTERGOTO, &remaining, 7, block);
    block[0] = 9;
    message_remove_events_with_object(&cancelled);
    CHECK(moves.empty(), "posting leaves dispatch until the update");

    message_update();
    if (CHECK_EQ(moves.size(), 1u)) {
        CHECK_EQ(moves[0].object, &remaining);
        CHECK_EQ(moves[0].x, 3);
        CHECK_EQ(moves[0].y, 5);
        CHECK_EQ(moves[0].param, 7);
    }

    message_update();
    if (CHECK_EQ(moves.size(), 2u)) {
        CHECK_EQ(moves[1].object, &deferredObject);
        CHECK_EQ(moves[1].x, 0);
        CHECK_EQ(moves[1].y, 0);
        CHECK_EQ(moves[1].param, 8);
    }
    message_update();
    CHECK_EQ(moves.size(), 2u);
    return lrr_test_result();
}
