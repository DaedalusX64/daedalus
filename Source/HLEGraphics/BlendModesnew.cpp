/*
Copyright (C) 2001,2006,2007,2010 StrmnNrmn

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

#endif

/*
	
	To Devs, Please make sure blendmodes are as simple as possible, we only accept max 3 lines per blendmode here !!! unless is really necesary which shouldn't happen

*/


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

//GoldenEye 007 - Dead Enemies
//case 0x00159a045ffefff8LL:
//aRGB0: (Texel0       - Env         ) * Shade_Alpha  + Env
//aA0  : (Texel0       - 0           ) * Env          + 0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00159a045ffefff8LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

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
// Mario 64 Head
//case 0x0030b26144664924LL:
//aRGB0: (Primitive    - Shade       ) * Texel0       + Shade       
//aA0  : (Primitive    - Shade       ) * Texel0       + Shade       
//aRGB1: (Primitive    - Shade       ) * Texel0       + Shade       
//aA1  : (Primitive    - Shade       ) * Texel0       + Shade       

//Mario 64
//case 0x00147e2844fe7b3dLL:
//aRGB0: (Texel0       - Shade       ) * Texel0_Alp   + Shade       
//aA0  : (0            - 0           ) * 0            + Env         
//aRGB1: (Texel0       - Shade       ) * Texel0_Alp   + Shade       
//aA1  : (0            - 0           ) * 0            + Env       

//SSB Walking Dust
//case 0x0030b2615566db6dLL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (Primitive    - Env         ) * Texel0       + Env         
//aRGB1: (Primitive    - Env         ) * Texel0       + Env         
//aA1  : (Primitive    - Env         ) * Texel0       + Env      
void BlendMode_0x0030b2615566db6dLL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Space Station Silicon Valley - Power Spheres
//case 0x00377fff1ffcf438LL:
//aRGB0: (Primitive    - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + Texel1
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00377fff1ffcf438LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Space Station Silicon Valley - Cave inside Waterfalls
// case 0x00277e041ffcf3fcLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Shade
void BlendMode_0x00277e041ffcf3fcLL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//
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
//Wetrix Wackdrop
//case 0x00127e2433fdf8fcLL:
//aRGB0: (Texel0       - Primitive   ) * Shade        + Primitive   
//aA0  : (0            - 0           ) * 0            + Shade       
//aRGB1: (Texel0       - Primitive   ) * Shade        + Primitive   
//aA1  : (0            - 0           ) * 0            + Shade      

void BlendMode_0x00127e2433fdf8fcLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB(details.PrimColour.ReplicateAlpha());
	details.ColourAdjuster.SetA(details.PrimColour);
	
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);

}


/*
//#X
*/
 

/*
//#Z
*/
// Zelda OOT Grass
//case 0x00267e041ffcfdf8LL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Combined    

void BlendMode_0x00267e041ffcfdf8LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OoT intro, MM Intro (N64 Logo) 
//case 0x00167e6035fcff7eLL:
//aRGB0: (Texel0       - Primitive   ) * Env_Alpha    + Texel0      
//aA0  : (0            - 0           ) * 0            + 0           
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + 1           
void BlendMode_0x00167e6035fcff7eLL( BLEND_MODE_ARGS )
{
	details.InstallTexture = false;
	details.ColourAdjuster.SetRGB (details.EnvColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OOT Sky - Still patchy, due to lack of Second Texture
//case 0x002527ff1ffc9238LL:
//aRGB0: (Texel1       - Texel0      ) * Prim_Alpha   + Texel0      
//aA0  : (Texel1       - Texel0      ) * Primitive    + Texel0      
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + Combined

void BlendMode_0x002527ff1ffc9238LL (BLEND_MODE_ARGS)
{
	if( num_cycles != 1)
	{
		details.PrimColour.ReplicateAlpha();
		details.ColourAdjuster.SetRGB(details.PrimColour);
	}
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGB);
}


// OOT - Eponas dust tracks
//case 0x0030b2045ffefff8LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (Primitive    - 0           ) * Texel0       + 0           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Combined    

void BlendMode_0x0030b2045ffefff8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( c32::White );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	
}

// Zelda OoT logo / flames // Placeholder (Flames do not show up) - Fix me / Simplify
//case 0x00272c60350ce37fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (Texel1       - 1           ) * 1            + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Primitive    + 0           
void BlendMode_0x00272c60350ce37fLL( BLEND_MODE_ARGS )
{
#ifdef CHECK_FIRST_CYCLE
	// XXXX placeholder implementation
	if( num_cycles == 1 )
	{
		// RGB = T0 + x(T1-Prim)
		// A   = T0+T1-1
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);		// XXXX No T1
	}
	else
#endif
	{
		// RGB = Blend(Env, Prim, (T0 + x(t1-Prim)))
		// A   = (T0+T1-1)*Prim
		details.ColourAdjuster.SetRGB( details.EnvColour );
		details.ColourAdjuster.SetA( details.PrimColour  );
		sceGuTexEnvColor( details.PrimColour.GetColour() );
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);		// XXXX No T1
	}
}

// Zelda Paths
//case 0x00121603ff5bfff8LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0           
//aA0  : (Texel0       - 0           ) * Primitive    + 0           
//aRGB1: (Combined     - 0           ) * Primitive    + 0           
//aA1  : (Texel1       - 0           ) * 1            + Combined    
void BlendMode_0x00121603ff5bfff8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.ModulateRGB( details.PrimColour );
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Zelda Fairies // Needs T1 to show attenuitive colors
//case 0x00262a60150c937fLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Primitive    + 0          

void BlendMode_0x00262a60150c937fLL (BLEND_MODE_ARGS)
{
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB(details.EnvColour);
	}
	
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Zelda Rupees
//case 0x0011fffffffffc38LL:
//aRGB0: (Texel0       - 0           ) * Primitive    + 0           
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + Combined    

void BlendMode_0x0011fffffffffc38LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Zelda Water
//case 0x00267e041f0cfdffLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Primitive    + 0    

void BlendMode_0x00267e041f0cfdffLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA(details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Oot - Big Tree in Kokiri Forest and Clouds on Ganondorf's castle
//case 0x00262a041f5893f8LL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Texel1       - 0           ) * 1            + Combined
void BlendMode_0x00262a041f5893f8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGBA( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Zelda Chest Opening Light
// case 0x0020ac60350c937fLL:
//aRGB0: (Texel1       - Primitive   ) * Texel0       + Texel0      
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Primitive    + 0   
void BlendMode_0x0020ac60350c937fLL (BLEND_MODE_ARGS)
{
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
}

// Zelda Kokiri Sword Blade
//	case 0x00177e6035fcfd7eLL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + 1           
void BlendMode_0x00177e6035fcfd7eLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGBA (details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Zelda Kokiri Sword Handle
//	case 0x0030fe045ffefdfeLL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + 1
void BlendMode_0x0030fe045ffefdfeLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGBA (details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Zelda Sign Cut (sword)
//case 0x0030b3ff5ffeda38LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (Primitive    - Env         ) * Texel0       + Env         
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + Combined   
void BlendMode_0x0030b3ff5ffeda38LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.EnvColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	
}

// Zelda Arrows
//case 0x0030ec045fdaedf6LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (1            - 1           ) * 1            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (1            - 1           ) * 1            + 1      
void BlendMode_0x0030ec045fdaedf6LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

// Zelda OOT Deku Nut Core
// case 0x00276c6035d8ed76LL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (1            - 1           ) * 1            + 1           
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (1            - 1           ) * 1            + 1   

void BlendMode_0x00276c6035d8ed76LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

OverrideBlendModeFn		LookupOverrideBlendModeFunction( u64 mux )
{
#ifndef DAEDALUS_PUBLIC_RELEASE
	if(!gGlobalPreferences.CustomBlendModes) return NULL;
#endif
	switch( mux )
	{
#define BLEND_MODE( x )		case (x):	return BlendMode_##x;
			BLEND_MODE(0x0011fffffffffc38LL); // Zelda Rupees
			BLEND_MODE(0x00121603ff5bfff8LL); // Zelda Paths
			BLEND_MODE(0x00127e2433fdf8fcLL); // Wetrix Background / Banjo Kazooie
			BLEND_MODE(0x00159a045ffefff8LL); // GE07 : Dead enemies
			BLEND_MODE(0x00167e6035fcff7eLL); // OOT, MM Intro (N64 Logo)
			BLEND_MODE(0x00177e6035fcfd7eLL); // Zelda Kokori Sword Blade
			BLEND_MODE(0x0020ac60350c937fLL); // Zelda Chest Opening Light
			BLEND_MODE(0x002527ff1ffc9238LL); // OOT Sky
			BLEND_MODE(0x00262a60150c937fLL); // Zelda Fairies
			BLEND_MODE(0x00267e041f0cfdffLL); // Zelda OOT Water
			BLEND_MODE(0x00267e041ffcfdf8LL); // Zelda OOT Grass
			BLEND_MODE(0x00262a041f5893f8LL); // Zelda Deku Tree
			BLEND_MODE(0x00272c60350ce37fLL); // OOT Logo / Flames
			BLEND_MODE(0x00276c6035d8ed76LL); // OOT Deku Nut Core
			BLEND_MODE(0x00277e041ffcf3fcLL); // SSV - Inside Caves
			BLEND_MODE(0x0030b2045ffefff8LL); // OOT - Eponas Dust
			BLEND_MODE(0x0030b2615566db6dLL); // SSB Character Dust
			BLEND_MODE(0x0030b3ff5ffeda38LL); // OOT Sign Cut (Sword)
			BLEND_MODE(0x0030ec045fdaedf6LL); // Zelda Arrows in Shop
			BLEND_MODE(0x0030fe045ffefdfeLL); // Zelda Kokori Sword Handle
			BLEND_MODE(0x00377fff1ffcf438LL); // SSV Power Spheres
			BLEND_MODE(0x00fffffffffcfa7dLL); // Mario 64 Stars
			
	#undef BLEND_MODE 
	}

	return NULL;
}
