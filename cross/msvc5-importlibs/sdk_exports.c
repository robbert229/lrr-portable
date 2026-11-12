/* SPDX-License-Identifier: LGPL-3.0-or-later */
/*
 * Prototype object for LIB /DEF (finding 574). LIB 5.0 maps an undecorated
 * .def export to the __stdcall symbol an object defines, so these bodies give
 * the import members their decorated names (_DirectInputCreateA@16,
 * _DirectSoundCreate@12) while the import names stay undecorated. The object
 * is never linked into the executable.
 */
long __stdcall DirectInputCreateA(void *instance, unsigned long version,
                                  void **direct_input, void *outer)
{
    return 0;
}

long __stdcall DirectSoundCreate(void *guid, void **direct_sound, void *outer)
{
    return 0;
}
