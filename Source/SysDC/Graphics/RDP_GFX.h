/*
Copyright (C) 2001 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#ifndef __DAEDALUS_RDP_GFX_H__
#define __DAEDALUS_RDP_GFX_H__

#include "ultra_sptask.h"
#include "Debug.h"



struct N64Light
{
	u32 dwRGBA, dwRGBACopy;
	float x,y,z;			// Direction
	BYTE pad;
};


struct Tile {
	u32 dwFormat;		// e.g. RGBA, YUV etc
	u32 dwSize;		// e.g 4/8/16/32bpp
	u32 dwLine;		// Ummm...
	u32 dwTMem;		// Texture memory location

	u32 dwAddress;	// Location of texture in RAM (not TMem, as it's too small)

	u32 dwPalette;	// 0..15 - a palette index?
	BOOL  bClampS, bClampT;
	BOOL  bMirrorS, bMirrorT;
	u32 dwMaskS, dwMaskT;
	u32 dwShiftS, dwShiftT;

	u32 nULS;		// Upper left S		- 8:3
	u32 nULT;		// Upper Left T		- 8:3
	u32 nLRS;		// Lower Right S
	u32 nLRT;		// Lower Right T

	u32 dwX1, dwY1, dwX2;
	u32 dwDXT;

	u32 dwPitch;
};



HRESULT RDP_GFX_Init();
HRESULT RDP_GFX_Reset();
void RDP_GFX_Cleanup();
void RDP_GFX_ExecuteTask(OSTask * pTask);


// Various debugger commands:
void RDP_GFX_DumpNextDisplayList();
void RDP_GFX_DumpNextScreen();
void RDP_GFX_DropTextures();
void RDP_GFX_MakeTexturesBlue();
void RDP_GFX_MakeTexturesNormal();
void RDP_GFX_DumpTextures();
void RDP_GFX_NoDumpTextures();

extern HANDLE g_hRDPDumpHandle;

// Lkb - I changed this to DAEDALUS_RELEASE_BUILD (from _DEBUG) because
// I tend to dump display lists a lot when developing. This #define 
// can be used for other features that don't need to be included in
// a public release 
#ifndef DAEDALUS_RELEASE_BUILD
inline void DL_PF(LPCTSTR szFormat, ...)
{
    char buffer[1024];

    va_list va;
    va_start( va, szFormat );

	vsprintf( buffer, szFormat, va );
	strcat( buffer, "\r\n");
    printf( buffer );

    va_end( va );
}
#else
#define DL_PF
#endif

#endif	// __DAEDALUS_RDP_GFX_H__
