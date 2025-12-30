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

#ifndef __TEXTUREHANDLER_H__
#define __TEXTUREHANDLER_H__

//#include "RDP.h"

#include "SurfaceHandler.h"

typedef struct
{
	DWORD dwAddress;
	DWORD dwFormat;
	DWORD dwSize;
	LONG nLeft;
	LONG nTop;
	DWORD dwWidth;
	DWORD dwHeight;
	DWORD dwPitch;
	DWORD dwPalAddress;
	DWORD dwPalSize;
	DWORD dwTLutFmt;

	BOOL bSwapped;
} GetTextureInfo;

typedef struct TextureEntry
{
	struct TextureEntry *pNext;		// Must be first element!

	DWORD dwAddress;		// Corresponds to dwAddress in Tile

	DWORD dwUses;			// Total times used (for stats)
	DWORD dwTimeLastUsed;	// timeGetTime of time of last usage

	SurfaceHandler		*pSurface;

	LONG  nLeft;
	LONG  nTop;
	DWORD dwWidth;
	DWORD dwHeight;
	DWORD dwPitch;

	DWORD dwFormat;		// e.g. RGBA, IA
	DWORD dwSize;		// e.g. 16bpp

	BOOL  bSwapped;			// Are odd lines word swapped?

	DWORD dwPalAddress;
	DWORD dwPalSize;
	DWORD dwTLutFmt;

	DWORD dwCRC;

	// TODO: when nXTimes and nYTimes in the mirror emulator are correctly determined, these should be put here
	SurfaceHandler *pMirroredSurface;

	TextureEntry()
	{
		pMirroredSurface = NULL;
	}

	~TextureEntry()
	{
		SAFE_DELETE(pMirroredSurface);
	}
} TextureEntry;

extern BOOL g_bTHMakeTexturesBlue;
extern BOOL g_bTHDumpTextures;


BOOL TH_InitTable(DWORD dwSize);
void TH_DestroyTable(void);

void TH_PurgeOldTextures();
void TH_DropTextures();


TextureEntry * TH_GetTexture(GetTextureInfo * pgti);


#endif
