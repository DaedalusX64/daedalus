/*
Copyright (C) 2001,2006 StrmnNrmn

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

#ifndef __DAEDALUS_DAEDALUSVTX_H__
#define __DAEDALUS_DAEDALUSVTX_H__

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Graphics/ColourValue.h"

// The ordering of these elements is required for the VectorTnL code.
ALIGNED_TYPE(struct, DaedalusVtx4, 16)
{
    v4	TransformedPos;
    v4	ProjectedPos;
    v4	Colour;
    v2	Texture;
	
	u32	ClipFlags;
	u8	Padding[4];

	void			Interpolate( const DaedalusVtx4 & lhs, const DaedalusVtx4 & rhs, float factor );
};

DAEDALUS_STATIC_ASSERT( sizeof(DaedalusVtx4) == 64 );


// The ordering of these elements is determined by the PSP hardware
struct DaedalusVtx
{
	DaedalusVtx()
	{
	}
	DaedalusVtx( const v3 & position, const u32 colour, const v2 & texture )
		:	Texture( texture )
		,	Colour( colour )
		,	Position( position )
	{

	}

    v2		Texture;
	c32		Colour;
    v3		Position;
};

DAEDALUS_STATIC_ASSERT( sizeof(DaedalusVtx) == 24 );

#define DAEDALUS_VERTEX_FLAGS (GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF )

#endif // __DAEDALUS_DAEDALUSVTX_H__
