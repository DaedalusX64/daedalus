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

#ifndef GSP_S2DEX_H
#define GSP_S2DEX_H


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


UcodeFunc( DLParser_S2DEX_BgCopy );
UcodeFunc( DLParser_S2DEX_SelectDl );
UcodeFunc( DLParser_S2DEX_ObjSprite );
UcodeFunc( DLParser_S2DEX_ObjRectangle );
UcodeFunc( DLParser_S2DEX_ObjRendermode );
UcodeFunc( DLParser_S2DEX_ObjLoadTxtr );
UcodeFunc( DLParser_S2DEX_ObjLdtxSprite );
UcodeFunc( DLParser_S2DEX_ObjLdtxRect );
UcodeFunc( DLParser_S2DEX_ObjLdtxRectR );
UcodeFunc( DLParser_S2DEX_RDPHalf_0 );
UcodeFunc( DLParser_S2DEX_ObjMoveMem );
UcodeFunc( DLParser_S2DEX_Bg1cyc );
UcodeFunc( DLParser_S2DEX_ObjRectangleR );
UcodeFunc( DLParser_S2DEX_ObjRendermode_2 );
UcodeFunc( DLParser_S2DEX_Bg1cyc_2 );

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

#endif /* GSP_S2DEX_H */
