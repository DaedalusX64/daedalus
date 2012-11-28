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
struct Sprite2DStruct
{
	u32 address;
	u32 tlut;

	u16 width;
	u16 stride;

	u8  size;
	u8  format;
	u16 height;

	u16 imageY;
	u16 imageX;

	char dummy[4];
};

struct Sprite2DInfo
{
    f32 scaleX;
    f32 scaleY;

    u8  flipX;
    u8  flipY;
};

Sprite2DInfo g_Sprite2DInfo;
//*****************************************************************************
//
//*****************************************************************************
inline void DLParser_Sprite2DScaleFlip( MicroCodeCommand command )
{
	g_Sprite2DInfo.scaleX = (((command.inst.cmd1)>>16)   &0xFFFF)/1024.0f;
	g_Sprite2DInfo.scaleY = ( (command.inst.cmd1)        &0xFFFF)/1024.0f;

	g_Sprite2DInfo.flipX = (u8)(((command.inst.cmd0)>>8)     &0xFF);
	g_Sprite2DInfo.flipY = (u8)( (command.inst.cmd0)         &0xFF);
}

//*****************************************************************************
//
//*****************************************************************************
inline void DLParser_Sprite2DDraw( MicroCodeCommand command, u32 address )
{
	Sprite2DStruct *sprite = (Sprite2DStruct *)(g_ps8RamBase + address);

    u16 px = (u16)(((command.inst.cmd1)>>16)&0xFFFF)/4;
    u16 py = (u16)( (command.inst.cmd1)     &0xFFFF)/4;

	DAEDALUS_ASSERT( sprite, "Sprite2DStruct is NULL" );

	// This a hack for Wipeout.
	// TODO : Find a workaround to remove this hack..
	if(sprite->width == 0)
	{
		DAEDALUS_ERROR("Hack: Width is 0. Skipping Sprite2DDraw");
		return;
	}

	// ToDO : Cache ti state as Sprite2D is mostly used for static BGs
	TextureInfo ti;

	ti.SetFormat            (sprite->format);
	ti.SetSize              (sprite->size);

	ti.SetLoadAddress       (RDPSegAddr(sprite->address));

	ti.SetWidth             (sprite->width);
	ti.SetHeight            (sprite->height);
	ti.SetPitch             (sprite->stride << sprite->size >> 1);

	ti.SetSwapped           (false);

	// Proper way, sets tlut index and pointer addr correctly which fixes palette issues in Wipeout/Flying dragon...
	ti.SetTLutIndex        (0);
	ti.SetTlutAddress      ((u32)(g_pu8RamBase + RDPSegAddr(sprite->tlut)));

	ti.SetTLutFormat       (G_TT_RGBA16);

	CRefPtr<CTexture>       texture( CTextureCache::Get()->GetTexture( &ti ) );
	DAEDALUS_ASSERT( texture, "Sprite2D texture is NULL" );

	texture->GetTexture()->InstallTexture();
	texture->UpdateIfNecessary();

	int imageX, imageY, imageW, imageH;

	imageX              = sprite->imageX;
	imageY              = sprite->imageY;
	imageW              = sprite->width;
	imageH              = sprite->height;

	int frameX, frameY, frameW, frameH;

	frameX              = px;
	frameY              = py;
	frameW              = (sprite->width / g_Sprite2DInfo.scaleX) + px;
	frameH              = (sprite->height / g_Sprite2DInfo.scaleY) + py;

	// Wipeout sets this, doesn't seem to do anything?
	/*if( g_Sprite2DInfo.flipX )
	{
		int temp = frameX;
		frameX = frameW;
		frameW = temp;
	}

	if( g_Sprite2DInfo.flipY )
	{
		int temp = frameY;
		frameY = frameH;
		frameH = temp;
	}*/

	PSPRenderer::Get()->Draw2DTexture( (float)frameX, (float)frameY, (float)frameW, (float)frameH, (float)imageX, (float)imageY, (float)imageW, (float)imageH );
}

//*****************************************************************************
//
//*****************************************************************************
// Used by Flying Dragon
void DLParser_GBI1_Sprite2DBase( MicroCodeCommand command )
{
    u32 address = RDPSegAddr(command.inst.cmd1) & (MAX_RAM_ADDRESS-1);

	MicroCodeCommand command2;
	MicroCodeCommand command3;

	//
	// Fetch the next two instructions (Scaleflip and Draw)
	//
	DLParser_FetchNextCommand( &command2 );
	DLParser_FetchNextCommand( &command3 );

	if( command2.inst.cmd == G_GBI1_SPRITE2D_SCALEFLIP )
	{
		DLParser_Sprite2DScaleFlip( command2 );
	}

	if( command3.inst.cmd == G_GBI1_SPRITE2D_DRAW )
	{
		DLParser_Sprite2DDraw( command3, address );
	}
}

#endif // UCODE_SPRITE2D_H__
