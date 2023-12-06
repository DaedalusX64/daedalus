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

	s16 width;
	s16 stride;

	s8  size;
	s8  format;
	s16 height;

	s16 imageY;
	s16 imageX;

	s8 dummy[4];
};  

DAEDALUS_STATIC_ASSERT( sizeof(Sprite2DStruct) == 24 );
 
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
static void DLParser_Sprite2DScaleFlip( MicroCodeCommand command, Sprite2DInfo *info )
{
	info->scaleX = ((command.inst.cmd1>>16)&0xFFFF) / 1024.0f;
	info->scaleY = (command.inst.cmd1&0xFFFF) / 1024.0f;

	info->flipX = ((command.inst.cmd0>>8)&0xFF);
	info->flipY = (command.inst.cmd0&0xFF);
}

//*****************************************************************************
//
//*****************************************************************************
// static void Load_Sprite2D( const Sprite2DStruct *sprite, const Sprite2DInfo info )
// {
// 	TextureInfo ti;
// 	ti.SetLoadAddress(RDPSegAddr(sprite->address));
// 	ti.SetFormat(sprite->format);
// 	ti.SetSize(sprite->size);
// 	ti.SetSwapped(false);
// 	ti.SetPalette(0);
// 	ti.SetTlutAddress(RDPSegAddr(sprite->tlut));
// 	ti.SetTLutFormat(kTT_RGBA16);

// 	u32 width = sprite->stride;
// 	u32 height = sprite->height + sprite->imageY;
// 	u32 pitch = (sprite->stride << sprite->size) >> 1;

// 	if(g_ROM.GameHacks == WCW_NITRO)
// 	{
// 		u32 scaleY = (u32)info.scaleY;
// 		width *= scaleY;
// 		height /= scaleY;
// 		pitch *= scaleY;
// 	}

// 	ti.SetWidth(width);
// 	ti.SetHeight(height);
// 	ti.SetPitch(pitch);

// 	DL_PF( "    Sprite2D Texture:[Width:%d, Height:%d] -> Address[0x%08x] Format[%s] TLUT[0x%x] Pitch[%d]",
// 		ti.GetWidth(), ti.GetHeight(), ti.GetLoadAddress(), ti.GetFormatName(), ti.GetTlutAddress(), ti.GetPitch());

// 	gRenderer->LoadTextureDirectly(ti);
// }

//*****************************************************************************
//
//*****************************************************************************
// static void Draw_Sprite2D( MicroCodeCommand command, const Sprite2DStruct *sprite, const Sprite2DInfo info )
// {
// 	f32 frameX = ((s16)((command.inst.cmd1>>16)&0xFFFF)) / 4.0f;
// 	f32 frameY = ((s16)(command.inst.cmd1&0xFFFF)) / 4.0f;
// 	f32 frameW = (u16)(sprite->imageW / info.scaleX);
// 	f32 frameH = (u16)(sprite->height / info.scaleY);

// 	f32 ulx, lrx, uly, lry;
// 	if (info.flipX)
// 	{
// 		ulx = frameX + frameW;
// 		lrx = frameX;
// 	} 
// 	else 
// 	{
// 		ulx = frameX;
// 		lrx = frameX + frameW;
// 	}
// 	if (info.flipY) 
// 	{
// 		uly = frameY + frameH;
// 		lry = frameY;
// 	} 
// 	else 
// 	{
// 		uly = frameY;
// 		lry = frameY + frameH;
// 	}

// 	f32 uls = sprite->imageX; //left
// 	f32 ult = sprite->imageY; //top
// 	f32 lrs = sprite->imageX + sprite->imageW; //right
// 	f32 lrt = sprite->imageY + sprite->height; //bottom
// 	if (g_ROM.GameHacks == WCW_NITRO)
// 	{
// 		ult /= info.scaleY;
// 		lrt /= info.scaleY;
// 	}
// 	DL_PF("    Sprite2D Screen(%.1f, %.1f) -> (%.1f, %.1f)", ulx, uly, lrx, lry);
// 	DL_PF("    Sprite2D Tex:(%.1f, %.1f) -> (%.1f, %.1f)", uls, lrs, ult, lrt);
	
// 	#ifdef DAEDALUS_CTR
// 	gRenderer->Draw2DTexture( ulx, uly, lrx, lry, uls, ult, lrs, lrt, texture );
// 	#else
// 	gRenderer->Draw2DTexture( ulx, uly, lrx, lry, uls, ult, lrs, lrt );
// 	#endif
// }

//*****************************************************************************
//
//*****************************************************************************
// static void DLParser_Sprite2DDraw( MicroCodeCommand command, const Sprite2DInfo info, const Sprite2DStruct *sprite )
// {
// 	Load_Sprite2D( sprite, info );
// 	Draw_Sprite2D( command, sprite, info );
// }

void DLParser_Sprite2DDraw( MicroCodeCommand command, const Sprite2DInfo &info, Sprite2DStruct *sprite )
{

	// Wipeout.
	if(sprite->width == 0)
		return;

	// ToDO : Cache ti state as Sprite2D is mostly used for static BGs
	TextureInfo ti;

	u32 address = RDPSegAddr(sprite->address);

	SImageDescriptor	desc = { static_cast<u32>(sprite->format), static_cast<u32>(sprite->size), static_cast<u32>(sprite->stride), static_cast<u32>(address) };

	ti.SetFormat(sprite->format);
	ti.SetSize(sprite->size);

	ti.SetLoadAddress(desc.GetAddress(sprite->imageX, sprite->imageY));

	ti.SetWidth(sprite->width);
	ti.SetHeight(sprite->height);
	ti.SetPitch((sprite->stride << sprite->size) >> 1);

	ti.SetSwapped(false);

	ti.SetPalette(0);
	ti.SetTlutAddress(RDPSegAddr(sprite->tlut));
	ti.SetTLutFormat(kTT_RGBA16);

	auto texture = gRenderer->LoadTextureDirectly(ti);

	s16 px = (s16)((command.inst.cmd1>>16)&0xFFFF)/4;
	s16 py = (s16)(command.inst.cmd1 &0xFFFF)/4;
	u16 pw = (u16)(sprite->width / info.scaleX);
	u16 ph = (u16)(sprite->height / info.scaleY);

	s32 frameX = px;
	s32 frameY = py;
	s32 frameW = px + pw;
	s32 frameH = py + ph;

	// SSV uses this
	if( info.flipX )
		std::swap< s32 >( frameX, frameW );

	if( info.flipY )
		std::swap< s32 >( frameY, frameH );

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
	u32 pc = gDlistStack.address[gDlistStackPointer]; // This points to the next instruction
	MicroCodeCommand * pCmdBase = (MicroCodeCommand*)(g_pu8RamBase + pc);
	
	// Try to execute as many sprite2d instructions as possible, I seen chains of over 700! in FB
	do
	{
		// Permissive
		Sprite2DStruct *sprite = ( Sprite2DStruct *)(g_pu8RamBase + RDPSegAddr(command.inst.cmd1));

	//	const Sprite2DStruct *sprite = (const Sprite2DStruct *)(g_pu8RamBase + RDPSegAddr(command.inst.cmd1));

		// Fetch the next 2 instructions at once (Sprite2D Flip and Sprite2D Draw)
		MicroCodeCommand command2 = *pCmdBase++;
		MicroCodeCommand command3 = *pCmdBase++;

		// Nothing todo if either next instruction is invalid
		// Glover does this by calling RDP Sync before drawing the sky
		if (command2.inst.cmd == G_GBI1_SPRITE2D_SCALEFLIP)
		{
			if (command3.inst.cmd != G_GBI1_SPRITE2D_DRAW)
			{
				pc += 16;
				break;
			}
		} 
		else
		{
			pc += 8;
			break;
		}

		// Avoid division by zero
		if (sprite->stride > 0)
		{
			Sprite2DInfo info;
			DLParser_Sprite2DScaleFlip( command2, &info );
			DLParser_Sprite2DDraw( command3, info, sprite );
		}
		
		command = *pCmdBase++;
		pc += 24;
	}while(command.inst.cmd == G_GBI1_SPRITE2D_BASE);

	gDlistStack.address[gDlistStackPointer] = pc-8;
}

#endif // HLEGRAPHICS_UCODES_UCODE_SPRITE2D_H_

