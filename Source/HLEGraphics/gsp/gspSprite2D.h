/*
Copyright (C) 2009 Grazz
Copyright (C) 2003-2009 Rice1964

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


#ifndef GSP_SPRITE2D_H
#define GSP_SPRITE2D_H

//*****************************************************************************
// Sprite2D
//*****************************************************************************

UcodeFunc( DLParser_GBI1_Sprite2DBase );
UcodeFunc( DLParser_GBI1_Sprite2DScaleFlip );
UcodeFunc( DLParser_GBI1_Sprite2DDraw );

//*****************************************************************************
// 
//*****************************************************************************

struct SpriteStruct
{
  u32 SourceImagePointer;
  u32 TlutPointer;

  u16 SubImageWidth;
  u16 Stride;

  u8  SourceImageBitSize;
  u8  SourceImageType;
  u16 SubImageHeight;

  u16 SourceImageOffsetT;
  u16 SourceImageOffsetS;

  char  dummy[4];
};

//*****************************************************************************
// 
//*****************************************************************************

struct Sprite2DInfo
{
    u16 px;
    u16 py;
    f32 scaleX;
    f32 scaleY;
    u8  flipX;
    u8  flipY;
    SpriteStruct *spritePtr;
};

#endif /* GSP_SPRITE2D_H */
