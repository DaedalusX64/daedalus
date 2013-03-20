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

//*****************************************************************************
//
//*****************************************************************************
inline void DLParser_Sprite2DScaleFlip( MicroCodeCommand command, Sprite2DInfo *info )
{
	info->scaleX = (((command.inst.cmd1)>>16)   &0xFFFF)/1024.0f;
	info->scaleY = ( (command.inst.cmd1)        &0xFFFF)/1024.0f;

	info->flipX = (u8)(((command.inst.cmd0)>>8)     &0xFF);
	info->flipY = (u8)( (command.inst.cmd0)         &0xFF);
}

//*****************************************************************************
//
//*****************************************************************************
inline void DLParser_Sprite2DDraw( MicroCodeCommand command, const Sprite2DInfo &info, Sprite2DStruct *sprite )
{
	u16 px = (u16)(((command.inst.cmd1)>>16)&0xFFFF)/4;
	u16 py = (u16)( (command.inst.cmd1)     &0xFFFF)/4;

	// Wipeout.
	if(sprite->width == 0)
		return;

	// ToDO : Cache ti state as Sprite2D is mostly used for static BGs
	TextureInfo ti;

	ti.SetFormat           (sprite->format);
	ti.SetSize             (sprite->size);

	ti.SetLoadAddress      (RDPSegAddr(sprite->address));

	ti.SetWidth            (sprite->width);
	ti.SetHeight           (sprite->height);
	ti.SetPitch            (sprite->stride << sprite->size >> 1);

	ti.SetSwapped          (false);

	ti.SetTLutIndex        (0);
	ti.SetTlutAddress      ((u32)(g_pu8RamBase + RDPSegAddr(sprite->tlut)));

	ti.SetTLutFormat       (kTT_RGBA16);

	CRefPtr<CNativeTexture> texture = CTextureCache::Get()->GetOrCreateTexture( ti );
	DAEDALUS_ASSERT( texture, "Sprite2D texture is NULL" );

	texture->InstallTexture();

	s32 frameX              = px;
	s32 frameY              = py;
	s32 frameW              = (sprite->width / info.scaleX) + px;
	s32 frameH              = (sprite->height / info.scaleY) + py;

	// SSV uses this
	if( info.flipX )
		Swap< s32 >( frameX, frameW );

	if( info.flipY )
		Swap< s32 >( frameY, frameH );

	gRenderer->Draw2DTexture( (float)frameX, (float)frameY, (float)frameW, (float)frameH, 0.0f, 0.0f, (float)sprite->width, (float)sprite->height );
}

//*****************************************************************************
//
//*****************************************************************************
// Used by Flying Dragon
void DLParser_GBI1_Sprite2DBase( MicroCodeCommand command )
{
	Sprite2DInfo info;
	Sprite2DStruct *sprite;

	u32 pc = gDlistStack.address[gDlistStackPointer];
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);
	
	// This assumes sprite2D is always followed by flip and draw 
	// according to the manual base and flip has to be called before drawing, so this assumption should be fine
	// Try to execute as many sprite2d ucodes as possible, I seen chains over 200! in FB
	do
	{
		u32 address = RDPSegAddr(command.inst.cmd1) & (MAX_RAM_ADDRESS-1);
		sprite = (Sprite2DStruct *)(g_ps8RamBase + address);

		// Fetch Sprite2D Flip
		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		DAEDALUS_ASSERT(command.inst.cmd == G_GBI1_SPRITE2D_SCALEFLIP, "Opps, was expecting Sprite2D Flip");
		DLParser_Sprite2DScaleFlip( command, &info );

		// Fetch Sprite2D Draw
		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		DAEDALUS_ASSERT(command.inst.cmd == G_GBI1_SPRITE2D_DRAW, "Opps, was expecting Sprite2D Draw");
		DLParser_Sprite2DDraw( command, info, sprite );

		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		pc += 24;
	} while ( command.inst.cmd == G_GBI1_SPRITE2D_BASE );
	
	gDlistStack.address[gDlistStackPointer] = pc-8;
}

#endif // UCODE_SPRITE2D_H__
