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

struct uObjMtx
{
	s32	  A, B, C, D;	

	short Y;			
	short X;			

	u16   BaseScaleY;	
	u16   BaseScaleX;	
};				


struct uObjSubMtx
{
	short Y;			
	short X;	

	u16   BaseScaleY;	
	u16   BaseScaleX;	
};

struct MAT2D
{
  float A, B, C, D;
  float X, Y;
  float BaseScaleX;
  float BaseScaleY;
} mat2D = {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };

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

struct	uObjTxtrBlock //PSP Format
{		
  u32	type;	
  u32	image;
  
  u16	tsize;	
  u16	tmem;	
  
  u16	sid;	
  u16	tline;	

  u32	flag;	
  u32	mask;	
};

struct uObjTxtrTile //PSP Format
{
  u32	type;	
  u32	image;

  u16	twidth;	
  u16	tmem;	

  u16	sid;	
  u16	theight;

  u32	flag;	
  u32	mask;	
};			// 24 bytes

struct uObjTxtrTLUT // PSP Format
{		
  u32	type;	
  u32	image;
  
  u16	pnum;	
  u16	phead;	
  
  u16	sid;	
  u16   zero;	
  
  u32	flag;	
  u32	mask;	
};		

union uObjTxtr
{
  uObjTxtrBlock      block;
  uObjTxtrTile       tile;
  uObjTxtrTLUT       tlut;
};

struct uObjSprite
{		
  u16  scaleW;		
  short  objX;			
  
  u16  paddingX;		
  u16  imageW;		
  
  u16  scaleH;		
  short  objY;			
  
  u16  paddingY;		
  u16  imageH;		
  
  u16  imageAdrs;	
  u16  imageStride;	

  u8   imageFlags;	
  u8   imagePal;		
  u8   imageSiz;		
  u8   imageFmt;		
};			


struct	uObjTxSprite
{
  uObjTxtr		txtr;
  uObjSprite	sprite;
};		/* 48 bytes */

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
	u16 frameW = (objBg->frameW >> 2) + frameX;
	u16 frameH = (objBg->frameH >> 2) + frameY;

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


	CRefPtr<CTexture>       texture( CTextureCache::Get()->GetTexture( &ti ) );
	texture->GetTexture()->InstallTexture();
	texture->UpdateIfNecessary();

	PSPRenderer::Get()->Draw2DTexture( (float)frameX, (float)frameY, (float)frameW, (float)frameH, (float)imageX, (float)imageY, (float)imageW, (float)imageH );
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
#ifndef DAEDALUS_TMEM
	uObjTxtr* ObjTxtr = (uObjTxtr*)(g_pu8RamBase + RDPSegAddr(command.inst.cmd1));
	if( ObjTxtr->block.type == S2DEX_OBJLT_TLUT )
	{
		uObjTxtrTLUT *ObjTlut = (uObjTxtrTLUT*)ObjTxtr;
		u32 ObjTlutAddr = (u32)(g_pu8RamBase + RDPSegAddr(ObjTlut->image));

		// Copy TLUT
		//u32 size = ObjTlut->pnum + 1;
		u32 offset = ObjTlut->phead - 0x100;

		//if( offset + size > 0x100) size = 0x100 - offset;

		gTextureMemory[ offset & 0xFF ] = (u32*)ObjTlutAddr;

		//printf("%p %d\n",(u32*)ObjTlutAddr ,ObjTlut->phead);
	}

#else

	DL_UNIMPLEMENTED_ERROR("S2DEX_ObjLoadTxtr");

#endif
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
// Text, smoke, and items in Yoshi
void DLParser_S2DEX_ObjLdtxRectR( MicroCodeCommand command )
{	
	uObjTxSprite* sprite = (uObjTxSprite*)(g_pu8RamBase+(RDPSegAddr(command.inst.cmd1)));

	f32 objX = sprite->sprite.objX/4.0f;
	f32 objY = sprite->sprite.objY/4.0f;
	f32 imageW = sprite->sprite.imageW / 32.0f;
	f32 imageH = sprite->sprite.imageH / 32.0f;
	f32 scaleW = sprite->sprite.scaleW/1024.0f;
	f32 scaleH = sprite->sprite.scaleH/1024.0f;

	f32 x0, y0, x1, y1;
	//if( rotate )	// With Rotation
	{
		x0 = mat2D.X + objX/mat2D.BaseScaleX;
		y0 = mat2D.Y + objY/mat2D.BaseScaleY;
		x1 = mat2D.X + (objX + imageW / scaleW) / mat2D.BaseScaleX - 1;
		y1 = mat2D.Y + (objY + imageH / scaleH) / mat2D.BaseScaleY - 1;
	}
	/*else
	{
		x0 = objX;
		y0 = objY;
		x1 = objX + width / scaleW - 1;
		y1 = objY + high / scaleH - 1;

		if( (sprite->sprite.imageFlags&1) ) // flipX
		{
			float temp = x0;
			x0 = x1;
			x1 = temp;
		}

		if( (sprite->sprite.imageFlags&0x10) ) // flipY
		{
			float temp = y0;
			y0 = y1;
			y1 = temp;
		}
	}*/

	TextureInfo ti;

	ti.SetFormat           (sprite->sprite.imageFmt);
	ti.SetSize             (sprite->sprite.imageSiz);

	ti.SetLoadAddress      (RDPSegAddr(sprite->txtr.block.image));

	if( sprite->txtr.block.type == S2DEX_OBJLT_TXTRBLOCK )
	{
		ti.SetWidth            (sprite->sprite.imageW/32);
		ti.SetHeight           (sprite->sprite.imageH/32);
		ti.SetPitch			   ( (2047/(sprite->txtr.block.tline-1)) << 3 );
	}
	/*else if( sprite->txtr.block.type == S2DEX_OBJLT_TXTRTILE )
	{
		ti.SetWidth            (((sprite->txtr.tile.twidth+1)>>2)<<(4-ti.GetSize()));
		ti.SetHeight           ((sprite->txtr.tile.theight+1)>>2);

		if( ti.GetSize() == G_IM_SIZ_4b )
		{
			ti.SetPitch			   (ti.GetWidth() >> 1);
		}
		else
			ti.SetPitch			   (ti.GetWidth() << (ti.GetSize()-1));
	}*/

	ti.SetSwapped          (0);
	ti.SetTLutIndex        (sprite->sprite.imagePal);
	ti.SetTLutFormat       (2 << 14);  //RGBA16 

	CRefPtr<CTexture>       texture( CTextureCache::Get()->GetTexture( &ti ) );
	texture->GetTexture()->InstallTexture();
	texture->UpdateIfNecessary();

	PSPRenderer::Get()->Draw2DTexture(x0, y0, x1, y1, 0, 0, imageW, imageH);
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

	const RDP_Tile & rdp_tile( gRDPStateManager.GetTile( tex_rect.tile_idx ) );

	u32	x0 = tex_rect.x0 >> 2;
	u32	y0 = tex_rect.y0 >> 2;
	u32	x1 = tex_rect.x1 >> 2;
	u32	y1 = tex_rect.y1 >> 2;

	// Get base address of texture
	u32 tile_addr = gRDPStateManager.GetTileAddress( rdp_tile.tmem );

	if (y1 > scissors.bottom)
		y1 = scissors.bottom;

	DL_PF ("MemRect : addr =0x%08x -(%d, %d, %d, %d), Width: %d\n", tile_addr, x0, y0, x1, y1, g_CI.Width);

	u32 y, width = x1 - x0;
	u32 tex_width = rdp_tile.line << 3;
	u8 * texaddr = g_pu8RamBase + tile_addr + tex_width*(tex_rect.s/32) + (tex_rect.t/32);
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
// Used for Sprite rotation
void DLParser_S2DEX_ObjMoveMem( MicroCodeCommand command )
{	
	u32 addr = RDPSegAddr(command.inst.cmd1);
	u32 index = command.inst.cmd0 & 0xFFFF;

	if( index == 0 )	// Mtx
	{
		uObjMtx* mtx = (uObjMtx *)(addr+g_pu8RamBase);
		mat2D.A = mtx->A/65536.0f;
		mat2D.B = mtx->B/65536.0f;
		mat2D.C = mtx->C/65536.0f;
		mat2D.D = mtx->D/65536.0f;
		mat2D.X = float(mtx->X>>2);
		mat2D.Y = float(mtx->Y>>2);
		mat2D.BaseScaleX = mtx->BaseScaleX/1024.0f;
		mat2D.BaseScaleY = mtx->BaseScaleY/1024.0f;
	}
	else if( index == 2 )	// Sub Mtx
	{
		uObjSubMtx* sub = (uObjSubMtx*)(addr+g_pu8RamBase);
		mat2D.X = float(sub->X>>2);
		mat2D.Y = float(sub->Y>>2);
		mat2D.BaseScaleX = sub->BaseScaleX/1024.0f;
		mat2D.BaseScaleY = sub->BaseScaleY/1024.0f;
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

	s16 frameX = objBg->frameX / 4;
	s16 frameY = objBg->frameY / 4;

	u16 frameW = (objBg->frameW / 4) + frameX;
	u16 frameH = (objBg->frameH / 4) + frameY;

	u16 imageX = objBg->imageX / 32;
	u16 imageY = objBg->imageY / 32;

	u16 scaleX = objBg->scaleW/1024;
	u16 scaleY = objBg->scaleH/1024;

	u16 imageW = objBg->imageW/4;
	u16 imageH = objBg->imageH/4;

	u32 x2 = frameX + (imageW-imageX)/scaleX;
	u32 y2 = frameY + (imageH-imageY)/scaleY;

	u32 u1 = (frameW-x2)*scaleX;
	u32 v1 = (frameH-y2)*scaleY;


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


	CRefPtr<CTexture>       texture( CTextureCache::Get()->GetTexture( &ti ) );
	texture->GetTexture()->InstallTexture();
	texture->UpdateIfNecessary();

	if (g_ROM.GameHacks != YOSHI)
	{
		PSPRenderer::Get()->Draw2DTexture( (float)frameX, (float)frameY, (float)frameW, (float)frameH, (float)imageX, (float)imageY, (float)imageW, (float)imageH );
	}
	else
	{
		PSPRenderer::Get()->Draw2DTexture((float)frameX, (float)frameY, (float)x2, (float)y2, (float)imageX, (float)imageY, (float)imageW, (float)imageH);
		PSPRenderer::Get()->Draw2DTexture((float)x2, (float)frameY, (float)frameW, (float)y2, 0, (float)imageY, (float)u1, (float)imageH);
		PSPRenderer::Get()->Draw2DTexture((float)frameX, (float)y2, (float)x2, (float)frameH, (float)imageX, 0, (float)imageW, (float)v1);
		PSPRenderer::Get()->Draw2DTexture((float)x2, (float)y2, (float)frameW, (float)frameH, 0, 0, (float)u1, (float)v1);
	}
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
	
	/*if( ((command.inst.cmd0)&0x00FFFFFF) != 0 )
	{
		DAEDALUS_ERROR("Mtx bg1cyc");
		DLParser_GBI1_Mtx(command);
		return;
	}*/

	DLParser_S2DEX_Bg1cyc(command);
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_S2DEX_ObjRendermode_2( MicroCodeCommand command )
{
	/*if( ((command.inst.cmd0)&0xFFFFFF) != 0 || ((command.inst.cmd1)&0xFFFFFF00) != 0 )
	{
		// This is a TRI2 cmd
		DAEDALUS_ERROR("tri2 Y");
		DLParser_GBI1_Tri2(command);
		return;
	}*/

	DLParser_S2DEX_ObjRendermode(command);
}

#endif // UCODE_S2DEX_H__
