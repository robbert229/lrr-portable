/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Minimal translation unit for Draw_Pixel24 (0x004869a0), issue #486. */
/* PADS */
typedef struct G { char pad[36]; void* buffer; int pitch; } G;
static G drawGlobs;
static void Draw_Pixel24(int x, int y, unsigned value)
{
    unsigned* addr = (unsigned*)((unsigned char*)drawGlobs.buffer + (y * drawGlobs.pitch) + (x * 3));
    *addr = (value << 8) | (*addr & 0xff);
}
void* keep = (void*)Draw_Pixel24;
