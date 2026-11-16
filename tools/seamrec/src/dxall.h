/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Every DX6-era header the seam recorder wraps, in C (CINTERFACE) form. */
#ifndef SEAMREC_DXALL_H
#define SEAMREC_DXALL_H
#define CINTERFACE
#define DIRECTINPUT_VERSION 0x0500
#define DIRECTSOUND_VERSION 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <objbase.h>
#include <ddraw.h>
#include <d3d.h>
#include <d3drm.h>
#include <dinput.h>
#include <dsound.h>
#include <mmstream.h>
#include <amstream.h>
#include <ddstream.h>
#include <vfw.h>
#endif
