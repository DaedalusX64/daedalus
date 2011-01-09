/*
Copyright (C) 2001,2006,2007 StrmnNrmn

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

#include "BlendModes.h"
#include "PSPRenderer.h"
#include "Texture.h"
#include "RDP.h"

#include "Graphics/NativeTexture.h"
#include "Graphics/ColourValue.h"

#include "Core/ROM.h"

#include "Utility/Preferences.h"

#include <pspgu.h>

extern bool bStarOrigin;

/* Define to handle first cycle */

//#define CHECK_FIRST_CYCLE

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
static const char * sc_colcombtypes32[32] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ", 
	"Shade       ", "Env         ",
	"1           ", "CombAlp     ",
	"Texel0_Alp  ", "Texel1_Alp  ",
	"Prim_Alpha  ", "Shade_Alpha ",
	"Env_Alpha   ", "LOD_Frac    ",
	"PrimLODFrac ", "K5          ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ",	"0           "
};
static const char *sc_colcombtypes16[16] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ", 
	"Shade       ", "Env         ",
	"1           ", "CombAlp     ",
	"Texel0_Alp  ", "Texel1_Alp  ",
	"Prim_Alp    ", "Shade_Alpha ",
	"Env_Alpha   ", "LOD_Frac    ",
	"PrimLOD_Frac", "0           "
};
static const char *sc_colcombtypes8[8] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ", 
	"Shade       ", "Env         ",
	"1           ", "0           ",
};


void PrintMux( FILE * fh, u64 mux )
{
	u32 mux0 = (u32)(mux>>32);
	u32 mux1 = (u32)(mux);
	
	u32 aRGB0  = (mux0>>20)&0x0F;	// c1 c1		// a0
	u32 bRGB0  = (mux1>>28)&0x0F;	// c1 c2		// b0
	u32 cRGB0  = (mux0>>15)&0x1F;	// c1 c3		// c0
	u32 dRGB0  = (mux1>>15)&0x07;	// c1 c4		// d0

	u32 aA0    = (mux0>>12)&0x07;	// c1 a1		// Aa0
	u32 bA0    = (mux1>>12)&0x07;	// c1 a2		// Ab0
	u32 cA0    = (mux0>>9 )&0x07;	// c1 a3		// Ac0
	u32 dA0    = (mux1>>9 )&0x07;	// c1 a4		// Ad0

	u32 aRGB1  = (mux0>>5 )&0x0F;	// c2 c1		// a1
	u32 bRGB1  = (mux1>>24)&0x0F;	// c2 c2		// b1
	u32 cRGB1  = (mux0    )&0x1F;	// c2 c3		// c1
	u32 dRGB1  = (mux1>>6 )&0x07;	// c2 c4		// d1
	
	u32 aA1    = (mux1>>21)&0x07;	// c2 a1		// Aa1
	u32 bA1    = (mux1>>3 )&0x07;	// c2 a2		// Ab1
	u32 cA1    = (mux1>>18)&0x07;	// c2 a3		// Ac1
	u32 dA1    = (mux1    )&0x07;	// c2 a4		// Ad1

	fprintf(fh, "\n\t\tcase 0x%08x%08xLL:\n", mux0, mux1);
	fprintf(fh, "\t\t//aRGB0: (%s - %s) * %s + %s\n", sc_colcombtypes16[aRGB0], sc_colcombtypes16[bRGB0], sc_colcombtypes32[cRGB0], sc_colcombtypes8[dRGB0]);		
	fprintf(fh, "\t\t//aA0  : (%s - %s) * %s + %s\n", sc_colcombtypes8[aA0], sc_colcombtypes8[bA0], sc_colcombtypes8[cA0], sc_colcombtypes8[dA0]);
	fprintf(fh, "\t\t//aRGB1: (%s - %s) * %s + %s\n", sc_colcombtypes16[aRGB1], sc_colcombtypes16[bRGB1], sc_colcombtypes32[cRGB1], sc_colcombtypes8[dRGB1]);		
	fprintf(fh, "\t\t//aA1  : (%s - %s) * %s + %s\n", sc_colcombtypes8[aA1],  sc_colcombtypes8[bA1], sc_colcombtypes8[cA1],  sc_colcombtypes8[dA1]);
}

//***************************************************************************
//*General blender used for testing //Corn
//*Inside a (empty) blender paste (only)-> BLEND_MODE_MAKER
//***************************************************************************
u32	gTXTFUNC=0;

u32	gSetRGB=0;
u32	gSetA=0;
u32	gSetRGBA=0;
u32	gModA=0;
u32	gAOpaque=0;

u32	gsceENV=0;

u32	gNumCyc=3;
u32 gTexInstall=1;

const char *gPSPtxtFunc[10] =
{
	"Modulate RGB",
	"Modulate RGBA",
	"Blend RGB",
	"Blend RGBA",
	"Add RGB",
	"Add RGBA",
	"Replace RGB",
	"Replace RGBA",
	"Decal RGB",
	"Decal RGBA"
};

const char *gCAdj[4] =
{
	"OFF",
	"Prim Color",
	"Prim Color Replicate Alpha",
	"Env Color"
};

#define BLEND_MODE_MAKER \
{ \
	const u32 PSPtxtFunc[5] = \
	{ \
		GU_TFX_MODULATE, \
		GU_TFX_BLEND, \
		GU_TFX_ADD, \
		GU_TFX_REPLACE, \
		GU_TFX_DECAL \
	}; \
	const u32 PSPtxtA[2] = \
	{ \
		GU_TCC_RGB, \
		GU_TCC_RGBA \
	}; \
	if( num_cycles & gNumCyc ) \
	{ \
		if( gSetRGB ) \
		{ \
			if( gSetRGB==1 ) details.ColourAdjuster.SetRGB( details.PrimColour ); \
			if( gSetRGB==2 ) details.ColourAdjuster.SetRGB( details.PrimColour.ReplicateAlpha() ); \
			if( gSetRGB==3 ) details.ColourAdjuster.SetRGB( details.EnvColour ); \
		} \
		if( gSetA ) \
		{ \
			if( gSetA==1 ) details.ColourAdjuster.SetA( details.PrimColour ); \
			if( gSetA==2 ) details.ColourAdjuster.SetA( details.PrimColour.ReplicateAlpha() ); \
			if( gSetA==3 ) details.ColourAdjuster.SetA( details.EnvColour ); \
		} \
		if( gSetRGBA ) \
		{ \
			if( gSetRGBA==1 ) details.ColourAdjuster.SetRGBA( details.PrimColour ); \
			if( gSetRGBA==2 ) details.ColourAdjuster.SetRGBA( details.PrimColour.ReplicateAlpha() ); \
			if( gSetRGBA==3 ) details.ColourAdjuster.SetRGBA( details.EnvColour ); \
		} \
		if( gModA ) \
		{ \
			if( gModA==1 ) details.ColourAdjuster.ModulateA( details.PrimColour ); \
			if( gModA==2 ) details.ColourAdjuster.ModulateA( details.PrimColour.ReplicateAlpha() ); \
			if( gModA==3 ) details.ColourAdjuster.ModulateA( details.EnvColour ); \
		} \
		if( gAOpaque ) details.ColourAdjuster.SetAOpaque(); \
		if( gsceENV ) \
		{ \
			if( gsceENV==1 ) sceGuTexEnvColor( details.EnvColour.GetColour() ); \
			if( gsceENV==2 ) sceGuTexEnvColor( details.PrimColour.GetColour() ); \
		} \
		if ( gTexInstall ) \
		{  if( gTexInstall==1 ) details.InstallTexture = false; \
		   if( gTexInstall==2 ) details.InstallTexture = true;  \
		} \
		sceGuTexFunc( PSPtxtFunc[ (gTXTFUNC >> 1) % 6 ], PSPtxtA[ gTXTFUNC & 1 ] ); \
	} \
} \

#endif
/* To Devs,
 Placeholder for blendmode maker guide
 */



// Start Blends

/* 
//##
*/

/*
//#A
*/

/* 
//#B
*/
/*
//#C
*/


/*
//#D
*/

/*
//#E
*/ 

/* 
//#F
*/

/*
#G
*/

/*
//#H
*/ 

/*
  #I
*/ 

/*
  #K
*/ 

/*
 //#M
*/ 

// Mario 64 - Star
//case 0x00fffffffffcfa7dLL:
//aRGB0	: (0		- 0		) * 0	+ Texel0 
//aA0	: (0		- 0		) * 0	+ Env 
//aRGB1	: (0		- 0		) * 0	+ Texel0 
//aA1	: (0		- 0		) * 0	+ Env 
void BlendMode_0x00fffffffffcfa7dLL (BLEND_MODE_ARGS)
{
	// Check to be sure we are blending the star !!!
	// We should make this check more robust to avoid messing any other stuff.
	if( num_cycles == 1 && bStarOrigin )
	{
		details.ColourAdjuster.SetRGB( c32::Gold );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
}

/* 
//#N
*/

/* 
//#O
*/

/*
//#P
*/


/*
//#Q
*/ 

/* 
//#R
*/

/* 
//#S
*/

/
/*
//#T
*/


/*
//#U
*/


/*
//#V
*/
/*
//#W
*/



/*
//#X
*/
 

/*
//#Z
*/


OverrideBlendModeFn		LookupOverrideBlendModeFunction( u64 mux )
{
#ifndef DAEDALUS_PUBLIC_RELEASE
	if(!gGlobalPreferences.CustomBlendModes) return NULL;
#endif
	switch(mux)
	{
#define BLEND_MODE( x )		case (x):	return BlendMode_##x;

	#undef BLEND_MODE
	}

	return NULL;
}
