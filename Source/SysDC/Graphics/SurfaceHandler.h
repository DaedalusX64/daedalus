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


#ifndef __SURFACEHANDLER_H__
#define __SURFACEHANDLER_H__

#include    "windef.h"

/////////////// Define a struct to use as
///////////////  storage for all the surfaces
///////////////  created so far.
class SurfaceHandler;
struct SurfaceNode {
	SurfaceNode *pNext;
	SurfaceHandler *pSH;
};

typedef struct {
	DWORD		dwWidth;			// Describes the width of the locked area. Use lPitch to move between successive lines
	DWORD		dwHeight;		// Describes the height of the locked area
	LONG		lPitch;			// Specifies the number of bytes on each row (not necessarily bitdepth*width/8)
	VOID		*lpSurface;		// Pointer to the top left pixel of the image
} DrawInfo;

class SurfaceHandler {
public:
	SurfaceHandler(DWORD dwWidth, DWORD dwHeight);
	~SurfaceHandler();

	BOOL      Resize(DWORD dwWidth, DWORD dwHeight);
	
	BOOL      StartUpdate(DrawInfo *di);
	void      EndUpdate(DrawInfo *di);

    pvr_ptr_t GetSurface() { return m_pvrSurf; }
    DWORD     GetWidth() { return m_dwWidth; }
    DWORD     GetHeight() { return m_dwHeight; }
    DWORD     GetCorrectedWidth() { return m_dwCorrectedWidth; }
    DWORD     GetCorrectedHeight() { return m_dwCorrectedHeight; }

	HRESULT MakeBlue(DWORD dwWidth, DWORD dwHeight);
	// Generic rendering operations
	void Clear(void);
private:
	pvr_ptr_t CreateSurface(DWORD dwWidth, DWORD dwHeight);
	void ScaleTempBufferToSurface();

private:
    pvr_ptr_t               m_pvrSurf;
	DWORD					m_dwWidth;			// The requested Texture w/h
	DWORD					m_dwHeight;

	DWORD					m_dwCorrectedWidth;	// What was actually created
	DWORD					m_dwCorrectedHeight;
};


#endif
