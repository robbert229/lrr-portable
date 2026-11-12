// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what record.cpp uses from mesh_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by record.cpp only. */
#ifndef LRR_GODS98_RECORD_FROM_MESH_UNIT_HPP
#define LRR_GODS98_RECORD_FROM_MESH_UNIT_HPP

extern uint32_t s_render_resource_instance_pool_count;
extern void* s_render_resource_instance_pool_blocks[20];
extern void* s_render_resource_instance_pool_head;
void* indexed_record_data_address_get_target( const void* record, int index);
void indexed_record_field64_set_target( void* record, int index, uint32_t value);
uint32_t screen_effect_item_opacity_set_target(void* record, int block, float opacity, int mode);

#endif /* LRR_GODS98_RECORD_FROM_MESH_UNIT_HPP */
