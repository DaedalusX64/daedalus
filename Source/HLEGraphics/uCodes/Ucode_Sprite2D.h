/*
Copyright (C) 2009 StrmnNrmn

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

#ifndef UCODE_SPRITE2D_H__
#define UCODE_SPRITE2D_H__

//*****************************************************************************
// Needed by Sprite2D
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


Sprite2DInfo g_Sprite2DInfo;
//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Sprite2DBase( MicroCodeCommand command )
{
    u32 address = RDPSegAddr(command.dlist.addr) & (MAX_RAM_ADDRESS-1);
    g_Sprite2DInfo.spritePtr = (SpriteStruct *)(g_ps8RamBase + address);

	//
	// Fetch the next instruction (Scaleflip)
	//
	DLParser_FetchNextCommand(&command);

	if( command.inst.cmd == G_GBI1_SPRITE2D_SCALEFLIP )
	{
		DLParser_GBI1_Sprite2DScaleFlip( command );
	}
	//
	// Fetch the next instruction (Draw)
	//
	DLParser_FetchNextCommand(&command);

	if( command.inst.cmd == G_GBI1_SPRITE2D_DRAW )
	{
		DLParser_GBI1_Sprite2DDraw( command );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Sprite2DScaleFlip( MicroCodeCommand command )
{
	g_Sprite2DInfo.scaleX = (((command.inst.cmd1)>>16)   &0xFFFF)/1024.0f;
	g_Sprite2DInfo.scaleY = ( (command.inst.cmd1)        &0xFFFF)/1024.0f;

	// The code below causes wrong background height in super robot spirit, so it is disabled.
	// Need to find, for which game this hack was made.
	/*
	if( ((command.inst.cmd1)&0xFFFF) < 0x100 )
	{
		g_Sprite2DInfo.scaleY = g_Sprite2DInfo.scaleX;
	}
	*/
	g_Sprite2DInfo.flipX = (u8)(((command.inst.cmd0)>>8)     &0xFF);
	g_Sprite2DInfo.flipY = (u8)( (command.inst.cmd0)         &0xFF);
}

//*****************************************************************************
//
//*****************************************************************************
// ToDo : Optimize : We compare lotsa of ints with floats..
//
void DLParser_GBI1_Sprite2DDraw( MicroCodeCommand command )
{
    //DL_PF("Not fully implemented");
    g_Sprite2DInfo.px = (u16)(((command.inst.cmd1)>>16)&0xFFFF)/4;
    g_Sprite2DInfo.py = (u16)( (command.inst.cmd1)     &0xFFFF)/4;

	DAEDALUS_ASSERT( g_Sprite2DInfo.spritePtr, "g_Sprite2DInfo Null" );

	// This a hack for Wipeout.
	// TODO : Find a workaround to remove this hack..
	if(g_Sprite2DInfo.spritePtr->SubImageWidth == 0)
	{
		DAEDALUS_ERROR("Hack: Width is 0. Skipping Sprite2DDraw");
		g_Sprite2DInfo.spritePtr = 0;
		return;
	}

	TextureInfo ti;

	ti.SetFormat            (g_Sprite2DInfo.spritePtr->SourceImageType);
	ti.SetSize              (g_Sprite2DInfo.spritePtr->SourceImageBitSize);

	ti.SetLoadAddress       (RDPSegAddr(g_Sprite2DInfo.spritePtr->SourceImagePointer));

	ti.SetWidth             (g_Sprite2DInfo.spritePtr->SubImageWidth);
	ti.SetHeight            (g_Sprite2DInfo.spritePtr->SubImageHeight);
	ti.SetPitch             (g_Sprite2DInfo.spritePtr->Stride << ti.GetSize() >> 1);

	ti.SetSwapped           (0);

	ti.SetTLutIndex        ((u32)(g_pu8RamBase+RDPSegAddr(g_Sprite2DInfo.spritePtr->TlutPointer)));
	ti.SetTLutFormat       (2 << 14);  //RGBA16 

	CRefPtr<CTexture>       texture( CTextureCache::Get()->GetTexture( &ti ) );
	texture->GetTexture()->InstallTexture();

	int imageX, imageY, imageW, imageH;

	imageX              = g_Sprite2DInfo.spritePtr->SourceImageOffsetS;
	imageY              = g_Sprite2DInfo.spritePtr->SourceImageOffsetT;
	imageW              = ti.GetWidth();
	imageH              = ti.GetHeight();

	int frameX, frameY, frameW, frameH;

	frameX              = g_Sprite2DInfo.px;
	frameY              = g_Sprite2DInfo.py;
	frameW              = g_Sprite2DInfo.spritePtr->SubImageWidth / g_Sprite2DInfo.scaleX;
	frameH              = g_Sprite2DInfo.spritePtr->SubImageHeight / g_Sprite2DInfo.scaleY;

	PSPRenderer::Get()->Draw2DTexture( (float)frameX, (float)frameY, (float)frameW, (float)frameH, (float)imageX, (float)imageY, (float)imageW, (float)imageH );
	//g_Sprite2DInfo.spritePtr = 0; // Why ?
}

#endif // UCODE_SPRITE2D_H__
