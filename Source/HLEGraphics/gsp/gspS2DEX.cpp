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

#include "gspCommon.h"
#include "../SysPSP/Utility/FastMemcpy.h"

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_BgCopy( MicroCodeCommand command )
{
	DL_PF("DLParser_S2DEX_BgCopy");

	uObjBg *objBg = (uObjBg*)(g_pu8RamBase + RDPSegAddr(command.inst.cmd1));

	u16 imageX = objBg->imageX >> 5;
	u16 imageY = objBg->imageY >> 5;

	u16 imageW = objBg->imageW >> 2;
	u16 imageH = objBg->imageH >> 2;

	s16 frameX = objBg->frameX >> 2;
	s16 frameY = objBg->frameY >> 2;
	u16 frameW = objBg->frameW >> 2;
	u16 frameH = objBg->frameH >> 2;

	TextureInfo ti;

	ti.SetFormat           (objBg->imageFmt);
	ti.SetSize             (objBg->imageSiz);

	ti.SetLoadAddress      (RDPSegAddr(objBg->imagePtr));
	ti.SetWidth            (imageW);
	ti.SetHeight           (imageH);
	ti.SetPitch			   (((imageW << ti.GetSize() >> 1)>>3)<<3); //force 8-bit alignment, this what sets our correct viewport.

	ti.SetSwapped          (0);

	ti.SetTLutIndex        (objBg->imagePal);
	ti.SetTLutFormat       (2 << 14);  //RGBA16 

	if(g_ROM.GameHacks == KIRBY64)
	{
		//Need to load PAL to TMEM //Corn
		//Calc offset to palette
		u8* p_source = (u8*)&g_pu8RamBase[ RDPSegAddr(objBg->imagePtr) + imageW * imageH];
		//Load to TMEM area
		u8* p_dest   = (u8*)&gTextureMemory[ ( 0x800 + ( objBg->imagePal << 5 ) ) & 0xFFF ];
		//Copy the palette to TMEM (Always RGBA16 eg. 512 bytes)
		memcpy_vfpu_BE(p_dest, p_source, 512);
	}

	CRefPtr<CTexture>       texture( CTextureCache::Get()->GetTexture( &ti ) );
	texture->GetTexture()->InstallTexture();

	PSPRenderer::Get()->Draw2DTexture( (float)imageX, (float)imageY, (float)frameX ,(float)frameY, (float)imageW, (float)imageH, (float)frameW, (float)frameH);
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_SelectDl( MicroCodeCommand command )
{	
	// YoshiStory - 0x04
	static bool warned = false;

	DL_PF( "~*Not Implemented" );
	if (!warned)
	{
		RDP_NOIMPL("RDP: S2DEX_SelectDl", command.inst.cmd0, command.inst.cmd1);
		warned = true;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjSprite( MicroCodeCommand command )
{	
	// YoshiStory uses this - 0x06
	static bool warned = false;

	DL_PF( "~*Not Implemented" );
	if (!warned)
	{
		RDP_NOIMPL("RDP: S2DEX_ObjSprite", command.inst.cmd0, command.inst.cmd1);
		warned = true;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjRectangle( MicroCodeCommand command )
{	
	// YoshiStory uses this - 0x01
	static bool warned = false;

	DL_PF( "~*Not Implemented" );
	if (!warned)
	{
		RDP_NOIMPL("RDP: S2DEX_ObjRectangle", command.inst.cmd0, command.inst.cmd1);
		warned = true;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjRendermode( MicroCodeCommand command )
{	
	// Majora's Mask ,Doubutsu no Mori, and YoshiStory Menus uses this - 0x0b
	static bool warned = false;

	DL_PF( "~*Not Implemented" );
	if (!warned)
	{
		RDP_NOIMPL("RDP: S2DEX_ObjRendermode", command.inst.cmd0, command.inst.cmd1);
		warned = true;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjLoadTxtr( MicroCodeCommand command )
{	
	// Command and Conquer and YoshiStory uses this - 0x05
	static bool warned = false;

	DL_PF( "~*Not Implemented" );
	if (!warned)
	{
		RDP_NOIMPL("RDP: S2DEX_ObjLoadTxtr)", command.inst.cmd0, command.inst.cmd1);
		warned = true;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjLdtxSprite( MicroCodeCommand command )
{	
	// YoshiStory uses this - 0xc2
	static bool warned = false;

	DL_PF( "~*Not Implemented" );
	if (!warned)
	{
		RDP_NOIMPL("RDP: S2DEX_ObjLdtxSprite", command.inst.cmd0, command.inst.cmd1);
		warned = true;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjLdtxRect( MicroCodeCommand command )
{	
	// YoshiStory uses this - 0x07
	static bool warned = false;

	DL_PF( "~*Not Implemented" );
	if (!warned)
	{
		RDP_NOIMPL("RDP: S2DEX_ObjLdtxRect", command.inst.cmd0, command.inst.cmd1);
		warned = true;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjLdtxRectR( MicroCodeCommand command )
{	
	// YoshiStory uses this - 0x08
	static bool warned = false;

	DL_PF( "~*Not Implemented" );
	if (!warned)
	{
		RDP_NOIMPL("RDP: S2DEX_ObjLdtxRectR", command.inst.cmd0, command.inst.cmd1);
		warned = true;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_RDPHalf_0( MicroCodeCommand command )
{	
	//RDP: RSP_S2DEX_RDPHALF_0 (0xe449c0a8 0x003b40a4)
	//0x001d3c88: e449c0a8 003b40a4 RDP_TEXRECT 
	//0x001d3c90: b4000000 00000000 RSP_RDPHALF_1
	//0x001d3c98: b3000000 04000400 RSP_RDPHALF_2

	u32 pc = gDisplayListStack.back().addr;             // This points to the next instruction
	u32 NextUcode = *(u32 *)(g_pu8RamBase + pc);

	if( (NextUcode>>24) != G_GBI2_SELECT_DL )
	{
		// Pokemom Puzzle League
		if( (NextUcode>>24) == 0xB4 )
		{
			DLParser_TexRect(command);
		}
		else
		{
			RDP_NOIMPL("RDP: S2DEX_RDPHALF_0 (0x%08x 0x%08x)", command.inst.cmd0, command.inst.cmd1);
		}
	}
	else
	{
		RDP_NOIMPL("RDP: S2DEX_RDPHALF_0 (0x%08x 0x%08x)", command.inst.cmd0, command.inst.cmd1);
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjMoveMem( MicroCodeCommand command )
{	
	// Ogre Battle 64 and YoshiStory uses this - 0xdc
	static bool warned = false;

	DL_PF( "~*Not Implemented" );
	if (!warned)
	{
		RDP_NOIMPL("RDP: S2DEX_ObjMoveMem", command.inst.cmd0, command.inst.cmd1);
		warned = true;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_Bg1cyc( MicroCodeCommand command )
{
	if( g_ROM.GameHacks == ZELDA_MM )
		return;

	uObjScaleBg *objBg = (uObjScaleBg *)(g_pu8RamBase + RDPSegAddr(command.inst.cmd1));

	u16 imageX = objBg->imageX >> 5;
	u16 imageY = objBg->imageY >> 5;

	u16 imageW = objBg->imageW >> 2;
	u16 imageH = objBg->imageH >> 2;

	s16 frameX = objBg->frameX >> 2;
	s16 frameY = objBg->frameY >> 2;
	u16 frameW = objBg->frameW >> 2;
	u16 frameH = objBg->frameH >> 2;

	TextureInfo ti;

	ti.SetFormat           (objBg->imageFmt);
	ti.SetSize             (objBg->imageSiz);

	ti.SetLoadAddress      (RDPSegAddr(objBg->imagePtr));
	ti.SetWidth            (imageW);
	ti.SetHeight           (imageH);
	ti.SetPitch			   (((imageW << ti.GetSize() >> 1)>>3)<<3); //force 8-bit alignment, this what sets our correct viewport.

	ti.SetSwapped          (0);

	ti.SetTLutIndex        (objBg->imagePal);
	ti.SetTLutFormat       (2 << 14);  //RGBA16 >> (2 << G_MDSFT_TEXTLUT)

	if(g_ROM.GameHacks == KIRBY64)
	{
		//Need to load PAL to TMEM //Corn
		//Calc offset to palette
		u8* p_source = (u8*)&g_pu8RamBase[ RDPSegAddr(objBg->imagePtr) + imageW * imageH];
		//Load to TMEM area
		u8* p_dest   = (u8*)&gTextureMemory[ ( 0x800 + ( objBg->imagePal << 5 ) ) & 0xFFF ];
		//Copy the palette to TMEM (Always RGBA16 eg. 512 bytes)
		memcpy_vfpu_BE(p_dest, p_source, 512);
	}

	CRefPtr<CTexture>       texture( CTextureCache::Get()->GetTexture( &ti ) );
	texture->GetTexture()->InstallTexture();

	PSPRenderer::Get()->Draw2DTexture( (float)imageX, (float)imageY, (float)frameX ,(float)frameY, (float)imageW, (float)imageH, (float)frameW, (float)frameH);
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjRectangleR( MicroCodeCommand command )
{	
	// Ogre Battle 64 and YoshiStory uses this - 0xda
	static bool warned = false;

	DL_PF( "~*Not Implemented" );
	if (!warned)
	{
		RDP_NOIMPL("RDP: S2DEX_ObjRectangleR", command.inst.cmd0, command.inst.cmd1);
		warned = true;
	}
}

void DLParser_S2DEX_Bg1cyc_2( MicroCodeCommand command )
{
	
	if( ((command.inst.cmd0)&0x00FFFFFF) != 0 )
	{
		DAEDALUS_ERROR("Mtx bg1cyc");
		DLParser_GBI1_Mtx(command);
		return;
	}

	DLParser_S2DEX_Bg1cyc(command);
}

void DLParser_S2DEX_ObjRendermode_2( MicroCodeCommand command )
{

	if( ((command.inst.cmd0)&0xFFFFFF) != 0 || ((command.inst.cmd1)&0xFFFFFF00) != 0 )
	{
		// This is a TRI2 cmd
		DAEDALUS_ERROR("tri2 Y");
		DLParser_GBI1_Tri2(command);
		return;
	}

	DLParser_S2DEX_ObjRendermode(command);
}