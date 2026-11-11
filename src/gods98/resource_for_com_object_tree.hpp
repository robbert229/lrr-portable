// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what com_object_tree uses from resource.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_RESOURCE_FOR_COM_OBJECT_TREE_HPP
#define LRR_GODS98_RESOURCE_FOR_COM_OBJECT_TREE_HPP

/* Direct3DRM object and object-array ABI used by the recursive resource-tree
 * helper. The reference calls the object's GetChildren slot at +0x4c, then
 * ObjectArray::GetSize/GetElement at +0x0c/+0x10, and queries each child for
 * the interface identified by the GUID at 0x0049f988 before recursing. */
#if defined(_MSC_VER)
#define RESOURCE_TREE_STDCALL __stdcall
#elif defined(__i386__)
#define RESOURCE_TREE_STDCALL __attribute__((stdcall))
#else
#define RESOURCE_TREE_STDCALL
#endif

typedef uint32_t (RESOURCE_TREE_STDCALL *ResourceTreeGetChildrenFn)(
    void*, void**);

typedef uint32_t (RESOURCE_TREE_STDCALL *ResourceTreeArraySizeFn)(void*);

typedef uint32_t (RESOURCE_TREE_STDCALL *ResourceTreeArrayElementFn)(
    void*, uint32_t, void**);

typedef uint32_t (RESOURCE_TREE_STDCALL *ResourceTreeQueryInterfaceFn)(
    void*, const void*, void**);

typedef uint32_t (RESOURCE_TREE_STDCALL *ResourceTreeReferenceFn)(void*);

extern const unsigned char s_resource_tree_child_iid[16];

#endif /* LRR_GODS98_RESOURCE_FOR_COM_OBJECT_TREE_HPP */
