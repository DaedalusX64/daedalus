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

#ifndef HLEGRAPHICS_DAEDALUSVTX_H_
#define HLEGRAPHICS_DAEDALUSVTX_H_

#include "Graphics/ColourValue.h"
#include "Math/Vector2.h"



// The ordering of these elements is required for the VectorTnL code.
struct alignas(DATA_ALIGN) DaedalusVtx4
{
    glm::vec4	TransformedPos;
    glm::vec4	ProjectedPos;
    glm::vec4	Colour;
    v2	Texture;

	u32	ClipFlags;
	u32 Pad;

	void Interpolate( const DaedalusVtx4 & lhs, const DaedalusVtx4 & rhs, float factor );
};

DAEDALUS_STATIC_ASSERT( sizeof(DaedalusVtx4) == 64 );

struct TexCoord
{
	s16		s;
	s16		t;

	TexCoord()
	{
	}
	TexCoord(s16 s_, s16 t_) : s(s_), t(t_)
	{
	}
	TexCoord(float s_, float t_) : s( (s16)(s_ * 32.f) ), t( (s16)(t_ * 32.f) )
	{
	}
};

// The ordering of these elements is determined by the PSP hardware
struct DaedalusVtx
{
	DaedalusVtx()
	{
	}
	DaedalusVtx( const glm::vec3 & position, const u32 colour, const v2 & texture )
		:	Texture( texture )
		,	Colour( colour )
		,	Position( position )
	{

	}

    v2		Texture ;
	c32		Colour ;
    glm::vec3		Position ;
};

DAEDALUS_STATIC_ASSERT( sizeof(DaedalusVtx) == 24 );

#endif // HLEGRAPHICS_DAEDALUSVTX_H_
