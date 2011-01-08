/*
Copyright (C) 2010 StrmnNrmn

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

#include "stdafx.h"

#include "RDP.h"
#include "DebugDisplayList.h"

#include <pspgu.h>


/* 

To Devs:

 Once blenders are complete please clean up after yourself before commiting.
 Incomplete blenders shouldn't be summitted, also please test and make sure the XX blender actually works !
 also please check common games (Mario, Zelda, Majora's Mask before commiting)
 Note : Please edo not create extra brakes and such, only do so if is really needed.
		For example adding a hack on XXX blender.

 Deafult case, should work most of the time, ignore most unknown blenders, if the XXX looks correct of course.

 Please avoid using 0xXXXX, instead use our defines for ex : BLEND_NOOP1

 To create new blenders :

 Daedalus should output everything you need to create new blenders, for example :
 Blender: c800 - First:  Fog * AShade + In * 1-A 0000 - Second:  In * AIn + In * 1-A

 As you can see c800 goes first, and is our defined ==> BLEND_FOG_ASHADE1,
 now our second is 0000, which is a BLEND_NOOP1.

 Now our blender has been finished, this how it looks
 case MAKE_BLEND_MODE( BLEND_FOG_ASHADE1, BLEND_NOOP1 ):

 Next step is figuring out how to handled the created blender.
 I'll leave this step to common sense since you only have three choices :) 

 The format that our blenders should be for example :

	// Blender:
	// Used on the F-Zero - Tracks Correction
case MAKE_BLEND_MODE( BLEND_FOG_ASHADE1, BLEND_PASS3 ):
	// c800 - First:  Fog * AShade + In * 1-A 
	// 3200 - Second:  Fog * AShade + In * 1-A


*/
/*

// P or M
#define	G_BL_CLR_IN	0
#define	G_BL_CLR_MEM	1
#define	G_BL_CLR_BL	2
#define	G_BL_CLR_FOG	3

// A inputs
#define	G_BL_A_IN	0
#define	G_BL_A_FOG	1
#define	G_BL_A_SHADE	2
#define	G_BL_0		3

// B inputs
#define	G_BL_1MA	0
#define	G_BL_A_MEM	1
#define	G_BL_1		2
#define	G_BL_0		3

#define	GBL_c1(m1a, m1b, m2a, m2b)	\
	(m1a) << 30 | (m1b) << 26 | (m2a) << 22 | (m2b) << 18
#define	GBL_c2(m1a, m1b, m2a, m2b)	\
	(m1a) << 28 | (m1b) << 24 | (m2a) << 20 | (m2b) << 16

#define	RM_AA_ZB_OPA_SURF(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	G_RM_FOG_SHADE_A		GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA)
#define	G_RM_FOG_PRIM_A			GBL_c1(G_BL_CLR_FOG, G_BL_A_FOG, G_BL_CLR_IN, G_BL_1MA)
#define	G_RM_PASS				GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)
#define	RM_AA_ZB_OPA_SURF(clk)	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

*/
/*

Possible Blending Inputs:

    In  -   Input from color combiner
    Mem -   Input from current frame buffer
    Fog -   Fog generator
    BL  -   Blender

Possible Blending Factors:
    A-IN    -   Alpha from color combiner
    A-MEM   -   Alpha from current frame buffer
    (1-A)   -   
    A-FOG   -   Alpha of fog color
    A-SHADE -   Alpha of shade
    1   -   1
    0   -   0

*/

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************
const char * sc_szBlClr[4] = { "In",  "Mem",  "Bl",     "Fog" };
const char * sc_szBlA1[4]  = { "AIn", "AFog", "AShade", "0" };
const char * sc_szBlA2[4]  = { "1-A", "AMem", "1",      "?" };
#endif


#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************
static void DebugBlender( u32 blender )	
{
	//u32 blendmode_1 = u32( gRDPOtherMode.blender & 0xcccc );
	//u32 blendmode_2 = u32( gRDPOtherMode.blender & 0x3333 );

	u32 m1A_1 = (gRDPOtherMode.blender>>14) & 0x3;
	u32 m1B_1 = (gRDPOtherMode.blender>>10) & 0x3;
	u32 m2A_1 = (gRDPOtherMode.blender>>6) & 0x3;
	u32 m2B_1 = (gRDPOtherMode.blender>>2) & 0x3;

	u32 m1A_2 = (gRDPOtherMode.blender>>12) & 0x3;
	u32 m1B_2 = (gRDPOtherMode.blender>>8) & 0x3;
	u32 m2A_2 = (gRDPOtherMode.blender>>4) & 0x3;
	u32 m2B_2 = (gRDPOtherMode.blender   ) & 0x3;

	DAEDALUS_ERROR( "Unknown Blender: %04x - :%s * %s + %s * %s || :%s * %s + %s * %s", blender,
			sc_szBlClr[m1A_1], sc_szBlA1[m1B_1], sc_szBlClr[m2A_1], sc_szBlA2[m2B_1],
			sc_szBlClr[m1A_2], sc_szBlA1[m1B_2], sc_szBlClr[m2A_2], sc_szBlA2[m2B_2]);

}
#endif

//*****************************************************************************
// This version uses a 16bit hash, which is around 4X times faster than the old 64bit version
//*****************************************************************************
void InitBlenderMode( u32 blender )					// Set Alpha Blender mode
{
	u32 blendmode = blender >> 16;

	switch ( blendmode )
	{	
	case 0x0c08:					// In * 0 + In * 1 || :In * AIn + In * 1-A				Tarzan - Medalion in bottom part of the screen
	case 0x0f0a:					// In * 0 + In * 1 || :In * 0 + In * 1 :				SSV - ??? and MM - Walls
	case 0xc302:					// Fog * AIn + In * 1-A || :In * 0 + In * 1				ISS64 - Ground
	case 0xc702:					// Fog * AFog + In * 1-A || :In * 0 + In * 1			Donald Duck - Sky
	case 0xfa00:					// Fog * AShade + In * 1-A || :Fog * AShade + In * 1-A	F-Zero - Power Roads
		break;
	//
	// Add here blenders which work fine with default case but causes too much spam, this disabled in release mode
	//
#ifdef DAEDALUS_DEBUG_DISPLAYLIST 
	case 0x0010:					// In * AIn + In * 1-A || :In * AIn + Mem * 1-A			Hey You Pikachu - Shadow
	case 0xc410:					// Fog * AFog + In * 1-A || :In * AIn + Mem * 1-A		Donald Duck - Stars
	case 0xc810:					// Fog * AShade + In * 1-A || :In * AIn + Mem * 1-A		SSV - Fog? and MM - Shadows
	case 0x0c18:					// In * 0 + In * 1 || :In * AIn + Mem * 1-A:			SSV - WaterFall and dust
	case 0x0050:					// In * AIn + Mem * 1-A || :In * AIn + Mem * 1-A:		SSV - TV Screen and SM64 text
	case 0x0040:					// In * AIn + Mem * 1-A || :In * AIn + In * 1-A			Mario - Princess peach text
	case 0x8410:					// Bl * AFog + In * 1-A || :In * AIn + Mem * 1-A		Paper Mario Menu	
		sceGuBlendFunc( GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuEnable( GU_BLEND );
		break;
#endif
	//
	// Default case should handle most blenders, ignore most unknown blenders unless something is messed up
	//
	default:
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		DebugBlender( blendmode );
#endif
		sceGuBlendFunc( GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
		sceGuEnable( GU_BLEND );
		break;
	}
}	
	
//*****************************************************************************
//
//*****************************************************************************
