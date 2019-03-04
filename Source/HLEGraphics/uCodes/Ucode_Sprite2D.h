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

#ifndef HLEGRAPHICS_UCODES_UCODE_SPRITE2D_H_
#define HLEGRAPHICS_UCODES_UCODE_SPRITE2D_H_

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
void DLParser_Sprite2DDraw( MicroCodeCommand command, const Sprite2DInfo &info, Sprite2DStruct *sprite )
{

	// Wipeout.
	if(sprite->width == 0)
		return;

	// ToDO : Cache ti state as Sprite2D is mostly used for static BGs
	TextureInfo ti;

	u32 address = RDPSegAddr(sprite->address);

	SImageDescriptor	desc = { sprite->format, sprite->size, sprite->stride, address };

	ti.SetFormat           (sprite->format);
	ti.SetSize             (sprite->size);

	ti.SetLoadAddress      (desc.GetAddress(sprite->imageX, sprite->imageY));

	ti.SetWidth            (sprite->width);
	ti.SetHeight           (sprite->height);
	ti.SetPitch            (sprite->stride << sprite->size >> 1);

	ti.SetSwapped          (0);

	ti.SetPalette		   (0);
	ti.SetTlutAddress      (RDPSegAddr(sprite->tlut));

	ti.SetTLutFormat       (kTT_RGBA16);

	CRefPtr<CNativeTexture> texture = gRenderer->LoadTextureDirectly(ti);

	s16 px = (s16)((command.inst.cmd1>>16)&0xFFFF)/4;
	s16 py = (s16)(command.inst.cmd1 &0xFFFF)/4;
	u16 pw = (u16)(sprite->width / info.scaleX);
	u16 ph = (u16)(sprite->height / info.scaleY);

	s32 frameX              = px;
	s32 frameY              = py;
	s32 frameW              = px + pw;
	s32 frameH              = py + ph;

	// SSV uses this
	if( info.flipX )
		Swap< s32 >( frameX, frameW );

	if( info.flipY )
		Swap< s32 >( frameY, frameH );

	gRenderer->Draw2DTexture( (f32)frameX, (f32)frameY, (f32)frameW, (f32)frameH,
							  0.0f, 0.0f, (f32)sprite->width, (f32)sprite->height,
							  texture );
}

//*****************************************************************************
//
//*****************************************************************************
// Used by Flying Dragon
void DLParser_GBI1_Sprite2DBase( MicroCodeCommand command )
{
	u32 address;
	Sprite2DInfo info;
	Sprite2DStruct *sprite;

	u32 pc = gDlistStack.address[gDlistStackPointer];
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

	// Try to execute as many sprite2d ucodes as possible, I seen chains over 200! in FB
	// NB Glover calls RDP Sync before draw for the sky.. so checks were added
	do
	{
		address = RDPSegAddr(command.inst.cmd1) & (MAX_RAM_ADDRESS-1);
		sprite = (Sprite2DStruct *)(g_ps8RamBase + address);

		// Fetch Sprite2D Flip
		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		if(command.inst.cmd != G_GBI1_SPRITE2D_SCALEFLIP)
		{
			pc += 8;
			break;
		}
		DLParser_Sprite2DScaleFlip( command, &info );

		// Fetch Sprite2D Draw
		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		if(command.inst.cmd != G_GBI1_SPRITE2D_DRAW)
		{
			pc += 16;	//We have executed atleast 2 instructions at this point
			break;
		}
		DLParser_Sprite2DDraw( command, info, sprite );

		// Fetch Sprite2D Base
		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		pc += 24;
	}while(command.inst.cmd == G_GBI1_SPRITE2D_BASE);

	gDlistStack.address[gDlistStackPointer] = pc-8;
}

#endif // HLEGRAPHICS_UCODES_UCODE_SPRITE2D_H_
