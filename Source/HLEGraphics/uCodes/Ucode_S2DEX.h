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

#ifndef UCODE_S2DEX_H__
#define UCODE_S2DEX_H__

//*****************************************************************************
// Needed by S2DEX
//*****************************************************************************

#define	G_GBI2_SELECT_DL		0x04
#define	S2DEX_OBJLT_TXTRBLOCK	0x00001033
#define	S2DEX_OBJLT_TXTRTILE	0x00fc1034
#define	S2DEX_OBJLT_TLUT		0x00000030
#define	S2DEX_BGLT_LOADBLOCK	0x0033
#define	S2DEX_BGLT_LOADTILE		0xfff4


//*****************************************************************************
// 
//*****************************************************************************
struct uObjBg
{
    u16 imageW;
    u16 imageX;
    u16 frameW;
    s16 frameX;
    u16 imageH;
    u16 imageY;
    u16 frameH;
    s16 frameY;

    u32 imagePtr;
    u8  imageSiz;
    u8  imageFmt;
    u16 imageLoad;
    u16 imageFlip;
    u16 imagePal;

    u16 tmemH;
    u16 tmemW;
    u16 tmemLoadTH;
    u16 tmemLoadSH;
    u16 tmemSize;
    u16 tmemSizeW;
};

//*****************************************************************************
//
//*****************************************************************************
struct	uObjScaleBg
{	
	u16	imageW;		
	u16	imageX;		

	u16	frameW;		
	s16	frameX;		

	u16	imageH;		
	u16	imageY; 	

	u16	frameH;		
	s16	frameY;		

	u32	imagePtr;	

	u8	imageSiz;	
	u8	imageFmt;	
	u16	imageLoad;	

	u16	imageFlip;	
	u16	imagePal; 	

	u16	scaleH;		
	u16	scaleW;		

	s32	imageYorig;	
	u8	padding[4];
};


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
#ifndef DAEDALUS_TMEM
		//Calc offset to palette
		u32 *p_source = (u32*)&g_pu8RamBase[ RDPSegAddr(objBg->imagePtr) + imageW * imageH];
		//Load to TMEM area
		gTextureMemory[ ( objBg->imagePal << 2 ) & 0xFF ] = p_source;
		//Copy the palette to TMEM (Always RGBA16 eg. 512 bytes)
#else
        //Calc offset to palette
        u8* p_source = (u8*)&g_pu8RamBase[ RDPSegAddr(objBg->imagePtr) + imageW * imageH];
        //Load to TMEM area
        u8* p_dest   = (u8*)&gTextureMemory[ ( 0x800 + ( objBg->imagePal << 5 ) ) & 0xFFF ];
        //Copy the palette to TMEM (Always RGBA16 eg. 512 bytes)
        memcpy_vfpu_BE(p_dest, p_source, 512);
#endif
	}

	CRefPtr<CTexture>       texture( CTextureCache::Get()->GetTexture( &ti ) );
	texture->GetTexture()->InstallTexture();

	PSPRenderer::Get()->Draw2DTexture( (float)imageX, (float)imageY, (float)frameX ,(float)frameY, (float)imageW, (float)imageH, (float)frameW, (float)frameH);
}

//*****************************************************************************
// YoshiStory - 0x04
//*****************************************************************************
void DLParser_S2DEX_SelectDl( MicroCodeCommand command )
{	
	DL_PF( "	S2DEX_SelectDl (Ignored)" );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjSprite( MicroCodeCommand command )
{	
	// YoshiStory uses this - 0x06
	DL_UNIMPLEMENTED_ERROR("S2DEX_ObjSprite");
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjRectangle( MicroCodeCommand command )
{	
	// YoshiStory uses this - 0x01
	DL_UNIMPLEMENTED_ERROR("S2DEX_ObjRectangle");
}

//*****************************************************************************
// Majora's Mask ,Doubutsu no Mori, and YoshiStory Menus uses this - 0x0b
//*****************************************************************************
void DLParser_S2DEX_ObjRendermode( MicroCodeCommand command )
{	
	DL_PF( "	S2DEX_ObjRendermode (Ignored)" );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjLoadTxtr( MicroCodeCommand command )
{	
	// Command and Conquer and YoshiStory uses this - 0x05
	DL_UNIMPLEMENTED_ERROR("S2DEX_ObjLoadTxtr");
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjLdtxSprite( MicroCodeCommand command )
{	
	// YoshiStory uses this - 0xc2
	DL_UNIMPLEMENTED_ERROR("S2DEX_ObjLdtxSprite");
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjLdtxRect( MicroCodeCommand command )
{	
	// YoshiStory uses this - 0x07
	DL_UNIMPLEMENTED_ERROR("S2DEX_ObjLdtxRect");
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjLdtxRectR( MicroCodeCommand command )
{	
	// YoshiStory uses this - 0x08
	DL_UNIMPLEMENTED_ERROR("S2DEX_ObjLdtxRectR");
}

//*****************************************************************************
//
//*****************************************************************************
void Yoshi_MemRect( MicroCodeCommand command )
{
	MicroCodeCommand command2;
	MicroCodeCommand command3;
	//
	// Fetch the next two instructions
	//
	DLParser_FetchNextCommand( &command2 );
	DLParser_FetchNextCommand( &command3 );

	RDP_TexRect tex_rect;
	tex_rect.cmd0 = command.inst.cmd0;
	tex_rect.cmd1 = command.inst.cmd1;
	tex_rect.cmd2 = command2.inst.cmd1;
	tex_rect.cmd3 = command3.inst.cmd1;

	u32	x0 = tex_rect.x0 >> 2;
	u32	y0 = tex_rect.y0 >> 2;
	u32	x1 = tex_rect.x1 >> 2;
	u32	y1 = tex_rect.y1 >> 2;

	if (y1 > scissors.bottom)
		y1 = scissors.bottom;

	DL_PF ("MemRect (%d, %d, %d, %d), Width: %d\n", x0, y0, x1, y1, g_CI.Width);

	const RDP_Tile & rdp_tile( gRDPStateManager.GetTile( tex_rect.tile_idx ) );

	u32 y, width = x1 - x0;
	u32 tex_width = rdp_tile.line << 3;
	u8 * texaddr = g_pu8RamBase + gRDPddress[rdp_tile.tmem] + tex_width*(tex_rect.s/32) + (tex_rect.t/32);
	u8 * fbaddr = g_pu8RamBase + g_CI.Address + x0;

	for (y = y0; y < y1; y++)
	{
		u8 *src = texaddr + (y - y0) * tex_width;
		u8 *dst = fbaddr + y * g_CI.Width;
		memcpy (dst, src, width);
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

	if (g_ROM.GameHacks == YOSHI)
	{
		Yoshi_MemRect( command );
		return;
	}

	DLParser_TexRect(command);
/*
	u32 pc = gDlistStack[gDlistStackPointer].pc;             // This points to the next instruction
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
			DAEDALUS_ERROR("RDP: S2DEX_RDPHALF_0 (0x%08x 0x%08x)\n", command.inst.cmd0, command.inst.cmd1);
		}
	}
	else
	{
		DAEDALUS_ERROR("RDP: S2DEX_RDPHALF_0 (0x%08x 0x%08x)\n", command.inst.cmd0, command.inst.cmd1);
	}
*/
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjMoveMem( MicroCodeCommand command )
{	
	// Ogre Battle 64 and YoshiStory uses this - 0xdc
	DL_UNIMPLEMENTED_ERROR("S2DEX_ObjMoveMem");
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
#ifndef DAEDALUS_TMEM
		//Calc offset to palette
		u32 *p_source = (u32*)&g_pu8RamBase[ RDPSegAddr(objBg->imagePtr) + imageW * imageH];
		//Load to TMEM area
		gTextureMemory[ ( objBg->imagePal << 2 ) & 0xFF ] = p_source;
		//Copy the palette to TMEM (Always RGBA16 eg. 512 bytes)
#else
        //Calc offset to palette
        u8* p_source = (u8*)&g_pu8RamBase[ RDPSegAddr(objBg->imagePtr) + imageW * imageH];
        //Load to TMEM area
        u8* p_dest   = (u8*)&gTextureMemory[ ( 0x800 + ( objBg->imagePal << 5 ) ) & 0xFFF ];
        //Copy the palette to TMEM (Always RGBA16 eg. 512 bytes)
        memcpy_vfpu_BE(p_dest, p_source, 512);
#endif
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
	DL_UNIMPLEMENTED_ERROR("S2DEX_ObjRectangleR");
}

//*****************************************************************************
//
//*****************************************************************************
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

//*****************************************************************************
//
//*****************************************************************************
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

#endif // UCODE_S2DEX_H__
