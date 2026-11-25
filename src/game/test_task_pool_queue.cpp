// SPDX-License-Identifier: LGPL-3.0-or-later
/* Standalone task-pool and queue contracts; no level, object models or boot. */
#include <array>
#include <cstdlib>
#include <set>
#include "game/aitask.h"
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/legoobject_record.h"
#include "test/lrr_test.h"

namespace {
constexpr unsigned Cancelled = 0x20, KeepTime = 0x80, Selected = 0x100;
constexpr unsigned Disabled = 0x80000, Override = 0x10000, Pause = 0x80000000;

struct Visit {
    std::set<AITask*> records;
    AITask* stop = nullptr;
};

int VisitRecord(AITask* task, void* data)
{
    auto& v = *static_cast<Visit*>(data);
    CHECK(v.records.insert(task).second, "each live record visited at most once");
    return task == v.stop;
}

void Pool()
{
    std::array<AITask*, 9> entries = {};
    for (auto& entry : entries) {
        entry = aitask_cmd_alloc(12);
        CHECK_EQ(entry->nextFree, entry);
    }
    CHECK_EQ(aiTaskGlobs.poolBlockCount, 4u); // 1 + 2 + 4 + 8 slots.
    std::set<AITask*> expected(entries.begin(), entries.end());
    CHECK_EQ(expected.size(), entries.size());

    entries[3]->buffer = malloc(32);
    CHECK(entries[3]->buffer != nullptr, "owned payload");
    entries[3]->flags = 0x100;
    entries[3]->blockX = 4;
    entries[3]->ref = entries[0];
    aitask_cmd_free(entries[3], 1);
    expected.erase(entries[3]);
    aitask_cmd_free(entries[7], 1);
    expected.erase(entries[7]);

    Visit all;
    CHECK_EQ(aitask_entity_pool_callback_search(VisitRecord, &all), 0);
    CHECK(all.records == expected, "enumeration excludes freed and never allocated slots");
    Visit stop;
    stop.stop = entries[1];
    CHECK_EQ(aitask_entity_pool_callback_search(VisitRecord, &stop), 1);
    CHECK_EQ(stop.records.size(), 2u);
    CHECK(stop.records.count(entries[0]) && stop.records.count(entries[1]), "early stop in pool order");

    auto* a = aitask_cmd_alloc(7);
    auto* b = aitask_cmd_alloc(3);
    CHECK_EQ(a, entries[7]);
    CHECK_EQ(b, entries[3]);
    CHECK_EQ(b->type, 3);
    CHECK_EQ(b->flags, 0u);
    CHECK_EQ(b->blockX, 0);
    CHECK_EQ(b->buffer, nullptr);
    CHECK_EQ(b->ref, nullptr);
    CHECK_EQ(b->next, nullptr);
    CHECK_EQ(b->nextFree, b);
    CHECK_EQ(aiTaskGlobs.poolBlockCount, 4u);
    for (auto* entry : entries) aitask_cmd_free(entry, 1);
    Visit empty;
    CHECK_EQ(aitask_entity_pool_callback_search(VisitRecord, &empty), 0);
    CHECK(empty.records.empty(), "no live records after individual release");
}

AITask* Pending(int type, int priority)
{
    auto* task = aitask_cmd_alloc(type);
    aitask_cmd_link_head(task, priority);
    return task;
}

void Aging()
{
    // Hold the assignment gate closed and use no entity candidates; the
    // actual global tick still ages both lists and purges cancelled records.
    Lego_Object healthy = {}, invalid = {};
    healthy.health = 50;
    invalid.health = 100;
    auto* ordinary = Pending(12, 4);
    ordinary->flags |= 0x400;
    ordinary->timeIn = 5;
    auto* repair = Pending(10, 4);
    repair->object = &healthy;
    repair->flags |= 0x400;
    auto* cancelled = Pending(10, 4);
    cancelled->object = &invalid;
    cancelled->flags |= 0x400;
    auto* exempt = Pending(10, 4);
    exempt->object = &invalid;
    exempt->flags |= 0x200400;
    auto* timer = aitask_cmd_alloc(12);
    aitask_cmd_set_kind(timer, 5);
    timer->flags = KeepTime | 1;
    aitask_cmd_link_tail(timer);
    CHECK_EQ(timer->flags, KeepTime);
    timer->timeIn = 3;
    auto* expired = aitask_cmd_alloc(12);
    aitask_cmd_link_tail(expired);
    expired->flags |= Cancelled;
    priority_entry_order_value_set(0, 40);
    priority_entry_order_value_set(1, 70);
    priority_entry_order_value_set(4, 65);
    aiTaskGlobs.flags |= Pause;
    aitask_cmd_tick_global_delta(2);
    CHECK_EQ(ordinary->time, 1500.0f);
    CHECK_EQ(timer->timeIn, 3.0f);
    CHECK_EQ(aiTaskGlobs.priorityValues[16], 71);
    CHECK_EQ(aiTaskGlobs.globalTail, expired);

    aiTaskGlobs.flags &= ~Pause;
    aitask_cmd_tick_global_delta(2);
    CHECK_EQ(ordinary->time, 1498.0f);
    CHECK_EQ(ordinary->timeIn, 3.0f);
    CHECK_EQ(timer->time, 1500.0f);
    CHECK_EQ(timer->timeIn, 1.0f);
    CHECK_EQ(aiTaskGlobs.globalTail, timer);
    CHECK_EQ(timer->next, nullptr);
    CHECK(cancelled->nextFree != cancelled, "ineligible target removed by tick");
    CHECK(expired->nextFree != expired, "cancelled timer removed");
    CHECK_EQ(repair->nextFree, repair);
    CHECK_EQ(exempt->nextFree, exempt);
    aitask_cmd_tick_global_delta(2);
    CHECK_EQ(ordinary->time, 1496.0f);
    CHECK_EQ(ordinary->timeIn, 1.0f);
    CHECK_EQ(timer->timeIn, -1.0f);
    CHECK_EQ(aiTaskGlobs.globalTail, timer); // no eligible candidate to consume it
    CHECK_EQ(aiTaskGlobs.assignCandidateCount, 0u);
    CHECK_EQ(aiTaskGlobs.timerCandidateCount, 0u);
    aitask_cmd_flush_all(0);
    CHECK_EQ(aiTaskGlobs.globalHead, nullptr);
    CHECK_EQ(aiTaskGlobs.globalTail, nullptr);
    aitask_cmd_flush_all(1);
}

void Cancellation()
{
    auto* companion = Pending(0x13, 5);
    auto* otherZone = Pending(7, 5);
    otherZone->zone = 8;
    auto* otherField = Pending(7, 5);
    otherField->zone = 6;
    otherField->field_20 = 1;
    otherField->field_24 = 2;
    auto* match = Pending(7, 5);
    match->zone = 6;
    match->field_20 = 1;
    match->field_24 = 3;
    match->ref = companion;
    auto* unrelated = Pending(12, 5);
    unrelated->zone = 6;
    unrelated->field_20 = 1;
    unrelated->field_24 = 3;
    aitask_cmd_cancel_by_coords(6, 1, 1, 3);
    CHECK_EQ(match->flags & Cancelled, Cancelled);
    CHECK_EQ(otherField->flags & Cancelled, 0u);
    CHECK_EQ(otherZone->flags & Cancelled, 0u);
    CHECK_EQ(unrelated->flags & Cancelled, 0u);
    aitask_cmd_global_list_purge(&aiTaskGlobs.globalHead);
    CHECK(match->nextFree != match, "matching task freed");
    CHECK(companion->nextFree != companion, "linked cancellation reaches companion");
    CHECK_EQ(otherField->nextFree, otherField);
    aitask_cmd_cancel_by_coords(6, 0, 0, 0);
    aitask_cmd_global_list_purge(&aiTaskGlobs.globalHead);
    CHECK(otherField->nextFree != otherField, "whole-zone cancellation");
    CHECK_EQ(otherZone->nextFree, otherZone);
    CHECK_EQ(unrelated->nextFree, unrelated);
    AITask* head = aiTaskGlobs.globalHead;
    if (CHECK(head && head->next, "two unrelated records remain")) {
        CHECK((head == otherZone && head->next == unrelated) ||
              (head == unrelated && head->next == otherZone), "only unmatched records remain");
        CHECK_EQ(head->next->next, nullptr);
    }
    aitask_cmd_flush_all(0);
    aitask_cmd_flush_all(1);
}

void Relink()
{
    // A local chain may contain records returning to either global list,
    // an ephemeral record to free, and records belonging to the local owner.
    Lego_Object owner = {};
    auto* pending = aitask_cmd_alloc(12);
    pending->flags = 0x10;
    auto* timed = aitask_cmd_alloc(12);
    timed->flags = 0x2000;
    auto* transient = aitask_cmd_alloc(12);
    transient->flags = 4;
    auto* local = aitask_cmd_alloc(12);
    auto* cancelled = aitask_cmd_alloc(12);
    cancelled->flags = 0x10 | Cancelled;
    owner.aiTask = pending;
    pending->next = local;
    local->next = transient;
    transient->next = timed;
    timed->next = cancelled;
    aitask_command_queue_sweep_relink(&owner);
    CHECK_EQ(aiTaskGlobs.globalHead, pending);
    CHECK_EQ(pending->next, nullptr);
    CHECK_EQ(aiTaskGlobs.globalTail, timed);
    CHECK_EQ(timed->next, nullptr);
    CHECK_EQ(owner.aiTask, local);
    CHECK_EQ(local->next, cancelled);
    CHECK_EQ(cancelled->next, nullptr);
    CHECK(transient->nextFree != transient, "temporary chain record returned to pool");

    // Removal of a cancelled record must free it rather than requeue it.
    CHECK_EQ(aitask_entity_command_chain_head_remove(&owner), cancelled);
    CHECK(local->nextFree != local, "local record freed");
    CHECK_EQ(aitask_entity_command_chain_head_remove(&owner), nullptr);
    CHECK(cancelled->nextFree != cancelled, "cancelled record not requeued");
    CHECK_EQ(aiTaskGlobs.globalHead, pending);
    CHECK_EQ(aiTaskGlobs.globalTail, timed);
    CHECK_EQ(aitask_entity_command_chain_head_remove(&owner), nullptr);
    aitask_cmd_flush_all(0);
    aitask_cmd_flush_all(1);
}

void PriorityWindow()
{
    // The purge processes a bounded prefix of 300, preserving overflow.
    // Fixed synthetic timestamps make selected-task ordering deterministic.
    priority_entry_order_value_set(4, -7);
    priority_entry_order_value_set(5, 150);
    priority_entry_order_value_set(6, 30);
    priority_entry_state_set(6, 1);
    auto* overflowLast = Pending(12, 4);
    auto* overflowFirst = Pending(12, 5);
    std::array<AITask*, 300> prefix = {};
    for (unsigned i = 0; i < prefix.size(); ++i)
        prefix[i] = Pending(12, i % 2 ? 4 : 5);
    auto* older = prefix[0];
    auto* newer = prefix[1];
    older->flags |= Selected;
    newer->flags |= Selected;
    older->field_50 = 10;
    newer->field_50 = 20;
    auto* disabled = prefix[2];
    auto* overrideTask = prefix[3];
    aitask_cmd_set_kind(disabled, 6);
    aitask_cmd_set_kind(overrideTask, 6);
    overrideTask->flags |= Override;
    aitask_cmd_global_list_purge(&aiTaskGlobs.globalHead);
    CHECK_EQ(aiTaskGlobs.globalHead, newer);
    CHECK_EQ(newer->next, older);
    CHECK_EQ(disabled->flags & Disabled, Disabled);
    CHECK_EQ(overrideTask->flags & Disabled, 0u);
    CHECK_EQ(overrideTask->priorityValue, 99);
    std::set<AITask*> visited;
    AITask* cur = aiTaskGlobs.globalHead;
    int last = 99;
    for (unsigned i = 0; i < prefix.size(); ++i) {
        if (!CHECK(cur != nullptr, "sorted prefix retained")) break;
        CHECK(visited.insert(cur).second, "no duplicated record or cycle");
        if (i >= 2) {
            CHECK(cur->priorityValue >= 0 && cur->priorityValue <= last, "bounded descending priority");
            last = cur->priorityValue;
        }
        cur = cur->next;
    }
    CHECK_EQ(visited.size(), prefix.size());
    CHECK_EQ(cur, overflowFirst);
    CHECK_EQ(overflowFirst->next, overflowLast);
    CHECK_EQ(overflowLast->next, nullptr);
    CHECK_EQ(overflowFirst->priorityValue, 150); // outside the sort window
    CHECK_EQ(overflowLast->priorityValue, -7);
    for (auto* entry : prefix) CHECK(visited.count(entry), "original prefix preserved");
    priority_entry_state_set(6, 0);
    aitask_cmd_global_list_purge(&aiTaskGlobs.globalHead);
    CHECK_EQ(disabled->flags & Disabled, 0u);
    CHECK_EQ(disabled->priorityValue, 30);
    aiTaskGlobs.commandCounts[0] = 3;
    aiTaskGlobs.commandCounts[4799] = 7;
    priority_entry_state_set(6, 1);
    aitask_cmd_flush_all(0);
    CHECK_EQ(aiTaskGlobs.commandCounts[0], 0u);
    CHECK_EQ(aiTaskGlobs.commandCounts[4799], 0u);
    CHECK_EQ(priority_entry_state_get(6), 0);
    CHECK(aiTaskGlobs.flags & 2, "flush sets suspension flag");
    Visit empty;
    CHECK_EQ(aitask_entity_pool_callback_search(VisitRecord, &empty), 0);
    CHECK(empty.records.empty(), "flush releases all pool records");
    CHECK_EQ(aiTaskGlobs.globalHead, nullptr);
    CHECK_EQ(aiTaskGlobs.globalTail, nullptr);
    aitask_handle_array_release_and_reset();
    CHECK_EQ(aiTaskGlobs.freeList, nullptr);
    CHECK_EQ(aiTaskGlobs.flags, 0u);
}
}

int main()
{
    aitask_cmd_registry_init_target();
    Pool();
    Aging();
    Cancellation();
    Relink();
    PriorityWindow();
    return lrr_test_result("task_pool_queue");
}
