/* SPDX-License-Identifier: LGPL-3.0-or-later */
/*
 * Prototype object for ddraw.def (finding 574's form). LIB 5.0 maps an
 * undecorated .def export to the __stdcall symbol an object defines, so the
 * three imports the program references get their decorated member names
 * (DirectDrawCreateClipper is referenced only by removed code, finding 694);
 * the other exports are only numbered, for their hints. The object is never
 * linked into the executable.
 */
long __stdcall DirectDrawCreate(void *guid, void **direct_draw, void *outer) { return 0; }
long __stdcall DirectDrawCreateClipper(unsigned long flags, void **clipper, void *outer) { return 0; }
long __stdcall DirectDrawEnumerateA(void *callback, void *context) { return 0; }
long __stdcall DirectDrawEnumerateW(void *callback, void *context) { return 0; }
void AcquireDDThreadLock(void) {}
void CompleteCreateSysmemSurface(void) {}
void D3DParseUnknownCommand(void) {}
void DDGetAttachedSurfaceLcl(void) {}
void DDInternalLock(void) {}
void DDInternalUnlock(void) {}
void DSoundHelp(void) {}
void DllCanUnloadNow(void) {}
void DllGetClassObject(void) {}
void GetDDSurfaceLocal(void) {}
void GetOLEThunkData(void) {}
void GetSurfaceFromDC(void) {}
void RegisterSpecialCase(void) {}
void ReleaseDDThreadLock(void) {}
