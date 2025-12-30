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

#include "main.h"
#include <stdlib.h>
#include "DBGConsole.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#endif

#include "SurfaceHandler.h"
#include "PngUtil.h"

//////////////////////////////////////////
// Constructors / Deconstructors

// Probably shouldn't need more than 4096 * 4096
static BYTE g_ucTempBuffer[512*512];

SurfaceHandler::SurfaceHandler(DWORD dwWidth, DWORD dwHeight) :
    m_pvrSurf(NULL),
	m_dwWidth(0),
	m_dwHeight(0),
	m_dwCorrectedWidth(0),
	m_dwCorrectedHeight(0)

{
	Resize(dwWidth, dwHeight);
}

SurfaceHandler::~SurfaceHandler()
{
	if (m_pvrSurf)
	{
        pvr_mem_free(m_pvrSurf);
		m_pvrSurf = NULL;
	}
	m_dwWidth = 0;
	m_dwHeight = 0;
}

static void Init()
{
}

//////////////////////////////////////////
// Resize the DIB. Just passes control to
// CreateDIB()
BOOL
SurfaceHandler::Resize(DWORD dwWidth, DWORD dwHeight)
{
	if (dwWidth < 8)
		dwWidth = 8;
	else if (dwWidth > 1024)
		dwWidth = 1024;

	if (dwHeight < 8)
		dwHeight = 8;
	else if (dwHeight > 1024)
		dwHeight = 1024;

	// Check if surface is already of required dimensions etc
	if( m_pvrSurf && dwWidth == m_dwWidth && dwHeight == m_dwHeight)
		return TRUE;

	if (dwWidth < m_dwWidth)
		DBGConsole_Msg(0, "New width (%d) < Old Width (%d)", dwWidth, m_dwWidth);
	if (dwHeight < m_dwHeight)
		DBGConsole_Msg(0, "New height (%d) < Old height (%d)", dwHeight, m_dwHeight);

    pvr_ptr_t lpSurf = CreateSurface(dwWidth, dwHeight);
	if (lpSurf == NULL)
		return FALSE;

	// Copy from old surface to new surface
	if( m_pvrSurf != NULL )
	{
        pvr_mem_free( m_pvrSurf );
        m_pvrSurf = NULL;
	}

	m_dwWidth = dwWidth;
	m_dwHeight = dwHeight;
	m_pvrSurf = lpSurf;

	//GetSurfaceInfo();

	return TRUE;

}

DWORD TH_GetHigherPowerOf2(DWORD dwX)
{
	DWORD temp = dwX;
	DWORD dwNewX;

	for (DWORD w = 0; w < 12; w++)
	{
		dwNewX = 1<<w;
		if (dwNewX >= dwX)
			break;
	}
	return dwNewX;
}


/////////////////////////////////////////////
// Create the surface. Destroys old surface if
// necessary
pvr_ptr_t
SurfaceHandler::CreateSurface(DWORD dwWidth, DWORD dwHeight)
{
	HRESULT hr;
	pvr_ptr_t lpSurf;

	m_dwCorrectedWidth = TH_GetHigherPowerOf2( dwWidth );
	m_dwCorrectedHeight = TH_GetHigherPowerOf2( dwHeight );
    //printf( "Creating Texture (wirklich): %d (%d) x %d (%d)\n", m_dwCorrectedWidth, dwWidth, m_dwCorrectedHeight, dwHeight );
    lpSurf = pvr_mem_malloc( m_dwCorrectedWidth * m_dwCorrectedHeight * sizeof(WORD) );

	// D3D likes texture w/h to be a power of two, so the condition below
	// will almost always hold.
	// D3D should usually create textures large enough (on nVidia cards anyway),
	// and so there will usually be some "slack" left in the texture (blank space
	// that isn't used). The D3DRender code takes care of this (mostly)
	// Voodoo cards are limited to 256x256 and so often textures will be
	// created that are too small for the required dimensions. We compensate for
	// this by passing in a dummy area of memory when the surface is locked,
	// and copying across pixels to the real surface when the surface is unlocked.
	// In this case there will be no slack and D3DRender takes this into account.
	//


	if( lpSurf == NULL )
	{
		DBGConsole_Msg(0, "!!Unable to create surface!! %d x %d", dwWidth, dwHeight );
		return NULL;
	}

	return lpSurf;
}

//////////////////////////////////////////////////
// Get information about the DIBitmap
// This locks the bitmap (and stops
// it from being resized). Must be matched by a
// call to EndUpdate();
BOOL
SurfaceHandler::StartUpdate(DrawInfo *di)
{
	if( m_pvrSurf == NULL )
		return FALSE;

	// Return a temporary buffer to use
	di->lpSurface = g_ucTempBuffer;
	di->dwWidth = m_dwWidth;
	di->dwHeight = m_dwHeight;
	di->lPitch = m_dwWidth * 2;

	return TRUE;
}

#define TWIDTAB(x) ( (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)| \
	((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9) )
#define TWIDOUT(x, y) ( TWIDTAB((y)) | (TWIDTAB((x)) << 1) )

#define MIN(a, b) ( (a)<(b)? (a):(b) )

///////////////////////////////////////////////////
// This releases the DIB information, allowing it
// to be resized again
void
SurfaceHandler::EndUpdate(DrawInfo *di)
{
    if( m_pvrSurf == NULL )
		return;
		
    if( m_dwWidth == m_dwCorrectedWidth && m_dwHeight == m_dwCorrectedHeight )
    {
        pvr_txr_load_ex( di->lpSurface, m_pvrSurf, m_dwWidth, m_dwHeight, PVR_TXRLOAD_16BPP );
    }
    else
    {
        // Textur muss skaliert werden. Faktoren berechnen
        float fX = m_dwWidth / m_dwCorrectedWidth;
        float fY = m_dwHeight / m_dwCorrectedHeight;
        
        // Textur skalieren und dabei in den Videospeicher kopieren
        uint32 w = m_dwCorrectedWidth;
        uint32 h = m_dwCorrectedHeight;
        uint32 m = MIN(w, h);
	    uint32 mask = m - 1;

	    uint16 * pixels = (uint16 *) di->lpSurface;
	    uint16 * vtex = (uint16*)m_pvrSurf;
	    for( uint16 y=0; y<h; y++ ) {
            uint16 row = (int)(y * fY) * m_dwWidth;
            for( uint16 x=0; x<w; x++ ) {
                vtex[TWIDOUT(x&mask,y&mask) + (x/m + y/m)*m*m] = pixels[row+(int)(x * fX)];
		  }
	    }
    }
}

//////////////////////////////////////////////////////
// Set all pixels to (0,0,0) (also clears alpha)
void
SurfaceHandler::Clear(void)
{
	DWORD * pSrc = (DWORD *)m_pvrSurf;
	LONG lPitch =  m_dwWidth * 2;

	for (DWORD y = 0; y < m_dwHeight; y++)
	{
		DWORD * dwSrc = (DWORD *)((BYTE *)pSrc + y*lPitch);
		for (DWORD x = 0; x < m_dwWidth; x++)
		{
			dwSrc[x] = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
}


// Make the pixels of the texture blue
HRESULT
SurfaceHandler::MakeBlue(DWORD dwWidth, DWORD dwHeight)
{
	DWORD * pSrc = (DWORD *)m_pvrSurf;
	LONG lPitch =  m_dwWidth * 2;

	for (DWORD y = 0; y < dwHeight; y++)
	{
		DWORD * dwSrc = (DWORD *)((BYTE *)pSrc + y*lPitch);
		for (DWORD x = 0; x < dwWidth; x++)
		{
			dwSrc[x] = 0x000000FF | ((rand()% 255)<<8) | ((rand()%255)<<24);
		}
	}

	return S_OK;
}
