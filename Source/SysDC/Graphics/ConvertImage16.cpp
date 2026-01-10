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

#include "StdAfx.h"
#include "RDP.h"
#include "core/memory.h"
#include "texture.h"
#include "texturecache.h"
#include "ConvertImage.h"

// Still to be swapped:
// IA16


void ConvertRGBA16_16(CTexture *pSurf, const TextureInfo & ti)
{
	DrawInfo dst;
	u32 x, y;
	long nFiddle;
	
    // Zeiger auf die Textur im Speicher holen
    u8 * pSrc = g_pu8RamBase + ti.Address;

	// Copy of the base pointer
	u8 * pByteSrc = (u8 *)pSrc;

	if (!pSurf->StartUpdate(&dst))
		return;

	if (ti.IsSwapped())
	{

		for (y = 0; y < ti.Height; y++)
		{
			if ((y%2) == 0)
				nFiddle = 0x2;
			else
				nFiddle = 0x2 | 0x4;

			// dwDst points to start of destination row
			WORD * wDst = (WORD *)((u8 *)dst.pSurface + y*dst.Pitch);

			// DWordOffset points to the current dword we're looking at
			// (process 2 pixels at a time). May be a problem if we don't start on even pixel
			u32 dwWordOffset = ((y+ti.Top) * ti.Pitch) + (ti.Left * 2);

			for (x = 0; x < ti.Width; x++)
			{
				WORD w = *(WORD *)&pByteSrc[dwWordOffset ^ nFiddle];

				wDst[x] = Convert555ToR4G4B4A4(w);

				// Increment word offset to point to the next two pixels
				dwWordOffset += 2;
			}
		}
	}
	else
	{
		for (y = 0; y < ti.Height; y++)
		{
			// dwDst points to start of destination row
			WORD * wDst = (WORD *)((u8 *)dst.pSurface + y*dst.Pitch);

			// DWordOffset points to the current dword we're looking at
			// (process 2 pixels at a time). May be a problem if we don't start on even pixel
			u32 dwWordOffset = ((y+ti.Top) * ti.Pitch) + (ti.Left * 2);

			for (x = 0; x < ti.Width; x++)
			{
				WORD w = *(WORD *)&pByteSrc[dwWordOffset ^ 0x2];

				wDst[x] = Convert555ToR4G4B4A4(w);

				// Increment word offset to point to the next two pixels
				dwWordOffset += 2;
			}
		}
	}

	pSurf->EndUpdate(&dst);
}

void ConvertRGBA32_16(CTexture *pSurf, const TextureInfo & ti)
{
	DrawInfo dst;
	if (!pSurf->StartUpdate(&dst))
		return;

    // Zeiger auf die Textur im Speicher holen
    u8 * pSrc = g_pu8RamBase + ti.Address;

	if (ti.IsSwapped())
	{

		for (u32 y = 0; y < ti.Height; y++)
		{
			if ((y%2) == 0)
			{

				WORD *pD = (WORD*)((u8 *)dst.pSurface + y * dst.Pitch);
				u8 *pS = (u8 *)pSrc + (y+ti.Top) * ti.Pitch + (ti.Left*4);

				for (u32 x = 0; x < ti.Width; x++)
				{

					*pD++ = R4G4B4A4_MAKE((pS[3]>>4),		// Red
										  (pS[2]>>4),
										  (pS[1]>>4),
										  (pS[0]>>4));		// Alpha
					pS+=4;
				}
			}
			else
			{

				WORD *pD = (WORD*)((u8 *)dst.pSurface + y * dst.Pitch);
				u8 *pS = (u8 *)pSrc + (y+ti.Top) * ti.Pitch + (ti.Left*4);
				long n;

				n = 0;
				for (u32 x = 0; x < ti.Width; x++)
				{
					*pD++ = R4G4B4A4_MAKE((pS[(n^0x8) + 3]>>4),		// Red
										  (pS[(n^0x8) + 2]>>4),
										  (pS[(n^0x8) + 1]>>4),
										  (pS[(n^0x8) + 0]>>4));	// Alpha

					n += 4;
				}
			}
		}
	}
	else
	{
		for (u32 y = 0; y < ti.Height; y++)
		{
			WORD *pD = (WORD*)((u8 *)dst.pSurface + y * dst.Pitch);
			u8 *pS = (u8 *)pSrc + (y+ti.Top) * ti.Pitch + (ti.Left*4);

			for (u32 x = 0; x < ti.Width; x++)
			{
				*pD++ = R4G4B4A4_MAKE((pS[3]>>4),		// Red
									  (pS[2]>>4),
									  (pS[1]>>4),
									  (pS[0]>>4));		// Alpha
				pS+=4;
			}
		}

	}
	pSurf->EndUpdate(&dst);

}

// E.g. Dear Mario text
// Copy, Score etc
void ConvertIA4_16(CTexture *pSurf, const TextureInfo & ti)
{
	DrawInfo dst;
	long nFiddle;

	if (!pSurf->StartUpdate(&dst))
		return;
		
    // Zeiger auf die Textur im Speicher holen
    u8 * pSrc = g_pu8RamBase + ti.Address;

	if (ti.IsSwapped())
	{
		for (u32 y = 0; y < ti.Height; y++)
		{
			WORD *pD = (WORD*)((u8 *)dst.pSurface + y * dst.Pitch);

			// For odd lines, swap words too
			if ((y%2) == 0)
				nFiddle = 0x3;
			else
				nFiddle = 0x7;


			// This may not work if X is not even?
			u32 dwByteOffset = (y+ti.Top) * ti.Pitch + (ti.Left/2);

			// Do two pixels at a time
			for (u32 x = 0; x < ti.Width; x+=2)
			{
				u8 b = pSrc[dwByteOffset ^ nFiddle];

				// Even
				*pD++ = R4G4B4A4_MAKE(TwoToFour[(b & 0xc0) >> 6],
									  TwoToFour[(b & 0xc0) >> 6],
									  TwoToFour[(b & 0xc0) >> 6],
									  TwoToFour[(b & 0x30) >> 4]);

				// Odd
				*pD++ = R4G4B4A4_MAKE(TwoToFour[(b & 0x0c) >> 2],
									  TwoToFour[(b & 0x0c) >> 2],
									  TwoToFour[(b & 0x0c) >> 2],
									  TwoToFour[(b & 0x03)     ]);

				dwByteOffset++;

			}

		}
	}
	else
	{
		for (u32 y = 0; y < ti.Height; y++)
		{
			WORD *pD = (WORD*)((u8 *)dst.pSurface + y * dst.Pitch);

			// This may not work if X is not even?
			u32 dwByteOffset = (y+ti.Top) * ti.Pitch + (ti.Left/2);

			// Do two pixels at a time
			for (u32 x = 0; x < ti.Width; x+=2)
			{
				u8 b = pSrc[dwByteOffset ^ 0x3];

				// Even
				*pD++ = R4G4B4A4_MAKE(TwoToFour[(b & 0xc0) >> 6],
									  TwoToFour[(b & 0xc0) >> 6],
									  TwoToFour[(b & 0xc0) >> 6],
									  TwoToFour[(b & 0x30) >> 4]);
				// Odd
				*pD++ = R4G4B4A4_MAKE(TwoToFour[(b & 0x0c) >> 2],
									  TwoToFour[(b & 0x0c) >> 2],
									  TwoToFour[(b & 0x0c) >> 2],
									  TwoToFour[(b & 0x03)     ]);


				dwByteOffset++;

			}
		}
	}

	pSurf->EndUpdate(&dst);

}

// E.g Mario's head textures
void ConvertIA8_16(CTexture *pSurf, const TextureInfo & ti)
{
	DrawInfo dst;
	long nFiddle;

	if (!pSurf->StartUpdate(&dst))
		return;
		
    // Zeiger auf die Textur im Speicher holen
    u8 * pSrc = g_pu8RamBase + ti.Address;

	if (ti.IsSwapped())
	{
		for (u32 y = 0; y < ti.Height; y++)
		{
			// For odd lines, swap words too
			if ((y%2) == 0)
				nFiddle = 0x3;
			else
				nFiddle = 0x7;


			WORD *pD = (WORD *)((u8*)dst.pSurface + y * dst.Pitch);
			// Points to current byte
			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + ti.Left;

			for (u32 x = 0; x < ti.Width; x++)
			{
				u8 b = pSrc[dwByteOffset ^ nFiddle];

				*pD++ = R4G4B4A4_MAKE( ((b&0xf0)>>4),((b&0xf0)>>4),((b&0xf0)>>4),(b&0x0f));

				dwByteOffset++;
			}

		}
	}
	else
	{

		for (u32 y = 0; y < ti.Height; y++)
		{
			WORD *pD = (WORD*)((u8 *)dst.pSurface + y * dst.Pitch);


			// Points to current byte
			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + ti.Left;

			for (u32 x = 0; x < ti.Width; x++)
			{
				u8 b = pSrc[dwByteOffset ^ 0x3];

				*pD++ = R4G4B4A4_MAKE(((b&0xf0)>>4),((b&0xf0)>>4),((b&0xf0)>>4),(b&0x0f));

				dwByteOffset++;
			}
		}
	}

	pSurf->EndUpdate(&dst);

}

// E.g. camera's clouds, shadows
void ConvertIA16_16(CTexture *pSurf, const TextureInfo & ti)
{
	DrawInfo dst;

	if (!pSurf->StartUpdate(&dst))
		return;
		
	// Zeiger auf die Textur im Speicher holen
    u8 * pSrc = g_pu8RamBase + ti.Address;
    u8 * pByteSrc = (u8 *)pSrc;

	for (u32 y = 0; y < ti.Height; y++)
	{
		WORD *pD = (WORD*)((u8 *)dst.pSurface + y * dst.Pitch);

		// Points to current word
		u32 dwWordOffset = ((y+ti.Top) * ti.Pitch) + (ti.Left * 2);

		for (u32 x = 0; x < ti.Width; x++)
		{
			WORD w = *(WORD *)&pByteSrc[dwWordOffset^0x2];

			u8 i = (u8)(w >> 12);
			u8 a = (u8)(w & 0xFF);

			*pD++ = R4G4B4A4_MAKE(i, i, i, (a>>4));

			dwWordOffset += 2;
		}
	}
	pSurf->EndUpdate(&dst);
}



// Used by MarioKart
void ConvertI4_16(CTexture *pSurf, const TextureInfo & ti)
{
	DrawInfo dst;
	long nFiddle;

	if (!pSurf->StartUpdate(&dst))
		return;
		
    // Zeiger auf die Textur im Speicher holen
    u8 * pSrc = g_pu8RamBase + ti.Address;

	if (ti.IsSwapped())
	{

		for (u32 y = 0; y < ti.Height; y++)
		{
			WORD *pD = (WORD*)((u8 *)dst.pSurface + y * dst.Pitch);

			// Might not work with non-even starting X
			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + (ti.Left / 2);

			// For odd lines, swap words too
			if ((y%2) == 0)
				nFiddle = 0x3;
			else
				nFiddle = 0x7;

			for (u32 x = 0; x < ti.Width; x+=2)
			{
				u8 b = pSrc[dwByteOffset ^ nFiddle]>>4;

				// Even
				*pD++ = R4G4B4A4_MAKE(b>>4,
									  b>>4,
									  b>>4,
									  b>>4);
				// Odd
				*pD++ = R4G4B4A4_MAKE(b & 0x0f,
									  b & 0x0f,
									  b & 0x0f,
									  b & 0x0f);

				dwByteOffset++;
			}

		}

	}
	else
	{

		for (u32 y = 0; y < ti.Height; y++)
		{
			WORD *pD = (WORD*)((u8 *)dst.pSurface + y * dst.Pitch);

			// Might not work with non-even starting X
			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + (ti.Left / 2);

			for (u32 x = 0; x < ti.Width; x+=2)
			{
				u8 b = pSrc[dwByteOffset ^ 0x3];

				// Even
				*pD++ = R4G4B4A4_MAKE(b>>4,
									  b>>4,
									  b>>4,
									  b>>4);

				// Odd
				*pD++ = R4G4B4A4_MAKE(b & 0x0f,
									  b & 0x0f,
									  b & 0x0f,
									  b & 0x0f);

				dwByteOffset++;
			}
		}
	}
	pSurf->EndUpdate(&dst);
}

// Used by MarioKart
void ConvertI8_16(CTexture *pSurf, const TextureInfo & ti)
{
	DrawInfo dst;
	long nFiddle;

	if (!pSurf->StartUpdate(&dst))
		return;

    // Zeiger auf die Textur im Speicher holen
    u8 * pSrc = g_pu8RamBase + ti.Address;

	if (ti.IsSwapped())
	{
		for (u32 y = 0; y < ti.Height; y++)
		{
			if ((y%2) == 0)
				nFiddle = 0x3;
			else
				nFiddle = 0x7;

			WORD *pD = (WORD*)((u8 *)dst.pSurface + y * dst.Pitch);

			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + ti.Left;

			for (u32 x = 0; x < ti.Width; x++)
			{
				u8 b = pSrc[dwByteOffset ^ nFiddle];

				*pD++ = R4G4B4A4_MAKE(b>>4,
									  b>>4,
									  b>>4,
									  b>>4);

				dwByteOffset++;
			}
		}
	}
	else
	{
		for (u32 y = 0; y < ti.Height; y++)
		{
			WORD *pD = (WORD*)((u8 *)dst.pSurface + y * dst.Pitch);

			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + ti.Left;

			for (u32 x = 0; x < ti.Width; x++)
			{
				u8 b = pSrc[dwByteOffset ^ 0x3];

				*pD++ = R4G4B4A4_MAKE(b>>4,
									  b>>4,
									  b>>4,
									  b>>4);

				dwByteOffset++;
			}
		}

	}
	pSurf->EndUpdate(&dst);

}


// Used by Starfox intro
void ConvertCI4_RGBA16_16(CTexture *pSurf, const TextureInfo & ti)
{
	DrawInfo dst;
	long nFiddle;

	if (!pSurf->StartUpdate(&dst))
		return;
		
    // Zeiger auf die Textur im Speicher holen
    u8 * pSrc = g_pu8RamBase + ti.Address;
    u16 * pPal = (u16 *)&gTextureMemory[ ti.TmemPalAddress << 3 ];

	if (ti.IsSwapped())
	{

		for (u32 y = 0; y <  ti.Height; y++)
		{
			if ((y%2) == 0)
				nFiddle = 0x3;
			else
				nFiddle = 0x7;


			WORD * pD = (WORD *)((u8 *)dst.pSurface + y * dst.Pitch);

			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + (ti.Left / 2);

			for (u32 x = 0; x < ti.Width; x+=2)
			{
				u8 b = pSrc[dwByteOffset ^ nFiddle];

				u8 bhi = (b&0xf0)>>4;
				u8 blo = (b&0x0f);

				pD[0] = Convert555ToR4G4B4A4(pPal[bhi^0x1]);	// Remember palette is in different endian order!
				pD[1] = Convert555ToR4G4B4A4(pPal[blo^0x1]);	// Remember palette is in different endian order!
				pD+=2;

				dwByteOffset++;
			}
		}

	}
	else
	{

		for (u32 y = 0; y <  ti.Height; y++)
		{
			WORD * pD = (WORD *)((u8 *)dst.pSurface + y * dst.Pitch);

			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + (ti.Left / 2);

			for (u32 x = 0; x < ti.Width; x+=2)
			{
				u8 b = pSrc[dwByteOffset ^ 0x3];

				u8 bhi = (b&0xf0)>>4;
				u8 blo = (b&0x0f);

				pD[0] = Convert555ToR4G4B4A4(pPal[bhi^0x1]);	// Remember palette is in different endian order!
				pD[1] = Convert555ToR4G4B4A4(pPal[blo^0x1]);	// Remember palette is in different endian order!
				pD+=2;

				dwByteOffset++;
			}
		}

	}
	pSurf->EndUpdate(&dst);
}

// Used by Starfox intro
void ConvertCI4_IA16_16(CTexture *pSurf, const TextureInfo & ti)
{
	DrawInfo dst;
	long nFiddle;

	if (!pSurf->StartUpdate(&dst))
		return;

    // Zeiger auf die Textur im Speicher holen
    u8 * pSrc = g_pu8RamBase + ti.Address;
    u16 * pPal = (u16 *)&gTextureMemory[ ti.TmemPalAddress << 3 ];

	if (ti.IsSwapped())
	{

		for (u32 y = 0; y <  ti.Height; y++)
		{
			if ((y%2) == 0)
				nFiddle = 0x3;
			else
				nFiddle = 0x7;


			WORD * pD = (WORD *)((u8 *)dst.pSurface + y * dst.Pitch);

			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + (ti.Left / 2);

			for (u32 x = 0; x < ti.Width; x+=2)
			{
				u8 b = pSrc[dwByteOffset ^ nFiddle];

				u8 bhi = (b&0xf0)>>4;
				u8 blo = (b&0x0f);

				pD[0] = ConvertIA16ToR4G4B4A4(pPal[bhi^0x1]);	// Remember palette is in different endian order!
				pD[1] = ConvertIA16ToR4G4B4A4(pPal[blo^0x1]);	// Remember palette is in different endian order!
				pD += 2;
				dwByteOffset++;
			}
		}

	}
	else
	{

		for (u32 y = 0; y <  ti.Height; y++)
		{
			WORD * pD = (WORD *)((u8 *)dst.pSurface + y * dst.Pitch);

			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + (ti.Left / 2);

			for (u32 x = 0; x < ti.Width; x+=2)
			{
				u8 b = pSrc[dwByteOffset ^ 0x3];

				u8 bhi = (b&0xf0)>>4;
				u8 blo = (b&0x0f);

				pD[0] = ConvertIA16ToR4G4B4A4(pPal[bhi^0x1]);	// Remember palette is in different endian order!
				pD[1] = ConvertIA16ToR4G4B4A4(pPal[blo^0x1]);	// Remember palette is in different endian order!
				pD+=2;

				dwByteOffset++;
			}
		}

	}
	pSurf->EndUpdate(&dst);
}




// Used by MarioKart for Cars etc
void ConvertCI8_RGBA16_16(CTexture *pSurf, const TextureInfo & ti)
{
	DrawInfo dst;
	long nFiddle;

	if (!pSurf->StartUpdate(&dst))
		return;
		
    // Zeiger auf die Textur im Speicher holen
    u8 * pSrc = g_pu8RamBase + ti.Address;
    u16 * pPal = (u16 *)&gTextureMemory[ ti.TmemPalAddress << 3 ];

	if (ti.IsSwapped())
	{


		for (u32 y = 0; y < ti.Height; y++)
		{
			if ((y%2) == 0)
				nFiddle = 0x3;
			else
				nFiddle = 0x7;

			WORD *pD = (WORD *)((u8 *)dst.pSurface + y * dst.Pitch);

			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + ti.Left;

			for (u32 x = 0; x < ti.Width; x++)
			{
				u8 b = pSrc[dwByteOffset ^ nFiddle];

				*pD++ = Convert555ToR4G4B4A4(pPal[b^0x1]);	// Remember palette is in different endian order!

				dwByteOffset++;
			}
		}


	}
	else
	{

		for (u32 y = 0; y < ti.Height; y++)
		{
			WORD *pD = (WORD *)((u8 *)dst.pSurface + y * dst.Pitch);

			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + ti.Left;

			for (u32 x = 0; x < ti.Width; x++)
			{
				u8 b = pSrc[dwByteOffset ^ 0x3];

				*pD++ = Convert555ToR4G4B4A4(pPal[b^0x1]);	// Remember palette is in different endian order!

				dwByteOffset++;
			}
		}
	}
	pSurf->EndUpdate(&dst);

}


// Used by MarioKart for Cars etc
void ConvertCI8_IA16_16(CTexture *pSurf, const TextureInfo & ti)
{
	DrawInfo dst;
	long nFiddle;

	if (!pSurf->StartUpdate(&dst))
		return;
		
    // Zeiger auf die Textur im Speicher holen
    u8 * pSrc = g_pu8RamBase + ti.Address;
    u16 * pPal = (u16 *)&gTextureMemory[ ti.TmemPalAddress << 3 ];

	if (ti.IsSwapped())
	{


		for (u32 y = 0; y < ti.Height; y++)
		{
			if ((y%2) == 0)
				nFiddle = 0x3;
			else
				nFiddle = 0x7;

			WORD *pD = (WORD *)((u8 *)dst.pSurface + y * dst.Pitch);

			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + ti.Left;

			for (u32 x = 0; x < ti.Width; x++)
			{
				u8 b = pSrc[dwByteOffset ^ nFiddle];

				*pD++ = ConvertIA16ToR4G4B4A4(pPal[b^0x1]);	// Remember palette is in different endian order!

				dwByteOffset++;
			}
		}


	}
	else
	{

		for (u32 y = 0; y < ti.Height; y++)
		{
			WORD *pD = (WORD *)((u8 *)dst.pSurface + y * dst.Pitch);

			u32 dwByteOffset = ((y+ti.Top) * ti.Pitch) + ti.Left;

			for (u32 x = 0; x < ti.Width; x++)
			{
				u8 b = pSrc[dwByteOffset ^ 0x3];

				*pD++ = ConvertIA16ToR4G4B4A4(pPal[b^0x1]);	// Remember palette is in different endian order!

				dwByteOffset++;
			}
		}
	}
	pSurf->EndUpdate(&dst);

}

