// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_ptl — first matching translation wins, without chaining mappings
 * or replacing the event payload, and a failed reload clears old mappings. */
#include "game/legoobject_record.h"
#include "game/message_unit_link_names.h"
#include "game/message_unit.h"
#include "game/ptl.h"
#include "gods98/config.h"
#include "test/lrr_test.h"

static BlockFile* configToOpen;

extern "C" BlockFile* __wrap_block_open(const char*) { return configToOpen; }
extern "C" void __wrap_block_close(BlockFile*) {}

int main(void)
{
    char game[] = "LegoRR", group[] = "{";
    char go[] = "Goto", rockGo[] = "RockMonsterGoto", deselect[] = "Deselect";
    char end[] = "End";
    BlockRec config[5] = {};
    config[0].key = game;
    config[0].value = group;
    config[1].key = go;
    config[1].value = rockGo;
    config[2].key = rockGo;
    config[2].value = deselect;
    config[3].key = go;
    config[3].value = deselect;
    config[4].key = end;
    for (unsigned int i = 0; i < 4; ++i) {
        config[i].next = &config[i + 1];
        config[i + 1].prev = &config[i];
    }
    config[1].depth = config[2].depth = config[3].depth = 1;

    message_initialise();
    configToOpen = config;
    CHECK_EQ(ptl_initialise("synthetic.ptl", game), 1);
    Lego_Object subject = {};
    Lego_Object target = {};
    Message_Event event = { MESSAGE_GOTO, &subject, reinterpret_cast<uintptr_t>(&target), { 2, 9 } };
    ptl_translate_event(&event);
    CHECK_EQ(event.type, static_cast<unsigned int>(MESSAGE_ROCKMONSTERGOTO));
    CHECK_EQ(event.argument1, &subject);
    CHECK_EQ(event.argument2, reinterpret_cast<uintptr_t>(&target));
    CHECK_EQ(event.blockPos[0], 2);
    CHECK_EQ(event.blockPos[1], 9);
    ptl_translate_event(&event);
    CHECK_EQ(event.type, static_cast<unsigned int>(MESSAGE_DESELECT));
    ptl_translate_event(&event);
    CHECK_EQ(event.type, static_cast<unsigned int>(MESSAGE_DESELECT));

    configToOpen = nullptr;
    CHECK_EQ(ptl_initialise("missing.ptl", game), 0);
    event.type = MESSAGE_GOTO;
    ptl_translate_event(&event);
    CHECK_EQ(event.type, static_cast<unsigned int>(MESSAGE_GOTO));
    return lrr_test_result();
}
