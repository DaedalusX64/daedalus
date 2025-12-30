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

// Manages textures for RDP code
// Uses a HashTable (hashing on TImg) to allow quick access
//  to previously used textures

#include "main.h"

#include "Debug.h"
#include "Memory.h"
#include "ROM.h"
#include "RDP.h"
#include "TextureHandler.h"
#include "ConvertImage.h"

#include "ultra_gbi.h"		// For image size/formats
#include "DBGConsole.h"
#include "daedalus_crc.h"



static TextureEntry * g_pFirstUsedSurface = NULL;


static TextureEntry ** g_pTextureHash = NULL;
static DWORD g_dwTextureHashSize;

static void TH_Lock();
static void TH_Unlock();
static void TH_AddToRecycleList(TextureEntry *pEntry);
static TextureEntry * TH_ReviveUsedTexture(DWORD dwWidth, DWORD dwHeight);
static void TH_AddTextureEntry(TextureEntry *pEntry);
static void TH_RemoveTextureEntry(TextureEntry * pEntry);
static DWORD TH_Hash(DWORD dwValue);
static TextureEntry * TH_GetEntry(DWORD dwAddress, DWORD dwPalAddress, LONG nLeft, LONG nTop, DWORD dwWidth, DWORD dwHeight);

static void TH_DecompressTexture(TextureEntry * pEntry);
static void TH_DecompressTexture_16(TextureEntry * pEntry);



static TextureEntry * TH_CreateEntry(DWORD dwAddress, DWORD dwWidth, DWORD dwHeight);


BOOL g_bTHMakeTexturesBlue = FALSE;
BOOL g_bTHDumpTextures = FALSE;


// Returns the first prime greater than or equal to nFirst
inline LONG GetNextPrime(LONG nFirst)
{
	LONG nCurrent;

	LONG i;

	nCurrent = nFirst;

	// Just make sure it's odd
	if ((nCurrent % 2) == 0)
		nCurrent++;

	for (;;)
	{
		LONG nSqrtCurrent;
		BOOL bIsComposite;

		// nSqrtCurrent = nCurrent^0.5 + 1 (round up)
		nSqrtCurrent = (LONG)fsqrt((long double)nCurrent) + 1;


		bIsComposite = FALSE;
		
		// Test all odd numbers from 3..nSqrtCurrent
		for (i = 3; i <= nSqrtCurrent; i+=2)
		{
			if ((nCurrent % i) == 0)
			{
				bIsComposite = TRUE;
				break;
			}
		}

		if (!bIsComposite)
		{			
			return nCurrent;
		}

		// Select next odd candidate...
		nCurrent += 2;
	}

}

BOOL TH_InitTable(DWORD dwSize)
{	
	dwSize = GetNextPrime(dwSize);
	
	g_pTextureHash = new TextureEntry *[dwSize];
	if (g_pTextureHash == NULL)
		return FALSE;

	g_dwTextureHashSize = dwSize;

	for (DWORD i = 0; i < dwSize; i++)
		g_pTextureHash[i] = NULL;

	return TRUE;	// Success!
}

void TH_DestroyTable(void)
{
	TH_DropTextures();

	delete []g_pTextureHash;
	g_pTextureHash = NULL;	

	
	while (g_pFirstUsedSurface)
	{
		TextureEntry * pVictim = g_pFirstUsedSurface;
		g_pFirstUsedSurface = pVictim->pNext;

		SAFE_DELETE(pVictim->pSurface);
		delete pVictim;
 }
}

// Purge any textures whos last usage was over 5 seconds ago
void TH_PurgeOldTextures()
{

	if (g_pTextureHash == NULL)
		return;
	
	TH_Lock();

	// Give us slightly longer before purging textures when using a (slow) debug build
#ifdef _DEBUG
	static const DWORD dwSecsToKill = 20;
#else
	static const DWORD dwSecsToKill = 5;
#endif	

	for (DWORD i = 0; i < g_dwTextureHashSize; i++)
	{
		TextureEntry * pEntry;
		TextureEntry * pNext;

		pEntry = g_pTextureHash[i];
		while (pEntry)
		{
			pNext = pEntry->pNext;

			if (g_dwRDPTime - pEntry->dwTimeLastUsed > (dwSecsToKill*1000))
			{
				TH_RemoveTextureEntry(pEntry);
			}
			pEntry = pNext;
		}
	}


	// Remove any old textures that haven't been recycled in 1 minute or so
	// Normally these would be reused
	TextureEntry * pPrev;
	TextureEntry * pCurr;
	TextureEntry * pNext;


	pPrev = NULL;
	pCurr = g_pFirstUsedSurface;

	while (pCurr)
	{
		pNext = pCurr->pNext;
		
		if (g_dwRDPTime - pCurr->dwTimeLastUsed > (30*1000))
		{
			// Everything from this point on should be too old!
			// Remove from list
			if (pPrev != NULL) pPrev->pNext        = pCurr->pNext;
			else			   g_pFirstUsedSurface = pCurr->pNext;

			//printf("Killing old used texture (%d x %d)", pCurr->dwWidth, pCurr->dwHeight);

				
			SAFE_DELETE(pCurr->pSurface);
			delete pCurr;
			
			// pPrev remains the same
			pCurr = pNext;

		}
		else
		{
			pPrev = pCurr;
			pCurr = pNext;
		}
	}
	

	TH_Unlock();

}

void TH_DropTextures()
{
	if (g_pTextureHash == NULL)
		return;
	
	TH_Lock();

	DWORD dwCount = 0;
	DWORD dwTotalUses = 0;

	for (DWORD i = 0; i < g_dwTextureHashSize; i++)
	{
		while (g_pTextureHash[i])
		{
			TextureEntry *pTVictim = g_pTextureHash[i];
			g_pTextureHash[i] = pTVictim->pNext;

			dwTotalUses += pTVictim->dwUses;
			dwCount++;

			TH_AddToRecycleList(pTVictim);
		}
	}
	
	DPF(DEBUG_THANDLER, "Texture Handler: %d entries in hash table, %d uses.", dwCount, dwTotalUses);

	TH_Unlock();

}

void TH_Lock()
{
}

void TH_Unlock()
{
}


// Add to the recycle list
void TH_AddToRecycleList(TextureEntry *pEntry)
{
	if (pEntry->pSurface == NULL)
	{
		// No point in saving!
		delete pEntry;
	}
	else
	{
		// Add to the list
		pEntry->pNext = g_pFirstUsedSurface;
		g_pFirstUsedSurface = pEntry;
	}
}

// Search for a texture of the specified dimensions to recycle
TextureEntry * TH_ReviveUsedTexture(DWORD dwWidth, DWORD dwHeight)
{
	TextureEntry * pPrev;
	TextureEntry * pCurr;

	pPrev = NULL;
	pCurr = g_pFirstUsedSurface;

	while (pCurr)
	{
		if (pCurr->dwWidth == dwWidth &&
			pCurr->dwHeight == dwHeight)
		{
			// Remove from list
			if (pPrev != NULL) pPrev->pNext        = pCurr->pNext;
			else			   g_pFirstUsedSurface = pCurr->pNext;

			//DBGConsole_Msg(0, "Reviving used texture (%d x %d)", dwWidth, dwHeight);
			// Init any fields:
			return pCurr;
		}

		pPrev = pCurr;
		pCurr = pCurr->pNext;
	}

	

	return NULL;
}



DWORD TH_Hash(DWORD dwValue)
{
	// Divide by four, because most textures will be on a 4 byte boundry, so bottom four
	// bits are null
	return (dwValue>>2) % g_dwTextureHashSize;
}


void TH_AddTextureEntry(TextureEntry *pEntry)
{	
	DWORD dwKey = TH_Hash(pEntry->dwAddress);

	if (g_pTextureHash == NULL)
		return;

	DPF(DEBUG_THANDLER, "TH: 0x%08x hashes to 0x%08x", pEntry->dwAddress, dwKey);

	TH_Lock();

		TextureEntry **p = &g_pTextureHash[dwKey];

		// Add to head (not tail, for speed - new textures are more likely to be accessed next)
		pEntry->pNext = g_pTextureHash[dwKey];
		g_pTextureHash[dwKey] = pEntry;

	TH_Unlock();

}

TextureEntry * TH_GetEntry(DWORD dwAddress, DWORD dwPalAddress,
						   LONG nLeft, LONG nTop,
						   DWORD dwWidth, DWORD dwHeight)
{
	TextureEntry *pEntry;

	if (g_pTextureHash == NULL)
		return NULL;

	// See if it is already in the hash table
	DWORD dwKey = TH_Hash(dwAddress);
	for (pEntry = g_pTextureHash[dwKey];
		 pEntry;
		 pEntry = pEntry->pNext)
	{
		if (pEntry->dwAddress == dwAddress &&
			pEntry->dwPalAddress == dwPalAddress &&
			pEntry->nLeft == nLeft &&
			pEntry->nTop == nTop &&			
			pEntry->dwWidth == dwWidth &&
			pEntry->dwHeight == dwHeight)
			return pEntry;
	}
	return NULL;
}

void TH_RemoveTextureEntry(TextureEntry * pEntry)
{
	TextureEntry * pPrev;
	TextureEntry * pCurr;
	
	if (g_pTextureHash == NULL)
		return;

	TH_Lock();

		//DBGConsole_Msg(0, "Remove Texture entry!");

		// See if it is already in the hash table
		DWORD dwKey = TH_Hash(pEntry->dwAddress);

		pPrev = NULL;
		pCurr = g_pTextureHash[dwKey];

		while (pCurr)
		{
			// Check that the attributes match
			if (pCurr->dwAddress == pEntry->dwAddress &&
				pCurr->dwPalAddress == pEntry->dwPalAddress &&
				pCurr->nLeft == pEntry->nLeft &&
				pCurr->nTop == pEntry->nTop &&
				pCurr->dwWidth == pEntry->dwWidth &&
				pCurr->dwHeight == pEntry->dwHeight)
			{
				if (pPrev != NULL) pPrev->pNext = pCurr->pNext;
				else			   g_pTextureHash[dwKey] = pCurr->pNext;
				break;
			}

			pPrev = pCurr;
			pCurr = pCurr->pNext;
		}

	TH_Unlock();

	if (pCurr == NULL)
	{
		DBGConsole_Msg(0, "Entry not found!!!");
	}
	
	TH_AddToRecycleList(pEntry);
}

TextureEntry * TH_CreateEntry(DWORD dwAddress, DWORD dwWidth, DWORD dwHeight)
{
	TextureEntry * pEntry;

	// Find a used texture
	pEntry = TH_ReviveUsedTexture(dwWidth, dwHeight);
	if (pEntry == NULL)
	{
		// Couldn't find on - recreate!
		pEntry = new TextureEntry;
		if (pEntry == NULL)
			return NULL;

		pEntry->pSurface = new SurfaceHandler(dwWidth, dwHeight);

		if (pEntry->pSurface == NULL || pEntry->pSurface->GetSurface() == NULL)
		{
			//delete pEntry;
			//return NULL;
	//		DBGConsole_Msg(0, "Warning, unable to create %d x %d texture!", dwWidth, dwHeight);
			//if (pEntry->pSurface)
			//{
			//	delete pEntry->pSurface;
			//}
			//return NULL;
		}
	}
	
	// Initialise
	pEntry->dwAddress = dwAddress;
	pEntry->pNext = NULL;
	pEntry->dwUses = 0;
	pEntry->dwTimeLastUsed = g_dwRDPTime;

	// Add to the hash table
	TH_AddTextureEntry(pEntry);
	return pEntry;	
}

// If already in table, return
// Otherwise, create surfaces, and load texture into 
// memory
TextureEntry * TH_GetTexture(GetTextureInfo * pgti)
{
	TextureEntry *pEntry;

	DWORD dwCRC = 0;

	// Exit if alread cached
	pEntry = TH_GetEntry(pgti->dwAddress,
						 pgti->dwPalAddress,
						 pgti->nLeft,
						 pgti->nTop,
						 pgti->dwWidth,
						 pgti->dwHeight);

	if (pEntry && pEntry->dwTimeLastUsed == g_dwRDPTime)
	{
		// We've already calculated a CRC this frame!
		dwCRC = pEntry->dwCRC;
	}
	else if (g_bCRCCheck)
	{
		BYTE * pStart;
		DWORD dwBytesPerLine;
		DWORD x,y;

		dwBytesPerLine = ((pgti->dwWidth<<pgti->dwSize)+1)/2;

		// A very simple crc - just summation
		dwCRC = 0;
		pStart = g_pu8RamBase + pgti->dwAddress;
		pStart += (pgti->nTop * pgti->dwPitch) + (((pgti->nLeft<<pgti->dwSize)+1)/2);
		for (y = 0; y < pgti->dwHeight; y++)		// Do every nth line?
		{
			// Byte fiddling won't work, but this probably doesn't matter
			// Now process 4 bytes at a time
			for (x = 0; x < dwBytesPerLine; x+=4)
			{
				dwCRC = (dwCRC + *(DWORD*)&pStart[x]);
			}
			pStart += pgti->dwPitch;

		}
		if (pgti->dwFormat == G_IM_FMT_CI)
		{
			pStart = g_pu8RamBase + pgti->dwPalAddress;
			for (y = 0; y < 256*2/*pgti->dwPalSize*/; y+=4)
			{
				dwCRC = (dwCRC + *(DWORD*)&pStart[y]);
			}
		}
	}

	if (pEntry)
	{
		
		if (pEntry->dwCRC == dwCRC)
		{
			// Tile is ok, return
			pEntry->dwUses++;
			pEntry->dwTimeLastUsed = g_dwRDPTime;			
			return pEntry;
		}
		else
		{
			if (pEntry->dwCRC != dwCRC)
			{
			//	DBGConsole_Msg(0, "Contents of 0x%08x changed (%d x %d)", 
			//		dwAddress, dwTileWidth, dwTileHeight);
			}

		}
	}
	
	if (pEntry == NULL)
	{
		// We need to create a new entry, and add it
		//  to the hash table.
		pEntry = TH_CreateEntry(pgti->dwAddress, pgti->dwWidth, pgti->dwHeight);
		if (pEntry == NULL)
			return NULL;
	}

	pEntry->dwFormat = pgti->dwFormat;
	pEntry->dwSize = pgti->dwSize;
	pEntry->dwCRC = dwCRC;
	pEntry->nLeft = pgti->nLeft;
	pEntry->nTop = pgti->nTop;
	pEntry->dwWidth = pgti->dwWidth;
	pEntry->dwHeight = pgti->dwHeight;
	pEntry->dwPalAddress = pgti->dwPalAddress;
	pEntry->dwPalSize = pgti->dwPalSize;
	pEntry->dwPitch = pgti->dwPitch;
	pEntry->dwTLutFmt = pgti->dwTLutFmt;
	pEntry->bSwapped = pgti->bSwapped;


	try 
	{
 		TH_DecompressTexture_16(pEntry);
	}
	catch (...)
	{
		DBGConsole_Msg(0, "Exception in texture decompression");
		if (!g_bTrapExceptions)
			throw;
	}
	

	return pEntry;

}




static const char *pszImgFormat[8] = {"RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"};
static const BYTE pnImgSize[4]   = {4, 8, 16, 32};

void TH_DecompressTexture_16(TextureEntry * pEntry)
{

	BOOL bHandled;	

	WORD * pPal = (WORD *)(g_pu8RamBase + pEntry->dwPalAddress);
	void * pSrc = g_pu8RamBase + pEntry->dwAddress;

	static DWORD dwCount = 0;

	bHandled = FALSE;
	switch (pEntry->dwFormat)
	{
	case G_IM_FMT_RGBA:
		switch (pEntry->dwSize)
		{
		case G_IM_SIZ_16b:
			ConvertRGBA16_16(pEntry->pSurface,
							(WORD *)pSrc, pEntry->dwPitch,
							pEntry->nLeft, pEntry->nTop, 
							pEntry->dwWidth, pEntry->dwHeight,
							pEntry->bSwapped);
			bHandled = TRUE;
			break;
		case G_IM_SIZ_32b:
			ConvertRGBA32_16(pEntry->pSurface,
							(DWORD *)pSrc, pEntry->dwPitch,
							pEntry->nLeft, pEntry->nTop, 
							pEntry->dwWidth, pEntry->dwHeight,
							pEntry->bSwapped);
			bHandled = TRUE;
			break;
		}
		break;

	case G_IM_FMT_YUV:
		break;
	case G_IM_FMT_CI:
		switch (pEntry->dwSize)
		{
		case G_IM_SIZ_4b: // 4bpp
			switch (pEntry->dwTLutFmt)
			{
			//case G_TT_NONE:
			case G_TT_RGBA16:
				ConvertCI4_RGBA16_16(pEntry->pSurface,
								(BYTE *)pSrc, pEntry->dwPitch,
								pPal, 
								pEntry->nLeft, pEntry->nTop, 
								pEntry->dwWidth, pEntry->dwHeight,
							pEntry->bSwapped);					
				bHandled = TRUE;
				break;
			case G_TT_IA16:
				ConvertCI4_IA16_16(pEntry->pSurface,
								(BYTE *)pSrc, pEntry->dwPitch,
								pPal, 
								pEntry->nLeft, pEntry->nTop, 
								pEntry->dwWidth, pEntry->dwHeight,
							pEntry->bSwapped);					
				bHandled = TRUE;
				break;
			}
			break;

		case G_IM_SIZ_8b: // 8bpp
			switch(pEntry->dwTLutFmt)
			{
			case G_TT_RGBA16:
				ConvertCI8_RGBA16_16(pEntry->pSurface,
								(BYTE *)pSrc, pEntry->dwPitch,
								pPal, 
								pEntry->nLeft, pEntry->nTop, 
								pEntry->dwWidth, pEntry->dwHeight,
							pEntry->bSwapped);					
				bHandled = TRUE;
				break;
			case G_TT_IA16:
				ConvertCI8_IA16_16(pEntry->pSurface,
								(BYTE *)pSrc, pEntry->dwPitch,
								pPal, 
								pEntry->nLeft, pEntry->nTop, 
								pEntry->dwWidth, pEntry->dwHeight,
							pEntry->bSwapped);					
				bHandled = TRUE;
				break;
			}
			break;
		}

		break;
	case G_IM_FMT_IA:
		switch (pEntry->dwSize)
		{
		case G_IM_SIZ_4b:
			ConvertIA4_16(pEntry->pSurface,
							(BYTE *)pSrc, pEntry->dwPitch,
							pEntry->nLeft, pEntry->nTop, 
							pEntry->dwWidth, pEntry->dwHeight,
							pEntry->bSwapped);
			bHandled = TRUE;
			break;
		case G_IM_SIZ_8b:
			ConvertIA8_16(pEntry->pSurface,
							(BYTE *)pSrc, pEntry->dwPitch,
							pEntry->nLeft, pEntry->nTop, 
							pEntry->dwWidth, pEntry->dwHeight,
							pEntry->bSwapped);
			bHandled = TRUE;
			break;
		case G_IM_SIZ_16b:
			ConvertIA16_16(pEntry->pSurface,
							(WORD *)pSrc, pEntry->dwPitch,
							pEntry->nLeft, pEntry->nTop, 
							pEntry->dwWidth, pEntry->dwHeight,
							pEntry->bSwapped);
			bHandled = TRUE;
			break;
		case G_IM_SIZ_32b:
			break;
		}
		break;
	case G_IM_FMT_I:
		switch (pEntry->dwSize)
		{
		case G_IM_SIZ_4b:

			ConvertI4_16(pEntry->pSurface,
							(BYTE *)pSrc, pEntry->dwPitch,
							pEntry->nLeft, pEntry->nTop, 
							pEntry->dwWidth, pEntry->dwHeight,
							pEntry->bSwapped);
			bHandled = TRUE;
			break;
		case G_IM_SIZ_8b:
			ConvertI8_16(pEntry->pSurface,
							(BYTE *)pSrc, pEntry->dwPitch,
							pEntry->nLeft, pEntry->nTop, 
							pEntry->dwWidth, pEntry->dwHeight,
							pEntry->bSwapped);
			bHandled = TRUE;
			break;
		}
		break;
	default:
        printf( "Frage mich nicht, was da schief l�uft\n");
		break;
	}

	if (bHandled)
	{
		// Should do this instead of decoding!
		if (g_bTHMakeTexturesBlue)
		{
			pEntry->pSurface->MakeBlue(pEntry->dwWidth, pEntry->dwHeight);
		}
	}
	else
	{
		static BOOL bWarningEmitted = FALSE;

		pEntry->pSurface->MakeBlue(pEntry->dwWidth, pEntry->dwHeight);

		if (!bWarningEmitted)
		{
			DPF(DEBUG_ALWAYS, "DecompressTexture: Unable to decompress %s/%dbpp", pszImgFormat[pEntry->dwFormat], pnImgSize[pEntry->dwSize]);

			printf("DecompressTexture: Unable to decompress %s/%dbpp", pszImgFormat[pEntry->dwFormat], pnImgSize[pEntry->dwSize]);

			//bWarningEmitted = TRUE;
		}
	}


	dwCount++;
}
