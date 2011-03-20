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


//*****************************************************************************
// GBI2
//*****************************************************************************
#define	G_GBI2_RDPHALF_2		0xf1
#define	G_GBI2_SETOTHERMODE_H	0xe3
#define	G_GBI2_SETOTHERMODE_L	0xe2
#define	G_GBI2_RDPHALF_1		0xe1
#define	G_GBI2_SPNOOP			0xe0
#define	G_GBI2_ENDDL			0xdf
#define	G_GBI2_DL				0xde
#define	G_GBI2_LOAD_UCODE		0xdd
#define	G_GBI2_MOVEMEM			0xdc
#define	G_GBI2_MOVEWORD			0xdb
#define	G_GBI2_MTX				0xda
#define G_GBI2_GEOMETRYMODE		0xd9
#define	G_GBI2_POPMTX			0xd8
#define	G_GBI2_TEXTURE			0xd7
#define	G_GBI2_DMA_IO			0xd6
#define	G_GBI2_SPECIAL_1		0xd5
#define	G_GBI2_SPECIAL_2		0xd4
#define	G_GBI2_SPECIAL_3		0xd3

#define	G_GBI2_NOOP				0x00
#define	G_GBI2_VTX				0x01
#define	G_GBI2_MODIFYVTX		0x02
#define	G_GBI2_CULLDL			0x03
#define	G_GBI2_BRANCH_Z			0x04
#define	G_GBI2_TRI1				0x05
#define G_GBI2_TRI2				0x06
#define G_GBI2_QUAD				0x07
#define G_GBI2_LINE3D			0x08

//*****************************************************************************
// GBI1
//*****************************************************************************
#define	G_GBI1_SPNOOP			0
#define	G_GBI1_MTX				1
#define G_GBI1_RESERVED0		2
#define G_GBI1_MOVEMEM			3
#define	G_GBI1_VTX				4
#define G_GBI1_RESERVED1		5
#define	G_GBI1_DL				6
#define G_GBI1_RESERVED2		7
#define G_GBI1_RESERVED3		8
#define G_GBI1_SPRITE2D_BASE	9

#define	G_GBI1_NOOP				0xc0	/*   0 */
#define	G_GBI1_TRI1				0xbf
#define G_GBI1_CULLDL			0xbe
#define	G_GBI1_POPMTX			0xbd
#define	G_GBI1_MOVEWORD			0xbc
#define	G_GBI1_TEXTURE			0xbb
#define	G_GBI1_SETOTHERMODE_H	0xba
#define	G_GBI1_SETOTHERMODE_L	0xb9
#define G_GBI1_ENDDL			0xb8
#define G_GBI1_SETGEOMETRYMODE	0xb7
#define G_GBI1_CLEARGEOMETRYMODE	0xb6
#define G_GBI1_LINE3D			0xb5
#define G_GBI1_RDPHALF_1		0xb4
#define G_GBI1_RDPHALF_2		0xb3
//#if (defined(F3DEX_GBI)||defined(F3DLP_GBI))
#  define G_GBI1_MODIFYVTX		0xb2
#  define G_GBI1_TRI2			0xb1
#  define G_GBI1_BRANCH_Z		0xb0
#  define G_GBI1_LOAD_UCODE		0xaf
//#else
#  define G_GBI1_RDPHALF_CONT	0xb2
//#endif

// Overloaded for sprite microcode
#define G_GBI1_SPRITE2D_SCALEFLIP    0xbe
#define G_GBI1_SPRITE2D_DRAW         0xbd


//*****************************************************************************
// DKR
//*****************************************************************************
// 4 is something like a conditional DL

#define G_DMATRI	0x05
#define G_DLINMEM	0x07

//*****************************************************************************
// RDP - Common to GBI1 and GBI2
//*****************************************************************************
#define	G_RDP_SETCIMG			0xff	/*  -1 */
#define	G_RDP_SETZIMG			0xfe	/*  -2 */
#define	G_RDP_SETTIMG			0xfd	/*  -3 */
#define	G_RDP_SETCOMBINE		0xfc	/*  -4 */
#define	G_RDP_SETENVCOLOR		0xfb	/*  -5 */
#define	G_RDP_SETPRIMCOLOR		0xfa	/*  -6 */
#define	G_RDP_SETBLENDCOLOR		0xf9	/*  -7 */
#define	G_RDP_SETFOGCOLOR		0xf8	/*  -8 */
#define	G_RDP_SETFILLCOLOR		0xf7	/*  -9 */
#define	G_RDP_FILLRECT			0xf6	/* -10 */
#define	G_RDP_SETTILE			0xf5	/* -11 */
#define	G_RDP_LOADTILE			0xf4	/* -12 */
#define	G_RDP_LOADBLOCK			0xf3	/* -13 */
#define	G_RDP_SETTILESIZE		0xf2	/* -14 */
#define	G_RDP_LOADTLUT			0xf0	/* -16 */
#define	G_RDP_RDPSETOTHERMODE	0xef	/* -17 */
#define	G_RDP_SETPRIMDEPTH		0xee	/* -18 */
#define	G_RDP_SETSCISSOR		0xed	/* -19 */
#define	G_RDP_SETCONVERT		0xec	/* -20 */
#define	G_RDP_SETKEYR			0xeb	/* -21 */
#define	G_RDP_SETKEYGB			0xea	/* -22 */
#define	G_RDP_RDPFULLSYNC		0xe9	/* -23 */
#define	G_RDP_RDPTILESYNC		0xe8	/* -24 */
#define	G_RDP_RDPPIPESYNC		0xe7	/* -25 */
#define	G_RDP_RDPLOADSYNC		0xe6	/* -26 */
#define G_RDP_TEXRECTFLIP		0xe5	/* -27 */
#define G_RDP_TEXRECT			0xe4	/* -28 */






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
#define G_IT_UNKNOWN		(1 << G_MDSFT_TEXTLUT)
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
// GBI1
#define G_ZBUFFER				0x00000001
#define G_TEXTURE_ENABLE		0x00000002	// Microcode use only 
#define G_SHADE					0x00000004	// enable Gouraud interp 
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
// flags for G_SETGEOMETRYMODE
// GBI2
#define G_ZELDA_ZBUFFER				G_ZBUFFER
#define G_ZELDA_CULL_BACK			0x00000200
#define G_ZELDA_CULL_FRONT			0x00000400
#define G_ZELDA_FOG					G_FOG
#define G_ZELDA_LIGHTING			G_LIGHTING
#define G_ZELDA_TEXTURE_GEN			G_TEXTURE_GEN
#define G_ZELDA_TEXTURE_GEN_LINEAR	G_TEXTURE_GEN_LINEAR
#define G_ZELDA_SHADING_SMOOTH		0x00200000

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

// 0,2,4,6 are reserved by G_MTX
# define G_GBI2_MV_VIEWPORT	8
# define G_GBI2_MV_LIGHT	10
# define G_GBI2_MV_POINT	12
# define G_GBI2_MV_MATRIX	14		// NOTE: this is in moveword table
# define G_GBI2_MVO_LOOKATX	(0*24)
# define G_GBI2_MVO_LOOKATY	(1*24)
# define G_GBI2_MVO_L0	(2*24)
# define G_GBI2_MVO_L1	(3*24)
# define G_GBI2_MVO_L2	(4*24)
# define G_GBI2_MVO_L3	(5*24)
# define G_GBI2_MVO_L4	(6*24)
# define G_GBI2_MVO_L5	(7*24)
# define G_GBI2_MVO_L6	(8*24)
# define G_GBI2_MVO_L7	(9*24)

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
//	These changed with v2.0 of the microcode
//
#define	G_GBI1_MTX_MODELVIEW	0x00
#define	G_GBI1_MTX_PROJECTION	0x01
#define	G_GBI1_MTX_MUL			0x00
#define	G_GBI1_MTX_LOAD			0x02
#define G_GBI1_MTX_NOPUSH		0x00
#define G_GBI1_MTX_PUSH			0x04


#define G_GBI2_MTX_MODELVIEW	0x00
#define G_GBI2_MTX_PROJECTION	0x04
#define G_GBI2_MTX_MUL			0x00
#define G_GBI2_MTX_LOAD			0x02
#define G_GBI2_MTX_PUSH			0x00
#define G_GBI2_MTX_NOPUSH		0x01

/*
 * The following commands are the "generated" RDP commands; the user
 * never sees them, the RSP microcode generates them.
 *
 * The layout of the bits is magical, to save work in the ucode.
 * These id's are -56, -52, -54, -50, -55, -51, -53, -49, ...
 *                                 edge, shade, texture, zbuff bits:  estz
 */

#define G_RDP_TRI_FILL				0xc8 /* fill triangle:            11001000 */
#define G_RDP_TRI_SHADE				0xcc /* shade triangle:           11001100 */
#define G_RDP_TRI_TXTR				0xca /* texture triangle:         11001010 */
#define G_RDP_TRI_SHADE_TXTR		0xce /* shade, texture triangle:  11001110 */
#define G_RDP_TRI_FILL_ZBUFF		0xc9 /* fill, zbuff triangle:     11001001 */
#define G_RDP_TRI_SHADE_ZBUFF		0xcd /* shade, zbuff triangle:    11001101 */
#define G_RDP_TRI_TXTR_ZBUFF		0xcb /* texture, zbuff triangle:  11001011 */
#define G_RDP_TRI_SHADE_TXTR_ZBUFF	0xcf /* shade, txtr, zbuff trngl: 11001111 */


#endif // __ULTRA_GBI_H__
