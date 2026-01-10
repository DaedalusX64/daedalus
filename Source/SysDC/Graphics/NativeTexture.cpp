/*
Copyright (C) 2005 StrmnNrmn

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

#include "stdafx.h"
#include "NativeTexture.h"

#include "DaedMathUtil.h"



namespace
{
//*****************************************************************************
//
//*****************************************************************************
void	CNativeTexture::CorrectDimensions( u32 width, u32 height, u32 & adjusted_width, u32 & adjusted_height )
{
	static const u32 MIN_TEXTURE_WIDTH = 8;
	static const u32 MIN_TEXTURE_HEIGHT = 8;

	adjusted_width = Max( GetNextPowerOf2( width ), MIN_TEXTURE_WIDTH );
	adjusted_height = Max( GetNextPowerOf2( height ), MIN_TEXTURE_HEIGHT );
}

//*****************************************************************************
//
//*****************************************************************************
CNativeTexture *	CNativeTexture::Create( u32 width, u32 height )
{
	u32		adjusted_width;
	u32		adjusted_height;

	CorrectDimensions( width, height, adjusted_width, adjusted_height );

    return new CNativeTexture( adjusted_width, adjusted_height);
}

//*****************************************************************************
//
//*****************************************************************************
CNativeTexture::CNativeTexture( u32 w, u32 h ) : mWidth( w ), mHeight( h )
{
	u32 * pdata( (u32*)pvr_mem_malloc(mWidth * mHeight * sizeof(u16)));
	if(pdata != NULL)
		mpData = pdata;
	}
}

//*****************************************************************************
//
//*****************************************************************************
CNativeTexture::~CNativeTexture()
{
    pvr_mem_free( mpData );
}

//*****************************************************************************
//
//*****************************************************************************
void	CNativeTexture::InstallTexture()
{
}

//*****************************************************************************
//
//*****************************************************************************
void	CNativeTexture::SetData( void * p_data, u32 w, u32 h )
{
    pvr_txr_load_ex(p_data, mpData, w, h, PVR_TXRLOAD_16BPP);
}

//*****************************************************************************
//
//*****************************************************************************
void	CNativeTexture::SetDataForceColour( void * p_data, u32 w, u32 h, u32 mask, u32 colour )
{
    pvr_txr_load_ex(p_data, mpData, w, h, PVR_TXRLOAD_16BPP);
}
