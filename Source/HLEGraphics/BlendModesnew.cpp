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
#endif

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
//Aerogauge Water
//case 0x00157fff2ffd7a38LL:
//aRGB0: (Texel0       - Texel1      ) * Prim_Alpha   + Texel1      
//aA0  : (0            - 0           ) * 0            + Env         
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x00157fff2ffd7a38LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

/* 
//#B
*/
// Banjo Kazooie - StrmnNrmn *** (N64 Logo, characters etc)
//case 0x001298043f15ffffLL:
//aRGB0: (Texel0       - Primitive   ) * Env          + Primitive   
//aA0  : (Texel0       - 0           ) * Shade        + 0           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Env          + 0      

void BlendMode_0x001298043f15ffffLL( BLEND_MODE_ARGS )
{

	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB( c32::White );		// Set RGB to 1.0, i.e. select Texture
	}
	else
	{
		// Leave RGB shade untouched
		details.ColourAdjuster.ModulateA( details.EnvColour );
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
	
}

// Banjo Kazooie -- Backdrop // StrmnNrmn
//case 0x0062fe043f15f9ffLL
//aRGB0: (1            - Primitive   ) * Env          + Primitive   
//aA0  : (0            - 0           ) * 0            + Shade       
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Env          + 0           
void BlendMode_0x0062fe043f15f9ffLL( BLEND_MODE_ARGS )
{
	details.InstallTexture = false; 
	c32		blend( details.PrimColour.Interpolate( c32::White, details.EnvColour ) );
	
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB( blend );
	}
	else
	{
		details.ColourAdjuster.ModulateRGB( blend );
		details.ColourAdjuster.ModulateA( details.EnvColour );
	}
}

// Banjo Kazooie - Paths
//case 0x002698041f14ffffLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (Texel0       - 0           ) * Shade        + 0           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Env          + 0          
void BlendMode_0x002698041f14ffffLL( BLEND_MODE_ARGS )
{
	
	//XXXX assume 2 cycles
	details.ColourAdjuster.ModulateA( details.EnvColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
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
//#G
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
//Space Station Silicon Valley - Power Spheres
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

//SSSV - Environments
//	case 0x0026a0041ffc93fcLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Shade 
void BlendMode_0x0026a0041ffc93fcLL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Super Mario 64 - Mario Face
//case 0x00147e2844fe7b3dLL:
//aRGB0: (Texel0       - Shade       ) * Texel0_Alp   + Shade       
//aA0  : (0            - 0           ) * 0            + Env         
//aRGB1: (Texel0       - Shade       ) * Texel0_Alp   + Shade       
//aA1  : (0            - 0           ) * 0            + Env     
void BlendMode_0x00147e2844fe7b3dLL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_DECAL,GU_TCC_RGBA);
}


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
//Wetrix backdrop
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
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
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
	details.ColourAdjuster.SetRGBA(details.PrimColour.ReplicateAlpha());
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
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
	sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	
}

// Zelda OoT logo / flames
//case 0x00272c60350ce37fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (Texel1       - 1           ) * 1            + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Primitive    + 0           
void BlendMode_0x00272c60350ce37fLL( BLEND_MODE_ARGS )
{
	details.ColourAdjuster.SetRGB(details.EnvColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
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
	else {
// Do not touch First cycle
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
	details.ColourAdjuster.SetRGB (details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
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
//		case 0x00276c6035d8ed76LL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (1            - 1           ) * 1            + 1           
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (1            - 1           ) * 1            + 1   

void BlendMode_0x00276c6035d8ed76LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

// OOT - Gold Skullata Badge placeholder
//case 0x00171c6035fd6578LL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel1
//aA0  : (Texel0       - 1           ) * 1            + Texel1
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00171c6035fd6578LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);	
}

// OOT - Gold Skulltula Eyes
//case 0x0030fe045f0ef3ffLL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x0030fe045f0ef3ffLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}


// OOT - Gold Skulltula Chin
//case 0x00177e6035fcfd78LL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + Combined  
void BlendMode_0x00177e6035fcfd78LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	details.ColourAdjuster.SetRGBA( c32::Gold );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

//OOT Queen Spider Fog
//case 0x00262a041f0c93ffLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00262a041f0c93ffLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA(details.EnvColour);
	details.ColourAdjuster.SetRGB(details.EnvColour.ReplicateAlpha());
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Zelda Mega Heart Container Frame
//case 0x00177e60350cf37fLL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (0            - 0           ) * 0            + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Primitive    + 0 

void BlendMode_0x00177e60350cf37fLL (BLEND_MODE_ARGS)
{
details.ColourAdjuster.SetRGB(details.PrimColour);
sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Zelda Mega Heart Container Filling
//case 0x00272c60150c937fLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0      
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Primitive    + 0           

void BlendMode_0x00272c60150c937fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Zelda Boss Portal
//case 0x0026a060150c937fLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Primitive    + 0  
void BlendMode_0x0026a060150c937fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Zelda Goddess Explosion
//		case 0x00271860350cff7fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (Texel0       - 0           ) * Shade        + 0           
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Primitive    + 0

void BlendMode_0x00271860350cff7fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	details.ColourAdjuster.SetA(details.EnvColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
//Zelda OOT Deku Tree Light Flash / Glass Bottle
	//		case 0x0017166035fcff78LL:
	//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel0      
	//aA0  : (Texel0       - 0           ) * Primitive    + 0           
	//aRGB1: (Primitive    - Env         ) * Combined     + Env         
	//aA1  : (0            - 0           ) * 0            + Combined    
	
	void BlendMode_0x0017166035fcff78LL (BLEND_MODE_ARGS)
	{
		details.ColourAdjuster.SetRGB(details.PrimColour.ReplicateAlpha());
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
	}

//Zelda OOT - Hylian Shield Triforce Badge
//	case 0x00176c6035d8ed76LL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (1            - 1           ) * 1            + 1           
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (1            - 1           ) * 1            + 1           
void BlendMode_0x00176c6035d8ed76LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Pokemon Stadium 2 - Pokeball
// ZELDA - OOT - Spiritual Stone Gems
//case 0x00272c60350c937fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00272c60350c937fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.EnvColour );
	sceGuTexFunc(GU_TFX_DECAL,GU_TCC_RGBA);	
}
//Zelda Triforce (Needs PrimLODFrac to be properly coloured and Shiny.
//case 0x00277e6035fcf778LL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (0            - 0           ) * 0            + Primitive   
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00277e6035fcf778LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( c32::Gold );
	details.ColourAdjuster.SetRGBA (details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
	
//Zelda Bottled Water
//case 0x00272c6035fc9378LL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x00272c6035fc9378LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.EnvColour);
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
}


// Zelda Poe
//case 0x00772c60f5fce378LL:
//aRGB0: (CombAlp      - 0           ) * PrimLODFrac  + Texel0      
//aA0  : (Texel1       - 1           ) * 1            + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x00772c60f5fce378LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.EnvColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Zelda Blue Fire Lamp
//	case 0x00272c6035fce378LL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (Texel1       - 1           ) * 1            + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00272c6035fce378LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.EnvColour);
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
}

//Zelda Fairy Spirit
//case 0x00271c6035fcf378LL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (Texel0       - 0           ) * 1            + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00271c6035fcf378LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Zelda Bottle Detail
//case 0x0030fe045ffef3f8LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (0            - 0           ) * 0            + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0030fe045ffef3f8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.EnvColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}


//Zelda Cukuaan Egg
//case 0x0030ec6155daed76LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (1            - 1           ) * 1            + 1           
//aRGB1: (Primitive    - Env         ) * Texel0       + Env         
//aA1  : (1            - 1           ) * 1            + 1           
void BlendMode_0x0030ec6155daed76LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

// Zelda Butterflies
//case 0x00119604ff5bfff8LL:
//aRGB0: (Texel0       - 0           ) * Primitive    + 0           
//aA0  : (Texel0       - 0           ) * Primitive    + 0           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Texel1       - 0           ) * 1            + Combined  
void BlendMode_0x00119604ff5bfff8LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);

//OOT Hyrule Castle Wall Shadow
//	case 0x00267e031ffcfdf8LL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Primitive    + 0           
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x00267e031ffcfdf8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.EnvColour.ReplicateAlpha());		
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//OOT Castle light
//case 0x00272c60340c933fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0      
//aRGB1: (Primitive    - Shade       ) * Combined     + Shade       
//aA1  : (Combined     - 0           ) * Primitive    + 0   
	
void BlendMode_0x00272c60340c933fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OOT - Song of Time
//		case 0x00262a603510937fLL:
//aRGB0: (Texel1       - Primitive   ) * Env_Alpha    + Texel0      
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Shade        + 0       

void BlendMode_0x00262a603510937fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.EnvColour);
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGB);
	
	
	
	
	
	
OverrideBlendModeFn		LookupOverrideBlendModeFunction( u64 mux )
{
#ifndef DAEDALUS_PUBLIC_RELEASE
	if(!gGlobalPreferences.CustomBlendModes) return NULL;
#endif
	switch(mux)
	{
#define BLEND_MODE( x )		case (x):	return BlendMode_##x;
			BLEND_MODE(0x00119604ff5bfff8LL); // Zelda Butterflies
			BLEND_MODE(0x0011fffffffffc38LL); // Zelda Rupees
			BLEND_MODE(0x00121603ff5bfff8LL); // Zelda Paths
			BLEND_MODE(0x00127e2433fdf8fcLL); // Wetrix Background / Banjo Kazooie
			BLEND_MODE(0x001298043f15ffffLL); // Banjo Kazooie N64 Logo / Characters
			BLEND_MODE(0x00157fff2ffd7a38LL); // Aerogauge Water 
			BLEND_MODE(0x00159a045ffefff8LL); // Goldeneye 007 - Dead Enemies
			BLEND_MODE(0x00167e6035fcff7eLL); // OOT, MM Intro (N64 Logo)
			BLEND_MODE(0x0017166035fcff78LL); // OOT Deku tree Flash
			BLEND_MODE(0x00171c6035fd6578LL); // Gold Skulltula Badge Placeholder
			BLEND_MODE(0x00176c6035d8ed76LL); // Zelda Hylian Shield Triforce Badge
			BLEND_MODE(0x00177e60350cf37fLL); // Zelda Heart Container Frame
			BLEND_MODE(0x00177e6035fcfd7eLL); // Zelda Kokori Sword Blade
			BLEND_MODE(0x00177e6035fcfd78LL); // Gold Skulltula Chin
			BLEND_MODE(0x0020ac60350c937fLL); // Zelda Chest Opening Light
			BLEND_MODE(0x002527ff1ffc9238LL); // OOT Sky
			BLEND_MODE(0x00262a041f0c93ffLL); // OOT Fog in Deku Tree
			BLEND_MODE(0x00262a603510937fLL); // OOT - Song of Time
			BLEND_MODE(0x0026a060150c937fLL); // Zelda Boss Portal
			BLEND_MODE(0x0026a0041ffc93fcLL); // SSSV Environments
			BLEND_MODE(0x00267e031ffcfdf8LL); // OOT Hyrule Castle Shadows
			BLEND_MODE(0x00262a041f5893f8LL); // Zelda Deku Tree
			BLEND_MODE(0x00262a60150c937fLL); // Zelda Fairies
			BLEND_MODE(0x00267e041f0cfdffLL); // Zelda OOT Water
			BLEND_MODE(0x00267e041ffcfdf8LL); // Zelda OOT Grass
			BLEND_MODE(0x002698041f14ffffLL); // Banjo Kazooie Paths
			BLEND_MODE(0x00271860350cff7fLL); // Deku Tree Light
			BLEND_MODE(0x00271c6035fcf378LL); // Zelda Fairy Spirit.
			BLEND_MODE(0x00272c60340c933fLL); // Zelda Castle Light
			BLEND_MODE(0x00272c60150c937fLL); // Zelda Heart Container
			BLEND_MODE(0x00272c60350c937fLL); // OOT Spiritual Stones / Pokeball
			BLEND_MODE(0x00272c60350ce37fLL); // OOT Logo / Flames
			BLEND_MODE(0x00272c6035fc9378LL); // Zelda Bottled Water
			BLEND_MODE(0x00272c6035fce378LL); // Zelda Blue Fire Lamp
			BLEND_MODE(0x00276c6035d8ed76LL); // OOT Deku Nut Core
			BLEND_MODE(0x00277e041ffcf3fcLL); // SSSV - Caves inside waterfalls
			BLEND_MODE(0x00277e6035fcf778LL); // Zelda Triforce
			BLEND_MODE(0x0030b2045ffefff8LL); // OOT - Eponas Dust
			BLEND_MODE(0x0030b2615566db6dLL); // SSB Character Dust
			BLEND_MODE(0x0030b3ff5ffeda38LL); // OOT Sign Cut (Sword)
			BLEND_MODE(0x0030ec6155daed76LL); // Cucukan Egg
			BLEND_MODE(0x0030ec045fdaedf6LL); // Zelda Arrows in Shop
			BLEND_MODE(0x0030fe045f0ef3ffLL); // Gold Skulltula Eyes
			BLEND_MODE(0x0030fe045ffef3f8LL); // Zelda Bottle Decal
			BLEND_MODE(0x0030fe045ffefdfeLL); // Zelda Kokori Sword Handle
			BLEND_MODE(0x00377fff1ffcf438LL); // Space Station Silicon Valley - Power Spheres
			BLEND_MODE(0x0062fe043f15f9ffLL); // Banjo Kazooie Backdrop
			BLEND_MODE(0x00772c60f5fce378LL); // Zelda Poe
			BLEND_MODE(0x00fffffffffcfa7dLL); // Mario 64 Stars
			
	#undef BLEND_MODE 
	}

	return NULL;
}
