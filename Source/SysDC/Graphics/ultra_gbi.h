/*
Copyright (C) 2001 StrmnNrmn

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

#ifndef __ULTRA_GBI_H__
#define __ULTRA_GBI_H__

// DMA commands:
#define	G_SPNOOP			0	// handle 0 gracefully 
#define	G_MTX				1
#define G_RESERVED0			2	// not implemeted 
#define G_MOVEMEM			3	// move a block of memory (up to 4 words) to dmem 
#define	G_VTX				4
#define G_RESERVED1			5	// not implemeted 
#define	G_DL				6
#define G_RESERVED2			7	// not implemeted 
#define G_RESERVED3			8	// not implemeted 
#define G_SPRITE2D   		9	// sprite command 

// IMMEDIATE commands:
#define	G_IMMFIRST			-65
#define	G_TRI1				(G_IMMFIRST-0)
#define G_CULLDL			(G_IMMFIRST-1)
#define	G_POPMTX			(G_IMMFIRST-2)
#define	G_MOVEWORD			(G_IMMFIRST-3)
#define	G_TEXTURE			(G_IMMFIRST-4)
#define	G_SETOTHERMODE_H	(G_IMMFIRST-5)
#define	G_SETOTHERMODE_L	(G_IMMFIRST-6)
#define G_ENDDL				(G_IMMFIRST-7)
#define G_SETGEOMETRYMODE	(G_IMMFIRST-8)
#define G_CLEARGEOMETRYMODE	(G_IMMFIRST-9)
#define G_LINE3D			(G_IMMFIRST-10)
#define G_RDPHALF_1			(G_IMMFIRST-11)
#define G_RDPHALF_2			(G_IMMFIRST-12)
#define G_RDPHALF_CONT		(G_IMMFIRST-13)

#define G_TRI2 0xb1

// RDP commands:
#define	G_NOOP				0xc0	//   0 
#define	G_SETCIMG			0xff	//  -1 
#define	G_SETZIMG			0xfe	//  -2 
#define	G_SETTIMG			0xfd	//  -3 
#define	G_SETCOMBINE		0xfc	//  -4 
#define	G_SETENVCOLOR		0xfb	//  -5 
#define	G_SETPRIMCOLOR		0xfa	//  -6 
#define	G_SETBLENDCOLOR		0xf9	//  -7 
#define	G_SETFOGCOLOR		0xf8	//  -8 
#define	G_SETFILLCOLOR		0xf7	//  -9 
#define	G_FILLRECT			0xf6	// -10 
#define	G_SETTILE			0xf5	// -11 
#define	G_LOADTILE			0xf4	// -12 
#define	G_LOADBLOCK			0xf3	// -13 
#define	G_SETTILESIZE		0xf2	// -14 
#define	G_LOADTLUT			0xf0	// -16 
#define	G_RDPSETOTHERMODE	0xef	// -17 
#define	G_SETPRIMDEPTH		0xee	// -18 
#define	G_SETSCISSOR		0xed	// -19 
#define	G_SETCONVERT		0xec	// -20 
#define	G_SETKEYR			0xeb	// -21 
#define	G_SETKEYGB			0xea	// -22 
#define	G_RDPFULLSYNC		0xe9	// -23 
#define	G_RDPTILESYNC		0xe8	// -24 
#define	G_RDPPIPESYNC		0xe7	// -25 
#define	G_RDPLOADSYNC		0xe6	// -26 
#define G_TEXRECTFLIP		0xe5	// -27 
#define G_TEXRECT			0xe4	// -28 




//
// G_SETOTHERMODE_L sft: shift count
 
#define	G_MDSFT_ALPHACOMPARE		0
#define	G_MDSFT_ZSRCSEL				2
#define	G_MDSFT_RENDERMODE			3
#define	G_MDSFT_BLENDER				16

//
// G_SETOTHERMODE_H sft: shift count
 
#define	G_MDSFT_BLENDMASK		0	// unsupported 
#define	G_MDSFT_ALPHADITHER		4
#define	G_MDSFT_RGBDITHER		6

#define	G_MDSFT_COMBKEY			8
#define	G_MDSFT_TEXTCONV		9
#define	G_MDSFT_TEXTFILT		12
#define	G_MDSFT_TEXTLUT			14
#define	G_MDSFT_TEXTLOD			16
#define	G_MDSFT_TEXTDETAIL		17
#define	G_MDSFT_TEXTPERSP		19
#define	G_MDSFT_CYCLETYPE		20
#define	G_MDSFT_COLORDITHER		22	// unsupported in HW 2.0 
#define	G_MDSFT_PIPELINE		23

// G_SETOTHERMODE_H gPipelineMode 
#define	G_PM_1PRIMITIVE		(1 << G_MDSFT_PIPELINE)
#define	G_PM_NPRIMITIVE		(0 << G_MDSFT_PIPELINE)

// G_SETOTHERMODE_H gSetCycleType 
#define	G_CYC_1CYCLE		(0 << G_MDSFT_CYCLETYPE)
#define	G_CYC_2CYCLE		(1 << G_MDSFT_CYCLETYPE)
#define	G_CYC_COPY			(2 << G_MDSFT_CYCLETYPE)
#define	G_CYC_FILL			(3 << G_MDSFT_CYCLETYPE)

// G_SETOTHERMODE_H gSetTexturePersp 
#define G_TP_NONE			(0 << G_MDSFT_TEXTPERSP)
#define G_TP_PERSP			(1 << G_MDSFT_TEXTPERSP)

// G_SETOTHERMODE_H gSetTextureDetail 
#define G_TD_CLAMP			(0 << G_MDSFT_TEXTDETAIL)
#define G_TD_SHARPEN		(1 << G_MDSFT_TEXTDETAIL)
#define G_TD_DETAIL			(2 << G_MDSFT_TEXTDETAIL)

// G_SETOTHERMODE_H gSetTextureLOD 
#define G_TL_TILE			(0 << G_MDSFT_TEXTLOD)
#define G_TL_LOD			(1 << G_MDSFT_TEXTLOD)

// G_SETOTHERMODE_H gSetTextureLUT 
#define G_TT_NONE			(0 << G_MDSFT_TEXTLUT)
#define G_TT_RGBA16			(2 << G_MDSFT_TEXTLUT)
#define G_TT_IA16			(3 << G_MDSFT_TEXTLUT)

// G_SETOTHERMODE_H gSetTextureFilter 
#define G_TF_POINT			(0 << G_MDSFT_TEXTFILT)
#define G_TF_AVERAGE		(3 << G_MDSFT_TEXTFILT)
#define G_TF_BILERP			(2 << G_MDSFT_TEXTFILT)

// G_SETOTHERMODE_H gSetTextureConvert 
#define G_TC_CONV			(0 << G_MDSFT_TEXTCONV)
#define G_TC_FILTCONV		(5 << G_MDSFT_TEXTCONV)
#define G_TC_FILT			(6 << G_MDSFT_TEXTCONV)

// G_SETOTHERMODE_H gSetCombineKey 
#define G_CK_NONE			(0 << G_MDSFT_COMBKEY)
#define G_CK_KEY			(1 << G_MDSFT_COMBKEY)

// G_SETOTHERMODE_H gSetColorDither 
#define	G_CD_MAGICSQ		(0 << G_MDSFT_RGBDITHER)
#define	G_CD_BAYER			(1 << G_MDSFT_RGBDITHER)
#define	G_CD_NOISE			(2 << G_MDSFT_RGBDITHER)

#ifndef _HW_VERSION_1
#define	G_CD_DISABLE		(3 << G_MDSFT_RGBDITHER)
#define	G_CD_ENABLE		G_CD_NOISE	// HW 1.0 compatibility mode 
#else
#define G_CD_ENABLE			(1 << G_MDSFT_COLORDITHER)
#define G_CD_DISABLE		(0 << G_MDSFT_COLORDITHER)
#endif

// G_SETOTHERMODE_H gSetAlphaDither 
#define	G_AD_PATTERN		(0 << G_MDSFT_ALPHADITHER)
#define	G_AD_NOTPATTERN		(1 << G_MDSFT_ALPHADITHER)
#define	G_AD_NOISE			(2 << G_MDSFT_ALPHADITHER)
#define	G_AD_DISABLE		(3 << G_MDSFT_ALPHADITHER)

// G_SETOTHERMODE_L gSetAlphaCompare 
#define	G_AC_NONE			(0 << G_MDSFT_ALPHACOMPARE)
#define	G_AC_THRESHOLD		(1 << G_MDSFT_ALPHACOMPARE)
#define	G_AC_DITHER			(3 << G_MDSFT_ALPHACOMPARE)

// G_SETOTHERMODE_L gSetDepthSource 
#define	G_ZS_PIXEL			(0 << G_MDSFT_ZSRCSEL)
#define	G_ZS_PRIM			(1 << G_MDSFT_ZSRCSEL)

// G_SETOTHERMODE_L gSetRenderMode 
#define	AA_EN			0x0008
#define	Z_CMP			0x0010
#define	Z_UPD			0x0020
#define	IM_RD			0x0040
#define	CLR_ON_CVG		0x0080

#define	CVG_DST_CLAMP	0x0000
#define	CVG_DST_WRAP	0x0100
#define	CVG_DST_FULL	0x0200
#define	CVG_DST_SAVE	0x0300

#define	ZMODE_OPA		0x0000
#define	ZMODE_INTER		0x0400
#define	ZMODE_XLU		0x0800
#define	ZMODE_DEC		0x0c00

#define	CVG_X_ALPHA		0x1000
#define	ALPHA_CVG_SEL	0x2000
#define	FORCE_BL		0x4000
#define	TEX_EDGE		0x0000 // used to be 0x8000 

#define	G_BL_CLR_IN		0
#define	G_BL_CLR_MEM	1
#define	G_BL_CLR_BL		2
#define	G_BL_CLR_FOG	3
#define	G_BL_1MA		0
#define	G_BL_A_MEM		1
#define	G_BL_A_IN		0
#define	G_BL_A_FOG		1
#define	G_BL_A_SHADE	2
#define	G_BL_1			2
#define	G_BL_0			3


//
// flags for G_SETGEOMETRYMODE
//
#define G_ZBUFFER				0x00000001
#define G_TEXTURE_ENABLE		0x00000002	// Microcode use only 
#define G_SHADE					0x00000004	// enable Gouraud interp 
//
#define G_SHADING_SMOOTH		0x00000200	// flat or smooth shaded 
#define G_CULL_FRONT			0x00001000
#define G_CULL_BACK				0x00002000
#define G_CULL_BOTH				0x00003000	// To make code cleaner 
#define G_FOG					0x00010000
#define G_LIGHTING				0x00020000
#define G_TEXTURE_GEN			0x00040000
#define G_TEXTURE_GEN_LINEAR	0x00080000
#define G_LOD					0x00100000	// NOT IMPLEMENTED 

//
// G_SETIMG fmt: set image formats
//
#define G_IM_FMT_RGBA	0
#define G_IM_FMT_YUV	1
#define G_IM_FMT_CI		2
#define G_IM_FMT_IA		3
#define G_IM_FMT_I		4

//
// G_SETIMG siz: set image pixel size
//
#define G_IM_SIZ_4b		0
#define G_IM_SIZ_8b		1
#define G_IM_SIZ_16b	2
#define G_IM_SIZ_32b	3

#define G_IM_SIZ_4b_BYTES		0
#define G_IM_SIZ_4b_TILE_BYTES	G_IM_SIZ_4b_BYTES
#define G_IM_SIZ_4b_LINE_BYTES	G_IM_SIZ_4b_BYTES

#define G_IM_SIZ_8b_BYTES		1
#define G_IM_SIZ_8b_TILE_BYTES	G_IM_SIZ_8b_BYTES
#define G_IM_SIZ_8b_LINE_BYTES	G_IM_SIZ_8b_BYTES

#define G_IM_SIZ_16b_BYTES		2
#define G_IM_SIZ_16b_TILE_BYTES	G_IM_SIZ_16b_BYTES
#define G_IM_SIZ_16b_LINE_BYTES	G_IM_SIZ_16b_BYTES

#define G_IM_SIZ_32b_BYTES		4
#define G_IM_SIZ_32b_TILE_BYTES	2
#define G_IM_SIZ_32b_LINE_BYTES	2

#define G_IM_SIZ_4b_LOAD_BLOCK	G_IM_SIZ_16b
#define G_IM_SIZ_8b_LOAD_BLOCK	G_IM_SIZ_16b
#define G_IM_SIZ_16b_LOAD_BLOCK	G_IM_SIZ_16b
#define G_IM_SIZ_32b_LOAD_BLOCK	G_IM_SIZ_32b

#define G_IM_SIZ_4b_SHIFT  2
#define G_IM_SIZ_8b_SHIFT  1
#define G_IM_SIZ_16b_SHIFT 0
#define G_IM_SIZ_32b_SHIFT 0

#define G_IM_SIZ_4b_INCR  3
#define G_IM_SIZ_8b_INCR  1
#define G_IM_SIZ_16b_INCR 0
#define G_IM_SIZ_32b_INCR 0


//
// Texturing macros
//

#define	G_TX_LOADTILE	7
#define	G_TX_RENDERTILE	0

#define	G_TX_NOMIRROR	0
#define	G_TX_WRAP		0
#define	G_TX_MIRROR		0x1
#define	G_TX_CLAMP		0x2
#define	G_TX_NOMASK		0
#define	G_TX_NOLOD		0



//
// MOVEMEM indices
//
// Each of these indexes an entry in a dmem table
// which points to a 1-4 word block of dmem in
// which to store a 1-4 word DMA.
//
//
#define G_MV_VIEWPORT	0x80
#define G_MV_LOOKATY	0x82
#define G_MV_LOOKATX	0x84
#define G_MV_L0			0x86
#define G_MV_L1			0x88
#define G_MV_L2			0x8a
#define G_MV_L3			0x8c
#define G_MV_L4			0x8e
#define G_MV_L5			0x90
#define G_MV_L6			0x92
#define G_MV_L7			0x94
#define G_MV_TXTATT		0x96
#define G_MV_MATRIX_1	0x9e	// NOTE: this is in moveword table 
#define G_MV_MATRIX_2	0x98
#define G_MV_MATRIX_3	0x9a
#define G_MV_MATRIX_4	0x9c

//
// MOVEWORD indices
//
// Each of these indexes an entry in a dmem table
// which points to a word in dmem in dmem where
// an immediate word will be stored.
//
//
#define G_MW_MATRIX		0x00	// NOTE: also used by movemem 
#define G_MW_NUMLIGHT	0x02
#define G_MW_CLIP		0x04
#define G_MW_SEGMENT	0x06
#define G_MW_FOG		0x08
#define G_MW_LIGHTCOL	0x0a
#define	G_MW_POINTS		0x0c
#define	G_MW_PERSPNORM	0x0e

//
// These are offsets from the address in the dmem table
// 
#define G_MWO_NUMLIGHT			0x00
#define G_MWO_CLIP_RNX			0x04
#define G_MWO_CLIP_RNY			0x0c
#define G_MWO_CLIP_RPX			0x14
#define G_MWO_CLIP_RPY			0x1c
#define G_MWO_SEGMENT_0			0x00
#define G_MWO_SEGMENT_1			0x01
#define G_MWO_SEGMENT_2			0x02
#define G_MWO_SEGMENT_3			0x03
#define G_MWO_SEGMENT_4			0x04
#define G_MWO_SEGMENT_5			0x05
#define G_MWO_SEGMENT_6			0x06
#define G_MWO_SEGMENT_7			0x07
#define G_MWO_SEGMENT_8			0x08
#define G_MWO_SEGMENT_9			0x09
#define G_MWO_SEGMENT_A			0x0a
#define G_MWO_SEGMENT_B			0x0b
#define G_MWO_SEGMENT_C			0x0c
#define G_MWO_SEGMENT_D			0x0d
#define G_MWO_SEGMENT_E			0x0e
#define G_MWO_SEGMENT_F			0x0f
#define G_MWO_FOG				0x00	
#define G_MWO_aLIGHT_1			0x00
#define G_MWO_bLIGHT_1			0x04
#define G_MWO_aLIGHT_2			0x20
#define G_MWO_bLIGHT_2			0x24
#define G_MWO_aLIGHT_3			0x40
#define G_MWO_bLIGHT_3			0x44
#define G_MWO_aLIGHT_4			0x60
#define G_MWO_bLIGHT_4			0x64
#define G_MWO_aLIGHT_5			0x80
#define G_MWO_bLIGHT_5			0x84
#define G_MWO_aLIGHT_6			0xa0
#define G_MWO_bLIGHT_6			0xa4
#define G_MWO_aLIGHT_7			0xc0
#define G_MWO_bLIGHT_7			0xc4
#define G_MWO_aLIGHT_8			0xe0
#define G_MWO_bLIGHT_8			0xe4
#define G_MWO_MATRIX_XX_XY_I	0x00
#define G_MWO_MATRIX_XZ_XW_I	0x04
#define G_MWO_MATRIX_YX_YY_I	0x08
#define G_MWO_MATRIX_YZ_YW_I	0x0c
#define G_MWO_MATRIX_ZX_ZY_I	0x10
#define G_MWO_MATRIX_ZZ_ZW_I	0x14
#define G_MWO_MATRIX_WX_WY_I	0x18
#define G_MWO_MATRIX_WZ_WW_I	0x1c
#define G_MWO_MATRIX_XX_XY_F	0x20
#define G_MWO_MATRIX_XZ_XW_F	0x24
#define G_MWO_MATRIX_YX_YY_F	0x28
#define G_MWO_MATRIX_YZ_YW_F	0x2c
#define G_MWO_MATRIX_ZX_ZY_F	0x30
#define G_MWO_MATRIX_ZZ_ZW_F	0x34
#define G_MWO_MATRIX_WX_WY_F	0x38
#define G_MWO_MATRIX_WZ_WW_F	0x3c
#define G_MWO_POINT_RGBA		0x10
#define G_MWO_POINT_ST			0x14
#define G_MWO_POINT_XYSCREEN	0x18
#define	G_MWO_POINT_ZSCREEN		0x1c



// flags to inhibit pushing of the display list (on branch)
#define G_DL_PUSH		0x00
#define G_DL_NOPUSH		0x01


//
// G_MTX: parameter flags
//
#define	G_MTX_MODELVIEW		0x00
#define	G_MTX_PROJECTION	0x01

#define	G_MTX_MUL			0x00
#define	G_MTX_LOAD			0x02

#define G_MTX_NOPUSH		0x00
#define G_MTX_PUSH			0x04


#endif // __ULTRA_GBI_H__
