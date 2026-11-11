// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what viewports uses from view.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_VIEW_FOR_VIEWPORTS_HPP
#define LRR_GODS98_VIEW_FOR_VIEWPORTS_HPP

/* The scene object wrapper stores its interface in its first word. These
 * renderer leaves dispatch through fixed vtable slots on that interface. */
class SceneRootInterface
{
public:
#define SCENE_INTERFACE_SLOT(n) virtual void slot_##n(void) = 0
    SCENE_INTERFACE_SLOT(0); SCENE_INTERFACE_SLOT(1);
    SCENE_INTERFACE_SLOT(2); SCENE_INTERFACE_SLOT(3);
    SCENE_INTERFACE_SLOT(4); SCENE_INTERFACE_SLOT(5);
    SCENE_INTERFACE_SLOT(6); SCENE_INTERFACE_SLOT(7);
    SCENE_INTERFACE_SLOT(8); SCENE_INTERFACE_SLOT(9);
    SCENE_INTERFACE_SLOT(10); SCENE_INTERFACE_SLOT(11);
    SCENE_INTERFACE_SLOT(12); SCENE_INTERFACE_SLOT(13);
    SCENE_INTERFACE_SLOT(14);
    virtual void clip_distance_set(float distance) = 0;
    SCENE_INTERFACE_SLOT(16); SCENE_INTERFACE_SLOT(17);
    SCENE_INTERFACE_SLOT(18); SCENE_INTERFACE_SLOT(19);
    virtual void transform_point(void* output, const void* input) = 0;
    SCENE_INTERFACE_SLOT(21);
    SCENE_INTERFACE_SLOT(22); SCENE_INTERFACE_SLOT(23);
    SCENE_INTERFACE_SLOT(24); SCENE_INTERFACE_SLOT(25);
    SCENE_INTERFACE_SLOT(26); SCENE_INTERFACE_SLOT(27);
    SCENE_INTERFACE_SLOT(28); SCENE_INTERFACE_SLOT(29);
    SCENE_INTERFACE_SLOT(30); SCENE_INTERFACE_SLOT(31);
    virtual unsigned int render_surface_value_pair_80_get(void) = 0;
    virtual unsigned int render_surface_value_pair_84_get(void) = 0;
    SCENE_INTERFACE_SLOT(34); SCENE_INTERFACE_SLOT(35);
    SCENE_INTERFACE_SLOT(36); SCENE_INTERFACE_SLOT(37);
    SCENE_INTERFACE_SLOT(38); SCENE_INTERFACE_SLOT(39);
    SCENE_INTERFACE_SLOT(40); SCENE_INTERFACE_SLOT(41);
    SCENE_INTERFACE_SLOT(42); SCENE_INTERFACE_SLOT(43);
    SCENE_INTERFACE_SLOT(44); SCENE_INTERFACE_SLOT(45);
    SCENE_INTERFACE_SLOT(46);
    virtual void fog_color_enable(unsigned int color) = 0;
    SCENE_INTERFACE_SLOT(48); SCENE_INTERFACE_SLOT(49);
    SCENE_INTERFACE_SLOT(50);
    virtual void fog_enable(int enabled) = 0;
    virtual void fog_color_set(unsigned int color) = 0;
    virtual void fog_mode_set(unsigned int mode) = 0;
    virtual void fog_parameters_set(unsigned int start, unsigned int end,
                                    unsigned int density) = 0;
    SCENE_INTERFACE_SLOT(55); SCENE_INTERFACE_SLOT(56);
    SCENE_INTERFACE_SLOT(57); SCENE_INTERFACE_SLOT(58);
    SCENE_INTERFACE_SLOT(59); SCENE_INTERFACE_SLOT(60);
    SCENE_INTERFACE_SLOT(61); SCENE_INTERFACE_SLOT(62);
    SCENE_INTERFACE_SLOT(63); SCENE_INTERFACE_SLOT(64);
    SCENE_INTERFACE_SLOT(65); SCENE_INTERFACE_SLOT(66);
    SCENE_INTERFACE_SLOT(67); SCENE_INTERFACE_SLOT(68);
    SCENE_INTERFACE_SLOT(69); SCENE_INTERFACE_SLOT(70);
    SCENE_INTERFACE_SLOT(71); SCENE_INTERFACE_SLOT(72);
    SCENE_INTERFACE_SLOT(73); SCENE_INTERFACE_SLOT(74);
    SCENE_INTERFACE_SLOT(75); SCENE_INTERFACE_SLOT(76);
    SCENE_INTERFACE_SLOT(77); SCENE_INTERFACE_SLOT(78);
    SCENE_INTERFACE_SLOT(79); SCENE_INTERFACE_SLOT(80);
    SCENE_INTERFACE_SLOT(81); SCENE_INTERFACE_SLOT(82);
    SCENE_INTERFACE_SLOT(83); SCENE_INTERFACE_SLOT(84);
    virtual void fog_render_mode_set(unsigned int mode) = 0;
#undef SCENE_INTERFACE_SLOT
};

void render_surface_pool_link_set(void* record, void* next);
extern void* s_scene_value_query_context;
extern SceneObjectReadyQueryFn s_scene_ready_query;
extern SceneObjectCurrentValueFn s_scene_current_value_query;
extern SceneObjectValueReleaseFn s_scene_value_release;

#endif /* LRR_GODS98_VIEW_FOR_VIEWPORTS_HPP */
