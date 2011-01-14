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

#endif
/* To Devs,
 Once blendmodes are complete please clean up after yourself before commiting.
 
* Blending Options
 details.ColourAdjuster.SetRGB();
 details.ColourAdjuster.SetA();
 details.ColourAdjuster.SetRGBA();
 details.ColourAdjuster.ModulateA();
 
**** These things go into above brackets
 
* Primitive
 details.PrimColour
 details.PrimColour.ReplicateAlpha()
 
* Environment
 details.ColourAdjuster.SetA( details.EnvColour );
 
* Environment Color in SDK 
 sceGuTexEnvColor( details.xxxColour.GetColour() ); - xxx = Env or Prim
 
 -- Closure of blend
 sceGuTexFunc(xx,yy); - This is used in the texture function when a constant color is needed.
 
* xx 
 GU_TFX_MODULATE - The texture is multiplied with the current diffuse fragment
 GU_TFX_REPLACE - The texture replaces the fragment
 GU_TFX_ADD - The texture is added on-top of the diffuse fragment
 GU_TFX_BLEND - 
 GU_TFX_DECAL - 
 
* yy
 The fields TCC_RGB and TCC_RGBA specify components that differ between the two different component modes.

 Component-modes only !!! (TCC)

 * GU_TCC_RGB - The texture alpha does not have any effect
 * GU_TCC_RGBA - The texture alpha is taken into account

 XXXXXXX
 tfx 	- Which apply-mode to use
 tcc 	- Which component-mode to use 

 TIPS:
 
 If Ghosting Occurs, use RGB instead of RGBA or remove Opaque.
 
 
 ** Textures are installed by default. To disable use : details.InstallTexture = false;
 Read the Cycle guide carefully
 */



// Start Blends

//*****************************************************************************
// Basic generic blendmode
//*****************************************************************************
// This should handle most inexact blends :)
//
inline void BlendMode_Generic( BLEND_MODE_ARGS ){	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);	}



/* 
//##
*/

// 1080 Snowboarding Sky

//case 0x00357e6a11fcf67bLL:
//aRGB0: (Primitive    - Texel0      ) * Prim_Alpha   + Texel0      
//aA0  : (0            - 0           ) * 0            + Primitive   
//aRGB1: (Primitive    - Texel0      ) * Prim_Alpha   + Texel0      
//aA1  : (0            - 0           ) * 0            + Primitive  
void BlendMode_0x00357e6a11fcf67bLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour.ReplicateAlpha() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA); 
}

// 007 - Floor, Turok Enviroments, Silicon Valley Enviroments, and Pilot WIngs' Castle.
//case 0x0026a0041ffc93fcLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Shade       

void BlendMode_0x0026a0041ffc93fcLL( BLEND_MODE_ARGS )
{
	// XXXX need 2nd texture
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	}
	else
#endif
	{
		details.ColourAdjuster.SetAOpaque();	// XXXX - hack Alpha 1.0 (should be coming from fog??)
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
	}
}

// 007 - Mountains
//case 0x0026e4041ffcfffcLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (1            - 0           ) * Texel1       + 0           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Shade
void BlendMode_0x0026e4041ffcfffcLL( BLEND_MODE_ARGS )
{
	// XXXX need 2nd texture
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetAOpaque();	// Alpha 1.0		XXXX Need to modulate with t1 alpha
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	}
	else
#endif
	{
		details.ColourAdjuster.SetAOpaque();	// Alpha 1.0
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
	}
}

// 007
//case 0x0026a0041f1093fbLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Shade        + Primitive   
void BlendMode_0x0026a0041f1093fbLL( BLEND_MODE_ARGS )
{
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB(details.PrimColour);
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	}
	else
#endif
	{
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}

}
// AFA/007/- Gun/Bits and Paper Mario/Pilot Wings/Beetle Adventure Racing 
//case 0x0026a0041f1093ffLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Shade        + 0           
void BlendMode_0x0026a0041f1093ffLL( BLEND_MODE_ARGS )
{
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetAOpaque();
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	}
	else
#endif
	{
	    details.ColourAdjuster.SetAOpaque();
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}

}
// 007 watch guns
//case 0x0026ea041f10ffffLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (1            - 0           ) * Env          + 0           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Shade        + 0 

void BlendMode_0x0026ea041f10ffffLL( BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetAOpaque();
		sceGuTexEnvColor( details.EnvColour.GetColour() );
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGB);
	}	
}

// 007 bridges and windows
//case 0x0026a0041f1493ffLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Env          + 0           
void BlendMode_0x0026a0041f1493ffLL( BLEND_MODE_ARGS )
{
	// XXXX need 2nd texture
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	}
	else
#endif
	{
		details.ColourAdjuster.SetA( details.EnvColour );
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
}

// 007
//case 0x001690031f0c93ffLL:
//aRGB0: (Texel0       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (Texel0       - Texel0      ) * Combined     + Texel0      
//aRGB1: (Combined     - 0           ) * Primitive    + 0           
//aA1  : (Combined     - 0           ) * Primitive    + 0           
void BlendMode_0x001690031f0c93ffLL( BLEND_MODE_ARGS )
{
	if( num_cycles != 1 )
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
	}
	// Use the primitive for the r,g,b,a
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
 

// 007
//case 0x001598045ffedbf8LL:
//aRGB0: (Texel0       - Env         ) * Shade_Alpha  + Env         
//aA0  : (Texel0       - Env         ) * Shade        + Env         
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x001598045ffedbf8LL( BLEND_MODE_ARGS )
{
	// XXXX incorrect, need env and prim, blend has wrong source
	// Use the primitive for the r,g,b, override the alpha with 1.0
	details.ColourAdjuster.SetRGB( c32::White );		// XXXX Really want to replicate
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

// 007 menu backdrop
//case 0x00167e2c33fdf6fbLL:
//aRGB0: (Texel0       - Primitive   ) * Env_Alpha    + Primitive   
//aA0  : (0            - 0           ) * 0            + Primitive   
//aRGB1: (Texel0       - Primitive   ) * Env_Alpha    + Primitive   
//aA1  : (0            - 0           ) * 0            + Primitive   
void BlendMode_0x00167e2c33fdf6fbLL( BLEND_MODE_ARGS )
{
	// XXXX Need to blend( Prim, T0, Enva )
	details.ColourAdjuster.SetRGB(details.EnvColour.ReplicateAlpha());
	details.ColourAdjuster.SetA( details.PrimColour);
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGB);
}


/*
//#A
*/

// AeroGauge - Water, Waterfalls, Lava, and other stuff under Stages.
//case 0x00157fff2ffd7a38LL:
//aRGB0: (Texel0       - Texel1      ) * Prim_Alpha   + Texel1
//aA0  : (0            - 0           ) * 0            + Env
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined 
void BlendMode_0x00157fff2ffd7a38LL( BLEND_MODE_ARGS )
{
	details.ColourAdjuster.SetA( details.EnvColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}


/* 
//#B
*/

//Beetle Adventure Racing - Globes, Trees, Clouds, and Rocks.
//case 0x00117e04fffffffaLL:
//aRGB0: (Texel0       - 0           ) * Texel1       + 0
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Texel1
void BlendMode_0x00117e04fffffffaLL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);	
}
//Banjo Tooie - Ground
//case 0x00ffe7ffffcf9fcfLL:
//aRGB0: (0            - 0           ) * 0            + 0           
//aA0  : (1            - Texel0      ) * Primitive    + 0           
//aRGB1: (0            - 0           ) * 0            + 0           
//aA1  : (1            - Texel0      ) * Primitive    + 0   
void BlendMode_0x00ffe7ffffcf9fcfLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);	
}

//Blitz and All NFL Games - Fields and outside of fileds
//case 0x0026a1ff1ffc9238LL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0026a1ff1ffc9238LL (BLEND_MODE_ARGS)
{
	//Textures look corrupt (lines)
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGB);
}
//Blitz and All NFL Games - End Fields
//case 0x00277fff1ffcf438LL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + Texel1
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00277fff1ffcf438LL (BLEND_MODE_ARGS)
{
	// Alpha causes ghosting =/
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
	}
	else
#endif
	{
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
	}
}
// Banjo Kazooie - StrmnNrmn
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


// Banjo Kazooie - StrmnNrmn
// XXXX need to come up with something more cunning for this :(
//case 0x00127e2433fdf8fcLL:
//aRGB0: (Texel0       - Primitive   ) * Shade        + Primitive   
//aA0  : (0            - 0           ) * 0            + Shade       
//aRGB1: (Texel0       - Primitive   ) * Shade        + Primitive   
//aA1  : (0            - 0           ) * 0            + Shade       
void BlendMode_0x00127e2433fdf8fcLL( BLEND_MODE_ARGS )
{
	// Just select texture RGB and shade A for now
	
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}

// Banjo Kazooie - StrmnNrmn *** (N64 Logo, characters etc)
//case 0x001298043f15ffffLL:
//aRGB0: (Texel0       - Primitive   ) * Env          + Primitive   
//aA0  : (Texel0       - 0           ) * Shade        + 0           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Env          + 0      

void BlendMode_0x001298043f15ffffLL( BLEND_MODE_ARGS )
{

	// //New Edit
	// RGB = Blend( Prim, T0, Env ) * Shade
	// A   = T0 * Shade * Env
	
	// Assume this is only ever used in 2 cycle
	
	// Hoplessly wrong - do Prim * Shade * T0, Env * Shade * T0
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB( c32::White );		// Set RGB to 1.0, i.e. select Texture
	}
	else
#endif
	{
		// Leave RGB shade untouched
		details.ColourAdjuster.ModulateA( details.EnvColour );
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
	
}

//Batman Beyond - Enemies die animation and explosions
//case 0x00377e041ffcf7f8LL:
//aRGB0: (Primitive    - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + Primitive
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00377e041ffcf7f8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Batman Beyond- Batman Itself
//case 0x00377e041ffcfdf8LL:
//aRGB0: (Primitive    - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Combined

void BlendMode_0x00377e041ffcfdf8LL (BLEND_MODE_ARGS)
{
	sceGuTexEnvColor( details.PrimColour.GetColour() );
}

//Batman Beyond - Enemies, Foes and boxes.
//case 0x00377e041ffcf3f8LL:
//aRGB0: (Primitive    - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Combined

void BlendMode_0x00377e041ffcf3f8LL (BLEND_MODE_ARGS)
{
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//ZELDA - OOT - Heart Container (outer)
//case 0x00177e60350cf37fLL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00177e60350cf37fLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if(num_cycles == 1)
	{
		//details.RecolourTextureWhite;
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

//Batman Beyond- Misc / Smog
//ZELDA - OOT - Heart Container (Inner)
//XXXX
//case 0x00272c60150c937fLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00272c60150c937fLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if(num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

// Banjo Kazooie -- Backdrop // StrmnNrmn
//case 0x0062fe043f15f9ffLL:
//aRGB0: (1            - Primitive   ) * Env          + Primitive   
//aA0  : (0            - 0           ) * 0            + Shade       
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Env          + 0           
void BlendMode_0x0062fe043f15f9ffLL( BLEND_MODE_ARGS )
{
	details.InstallTexture = false; 
	c32		blend( details.PrimColour.Interpolate( c32::White, details.EnvColour ) );

#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB( blend );
	}
	else
#endif
	{
		details.ColourAdjuster.ModulateRGB( blend );
		details.ColourAdjuster.ModulateA( details.EnvColour );
	}
}


/*
//#C
*/
// Conker Table
//case 0x001218245531feffLL:
//aRGB0: (Texel0       - Env         ) * Shade        + Primitive   
//aA0  : (Texel0       - 0           ) * Shade        + 0           
//aRGB1: (Texel0       - Env         ) * Shade        + Primitive   
//aA1  : (Texel0       - 0           ) * Shade        + 0           
void BlendMode_0x001218245531feffLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB (details.PrimColour);
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

//Conker - Window
//case 0x00127e2455fdf2f9LL:
//aRGB0: (Texel0 - Env ) * Shade + Primitive
//aA0 : (0 - 0 ) * 0 + Texel0
//aRGB1: (Texel0 - Env ) * Shade + Primitive
//aA1 : (0 - 0 ) * 0 + Texel0 
void BlendMode_0x00127e2455fdf2f9LL( BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

// Conker Ground
//case 0x0026a004151092ffLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0      
//aRGB1: (Combined     - Env         ) * Shade        + Primitive   
//aA1  : (Combined     - 0           ) * Shade        + 0    
void BlendMode_0x0026a004151092ffLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour);
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

//Command $ Conquer - Everything
//case 0x0015982bff327f3fLL:
//aRGB0: (Texel0       - 0           ) * Shade_Alpha  + Shade
//aA0  : (Texel0       - 0           ) * Shade        + 0
//aRGB1: (Texel0       - 0           ) * Shade_Alpha  + Shade
//aA1  : (Texel0       - 0           ) * Shade        + 0
void BlendMode_0x0015982bff327f3fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);	
}
//Command & Conquer - Water
//case 0x001147fffffffe38LL:
//aRGB0: (Texel0       - 0           ) * Texel1       + 0
//aA0  : (Shade        - 0           ) * Primitive    + 0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x001147fffffffe38LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);	
}
//Command $ Conquer - Shades
//case 0x0015fe2bfffff3f9LL:
//aRGB0: (Texel0       - 0           ) * Shade_Alpha  + 0
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Texel0       - 0           ) * Shade_Alpha  + 0
//aA1  : (0            - 0           ) * 0            + Texel0

void BlendMode_0x0015fe2bfffff3f9LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);	
}

//Castlevania - Intro
//case 0x0061fec311fcf67bLL:
//aRGB0: (1            - Texel0      ) * Primitive    + Texel0      
//aA0  : (0            - 0           ) * 0            + Primitive   
//aRGB1: (1            - Texel0      ) * Primitive    + Texel0      
//aA1  : (0            - 0           ) * 0            + Primitive         
void BlendMode_0x0061fec311fcf67bLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetA( details.PrimColour  );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
// Castlevania
//case 0x00ffd5fffffcf238LL:
//aRGB0: (0            - 0           ) * 0            + Texel0      
//aA0  : (Env          - 0           ) * Texel1       + Texel0      
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + Combined 
void BlendMode_0x00ffd5fffffcf238LL( BLEND_MODE_ARGS )
{
	// XXXX needs Env*T1 for alpha
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

/*
//#D
*/

//Doom Ceiling and detail and sky
//case 0x00177e2efffefd7eLL:
//aRGB0: (Texel0       - 0           ) * PrimLODFrac  + Env         
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Texel0       - 0           ) * PrimLODFrac  + Env         
//aA1  : (0            - 0           ) * 0            + 1       
void BlendMode_0x00177e2efffefd7eLL (BLEND_MODE_ARGS)
{
	
	if( num_cycles != 1 )
	{
		details.ColourAdjuster.SetAOpaque();
	}
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
//Doom Weapons
//case 0x00671603fffcff78LL:
//aRGB0: (1            - 0           ) * PrimLODFrac  + Texel0      
//aA0  : (Texel0       - 0           ) * Primitive    + 0           
//aRGB1: (Combined     - 0           ) * Primitive    + Env         
//aA1  : (0            - 0           ) * 0            + Combined   
void BlendMode_0x00671603fffcff78LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	}
	else
#endif
	{
		details.ColourAdjuster.SetA( details.PrimColour );
		sceGuTexEnvColor( details.EnvColour.GetColour() );
	}
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Doom Level
//case 0x00671604fffcff78LL:
//aRGB0: (1            - 0           ) * PrimLODFrac  + Texel0      
//aA0  : (Texel0       - 0           ) * Primitive    + 0           
//aRGB1: (Combined     - 0           ) * Shade        + Env         
//aA1  : (0            - 0           ) * 0            + Combined 

void BlendMode_0x00671604fffcff78LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);

	//details.ColourAdjuster.SetRGB( details.EnvColour );
	//details.ColourAdjuster.SetA( details.PrimColour );
	//sceGuTexEnvColor( details.PrimColour.GetColour() );
	//sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

// Duke 3D menu text and Mario 64 : Mario's 3D head
//case 0x0030b26144664924LL:
//aRGB0: (Primitive    - Shade       ) * Texel0       + Shade       
//aA0  : (Primitive    - Shade       ) * Texel0       + Shade       
//aRGB1: (Primitive    - Shade       ) * Texel0       + Shade       
//aA1  : (Primitive    - Shade       ) * Texel0       + Shade       

void BlendMode_0x0030b26144664924LL( BLEND_MODE_ARGS )
{
	//This blend only partially fixes Duke 32
	//Complete fix interferes with Mario head
	//Combiner needs debugging
	// Need to modulate the texture*shade for RGBA for Duke
	//
	// This makes Mario's 3D head shiny as supposed to be.
	
	details.ColourAdjuster.SetA( details.EnvColour );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
}

// Duke 3D Gun and other thing.
//case 0x0050fea144fe7339LL:
//aRGB0: (Env          - Shade       ) * Texel0       + Shade       
//aA0  : (0            - 0           ) * 0            + Texel0      
//aRGB1: (Env          - Shade       ) * Texel0       + Shade       
//aA1  : (0            - 0           ) * 0            + Texel0
void BlendMode_0x0050fea144fe7339LL (BLEND_MODE_ARGS)
{
	//details.ColourAdjuster.SetRGB (details.EnvColour);
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}


/*
//#E
*/ 

//Extreme XG2  - Menu triangles
//case 0x00157e2a33fdfcfeLL:
//aRGB0: (Texel0       - Primitive   ) * Prim_Alpha   + Primitive   
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Texel0       - Primitive   ) * Prim_Alpha   + Primitive   
//aA1  : (0            - 0           ) * 0            + 1      
void BlendMode_0x00157e2a33fdfcfeLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour.ReplicateAlpha());
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

//ExtremeXG2 - Inside the inner ring
  //case 0x00327ecbf0fffc3eLL:
  //aRGB0: (Primitive    - 0           ) * Shade        + 0           
  //aA0  : (0            - 0           ) * 0            + 1           
  //aRGB1: (1            - Combined    ) * Shade_Alpha  + Combined    
  //aA1  : (0            - 0           ) * 0            + 1     
void BlendMode_0x00327ecbf0fffc3eLL (BLEND_MODE_ARGS)
{	
	details.ColourAdjuster.SetRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_DECAL,GU_TCC_RGB);
}
								  							  
//Extreme XG2 - inner ring
//case 0x00127eccf0fffc38LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0           
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (1            - Combined    ) * Env_Alpha    + Combined    
//aA1  : (0            - 0           ) * 0            + Combined 
void BlendMode_0x00127eccf0fffc38LL (BLEND_MODE_ARGS){} // WTH ???? We just need to install texture?

//Extreme GX2- Intro ring
//case 0x00127ecbf0fffc3eLL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0           
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (1            - Combined    ) * Shade_Alpha  + Combined    
//aA1  : (0            - 0           ) * 0            + 1           
void BlendMode_0x00127ecbf0fffc3eLL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

//Extreme G - Transition Screen Intro
//case 0x0015fe2b33fdf6fbLL:
//aRGB0: (Texel0       - Primitive   ) * Shade_Alpha  + Primitive
//aA0  : (0            - 0           ) * 0            + Primitive
//aRGB1: (Texel0       - Primitive   ) * Shade_Alpha  + Primitive
//aA1  : (0            - 0           ) * 0            + Primitive 
void BlendMode_0x0015fe2b33fdf6fbLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
}

/* 
//#F
*/
// F1 GP Road
//case 0x00157e80fffdfd7eLL:
//aRGB0: (Texel0       - 0           ) * Prim_Alpha   + Primitive   
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Shade        - 0           ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + 1           
void BlendMode_0x00157e80fffdfd7eLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB(details.PrimColour.ReplicateAlpha());
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}

// 
// F1 World GP Qantas Signs / Back of Car / Other Billboard
//case 0x00347e04fffcfdfeLL:
//aRGB0: (Primitive    - 0           ) * Texel0_Alp   + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + 1   
void BlendMode_0x00347e04fffcfdfeLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.ModulateRGB(details.PrimColour);
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGB);
}

// F1 World GP Billboard 1 (Melbourne)
//case 0x0061fe041ffcfdfeLL:
//aRGB0: (1            - Texel0      ) * Primitive    + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + 1           
void BlendMode_0x0061fe041ffcfdfeLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB(details.PrimColour);
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}


//F1 World GP Starting Grid
//case 0x00257e04fffcfd7eLL:
//aRGB0: (Texel1       - 0           ) * Prim_Alpha   + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + Env         
//aA1  : (0            - 0           ) * 0            + 1
void BlendMode_0x00257e04fffcfd7eLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE	
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB(details.PrimColour.ReplicateAlpha());
		details.ColourAdjuster.SetAOpaque();
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGB(details.EnvColour);
		details.ColourAdjuster.SetAOpaque();
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
	}
}
	
// F1 World GP Sky
//case 0x0055a68730fd923eLL:
//aRGB0: (Env          - Primitive   ) * Shade_Alpha  + Primitive   
//aA0  : (Texel1       - Texel0      ) * Primitive    + Texel0      
//aRGB1: (Shade        - Combined    ) * CombAlp      + Combined    
//aA1  : (0            - 0           ) * 0            + 1    
void BlendMode_0x0055a68730fd923eLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//F1 World GP Vehicle Decal and Helmet
//case 0x0010fe043ffdfdfeLL:
//aRGB0: (Texel0       - Primitive   ) * Texel0       + Primitive   
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + 1  
void BlendMode_0x0010fe043ffdfdfeLL(BLEND_MODE_ARGS)
{
	details.ColourAdjuster.ModulateRGB(details.PrimColour);
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGB);
	
}

// F1 Gran Prix - Inner Wheel Rims
//case 0x0027fe041ffcfdfeLL:
//aRGB0: (Texel1       - Texel0      ) * K5           + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + 1   
void BlendMode_0x0027fe041ffcfdfeLL(BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);	
}

// F1 Gran Prix - Outer Wheel Rims
//case 0x00257e041ffcfdfeLL:
//aRGB0: (Texel1       - Texel0      ) * Prim_Alpha   + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + 1     
void BlendMode_0x00257e041ffcfdfeLL(BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}
// Fighting Force - Ground
//case 0x0026a0031ffc93f9LL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (0            - 0           ) * 0            + Texel0  
void BlendMode_0x0026a0031ffc93f9LL( BLEND_MODE_ARGS )
{
	// XXXX need 2nd texture
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
	}
}

//F-Zero X - Texture Cars second layer <- Needs more work.
//case 0x0030fe045ffefbf8LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env
//aA0  : (0            - 0           ) * 0            + Env
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Combined 
void BlendMode_0x0030fe045ffefbf8LL (BLEND_MODE_ARGS)
{
	if( num_cycles == 2 )
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGB);
	}
}

// F-Zero X - Texture for cars.
//case 0x00147e045ffefbf8LL:
//aRGB0: (Texel0       - Env         ) * Texel0_Alp   + Env         
//aA0  : (0            - 0           ) * 0            + Env         
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Combined 

void BlendMode_0x00147e045ffefbf8LL( BLEND_MODE_ARGS )
{
	// RGB = Blend(E,T0,T0a)*Shade	~= Blend(SE, ST0, T0a)
	// A   = Env
	
	// XXXX needs to modulate again by shade
	
	details.ColourAdjuster.SetRGBA( details.EnvColour );
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_DECAL,GU_TCC_RGBA);		
}

//F-Zero X - Sky
//case 0x0030f861fff393c9LL:
//aRGB0: (Primitive    - 0           ) * Texel0       + 0
//aA0  : (0            - Texel0      ) * Shade        + Texel0
//aRGB1: (Primitive    - 0           ) * Texel0       + 0
//aA1  : (0            - Texel0      ) * Shade        + Texel0  
void BlendMode_0x0030f861fff393c9LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// F-Zero X - Textures for stages to Blend ( stills glitchy :/)
//case 0x00327e6411fcf87cLL:
//aRGB0: (Primitive    - Texel0      ) * Shade        + Texel0
//aA0  : (0            - 0           ) * 0            + Shade
//aRGB1: (Primitive    - Texel0      ) * Shade        + Texel0
//aA1  : (0            - 0           ) * 0            + Shade
void BlendMode_0x00327e6411fcf87cLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGBA( details.PrimColour );
	//details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.PrimColour.GetColour() );
}

/*
#G
*/

//GoldenEye 007 - Sky
//case 0x0040fe8155fef97cLL:
//aRGB0: (Shade        - Env         ) * Texel0       + Env
//aA0  : (0            - 0           ) * 0            + Shade
//aRGB1: (Shade        - Env         ) * Texel0       + Env
//aA1  : (0            - 0           ) * 0            + Shade
void BlendMode_0x0040fe8155fef97cLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetAOpaque();
	details.ColourAdjuster.SetRGB( details.EnvColour );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGB);
}

/*
  #I
*/ 

//ISS 64 - Ground
// Tested okay by Wally 1/06/2010
//case 0x0012680322fd7eb8LL:
//aRGB0: (Texel0       - Texel1      ) * Shade        + Texel1
//aA0  : (1            - 0           ) * Shade        + 0
//aRGB1: (Combined     - Texel1      ) * Primitive    + Texel1
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0012680322fd7eb8LL (BLEND_MODE_ARGS)
{
	// We neeeded XXX blender, now ground looks perfect :)
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);	
}

//ISS 2000 - Ground
// Tested okay by Wally 1/06/2010
//case 0x0011fe052ffd73f8LL:
//aRGB0: (Texel0       - Texel1      ) * Primitive    + Texel1
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Combined     - 0           ) * Env          + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0011fe052ffd73f8LL (BLEND_MODE_ARGS)
{
	// XXXX - needs t1, but overall blend looks perfect :)
	
	// RGB = Blend( T1, T0, Prim )
	// A   = 1
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}
/*
  #K
*/ 

// Killer Instinct - Background and Ground
//case 0x00127e2455fdf8fcLL:
//aRGB0: (Texel0       - Env         ) * Shade        + Primitive
//aA0  : (0            - 0           ) * 0            + Shade
//aRGB1: (Texel0       - Env         ) * Shade        + Primitive
//aA1  : (0            - 0           ) * 0            + Shade
void BlendMode_0x00127e2455fdf8fcLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);	
}

//Kirby 64 - Ground
//case 0x0030fe045ffefdf8LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0030fe045ffefdf8LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
	// This looks like the real deal but doesn't match with the terrain blendmode :(
	/*details.ColourAdjuster.SetRGBA( details.EnvColour );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);	*/
}

//Kirby 64 - some parts of the Ground
//case 0x00309e045ffefdf8LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (Texel0       - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x00309e045ffefdf8LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
	// This looks like the real deal but doesn't match with the terrain blendmode :(
	/*details.ColourAdjuster.SetRGBA( details.EnvColour );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);	*/
}

//Kirby 64 - Flowers
//case 0x0040fe8155fef379LL:
//aRGB0: (Shade        - Env         ) * Texel0       + Env
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Shade        - Env         ) * Texel0       + Env
//aA1  : (0            - 0           ) * 0            + Texel0
void BlendMode_0x0040fe8155fef379LL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
//Kirby 64 - Far Terrain
//case 0x0040fe8155fefd7eLL:
//aRGB0: (Shade        - Env         ) * Texel0       + Env
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Shade        - Env         ) * Texel0       + Env
//aA1  : (0            - 0           ) * 0            + 1

void BlendMode_0x0040fe8155fefd7eLL (BLEND_MODE_ARGS)
{
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

/*
 //#M
*/ 
//Mario Party - River in Mini Game
//case 0x001277ffffff9238LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (0            - Texel0      ) * Primitive    + Texel0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x001277ffffff9238LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetA( details.PrimColour);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

// Monster Truck Madness - Sky
//case 0x00127e8bf0fffc3eLL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Shade        - Combined    ) * Shade_Alpha  + Combined
//aA1  : (0            - 0           ) * 0            + 1

 void BlendMode_0x00127e8bf0fffc3eLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( c32::White );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

// Monster Truck - Text
//case 0x003135ff5f0ada3fLL:
//aRGB0: (Primitive    - Env         ) * Texel1       + Env
//aA0  : (Primitive    - Env         ) * Texel1       + Env
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (Combined     - 0           ) * Texel1       + 0
void BlendMode_0x003135ff5f0ada3fLL (BLEND_MODE_ARGS)
{
	
	// XXXX needs t1 in alpha..*sigh*
	details.ColourAdjuster.SetRGB ( details.PrimColour );
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
// M64 eyes decal.
//case 0x00147e2844fe7b3dLL:
//aRGB0: (Texel0       - Shade       ) * Texel0_Alp   + Shade       
//aA0  : (0            - 0           ) * 0            + Env         
//aRGB1: (Texel0       - Shade       ) * Texel0_Alp   + Shade       
//aA1  : (0            - 0           ) * 0            + Env         
void BlendMode_0x00147e2844fe7b3dLL( BLEND_MODE_ARGS )
{
	// RGB = Blend( Shade, T0, T0Alpha )  -- essentially DECAL mode on PSP
	// A   = Env
	
	details.ColourAdjuster.SetA( details.EnvColour );
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_DECAL,GU_TCC_RGBA);		
}

// Mario 64 Penguin / Owl / Canon dude Eyes / Face / F- Zero Tracks
//case 0x00147e2844fe793cLL:
//aRGB0: (Texel0       - Shade       ) * Texel0_Alp   + Shade       
//aA0  : (0            - 0           ) * 0            + Shade       
//aRGB1: (Texel0       - Shade       ) * Texel0_Alp   + Shade       
//aA1  : (0            - 0           ) * 0            + Shade
void BlendMode_0x00147e2844fe793cLL( BLEND_MODE_ARGS )
{
	if( num_cycles == 1 )
	{
		sceGuTexFunc(GU_TFX_DECAL,GU_TCC_RGBA);
	}
	else
	{
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
	
}

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
// Mario Golf Trees
//case 0x005632801ffcfff8LL:
//aRGB0: (Env          - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (Primitive    - 0           ) * Texel0       + 0           
//aRGB1: (Shade        - 0           ) * Combined     + 0           
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x005632801ffcfff8LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.ModulateA( details.PrimColour );
	details.ColourAdjuster.SetRGBA( details.EnvColour);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

// Mario Golf Ground XXXX
//case 0x00115407f1ffca7eLL:
//aRGB0: (Texel0       - 0           ) * Texel1       + 0           
//aA0  : (Env          - Shade       ) * Texel1       + Env         
//aRGB1: (Combined     - Texel0      ) * CombAlp      + Texel0      
//aA1  : (0            - 0           ) * 0            + 1           
void BlendMode_0x00115407f1ffca7eLL (BLEND_MODE_ARGS)
{
	
	// RGB = T0 * T1 + T0 * CombAlp
	// A = Env * Shade + 1
	
	// Placeholder Texture for ground
	details.ColourAdjuster.SetRGB(details.EnvColour);
	details.ColourAdjuster.SetAOpaque(); // CombAlph and 1
	
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}


// Mario Golf Flag
//case 0x00157e602ffd77f8LL
//aRGB0: (Texel0       - Texel1      ) * Prim_Alpha   + Texel1      
//aA0  : (0            - 0           ) * 0            + Primitive   
//aRGB1: (Primitive    - 0           ) * Combined     + 0           
//aA1  : (0            - 0           ) * 0            + Combined   
void BlendMode_0x00157e602ffd77f8LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGBA( details.PrimColour.ReplicateAlpha() );
	
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
//Mortal Kombat 4 - Tittle Screen/Character Screen
//case 0x0011fe2355fefd7eLL:
//aRGB0: (Texel0       - Env         ) * Primitive    + Env
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Texel0       - Env         ) * Primitive    + Env
//aA1  : (0            - 0           ) * 0            + 1
void BlendMode_0x0011fe2355fefd7eLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour  );
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}

//Mortal Kombat 4 - Text              
//case 0x0011fe2344fe7339LL:
//aRGB0: (Texel0       - Shade       ) * Primitive    + Shade
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Texel0       - Shade       ) * Primitive    + Shade
//aA1  : (0            - 0           ) * 0            + Texel0
void BlendMode_0x0011fe2344fe7339LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour  );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
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

//PGA - Ground	
//case 0x00272c0415fc93feLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Combined     - Env         ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + 1
void BlendMode_0x00272c0415fc93feLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}

//PGA - Water
//case 0x001114a3f0fff638LL:
//aRGB0: (Texel0       - 0           ) * Texel1       + 0
//aA0  : (Texel0       - 0           ) * Texel1       + Primitive
//aRGB1: (Env          - Combined    ) * Primitive    + Combined
//aA1  : (0      
void BlendMode_0x001114a3f0fff638LL (BLEND_MODE_ARGS)
{
	// XXXX needs t1 in alpha anyways
	//details.ColourAdjuster.SetRGB ( details.PrimColour );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

//Pilot Wing - Beaches and Beetle Adventure Racing - Ground
//case 0x0015fe042ffd79fcLL:
//aRGB0: (Texel0       - Texel1      ) * Shade_Alpha  + Texel1
//aA0  : (0            - 0           ) * 0            + Shade
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Shade
void BlendMode_0x0015fe042ffd79fcLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);

}
//Paper Mario Lava Room - Mario & His Partner
//case 0x00117e80f5fff438LL:
//aRGB0: (Texel0       - 0           ) * Texel1       + 0           
//aA0  : (0            - 0           ) * 0            + Texel1      
//aRGB1: (Shade        - Env         ) * Combined     + Combined    
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x00117e80f5fff438LL (BLEND_MODE_ARGS)
{
	//Needs T1 for full fix!!!!!!! 
	//Makes Mario & his partner appear as black boxes.( This game has this same problem everywhere.)
	//Seems like a core issue to me -Salvy

	details.ColourAdjuster.SetRGBA( details.EnvColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

//Paper Mario Lava Room -  Floor, walls, and ceiling.
//case 0x00127e0af3fff238LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0           
//aA0  : (0            - 0           ) * 0            + Texel0      
//aRGB1: (Combined     - Primitive   ) * Prim_Alpha   + Combined    
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x00127e0af3fff238LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGBA( details.EnvColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

//Paper Mario - Light Effects
//case 0x00309861550eff4fLL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (Texel0       - 0           ) * Shade        + 0           
//aRGB1: (Primitive    - Env         ) * Texel0       + Env         
//aA1  : (Combined     - Texel0      ) * Primitive    + 0  
void BlendMode_0x00309861550eff4fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.EnvColour  );
	details.ColourAdjuster.SetA( details.PrimColour  );
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGBA);
}

//Paper Mario - Remove Dark Room..
//case 0x00619ac31137f7fbLL:
//aRGB0: (1            - Texel0      ) * Primitive    + 0           
//aA0  : (Texel0       - 0           ) * Env          + Primitive   
//aRGB1: (1            - Texel0      ) * Primitive    + 0           
//aA1  : (Texel0       - 0           ) * Env          + Primitive        
     
void BlendMode_0x00619ac31137f7fbLL (BLEND_MODE_ARGS)
{
	//XXX
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetA( details.EnvColour);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);	
}

// Paper Mario - Intro 1
//case 0x00357e6a11fcfc7eLL:
//aRGB0: (Primitive    - Texel0      ) * Prim_Alpha   + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Primitive    - Texel0      ) * Prim_Alpha   + Texel0      
//aA1  : (0            - 0           ) * 0            + 1           
void BlendMode_0x00357e6a11fcfc7eLL (BLEND_MODE_ARGS)
{
	//XXX Needs debugging
	details.ColourAdjuster.SetRGBA( details.PrimColour.ReplicateAlpha() );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

//Paper Mario - Stars
//case 0x007196e3332cfe7fLL:
//aRGB0: (CombAlp      - Primitive   ) * Primitive    + Texel0      
//aA0  : (Texel0       - 0           ) * Primitive    + 0           
//aRGB1: (CombAlp      - Primitive   ) * Primitive    + Texel0      
//aA1  : (Texel0       - 0           ) * Primitive    + 0        
void BlendMode_0x007196e3332cfe7fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGBA);
}

//Paper Mario - Sides of the letter in Intro and In Game Menu
//case 0x00117e60f5fff578LL:
//aRGB0: (Texel0       - 0           ) * Texel1       + 0           
//aA0  : (0            - 0           ) * 0            + Texel1      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + Combined    

void BlendMode_0x00117e60f5fff578LL (BLEND_MODE_ARGS)
{
	c32		blend( details.EnvColour.Interpolate( details.EnvColour, details.PrimColour ) );
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB( blend );
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGB( blend );
	}
}

//Paper Mario - Water in Fountain
//case 0x0020a204ff13ffffLL:
//aRGB0: (Texel1       - 0           ) * Texel0       + 0           
//aA0  : (Texel1       - 0           ) * Texel0       + 0           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Shade        + 0           

void BlendMode_0x0020a204ff13ffffLL (BLEND_MODE_ARGS)
{
	// XXXX need T1 in both textures..
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	}
	else
#endif
	{
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
}

//Paper Mario - Dust When Characters Walk.
//case 0x00ffabffff0d92ffLL:
//aRGB0: (0            - 0           ) * 0            + Primitive   
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0      
//aRGB1: (0            - 0           ) * 0            + Primitive   
//aA1  : (Combined     - 0           ) * Primitive    + 0       
void BlendMode_0x00ffabffff0d92ffLL (BLEND_MODE_ARGS)
{
	
	if( num_cycles != 1 )
	{
		details.ColourAdjuster.SetA( details.EnvColour );
	}
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGBA);
}

//Paper Mario - Candles Light
//case 0x0010e5e0230b1d52LL:
//aRGB0: (Texel0       - Texel1      ) * Texel0       + 1           
//aA0  : (1            - Texel0      ) * Texel1       + 1           
//aRGB1: (0            - Primitive   ) * Combined     + Env         
//aA1  : (Combined     - Texel1      ) * Texel1       + Texel1         
void BlendMode_0x0010e5e0230b1d52LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.EnvColour);
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Paper Mario - Hit enemies Effect
//case 0x0030abff5ffe9238LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0      
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + Combined     
void BlendMode_0x0030abff5ffe9238LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour);
	details.ColourAdjuster.SetA( details.EnvColour);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
//Paper Mario - Bowser's Star Powers.
//case 0x00f09a61501374ffLL:
//aRGB0: (0            - Env         ) * Texel0       + 1           
//aA0  : (Texel0       - 0           ) * Env          + Texel1      
//aRGB1: (Primitive    - Combined    ) * Texel0       + Primitive   
//aA1  : (Combined     - 0           ) * Shade        + 0            
void BlendMode_0x00f09a61501374ffLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Paper Mario - Bowser's Fire Attack
//case 0x00322bff5f0e923fLL:
//aRGB0: (Primitive    - Env         ) * Shade        + Env         
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0      
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (Combined     - 0           ) * Primitive    + 0       
void BlendMode_0x00322bff5f0e923fLL (BLEND_MODE_ARGS)
{	
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetA( details.PrimColour  );
	}
	else
#endif
	{
		// XXXX T1
		details.ColourAdjuster.SetRGB( details.EnvColour  );
		sceGuTexEnvColor( details.PrimColour.GetColour() );
		//Prim Constatn does the trick	
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
	}
	
}
//Paper Mario - Removes black placeholder when Mario gets hit...
//case 0x00262a6016fc9378LL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0      
//aRGB1: (Primitive    - 1           ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + Combined    
     
void BlendMode_0x00262a6016fc9378LL (BLEND_MODE_ARGS)
{
	//XXX Needs a bit of Debugging.
	
	sceGuTexEnvColor( details.PrimColour.GetColour() ); // Removing this removes black place
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);	
}
//Paper Mario - Thunder Strike Animation
//case 0x0010a2c3f00fd23fLL:
//aRGB0: (Texel0       - 0           ) * Texel0       + 0           
//aA0  : (Texel1       - Env         ) * Texel0       + Texel0      
//aRGB1: (1            - Combined    ) * Primitive    + Combined    
//aA1  : (Combined     - 0           ) * Primitive    + 0       
     
void BlendMode_0x0010a2c3f00fd23fLL (BLEND_MODE_ARGS)
{
	//XXX Correct :D
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);	
}

//Paper Mario - Main Screen where SaveGames are. <== Needs work
//case 0x00317fff5ffef438LL:
//aRGB0: (Primitive    - Env         ) * Texel1       + Env         
//aA0  : (0            - 0           ) * 0            + Texel1      
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x00317fff5ffef438LL (BLEND_MODE_ARGS)
{
	//BLEND_MODE_MAKER
}
// Pokemon Stadium 2 - Sky Changing Effect and Bases of Stadiums.
//case 0x00127ffffffdfe3fLL:
//aRGB0: (Texel0       - 0           ) * Shade        + Primitive
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + 0
void BlendMode_0x00127ffffffdfe3fLL (BLEND_MODE_ARGS)
{
	// Do not install, it brakes the bases on most stages
	// 
	details.InstallTexture = false;

	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB); // Not needed?
}

// Pokemon Stadium 2 - Fences on Top of Little Cup Stadium
//case 0x001217ff3ffe7e38LL:
//aRGB0: (Texel0       - Primitive   ) * Shade        + Shade
//aA0  : (Texel0       - 0           ) * Primitive    + 0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined

void BlendMode_0x001217ff3ffe7e38LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour);
	details.ColourAdjuster.SetA( details.PrimColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Pokemon Stadium 2 - Little cup top buildings
//case 0x0030fe045ffeff3fLL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (Combined     - 0           ) * Shade        + Shade
//aA1  : (0            - 0           ) * 0            + 0
void BlendMode_0x0030fe045ffeff3fLL (BLEND_MODE_ARGS)
{
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

// Pokemon Stadium 2 - Little cup ground shade
//case 0x006093ff3f0dfe3fLL:
//aRGB0: (1            - Primitive   ) * Texel0       + Primitive
//aA0  : (Texel0       - 0           ) * Texel0       + 0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (Combined     - 0           ) * Primitive    + 0

void BlendMode_0x006093ff3f0dfe3fLL (BLEND_MODE_ARGS)
{
	if( num_cycles != 1 )
	{
		details.ColourAdjuster.SetRGB( details.PrimColour);
		details.ColourAdjuster.SetA( details.PrimColour);
	}
	sceGuTexFunc(GU_TFX_DECAL,GU_TCC_RGBA);
}
// Pokemon Stadium 2 - Little cup ground
//case 0x00277e0413fcff3fLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (Combined     - Primitive   ) * Shade        + Shade
//aA1  : (0            - 0           ) * 0            + 0
void BlendMode_0x00277e0413fcff3fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}
// Pokemon Stadium 2 - Gym Leaders Buildings
//case 0x00127e03fffe73f8LL:
//aRGB0: (Texel0       - 0           ) * Shade        + Shade
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00127e03fffe73f8LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);	
}

// Pokemon Stadium 2 - Leader gym floor
//case 0x00117e03fffe7fffLL:
//aRGB0: (Texel0       - 0           ) * Texel1       + Shade
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (0            - 0           ) * 0            + 0
void BlendMode_0x00117e03fffe7fffLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);	
}
// Pokemon Stadium 2 - Leader gym Texture under floor.
//case 0x004193ffff0ffe3fLL:
//aRGB0: (Shade        - 0           ) * Primitive    + 0
//aA0  : (Texel0       - 0           ) * Texel0       + 0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x004193ffff0ffe3fLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);	
}

// Pokemon Stadium 2 - Leader gym Backgrounds
//case 0x00127e03fffe7fffLL:
//aRGB0: (Texel0       - 0           ) * Shade        + Shade
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (0            - 0           ) * 0            + 0
void BlendMode_0x00127e03fffe7fffLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);	
}

// Pokemon Stadium 2 - Backgrounds for First Arena and Little Cup
//case 0x00127fff3ffe7e3fLL:
//aRGB0: (Texel0       - Primitive   ) * Shade        + Shade
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + 0
void BlendMode_0x00127fff3ffe7e3fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);	
}

// Pokemon Stadium 2 - Buildings First Arena and little cup
//case 0x00127fff3ffe7238LL:
//aRGB0: (Texel0       - Primitive   ) * Shade        + Shade
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00127fff3ffe7238LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();	// testing opaque
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);	// Testing replace
}

// Pokemon Stadium 2 - Text In Game and In Menu
//case 0x00517e023f55ffffLL:
//aRGB0: (Env          - Primitive   ) * Texel1       + Primitive
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (Combined     - 0           ) * Texel1       + 0
//aA1  : (Texel1       - 0           ) * Env          + 0
void BlendMode_0x00517e023f55ffffLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.EnvColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);	
}

// Pokemon Stadium 2 Arena (Beach) Center Floor
//case 0x001616c0fffdf3f8LL:
//aRGB0: (Texel0       - 0           ) * Env_Alpha    + Primitive   
//aA0  : (Texel0       - 0           ) * Primitive    + Texel0      
//aRGB1: (1            - 0           ) * Combined     + 0           
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x001616c0fffdf3f8LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);	// Testing replace
}

// Pokemon stadium 2 Sky
//case 0x00127fff3ffefe3fLL:
//aRGB0: (Texel0       - Primitive   ) * Shade        + Env         
//aA0  : (0            - 0           ) * 0            + 0           
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + 0           
void BlendMode_0x00127fff3ffefe3fLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();
	//sceGuTexEnvColor( details.EnvColour.GetColour() );
	details.ColourAdjuster.SetRGB( details.EnvColour );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
//Pokemon Stadium 2 - Menus and Gym Leaders Map.
//case 0x00171a2e3336ff7fLL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Env
//aA0  : (Texel0       - 0           ) * Env          + 0
//aRGB1: (Texel0       - Primitive   ) * PrimLODFrac  + Env
//aA1  : (Texel0       - 0           ) * Env          + 0

 void BlendMode_0x00171a2e3336ff7fLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetA( details.EnvColour ); 
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
// Pokemon Stadium 2 - 2nd Ground Shade
//case 0x004093ffff0dfe3fLL:
//aRGB0: (Shade        - 0           ) * Texel0       + Primitive
//aA0  : (Texel0       - 0           ) * Texel0       + 0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x004093ffff0dfe3fLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetA ( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Pokemon Stadium 2 - Stadiums Misc
//case 0x0060fe043ffdf3f8LL:
//aRGB0: (1            - Primitive   ) * Texel0       + Primitive
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0060fe043ffdf3f8LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}

// Pokemon Stadium 2 - Ground in Most Stadiums !
//case 0x00457fff3ffcfe3fLL:
//aRGB0: (Shade        - Primitive   ) * Prim_Alpha   + Texel0
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + 0
void BlendMode_0x00457fff3ffcfe3fLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}

//Pokemon Stadium 2 Intro
//case 0x0017666025fd7f78LL: 
//aRGB0: (Texel0       - Texel1      ) * PrimLODFrac  + Texel1      
//aA0  : (1            - 0           ) * Primitive    + 0           
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + Combined  
void BlendMode_0x0017666025fd7f78LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE	
	if (num_cycles == 1 )
	{
		details.ColourAdjuster.ModulateRGB( details.PrimColour );
		
		// Needs T1 :/ 
	}
	else
#endif
	{
		details.ColourAdjuster.SetA( details.EnvColour );
		//	sceGuTexEnvColor( details.EnvColour.GetColour() );
		details.ColourAdjuster.ModulateRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
}
// Pokemon Stadium 2 Arena Floor
//case 0x0025fe0513fcff3fLL:
//aRGB0: (Texel1       - Texel0      ) * Shade_Alpha  + Texel0      
//aA0  : (0            - 0           ) * 0            + 0           
//aRGB1: (Combined     - Primitive   ) * Env          + Shade       
//aA1  : (0            - 0           ) * 0            + 0  

 void BlendMode_0x0025fe0513fcff3fLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
//Pokemon Stadium 2 - Water in Beach Stadium
//case 0x0020a205f3fff738LL:
//aRGB0: (Texel1       - 0           ) * Texel0       + 0
//aA0  : (Texel1       - 0           ) * Texel0       + Primitie
//aRGB1: (Combined     - Primitive   ) * Env          + Shade
//aA1  : (0            - 0           ) * 0            + Combine
void BlendMode_0x0020a205f3fff738LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
// Pokemon Stadium 2 - Pokemon Menus
//case 0x0050d2a133a5b6dbLL:
//aRGB0: (Env          - Primitive   ) * Texel0       + Primitive
//aA0  : (Env          - Primitive   ) * Texel0       + Primitive               
//aRGB1: (Env          - Primitive   ) * Texel0       + Primitive
//aA1  : (Env          - Primitive   ) * Texel0       + Primitive
void BlendMode_0x0050d2a133a5b6dbLL( BLEND_MODE_ARGS )
{
	// Modulate the texture*shade for RGBA
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetA( details.EnvColour );
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);			//Modulate(prim) for now
}

//Pokemon Stadium 2 - (Beach Arena) Buildings Far, Top Base, and Chairs.
//case 0x00fffe04f3fcf378LL:
//aRGB0: (0            - 0           ) * 0            + Texel0
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Combined     - Primitive   ) * Shade        + Env
//aA1  : (0            - 0           ) * 0            + Combined

void BlendMode_0x00fffe04f3fcf378LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}


//Pokemon Stadium 2 -  Pole
//case 0x00121804f3ffff78LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (Texel0       - 0           ) * Shade        + 0
//aRGB1: (Combined     - Primitive   ) * Shade        + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00121804f3ffff78LL (BLEND_MODE_ARGS)
{
	//RGB: INEXACT = ( Shade * blend(Env,Prim,Texel1) )
	//Alpha: Texel0
	
	// Correct
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
//Pokemon Stadium 2 -  Far Buildings 2
//case 0x00121604f3ffff78LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (Texel0       - 0           ) * Primitive    + 0
//aRGB1: (Combined     - Primitive   ) * Shade        + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00121604f3ffff78LL (BLEND_MODE_ARGS)
{
	//RGB: INEXACT = ( Shade * blend(Env,Prim,Texel1) )
	//Alpha: Texel0
	
	// Correct
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

//Pokemon Stadium 2 - Sky in Leader Stadium and Entrance
//case 0x00127e03ffffffffLL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (0            - 0           ) * 0            + 0

void BlendMode_0x00127e03ffffffffLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}
//Pokemon Stadium 2 - Base in Leader Gym
//case 0x00127e035ffe7fffLL:
//aRGB0: (Texel0       - Env         ) * Shade        + Shade
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (0            - 0           ) * 0            + 0

void BlendMode_0x00127e035ffe7fffLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}


//Pokemon Stadium 2 - Mr. Mime Mini Game "Barriers" (Front Half)
//case 0x00f5fa67f50c997fLL:
//aRGB0: (0            - 0           ) * Shade_Alpha  + Texel0
//aA0  : (0            - Texel0      ) * Env          + Shade
//aRGB1: (Primitive    - Env         ) * CombAlp      + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0

void BlendMode_0x00f5fa67f50c997fLL (BLEND_MODE_ARGS)
{ 
    details.ColourAdjuster.SetRGB( details.PrimColour );
    sceGuTexFunc(GU_TFX_ADD, GU_TCC_RGBA);
}

//Pokemon Stadium 2 - Mr. Mime Mini Game "Barriers" (Back Half)
//case 0x00262a04130cf37dLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0
//aA0  : (Texel1       - 0           ) * Env          + Texel0
//aRGB1: (Combined     - Primitive   ) * Shade        + Env
//aA1  : (Combined     - 0           ) * Primitive    + Env

void BlendMode_0x00262a04130cf37dLL (BLEND_MODE_ARGS)
{
	//XXXX needs T1 for full fix.
        
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA); //Needs T1, partially fixed. 
} 

//Pokemon Stadium 2 - Flame Wheel Attack
//case 0x00272c60350cf37fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - 0           ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00272c60350cf37fLL (BLEND_MODE_ARGS)
{
	//XXXX Needs T1 for full fix.
#ifdef CHECK_FIRST_CYCLE
	if(num_cycles == 1)
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

//Pokemon Stadium 2 - Reflect Attack (purplish,needs T1)
//case 0x00171660f50d757dLL:
//aRGB0: (Texel0       - 0           ) * PrimLODFrac  + Texel1
//aA0  : (Texel0       - 0           ) * Primitive    + Texel1
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + Env
void BlendMode_0x00171660f50d757dLL (BLEND_MODE_ARGS)
{
//XXXX Needs T1 for full fix.

details.ColourAdjuster.SetRGBA( details.EnvColour );
sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
}

/*
//#Q
*/ 

/* 
//#R
*/

//Road Rush 64 trees
//case 0x00129bfffffdf638LL:
//aRGB0: (Texel0       - 0           ) * Env          + Primitive
//aA0  : (Texel0       - 0           ) * Env          + Primitive
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00129bfffffdf638LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.EnvColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
	 
//Rush 2 - Cars
//case 0x001216acf0fffe38LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (Texel0       - 0           ) * Primitive    + 0
//aRGB1: (Env          - Combined    ) * Env_Alpha    + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x001216acf0fffe38LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.EnvColour);
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
// Ridge Racer 64 - StrmnNrmn
//case 0x0022aa041f0c93ffLL:
//aRGB0: (Texel1       - Texel0      ) * Env          + Texel0
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x0022aa041f0c93ffLL( BLEND_MODE_ARGS )
{
	//RGB:	INEXACT = ( Shade * blend(Texel0,Texel1,Env) )
	//Alpha: INEXACT: ( Prim * blend(Texel0,Texel1,Env) )
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
	}
	else
#endif
	{
		details.ColourAdjuster.ModulateRGB( details.EnvColour );
		details.ColourAdjuster.SetA( details.EnvColour.ModulateA( details.PrimColour ) );
	}
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Ridge Racer 64 - StrmnNrmn
//case 0x00272c041ffc93f8LL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00272c041ffc93f8LL( BLEND_MODE_ARGS )
{	
	if( num_cycles != 1 )
	{
		details.ColourAdjuster.SetAOpaque();	// Alpha 1.0
	}
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

// Ridge Racer 64 menu text 
//case 0x0030b2615566db6dLL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env
//aA0  : (Primitive    - Env         ) * Texel0       + Env
//aRGB1: (Primitive    - Env         ) * Texel0       + Env
//aA1  : (Primitive    - Env         ) * Texel0       + Env
void BlendMode_0x0030b2615566db6dLL( BLEND_MODE_ARGS )
{
	// Modulate the texture*shade for RGBA
	
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);			// XXXX Argh - need to interpolate alpha too!? We're just doing modulate(t,prim) for now
}

// Ridge Racer 64 and Zelda letter's word, breaking Witches legs - StrmnNrmn
//case 0x0030fe045ffef3f8LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Combined
/*
void BlendMode_0x0030fe045ffef3f8LL( BLEND_MODE_ARGS )
{
	//RGB: INEXACT = ( Shade * blend(Env,Prim,Texel0) )
	//Alpha: Texel0
	
	// Correct
	//details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);			// _RGBA = blend(e,p,t) for RGB, Modulate(t,1) for alpha blend breaks wictg
}*/

void BlendMode_0x0030fe045ffef3f8LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		//details.RecolourTextureWhite;
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

// Ridge Racer 64 Car backs
//case 0x0040b467f0fffe3eLL:
//aRGB0: (Shade        - 0           ) * Texel0       + 0
//aA0  : (Primitive    - 0           ) * Texel1       + 0
//aRGB1: (Primitive    - Combined    ) * CombAlp      + Combined
//aA1  : (0            - 0           ) * 0            + 1
void BlendMode_0x0040b467f0fffe3eLL( BLEND_MODE_ARGS )
{
	// Assuming this is always 2 cycles
	//RGB: INEXACT = blend(( Texel0 * Shade ),Prim,(T0a*T1a))
	//Alpha: 1
	
	details.ColourAdjuster.ModulateRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Ridge Racer 64, road- StrmnNrmn
//case 0x00117e045ffef3f8LL:
//aRGB0: (Texel0       - Env         ) * Texel1       + Env
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00117e045ffef3f8LL( BLEND_MODE_ARGS )
{
	// RGB:  ( Shade * blend(Env,Texel0,Texel1) )
	// A:	Texel0
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		// XXXX need blend Env->T0
		details.ColourAdjuster.SetRGB( details.EnvColour );
	}
	else
#endif
	{
		details.ColourAdjuster.ModulateRGB( details.EnvColour );
	}
	details.ColourAdjuster.SetAOpaque();	// Alpha 1.0
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

/* 
//#S
*/
// SpM/RR64/OOT - StrmnNrmn
//case 0x0030b3ff5ffeda38LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (Primitive    - Env         ) * Texel0       + Env         
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + Combined    
/*
void BlendMode_0x0030b3ff5ffeda38LL( BLEND_MODE_ARGS )
{
	
	// RGB = Blend( Env, Primitive, T0 )
	// A   = T0 * Primitive
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	// XXXX Needs to BLEND alpha values!
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);			// _RGBA = blend(e,p,t) for RGB, Modulate(t,p) for alpha
}*/
void BlendMode_0x0030b3ff5ffeda38LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}
//SCARS - Building Stages Effect and Sky - can't get it better : (
//case 0x0050fe6b20fd7c3dLL:
//aRGB0: (Env          - Texel1      ) * Texel0       + Texel1
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Primitive    - Combined    ) * Shade_Alpha  + Combined
//aA1  : (0            - 0           ) * 0            + Env

void BlendMode_0x0050fe6b20fd7c3dLL (BLEND_MODE_ARGS)
{
	
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB(details.PrimColour);
		details.ColourAdjuster.SetA(details.EnvColour);
		
	}
	else
#endif
	{
		
		details.ColourAdjuster.SetRGB( details.EnvColour );
		details.ColourAdjuster.SetAOpaque();
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
	}
	
}
//SCARS - Tracks / Stages
//case 0x00127e61f0fff83eLL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (0            - 0           ) * 0            + Shade
//aRGB1: (Primitive    - Combined    ) * Texel0       + Combined
//aA1  : (0            - 0           ) * 0            + 1

void BlendMode_0x00127e61f0fff83eLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE	
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB(details.PrimColour);
		details.ColourAdjuster.SetAOpaque();
	}
	else
#endif
	{
		details.ColourAdjuster.SetAOpaque();
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
	}
	
}
//SCARS -  Blend First Layer / Intro Stages
//case 0x00127e0bf1fffc7bLL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Combined     - Texel0      ) * Shade_Alpha  + Texel0
//aA1  : (0            - 0           ) * 0            + Primitive

void BlendMode_0x00127e0bf1fffc7bLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
//Space Station Silicon Valley - Teleporter big
//case 0x0030b3ff5f12da3fLL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env
//aA0  : (Primitive    - Env         ) * Texel0       + Env
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (Combined     - 0           ) * Shade        + 0 

void BlendMode_0x0030b3ff5f12da3fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Space Station Silicon Valley - Teleporter small
//case 0x0021246015fc9378LL:
//aRGB0: (Texel1       - Texel0      ) * Texel1       + Texel0
//aA0  : (Texel1       - Texel0      ) * Texel1       + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0021246015fc9378LL (BLEND_MODE_ARGS)
{	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
}
//Space Station Silicon Valley - Fences and T1
//case 0x0026a0041ffc93e0LL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - Shade       ) * 0            + Combined
void BlendMode_0x0026a0041ffc93e0LL( BLEND_MODE_ARGS )
{
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	else
#endif
	{
		details.ColourAdjuster.SetAOpaque();
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
}
//Space Station Silicon Valley - Water
//case 0x0025fe6014fcf73bLL:
//aRGB0: (Texel1       - Texel0      ) * Shade_Alpha  + Texel0
//aA0  : (0            - 0           ) * 0            + Primitive
//aRGBe1: (Primitive    - Shade       ) * Combined     + Shade
//aA1  : (0            - 0           ) * 0            + Primitive

void BlendMode_0x0025fe6014fcf73bLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA( details.PrimColour );
	//sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
//Silicon valley - Waterfalls looks trasparent now
//case 0x0025a86014fcb738LL:
//aRGB0: (Texel1       - Texel0      ) * Shade_Alpha  + Texel0
//aA0  : (Texel1       - Primitive   ) * Shade        + Primitie
//aRGB1: (Primitive    - Shade       ) * Combined     + Shade
//aA1  : (0            - 0           ) * 0            + Combine
void BlendMode_0x0025a86014fcb738LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
//Space Station Silicon Valley Ship Exhuast and Dust
//case 0x00272c6015fc9378LL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00272c6015fc9378LL( BLEND_MODE_ARGS )	
{
	
	if( num_cycles != 1 )
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		details.ColourAdjuster.SetA( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
	}
#ifdef CHECK_FIRST_CYCLE
	else
	{
		details.ColourAdjuster.SetAOpaque();
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	}
#endif
}


//Super Bowling 64 - Character
//case 0x00327feffffff638LL:
//aRGB0: (Primitive    - 0           ) * Shade        + 0
//aA0  : (0            - 0           ) * 0            + Primitive
//aRGB1: (0            - 0           ) * K5           + Combined
//aA1  : (0            - 0           ) * 0            + Combined

void BlendMode_0x00327feffffff638LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
	}
	else
#endif
	{
		details.ColourAdjuster.SetA( details.PrimColour );
		sceGuTexEnvColor( details.PrimColour.GetColour() );
	}
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
//Super Bowling 64 - Ceiling
//case 0x00f7ffeffffcf67bLL:
//aRGB0: (0            - 0           ) * K5           + Texel0
//aA0  : (0            - 0           ) * 0            + Primitive
//aRGB1: (0            - 0           ) * K5           + Texel0
//aA1  : (0            - 0           ) * 0            + Primitive

void BlendMode_0x00f7ffeffffcf67bLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetA( details.PrimColour);
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}


// San frasisco racing
//case 0x00ffabfffffc9238LL:
//aRGB0: (0            - 0           ) * 0            + Texel0
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00ffabfffffc9238LL( BLEND_MODE_ARGS )
{
	// XXXX placeholder implementation - needs t1
	// RGB = T0
	// A   =  blend(Texel0,Texel1,Env)
	
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
//SSB - CPU Select Badge 
//case 0x0012fe2533fdf2f9LL:
//aRGB0: (Texel0       - Primitive   ) * Env          + Primitive
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Texel0       - Primitive   ) * Env          + Primitive
//aA1  : (0            - 0           ) * 0            + Texel0
void BlendMode_0x0012fe2533fdf2f9LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );	
	}
	else
	{
		//details.ColourAdjuster.SetRGB( details.EnvColour );		
	}
#endif
	// XXX Adds glow to badges
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA); //We need to select Alpha
}

//SSB Bomb Partial-Flashing Animation
//case 0x00127eacf0fff238LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0   
//aA0  : (0            - 0           ) * 0            + Texel0      
//aRGB1: (Env          - Combined    ) * Env_Alpha    + Combined    
//aA1  : (0            - 0           ) * 0            + Combined 
void BlendMode_0x00127eacf0fff238LL( BLEND_MODE_ARGS )
{
	// By Kreationz & Shinydude100
	
	details.ColourAdjuster.SetAOpaque();//Opaque still isn't doing much on the Alpha..?
	details.ColourAdjuster.SetRGB( details.EnvColour );	
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA); //Not sure if BLEND is doing anything?
}

//Super Mario 64 - Bowser's picture 
//case 0x0026a1ff1ffc923cLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Shade

void BlendMode_0x0026a1ff1ffc923cLL (BLEND_MODE_ARGS)
{
	// Needs Texel1 for Peach picture to display 
	
	if( num_cycles != 1 )
	{
		details.ColourAdjuster.SetAOpaque();
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
}
//Sin and Punishment Grass
//case 0x00541aa83335feffLL:
//aRGB0: (Env          - Primitive   ) * Texel0_Alp   + Primitive   
//aA0  : (Texel0       - 0           ) * Env          + 0           
//aRGB1: (Env          - Primitive   ) * Texel0_Alp   + Primitive   
//aA1  : (Texel0       - 0           ) * Env          + 0       

void BlendMode_0x00541aa83335feffLL (BLEND_MODE_ARGS)
{	
	details.ColourAdjuster.SetRGB( details.PrimColour);
	details.ColourAdjuster.SetA( details.EnvColour );
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
//Sin and Punishment - Sky <----- Needs work
//case 0x001114a7f3fffef8LL:
//aRGB0: (Texel0       - 0           ) * Texel1       + 0
//aA0  : (Texel0       - 0           ) * Texel1       + 0
//aRGB1: (Env          - Primitive   ) * CombAlp      + Primitive
//aA1  : (0            - 0           ) * 0            + Combined   
void BlendMode_0x001114a7f3fffef8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.ModulateRGB( details.EnvColour);
	details.ColourAdjuster.SetAOpaque();
	//sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Sin and Punishment - Ground
//case 0x00547ea833fdf2f9LL:
//aRGB0: (Env          - Primitive   ) * Texel0_Alp   + Primitive
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Env          - Primitive   ) * Texel0_Alp   + Primitive
//aA1  : (0            - 0           ) * 0            + Texel0   

void BlendMode_0x00547ea833fdf2f9LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour);

	// A Needs to be 1 or Shade to be Opaque
	//details.ColourAdjuster.SetAOpaque(); 
	sceGuTexEnvColor( details.EnvColour.GetColour() );

	 //T0Alpha = DECAL mode on PSP
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Sin and Punishment - Particles and Explosions
//case 0x00551aaa1134fe7fLL:
//aRGB0: (Env          - Texel0      ) * Prim_Alpha   + Texel0
//aA0  : (Texel0       - 0           ) * Env          + 0
//aRGB1: (Env          - Texel0      ) * Prim_Alpha   + Texel0
//aA1  : (Texel0       - 0           ) * Env          + 0     

void BlendMode_0x00551aaa1134fe7fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGBA( details.PrimColour.ReplicateAlpha() );
	details.ColourAdjuster.SetA( details.EnvColour);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
/*
//#T
*/

//Tarzan birds wings, Marios drop shadows in SM64
//case 0x00121824ff33ffffLL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (Texel0       - 0           ) * Shade        + 0
//aRGB1: (Texel0       - 0           ) * Shade        + 0
//aA1  : (Texel0       - 0           ) * Shade        + 0
void BlendMode_0x00121824ff33ffffLL( BLEND_MODE_ARGS )
{
	//details.ColourAdjuster.SetA( c32(0,0,0,0xD8) );
	if( g_ROM.GameHacks == TARZAN ) details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Tetrissphere
//case 0x0026a0041f0c93ffLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Primitive    + 0           
void BlendMode_0x0026a0041f0c93ffLL( BLEND_MODE_ARGS )
{
	
	//XXXX placeholder
	
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);			// in RGBA mode alpha = texture*diffuse fragment
}

// Tony Hawks - StrmnNrmn
//case 0x005094023f15ffffLL:
//aRGB0: (Env          - Primitive   ) * Texel0       + Primitive   
//aA0  : (Texel0       - 0           ) * Texel1       + 0           
//aRGB1: (Combined     - 0           ) * Texel1       + 0           
//aA1  : (Combined     - 0           ) * Env          + 0           
void BlendMode_0x005094023f15ffffLL( BLEND_MODE_ARGS )
{
#ifdef CHECK_FIRST_CYCLE	
	// XXXX Needs T1
	if( num_cycles == 1 )
	{
		// RGB = Blend( Primitive, Env, T0 )
		// A   = T0 * T1
		details.ColourAdjuster.SetRGB( details.PrimColour );
		details.ColourAdjuster.SetAOpaque();
		sceGuTexEnvColor( details.EnvColour.GetColour() );
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);			// _RGBA = blend(p,e,t) for RGB, T0*1 for alpha
	}
	else
#endif
	{
		// RGB = Blend( Primitive, Env, T0 )
		// A   = T0 * T1 * Env
		details.ColourAdjuster.SetRGB( details.PrimColour );
		details.ColourAdjuster.SetA( details.EnvColour );
		sceGuTexEnvColor( details.EnvColour.GetColour() );
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);			// _RGBA = blend(p,e,t) for RGB, T0*Env for alpha
	}
}

// THPS  - Text Place holder
//case 0x00ffffffff09f63fLL:
//aRGB0: (0            - 0           ) * 0            + Primitive   
//aA0  : (0            - 0           ) * 0            + Primitive   
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (Combined     - 0           ) * Texel1       + 0 

// THPS Title Screen 
void BlendMode_0x00ffffffff09f63fLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB(details.PrimColour);
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
	else
#endif
	{
		
		details.ColourAdjuster.SetRGBA(details.PrimColour);
	}
}

// Tony Hawk Pro Skater and Top Gear Stages - StrmnNrmn
//case 0x0026a0041ffc93f8LL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0      
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x0026a0041ffc93f8LL( BLEND_MODE_ARGS )
{
	// XXXX need 2nd texture
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	else
#endif
	{
		details.ColourAdjuster.SetAOpaque();	// Alpha 1.0
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
}
/*
//#U
*/


/*
//#V
*/
//Vigilante 8 - Fix game
//case 0x0017fe2f77fcf87cLL:
//aRGB0: (Texel0       - CombAlp     ) * K5           + Texel0      
//aA0  : (0            - 0           ) * 0            + Shade       
//aRGB1: (Texel0       - CombAlp     ) * K5           + Texel0      
//aA1  : (0            - 0           ) * 0            + Shade     

void BlendMode_0x0017fe2f77fcf87cLL (BLEND_MODE_ARGS)
{
	//*K5 not handled .
	
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
//Vigilante 8 - Ground
//case 0x0055fe041ffcf3f8LL:
//aRGB0: (Env          - Texel0      ) * Shade_Alpha  + Texel0      
//aA0  : (0            - 0           ) * 0            + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Combined     

void BlendMode_0x0055fe041ffcf3f8LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.EnvColour  );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

/*
//#W
*/

// Wave Racer - Sky
//case 0x0022ffff1ffcfa38LL:
//aRGB0: (Texel1       - Texel0      ) * Env          + Texel0
//aA0  : (0            - 0           ) * 0            + Env
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0022ffff1ffcfa38LL (BLEND_MODE_ARGS)
{
	// XXXX placeholder implementation
	
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}
// Wave Racer - Placeholders and Signs !!!
//case 0x0021a6ac10fc9238LL:
//aRGB0: (Texel1       - Texel0      ) * Primitive    + Texel0
//aA0  : (Texel1       - Texel0      ) * Primitive    + Texel0
//aRGB1: (Env          - Combined    ) * Env_Alpha    + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0021a6ac10fc9238LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
// Wave Racer - Big balloons and Small balloons <-- Needs more work
//case 0x00567eac11fcf279LL:
//aRGB0: (Env          - Texel0      ) * Env_Alpha    + Texel0
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Env          - Texel0      ) * Env_Alpha    + Texel0
//aA1  : (0            - 0           ) * 0            + Texel0
void BlendMode_0x00567eac11fcf279LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB(details.EnvColour);
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
}
// Wetrix
//case 0x0011ffff2ffd7c38LL:
//aRGB0: (Texel0       - Texel1      ) * Primitive    + Texel1
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0011ffff2ffd7c38LL( BLEND_MODE_ARGS )
{
	// XXXX - needs t1
	
	// RGB = Blend( T1, T0, Prim )
	// A   = 1
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}

/*
//#X
*/
 
// Xena - StrmnNrmn
//case 0x00309661552efb7dLL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (Texel0       - 0           ) * Primitive    + Env         
//aRGB1: (Primitive    - Env         ) * Texel0       + Env         
//aA1  : (Texel0       - 0           ) * Primitive    + Env         
void BlendMode_0x00309661552efb7dLL( BLEND_MODE_ARGS )
{
	
	// RGB = Blend( Env, Primitive, T0 )
	// A   = T0 * Primitive + Env
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);			// _RGBA = blend(e,p,t) for RGB, Modulate(t,p) for alpha
	
	// XXXX need to add in Env for alpha
}

// Xena - StrmnNrmn 
// OOT Horse dust
//case 0x0030b2045ffefff8LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (Primitive    - 0           ) * Texel0       + 0           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x0030b2045ffefff8LL(BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( c32::White );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
}

// Xena Warrior Princess - StrmnNrmn
//case 0x00ff97ffff2cfa7dLL:
//aRGB0: (0            - 0           ) * 0            + Texel0      
//aA0  : (Texel0       - 0           ) * Primitive    + Env         
//aRGB1: (0            - 0           ) * 0            + Texel0      
//aA1  : (Texel0       - 0           ) * Primitive    + Env         
void BlendMode_0x00ff97ffff2cfa7dLL( BLEND_MODE_ARGS )
{
	
	details.ColourAdjuster.SetRGB( c32::White );
	details.ColourAdjuster.SetA( details.PrimColour );

	// XXXX need to add in Env to alpha
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

/*
//#Z
*/
// Zelda Goron Bracelet, Giant Knife
//case 0x00176c6035d8ed76LL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel0      
//aA0  : (1            - 1           ) * 1            + 1           
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (1            - 1           ) * 1            + 1   
void BlendMode_0x00176c6035d8ed76LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

//Kokiri Sword Blade - Zelda OOT
//case 0x00177e6035fcfd7eLL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + 1
void BlendMode_0x00177e6035fcfd7eLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE	
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);	
	}	
}

// OOT - Light Surrounding Link and Warps
//case 0x0026a060150c937fLL:
//aRGB0: (Texel1       - Texel0      ) * LOD_Frac     + Texel0
//aA0  : (Texel1       - Texel0      ) * Combined     + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x0026a060150c937fLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

// Oot Lizfaldo Sword
//case 0x00127ec1f0fffa38LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0           
//aA0  : (0            - 0           ) * 0            + Env         
//aRGB1: (1            - Combined    ) * Texel0       + Combined    
//aA1  : (0            - 0           ) * 0            + Combined  
void BlendMode_0x00127ec1f0fffa38LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA(details.EnvColour);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}	
// OOT Lizfaldo spikes
//case 0x00119bffff5bfe38LL:
//aRGB0: (Texel0       - 0           ) * Primitive    + 0           
//aA0  : (Texel0       - 0           ) * Env          + 0           
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (Texel1       - 0           ) * 1            + Combined
void BlendMode_0x00119bffff5bfe38LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB( details.PrimColour);
	//details.ColourAdjuster.SetA( details.EnvColour);
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OOT Dungeon Cow
//case 0x00262a041ffc93f8LL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Combined  
void BlendMode_0x00262a041ffc93f8LL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetAOpaque();
	details.EnvColour.ReplicateAlpha();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

 //Zelda Arrow Switches in Dungeons
 //case 0x00267e051ffcfdf8LL:
 //aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
 //aA0  : (0            - 0           ) * 0            + 1           
 //aRGB1: (Combined     - 0           ) * Env          + 0           
 //aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x00267e051ffcfdf8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.EnvColour );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

// Zelda's Song Purple Waves
// Okay except T1
//case 0x00262a603510937fLL:
//aRGB0: (Texel1       - Primitive   ) * Env_Alpha    + Texel0      
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Shade        + 0  
void BlendMode_0x00262a603510937fLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{	
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

// Zelda OoT logo flames
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
//Zelda MM (GC) - Fountains in West Termina Field
//case 0x00272c80350cf37fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - 0           ) * 1            + Texel0
//aRGB1: (Shade        - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00272c80350cf37fLL( BLEND_MODE_ARGS )
{
	//Semi-fix, a few white streaks through the water, T1 for full fix.
	
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetA ( details.PrimColour );
    sceGuTexEnvColor ( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGB);
}

//Zelda MM (GC) - Song of Soaring
//case 0x0012fec8f2fdfe3bLL:
//aRGB0: (Texel0       - 0           ) * Env          + Primitive
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (1            - Texel1      ) * Texel0_Alp   + Combined
//aA1  : (0            - 0           ) * 0            + Primitive
void BlendMode_0x0012fec8f2fdfe3bLL ( BLEND_MODE_ARGS )
{   
    details.ColourAdjuster.SetRGB ( details.EnvColour );
    details.ColourAdjuster.ModulateA ( details.PrimColour );
    sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

//Zelda MM (GC) - Termina Field, Ice Blocking Way To Mountian Path
 //case 0x00272c603514937fLL:
 //aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
 //aA0  : (Texel1       - Texel0      ) * 1            + Texel0
 //aRGB1: (Primitive    - Env         ) * Combined     + Env
 //aA1  : (Combined     - 0           ) * Env          + 0
void BlendMode_0x00272c603514937fLL ( BLEND_MODE_ARGS )
{
	details.ColourAdjuster.SetRGBA ( details.EnvColour );
	sceGuTexEnvColor ( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
}

//Zelda MM (GC) - Telescope on Astral Observetory
//case 0x0020ac03ff0f93ffLL:
//aRGB0: (Texel1       - 0           ) * Texel0       + 0
//aA0  : (Texel1    - Texel0         ) * 1            + Texel0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x0020ac03ff0f93ffLL ( BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Zelda MM (GC) - Stars on Gate Surrounding Astral Oservetory
//case 0x00272c031f0c93ffLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00272c031f0c93ffLL ( BLEND_MODE_ARGS )
{
	details.ColourAdjuster.SetRGBA ( details.PrimColour );
    sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Zelda MM (GC) - Moon's Beam Sucking Up Majora's Mask
//case 0x00272c603410f33fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - 0           ) * 1            + Texel0
//aRGB1: (Primitive    - Shade       ) * Combined     + Shade
//aA1  : (Combined     - 0           ) * Shade        + 0
void BlendMode_0x00272c603410f33fLL ( BLEND_MODE_ARGS )
{
      details.ColourAdjuster.SetRGB ( details.PrimColour );
      details.ColourAdjuster.ModulateA ( details.PrimColour );
      sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
// OoT SkyBox Textures, Includes rooms and outdoor
//case 0x002527ff1ffc9238LL:
//aRGB0: (Texel1       - Texel0      ) * Prim_Alpha   + Texel0      
//aA0  : (Texel1       - Texel0      ) * Primitive    + Texel0      
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x002527ff1ffc9238LL( BLEND_MODE_ARGS )
{
	// XXXX placeholder implementation - is blending between T0/T1 using primitive colour
	
	details.ColourAdjuster.SetRGBA( details.PrimColour.ReplicateAlpha() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA); //Can't mod black, needs REPLACE (Wally)
}

// OoT and Majora Mask - Fairies, witches hair glow, and Suns.
//case 0x00262a60150c937fLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Primitive    + 0           
void BlendMode_0x00262a60150c937fLL( BLEND_MODE_ARGS )
{
	// XXXX placeholder implementation
	
	details.ColourAdjuster.SetRGB ( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OoT intro, MM Intro
//case 0x00167e6035fcff7eLL:
//aRGB0: (Texel0       - Primitive   ) * Env_Alpha    + Texel0      
//aA0  : (0            - 0           ) * 0            + 0           
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (0            - 0           ) * 0            + 1           
void BlendMode_0x00167e6035fcff7eLL( BLEND_MODE_ARGS )
{
	// XXXX incorrect, need env and prim, blend has wrong source

	// Use the primitive for the r,g,b, override the alpha with 1.0
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque(); 		// 1.0 for Alpha
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
}

// Oot Lake Hylia should be correct except T1
//case 0x00267e041f0cfdffLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Primitive    + 0           
void BlendMode_0x00267e041f0cfdffLL( BLEND_MODE_ARGS )
{
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		
		details.ColourAdjuster.SetRGB ( details.EnvColour );
		details.ColourAdjuster.SetAOpaque(); // Alpha 1.0
	}
	else
#endif
	{
		details.ColourAdjuster.SetAOpaque(); // Alpha 1.0
		details.ColourAdjuster.ModulateA( details.PrimColour );
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
	}
	
}

// OoT ground texture and water temple walls
//case 0x00267e041ffcfdf8LL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x00267e041ffcfdf8LL( BLEND_MODE_ARGS )
{
	// XXXX placeholder implementation - ok except t1?
	// RGB = Blend(T0, T1, EnvAlpha) * Shade
	// A   = 1
	details.ColourAdjuster.SetAOpaque();	// Alpha 1.0
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

// OoT intro
//case 0x00267e60350cf37fLL:
//aRGB0: (Texel1       - Primitive   ) * Env_Alpha    + Texel0      
//aA0  : (0            - 0           ) * 0            + Texel0      
//aRGB1: (Primitive    - Env         ) * Combined     + Env         
//aA1  : (Combined     - 0           ) * Primitive    + 0           
void BlendMode_0x00267e60350cf37fLL( BLEND_MODE_ARGS )
{
	// XXXX need 2nd texture
	// XXXX need to blend using env
	
	details.ColourAdjuster.SetRGBA( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OoT paths
//case 0x00121603ff5bfff8LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0           
//aA0  : (Texel0       - 0           ) * Primitive    + 0           
//aRGB1: (Combined     - 0           ) * Primitive    + 0           
//aA1  : (Texel1       - 0           ) * 1            + Combined  
void BlendMode_0x00121603ff5bfff8LL( BLEND_MODE_ARGS )
{
	// XXXX placeholder implementation - think is correct except t1
	// RGB = T0 * Shade * Primitive
	// A   = T0 * Primitive + T1
	/*if( num_cycles != 1 )
	{
		
	}*/
	details.ColourAdjuster.ModulateRGB( details.PrimColour );
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Zelda Shop Items (Second Shelf) and Zelda Letter, and Bombs, Jars
//case 0x0030ec045fdaedf6LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env         
//aA0  : (1            - 1           ) * 1            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (1            - 1           ) * 1            + 1      
void BlendMode_0x0030ec045fdaedf6LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

// Zelda Rupees, dungeon spike moving thingy, and withes' saphire.
//case 0x0011fffffffffc38LL:
//aRGB0: (Texel0       - 0           ) * Primitive    + 0           
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + Combined

void BlendMode_0x0011fffffffffc38LL (BLEND_MODE_ARGS)
{
	//BLEND_MODE_MAKER
}

// OOT - Lens of Truth
//case 0x00373c6e117b9fcfLL:
//aRGB0: (Primitive    - Texel0      ) * PrimLODFrac  + 0           
//aA0  : (Primitive    - Texel0      ) * 1            + 0           
//aRGB1: (Primitive    - Texel0      ) * PrimLODFrac  + 0           
//aA1  : (Primitive    - Texel0      ) * 1            + 0   
void BlendMode_0x00373c6e117b9fcfLL (BLEND_MODE_ARGS)
{
	// XXXX Needs T1 for full fix
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGB);
}

//Oot and Majora Mask - Master Sword lightning effect and hitting enemies
//case 0x00262a60350ce37fLL:
//aRGB0: (Texel1       - Primitive   ) * Env_Alpha    + Texel0
//aA0  : (Texel1       - 1           ) * Env          + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00262a60350ce37fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB ( details.EnvColour ); 
	details.EnvColour.ReplicateAlpha();
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGBA);
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

//Beginner's sword on Legend of Zelda: OOT (bottom part) and Chikens
//aRGB0: (Primitive    - Env         ) * Texel0       + Env
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + 1
void BlendMode_0x0030fe045ffefdfeLL( BLEND_MODE_ARGS )
{
	// By Shinydude100 & Wally & Salvy : P
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OOT - Light when opening chest
//case 0x0020ac60350c937fLL:
//aRGB0: (Texel1       - Primitive   ) * Texel0       + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive  + 0
void BlendMode_0x0020ac60350c937fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB ( details.EnvColour ); /// Env does the trick !
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Majora's Mask - Telescope Sky
//case 0x00ffedffffd996cbLL:
//aRGB0: (0            - 0           ) * 0            + Primitive
//aA0  : (1            - Texel0      ) * 1            + Primitive
//aRGB1: (0            - 0           ) * 0            + Primitive
//aA1  : (1            - Texel0      ) * 1            + Primitive
void BlendMode_0x00ffedffffd996cbLL (BLEND_MODE_ARGS)
{
	// XXXX Correct no need to show Alpha, atleast for now.
	// Alpha shows white circles on the screen...
	details.ColourAdjuster.SetRGBA( details.PrimColour );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
}

//Majora's Mask - Telescope Miscs
//case 0x0020fe04ff0ff7ffLL:
//aRGB0: (Texel1       - 0           ) * Texel0       + 0
//aA0  : (0            - 0           ) * 0            + Primitive
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x0020fe04ff0ff7ffLL (BLEND_MODE_ARGS)
{
	// XXXX Needed to fix Telescope : S
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Majora's Mask - Clouds Over Mountain
//case 0x0020ac0aff0f93ffLL:
//aRGB0: (Texel1       - 0           ) * Texel0       + 0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Combined     - 0           ) * Prim_Alpha   + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x0020ac0aff0f93ffLL (BLEND_MODE_ARGS)
{
	//( Prim * ( Texel0 + Texel1 - Texel0 ) )
	details.ColourAdjuster.SetRGBA( details.PrimColour.ReplicateAlpha() );
	//details.ColourAdjuster.SetA( details.PrimColour ); // <=== REMOVE ME
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

//Majora's Mask - Background Forest bottom
//case 0x002714041f0cffffLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel0       - 0           ) * Texel1       + 0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x002714041f0cffffLL (BLEND_MODE_ARGS)
{	
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
//MM - River under Clock Town
//case 0x0020fe05f3fff738LL:
//aRGB0: (Texel1       - 0           ) * Texel0       + 0
//aA0  : (0            - 0           ) * 0            + Primitive
//aRGB1: (Combined     - Primitive   ) * Env          + Shade
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0020fe05f3fff738LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.EnvColour  );
	details.ColourAdjuster.SetA( details.PrimColour  );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

//Majora's Mask - Paths <=== It Can't be fixed with blendmode?
//case 0x002712041f0cffffLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel0       - 0           ) * Texel0       + 0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x002712041f0cffffLL( BLEND_MODE_ARGS )
{
	// XXXX Z Fighting....
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		//details.ColourAdjuster.SetA( details.PrimColour  );
		//sceGuTexEnvColor( details.PrimColour.GetColour() );
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);		
	}
	else
#endif
	{
		// XXXX T1
		//details.ColourAdjuster.SetAOpaque();
		//sceGuTexEnvColor( details.PrimColour.GetColour() );
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	}
}
//MM - Lens of Truth
//case 0x0061e6c311cf9fcfLL:
//aRGB0: (1            - Texel0      ) * Primitive    + 0
//aA0  : (1            - Texel0      ) * Primitive    + 0
//aRGB1: (1            - Texel0      ) * Primitive    + 0
//aA1  : (1            - Texel0      ) * Primitive    + 0
void BlendMode_0x0061e6c311cf9fcfLL (BLEND_MODE_ARGS)
{
	// XXXX Needs S2DEX for full fix
	// Alpha not needed we're missing S2DEX texture.
	details.ColourAdjuster.SetRGBA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGB);
}

//MM - Clouds in Mountain
//case 0x002722041f0cffffLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - 0           ) * Texel0       + 0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x002722041f0cffffLL (BLEND_MODE_ARGS)
{
	if( num_cycles != 1 )
	{
		details.ColourAdjuster.SetA( details.PrimColour );
	}
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
//MM - Shade on Moon's Tree and T1 Buildings in Fountain.
//case 0x0020a204ff0fffffLL:
//aRGB0: (Texel1       - 0           ) * Texel0       + 0
//aA0  : (Texel1       - 0           ) * Texel0       + 0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x0020a204ff0fffffLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

//MM - Ground inside the Moon
//case 0x00242c04ff0f93ffLL:
//aRGB0: (Texel1       - 0           ) * Texel0_Alp   + 0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00242c04ff0f93ffLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// MM - Masks Waves <=== FIX ME
//case 0x00273c60f514e37fLL:
//aRGB0: (Texel1       - 0           ) * PrimLODFrac  + Texel0
//aA0  : (Primitive    - 1           ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Env          + 0
void BlendMode_0x00273c60f514e37fLL (BLEND_MODE_ARGS)
{
	// S2DEX for full fix :/
	details.ColourAdjuster.ModulateRGB( details.PrimColour );
	details.ColourAdjuster.ModulateA( details.PrimColour );
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
// Majora's Mask - Sky
//case 0x0025266015fc9378LL:
//aRGB0: (Texel1       - Texel0      ) * Prim_Alpha   + Texel0
//aA0  : (Texel1       - Texel0      ) * Primitive    + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined  
void BlendMode_0x0025266015fc9378LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGBA( details.PrimColour.ReplicateAlpha() );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA); //Wally says is REPLACE
}

// Majora's Mask - Buildings T1 Clock
//case 0x0020ac04ff0f93ffLL:
//aRGB0: (Texel1       - 0           ) * Texel0       + 0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
 void BlendMode_0x0020ac04ff0f93ffLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Majora's Mask - Fog
//case 0x00272c031f1093ffLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (Combined     - 0           ) * Shade        + 0
void BlendMode_0x00272c031f1093ffLL (BLEND_MODE_ARGS)
{	
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Majora's Mask - Hearts  <--- Needs more work
//case 0x00167e6035fcf378LL:
//aRGB0: (Texel0       - Primitive   ) * Env_Alpha    + Texel0
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined
 void BlendMode_0x00167e6035fcf378LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.EnvColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Majora's Mask - Sides of Ground outside Clock Town
//case 0x00272e041f0c93ffLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 0            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00272e041f0c93ffLL (BLEND_MODE_ARGS)
{
	// XXXX Z-Figthing...and T1...
	if( num_cycles != 1 )
	{
		details.ColourAdjuster.SetA( details.PrimColour );
	}
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

//Majora's Mask - Water,bells,trees,ground,inside rooms and grass.
//case 0x00272c041f0c93ffLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00272c041f0c93ffLL (BLEND_MODE_ARGS)
{	
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Majora's Mask - Globes on Sky, and Light on Sky.
//case 0x00209c03ff0f93ffLL:
//aRGB0: (Texel1       - 0           ) * Texel0       + 0
//aA0  : (Texel0       - Texel0      ) * 1            + Texel0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00209c03ff0f93ffLL (BLEND_MODE_ARGS)
{	
	details.ColourAdjuster.SetAOpaque();
	details.ColourAdjuster.SetRGB( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// Fix Me, the smoke in MM looks really bad :(
//Majora's Mask - Fumes, Smog and SSB Lava.
//case 0x00272c041f1093ffLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Shade        + 0

void BlendMode_0x00272c041f1093ffLL (BLEND_MODE_ARGS)
{	
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		//details.ColourAdjuster.SetAOpaque();
	}
	else
#endif
	{
		details.ColourAdjuster.SetAOpaque();
	}
	// XXX Adds glow to badges
	sceGuTexFilter(GU_TFX_ADD,GU_TCC_RGBA); //We need to select Alpha
}
//Majora's Mask - Intro Glitch, Trees and Floor 2nd Placeholder
//case 0x0020ac04ff0f92ffLL:
//aRGB0: (Texel1       - 0           ) * Texel0       + 0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + Primitive
//aA1  : (Combined     - 0           ) * Primitive    + 0

void BlendMode_0x0020ac04ff0f92ffLL (BLEND_MODE_ARGS)
{	
	
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
//Majora Mask - Mountains Outside Clock Town
//case 0x00277e041f0cf7ffLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + Primitive
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00277e041f0cf7ffLL (BLEND_MODE_ARGS)
{
	
	if( num_cycles != 1 )
	{
		details.ColourAdjuster.SetA( details.PrimColour );
	}
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
// OOT
// Zora's Domain Water // Correct except T1
//case 0x00267e031f0cfdffLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Primitive    + 0           
//aA1  : (Combined     - 0           ) * Primitive    + 0   
void BlendMode_0x00267e031f0cfdffLL (BLEND_MODE_ARGS)
{
	
	details.ColourAdjuster.SetRGB ( details.EnvColour );
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);	
}
// OOT
// Zora's Domain Zelda Waterfall
//case 0x00262a041f1093ffLL:
// Needs more work
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0      
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Shade        + 0           
void BlendMode_0x00262a041f1093ffLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

// Zora's Domain Waterfall part 2 and Desert's sand.
// case 0x00267e041f10fdffLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0      
//aA0  : (0            - 0           ) * 0            + 1           
//aRGB1: (Combined     - 0           ) * Shade        + 0           
//aA1  : (Combined     - 0           ) * Shade        + 0  
void BlendMode_0x00267e041f10fdffLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

// OoT, letters
//case 0x00ffadfffffd9238LL:
//aRGB0: (0            - 0           ) * 0            + Primitive   
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0      
//aRGB1: (0            - 0           ) * 0            + Combined    
//aA1  : (0            - 0           ) * 0            + Combined    
void BlendMode_0x00ffadfffffd9238LL( BLEND_MODE_ARGS )
{
	// XXXX placeholder implementation - needs t1
	// RGB = Prim
	// A   = (T1-T0)+T0 = T1
	
	details.RecolourTextureWhite = true;
	details.ColourAdjuster.SetRGB( details.PrimColour );		// Want to select texture alpha
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// ?? Zelda and SSB hidden characters screen.
//case 0x0071fee311fcf279LL:
//aRGB0: (CombAlp      - Texel0      ) * Primitive    + Texel0      
//aA0  : (0            - 0           ) * 0            + Texel0      
//aRGB1: (CombAlp      - Texel0      ) * Primitive    + Texel0      
//aA1  : (0            - 0           ) * 0            + Texel0     
void BlendMode_0x0071fee311fcf279LL (BLEND_MODE_ARGS)
{
	//BLEND_MODE_MAKER
}

// OOT - Effect when you get hit by enemies
//case 0x00272c60150ce37fLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - 1           ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0

void BlendMode_0x00272c60150ce37fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB ( details.EnvColour ); /// Env does the trick !
	details.ColourAdjuster.SetAOpaque();
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
	details.ColourAdjuster.SetRGB ( details.EnvColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
}
// OOT - Gold Skulltula Badge placeholder
//case 0x00171c6035fd6578LL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel1
//aA0  : (Texel0       - 1           ) * 1            + Texel1
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00171c6035fd6578LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB ( details.EnvColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGBA);
	
}

// OOT - Dessert
//case 0x00272a60150c937fLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00272a60150c937fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB ( details.EnvColour );
	details.ColourAdjuster.SetA ( details.PrimColour );
	sceGuTexEnvColor( details.EnvColour.GetColour() ); /// Testing Env
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OOT - Ghosts body
//case 0x00121a03ff5bfff8LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (Texel0       - 0           ) * Env          + 0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (Texel1       - 0           ) * 1            + Combined
void BlendMode_0x00121a03ff5bfff8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB ( details.PrimColour );
	details.ColourAdjuster.SetA ( details.EnvColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
}

// OOT - Dungeon Entrances and Gold Triangle at the great fairy fountain
//case 0x00121803ff5bfff8LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (Texel0       - 0           ) * Shade        + 0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (Texel1       - 0           ) * 1            + Combined

void BlendMode_0x00121803ff5bfff8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
// OOT - Ice Blocks and Normal Blocks
//case 0x00277e041ffcfdf8LL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00277e041ffcfdf8LL (BLEND_MODE_ARGS)
{
	if( num_cycles != 1 )
	{
		details.ColourAdjuster.SetAOpaque();
	}
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
}

// OOT - Magical Stuff when Teleport to Temples
//case 0x0017166035fcff78LL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel0       - 0           ) * Primitive    + 0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined

void BlendMode_0x0017166035fcff78LL (BLEND_MODE_ARGS)
{
	// XXXX Needs T1
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		details.ColourAdjuster.SetA( details.EnvColour );
		sceGuTexEnvColor( details.PrimColour.GetColour() );
		//sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);	
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		details.ColourAdjuster.SetAOpaque();
		sceGuTexEnvColor( details.EnvColour.GetColour() );
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
}
// OOT - Desert Enememies and Butterflies : )
//case 0x00119604ff5bfff8LL:
//aRGB0: (Texel0       - 0           ) * Primitive    + 0
//aA0  : (Texel0       - 0           ) * Primitive    + 0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Texel1       - 0           ) * 1            + Combined
void BlendMode_0x00119604ff5bfff8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGBA( details.PrimColour );
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OOT - Water Temple / Water Current / lost forest light
//case 0x00262a041f0c93ffLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0
//aRGB1: (Combined     - 0           ) * Shade        + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00262a041f0c93ffLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetA( details.EnvColour );
	sceGuTexEnvColor( details.EnvColour.GetColour() ); // Testing Env
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OOT - Water Temple Blocks
//case 0x00262a601510937fLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Shade        + 0

void BlendMode_0x00262a601510937fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour ); //I think I can disable it
	details.ColourAdjuster.SetA( details.EnvColour ); // Does the trick !
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OOT - Goron city symbol design.
//case 0x001197ffff5bfe38LL:
//aRGB0: (Texel0       - 0           ) * Primitive    + 0
//aA0  : (Texel0       - 0           ) * Primitive    + 0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (Texel1       - 0           ) * 1            + Combined

void BlendMode_0x001197ffff5bfe38LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetA( details.PrimColour ); 
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OOT - Ice
//case 0x00272c6035a0937fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Env          - 0           ) * Combined     + 0
void BlendMode_0x00272c6035a0937fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetA( details.EnvColour ); 
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);	
}

//ZELDA - OOT - Light inside Zelda Castle
//case 0x00272c60340c933fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Primitive    - Shade       ) * Combined     + Shade
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00272c60340c933fLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

//ZELDA - OOT - Triforce
//Hmm... Needs more work... At least it's yellow :)
//case 0x00277e6035fcf778LL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + Primitive
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00277e6035fcf778LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour ); // Might want to use c32::Gold here?
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

// OOT - Triforce Light
//case :0x00272c603510937fLL
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Shade        + 0
void BlendMode_0x00272c603510937fLL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexEnvColor( details.EnvColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

// OOT - Zelda's Castle Shade and Door, and Triforce stone.
//case 0x00267e031ffcfdf8LL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00267e031ffcfdf8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.PrimColour.ModulateRGB( c32::Gold ) );
	details.ColourAdjuster.SetAOpaque();
	sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
}

//Ocarina of Time - Withes Ground
//case 0x00262a60150d157fLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel1
//aA0  : (Texel1       - Texel0      ) * Env          + Texel1
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00262a60150d157fLL (BLEND_MODE_ARGS)
{
	// XXXX placeholder implementation
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		details.ColourAdjuster.SetA( details.PrimColour  );
		
		// Show real color and not red.
		sceGuTexEnvColor( details.PrimColour.GetColour() ); 
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
	}	
}

// OOT - Master Sword
//case 0x00167e6035fcfd78LL:
//aRGB0: (Texel0       - Primitive   ) * Env_Alpha    + Texel0
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00167e6035fcfd78LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

// OOT - Lava Monsters
//case 0x00267e051ffcf7f8LL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0
//aA0  : (0            - 0           ) * 0            + Primitive
//aRGB1: (Combined     - 0           ) * Env          + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00267e051ffcf7f8LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetA( details.PrimColour ); 
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}
//Ocarina of time - Witches' Ice Hairs
//case 0x0030b3fffffefa38LL:
//aRGB0: (Primitive    - 0           ) * Texel0       + Env
//aA0  : (Primitive    - 0           ) * Texel0       + Env
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0030b3fffffefa38LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.SetA( details.PrimColour );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);	
}

//MM - Inside Globe End <=== Fix Me
//case 0x00272c601510f37fLL:
//aRGB0: (Texel1       - Texel0      ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - 0           ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Shade        + 0
void BlendMode_0x00272c601510f37fLL (BLEND_MODE_ARGS)
{
	// XXXX Z Fighting....
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGB( details.PrimColour  );
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);		
	}
	else
#endif
	{
		// XXXX T1
		details.ColourAdjuster.SetAOpaque();
		details.ColourAdjuster.SetRGB( details.EnvColour  );
		sceGuTexEnvColor( details.PrimColour.GetColour() );
		sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	}
}

//Rare Logo
//case 0x0022aa031f0c93ffLL:
//aRGB0: (Texel1       - Texel0      ) * Env          + Texel0
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x0022aa031f0c93ffLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
	}
	else
#endif
	{
		details.ColourAdjuster.ModulateRGB( details.EnvColour );
		details.ColourAdjuster.SetA( details.EnvColour.ModulateA( details.PrimColour ) );
	}
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
}

//Yoshi Story - Smoke
//XXX Micrcode not ready yet.
//case 0x00161a6025fd2578LL:
//aRGB0: (Texel0       - Texel1      ) * Env_Alpha    + Texel1
//aA0  : (Texel0       - Texel1      ) * Env          + Texel1
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00161a6025fd2578LL (BLEND_MODE_ARGS)
{
	// XXXX placeholder implementation
#ifdef CHECK_FIRST_CYCLE
	if( num_cycles == 1 )
	{
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		details.ColourAdjuster.SetA( details.PrimColour  );
		sceGuTexEnvColor( details.PrimColour.GetColour() ); 
		sceGuTexFunc(GU_TFX_BLEND,GU_TCC_RGBA);
	}	
}

//ZELDA - OOT - Sage Medallions
//case 0x0030ec6155daed76LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env
//aA0  : (1            - 1           ) * 1            + 1
//aRGB1: (Primitive    - Env         ) * Texel0       + Env
//aA1  : (1            - 1           ) * 1            + 1
void BlendMode_0x0030ec6155daed76LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

//ZELDA - OOT - Bottle/Silver Scale/Rupees.
//case 0x00177e6035fcf378LL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + 
//aA0  : (0            - 0           ) * 0            + 
//aRGB1: (Primitive    - Env         ) * Combined     + 
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00177e6035fcf378LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

//ZELDA - OOT - Jellyfish tentacles
//case 0x0030fe035ffef3f8LL:
//aRGB0: (Primitive    - Env         ) * Texel0       + Env
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0030fe035ffef3f8LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		//details.RecolourTextureWhite;
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

//ZELDA - OOT - Tail Creatures inside Jabu-Jabu
//case 0x007197fffffcfe38LL:
//aRGB0: (CombAlp      - 0           ) * Primitive    + Texel0
//aA0  : (Texel0       - 0           ) * Primitive    + 0
//aRGB1: (0            - 0           ) * 0            + Combined
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x007197fffffcfe38LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

//ZELDA - OOT - Tentacle inside Jabu-Jabu
//case 0x00267e031ffcfffeLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0
//aA0  : (0            - 0           ) * 0            + 0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (0            - 0           ) * 0            + 1
void BlendMode_0x00267e031ffcfffeLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

//ZELDA - OOT - Barinade Jellyfish Energy (Incorrect Colour)
//case 0x0011fe60f5fff378LL:
//aRGB0: (Texel0       - 0           ) * Primitive    + 0
//aA0  : (0            - 0           ) * 0            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x0011fe60f5fff378LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

//ZELDA - OOT - BARINADE BOSS (Hidden Texture)
//case 0x001218c1f0c7fe00LL:
//aRGB0: (Texel0       - 0           ) * Shade        + 0
//aA0  : (Texel0       - 0           ) * Shade        + 0
//aRGB1: (1            - Combined    ) * Texel0       + Combined
//aA1  : (1            - Combined    ) * Texel0       + Combined
void BlendMode_0x001218c1f0c7fe00LL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

//ZELDA - OOT - Barinade Underside
//case 0x00272a603510937fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + 
//aA1  : (Combined     - 0           ) * Shade        + 0
void BlendMode_0x00272a603510937fLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

//ZELDA - OOT - Barinade Explosion
//case 0x00262a031f0c93ffLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0
//aA0  : (Texel1       - Texel0      ) * Env          + Texel0
//aRGB1: (Combined     - 0           ) * Primitive    + 0
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00262a031f0c93ffLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	}
}

//ZELDA - OOT
//Ganondorf's Spell Flare/Temple of Time Triforce Beam
//case 0x00262a60150ce37fLL:
//aRGB0: (Texel1       - Texel0      ) * Env_Alpha    + Texel0
//aA0  : (Texel1       - 1           ) * Env          + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00262a60150ce37fLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

// OOT - Gold Skulltula Mouth and Mystical Stones
//ZELDA - OOT - Spiritual Stones Setting(Kokiri, Goron, Zora)
//case 0x00177e6035fcfd78LL:
//aRGB0: (Texel0       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (0            - 0           ) * 0            + 1
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00177e6035fcfd78LL (BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGBA( c32::Gold );
	//details.ColourAdjuster.SetRGB( details.EnvColour.ModulateRGB( c32::Gold ) );
	sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
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
	details.InstallTexture = true;
	details.ColourAdjuster.SetRGB( details.EnvColour );
	sceGuTexEnvColor( details.PrimColour.GetColour() );
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);	
}

//ZELDA - OOT - Graveyard Beam
//case 0x00261a6035fce578LL:
//aRGB0: (Texel1       - Primitive   ) * Env_Alpha    + Texel0
//aA0  : (Texel0       - 1           ) * Env          + Texel1
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (0            - 0           ) * 0            + Combined
void BlendMode_0x00261a6035fce578LL(BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

// Pokemon Stadium 2 - Pokeball Light
//ZELDA - OOT - Din's Fire - Fireball
//case 0x00272c603510e37fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - 1           ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Shade        + 0
void BlendMode_0x00272c603510e37fLL(BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGB( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

//ZELDA - OOT
//Death Mountain Clouds (Adult Link)
//case 0x00272c603514e37fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel1       - 1           ) * 1            + Texel0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Env          + 0
void BlendMode_0x00272c603514e37fLL(BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if (num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		details.ColourAdjuster.SetRGBA( details.EnvColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

//ZELDA - OOT - SPIRIT SHOCKWAVE
//case 0x00271860350cff7fLL:
//aRGB0: (Texel1       - Primitive   ) * PrimLODFrac  + Texel0
//aA0  : (Texel0       - 0           ) * Shade        + 0
//aRGB1: (Primitive    - Env         ) * Combined     + Env
//aA1  : (Combined     - 0           ) * Primitive    + 0
void BlendMode_0x00271860350cff7fLL (BLEND_MODE_ARGS)
{
#ifdef CHECK_FIRST_CYCLE
	if(num_cycles == 1)
	{
		details.ColourAdjuster.SetRGB( details.PrimColour );
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGB);
	}
	else
#endif
	{
		//details.RecolourTextureWhite;
		sceGuTexFunc(GU_TFX_BLEND, GU_TCC_RGBA);
	}
}

//ZELDA - OOT - Nayru's Love
//Fixed by NintendoBoy
//case 0x00262a60350c937fLL
//aRGB0: (Texel1 - Primitive ) * Env_Alpha + Texel0
//aA0 : (Texel1 - Texel0 ) * Env + Texel0
//aRGB1: (Primitive - Env ) * Combined + Env
//aA1 : (Combined - 0 ) * Primitive + 0
void BlendMode_0x00262a60350c937fLL(BLEND_MODE_ARGS)
{
	details.ColourAdjuster.SetRGB( details.EnvColour );
	details.ColourAdjuster.ModulateA(details.PrimColour);
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGB);
}

/*
 We just need to declare correct installed blends with TextureInstalled = true;
 More commonly found with Second Texture not being found.
 */
void BlendMode_0x00127e00f003f200LL (BLEND_MODE_ARGS) {} // Super Bowling 64 Blue Stuff
void BlendMode_0x0026a0031ffc9378LL (BLEND_MODE_ARGS) {} // Paper Mario Bushes
void BlendMode_0x00ffe7ffffcd92c9LL (BLEND_MODE_ARGS) {} // Paper Mario Figure Transitions

OverrideBlendModeFn		LookupOverrideBlendModeFunction( u64 mux )
{
#ifndef DAEDALUS_PUBLIC_RELEASE
	if(!gGlobalPreferences.CustomBlendModes) return NULL;
#endif
	switch(mux)
	{
#define BLEND_MODE( x )		case (x):	return BlendMode_##x;

	BLEND_MODE(0x0010a2c3f00fd23fLL);
	BLEND_MODE(0x0010e5e0230b1d52LL);
	BLEND_MODE(0x0010fe043ffdfdfeLL);
	BLEND_MODE(0x001114a3f0fff638LL);
	BLEND_MODE(0x001114a7f3fffef8LL);
	BLEND_MODE(0x001147fffffffe38LL);
	BLEND_MODE(0x00115407f1ffca7eLL);
	BLEND_MODE(0x00117e03fffe7fffLL);
	BLEND_MODE(0x00117e045ffef3f8LL);
	BLEND_MODE(0x00117e60f5fff578LL);
	BLEND_MODE(0x00117e80f5fff438LL);
	BLEND_MODE(0x00119604ff5bfff8LL);
	BLEND_MODE(0x001197ffff5bfe38LL);
	BLEND_MODE(0x00119bffff5bfe38LL);
	BLEND_MODE(0x0011fe052ffd73f8LL);
	BLEND_MODE(0x0011fe2344fe7339LL);
	BLEND_MODE(0x0011fe2355fefd7eLL);
	BLEND_MODE(0x0011fe60f5fff378LL);//Barinadejellyfish(incorrectcolour)
	BLEND_MODE(0x0011ffff2ffd7c38LL);
	BLEND_MODE(0x0011fffffffffc38LL);
	BLEND_MODE(0x00121603ff5bfff8LL);
	BLEND_MODE(0x00121604f3ffff78LL);
	BLEND_MODE(0x001217ff3ffe7e38LL);
	BLEND_MODE(0x00121803ff5bfff8LL);
	BLEND_MODE(0x00121804f3ffff78LL);
	BLEND_MODE(0x001218245531feffLL);
	BLEND_MODE(0x00121824ff33ffffLL);//Tarzan
	BLEND_MODE(0x001218c1f0c7fe00LL);//HiddentextureforBarinade(??)
	BLEND_MODE(0x00121a03ff5bfff8LL);
	BLEND_MODE(0x0012680322fd7eb8LL);
	BLEND_MODE(0x001277ffffff9238LL);
	BLEND_MODE(0x00127e00f003f200LL);
	BLEND_MODE(0x00127e035ffe7fffLL);
	BLEND_MODE(0x00127e03fffe73f8LL);
	BLEND_MODE(0x00127e03fffe7fffLL);
	BLEND_MODE(0x00127e03ffffffffLL);
	BLEND_MODE(0x00127e0af3fff238LL);
	BLEND_MODE(0x00127e0bf1fffc7bLL);
	BLEND_MODE(0x00127e2433fdf8fcLL);
	BLEND_MODE(0x00127e2455fdf8fcLL);
	BLEND_MODE(0x00127e61f0fff83eLL);
	BLEND_MODE(0x00127eacf0fff238LL);
	BLEND_MODE(0x00127ec1f0fffa38LL);
	BLEND_MODE(0x00127ecbf0fffc3eLL);
	BLEND_MODE(0x00127eccf0fffc38LL);
	BLEND_MODE(0x00127fff3ffe7238LL);
	BLEND_MODE(0x00127fff3ffe7e3fLL);
	BLEND_MODE(0x00127fff3ffefe3fLL);
	BLEND_MODE(0x00127ffffffdfe3fLL);
	BLEND_MODE(0x001298043f15ffffLL);
	BLEND_MODE(0x0012fe2533fdf2f9LL);
	BLEND_MODE(0x0012fec8f2fdfe3bLL);
	BLEND_MODE(0x00147e045ffefbf8LL);
	BLEND_MODE(0x00147e2844fe793cLL);
	BLEND_MODE(0x00147e2844fe7b3dLL);
	BLEND_MODE(0x00157e2a33fdfcfeLL);
	BLEND_MODE(0x00157e602ffd77f8LL);
	BLEND_MODE(0x00157e80fffdfd7eLL);
	BLEND_MODE(0x00157fff2ffd7a38LL);
	BLEND_MODE(0x001598045ffedbf8LL);
	BLEND_MODE(0x0015982bff327f3fLL);
	BLEND_MODE(0x0015fe042ffd79fcLL);
	BLEND_MODE(0x0015fe2b33fdf6fbLL);
	BLEND_MODE(0x0015fe2bfffff3f9LL);
	BLEND_MODE(0x001616c0fffdf3f8LL);
	BLEND_MODE(0x00161a6025fd2578LL);
	BLEND_MODE(0x00167e2c33fdf6fbLL);
	BLEND_MODE(0x00167e6035fcf378LL);
	BLEND_MODE(0x00167e6035fcfd78LL);
	BLEND_MODE(0x00167e6035fcff7eLL);
	BLEND_MODE(0x001690031f0c93ffLL);
	BLEND_MODE(0x0017166035fcff78LL);
	BLEND_MODE(0x00171660f50d757dLL);
	BLEND_MODE(0x00171a2e3336ff7fLL);
	BLEND_MODE(0x00171c6035fd6578LL);
	BLEND_MODE(0x0017666025fd7f78LL);
	BLEND_MODE(0x00176c6035d8ed76LL);
	BLEND_MODE(0x00177e2efffefd7eLL);
	BLEND_MODE(0x00177e60350cf37fLL);//Heart container outer
	BLEND_MODE(0x00177e6035fcf378LL);//Bottle,SilverScaleandRupees(Semi-fixed)
	BLEND_MODE(0x00177e6035fcfd78LL);//SpiritualStoneSetting
	BLEND_MODE(0x00177e6035fcfd7eLL);
	BLEND_MODE(0x0017fe2f77fcf87cLL);
	BLEND_MODE(0x00209c03ff0f93ffLL);
	BLEND_MODE(0x0020a204ff0fffffLL);
	BLEND_MODE(0x0020a204ff13ffffLL);
	BLEND_MODE(0x0020a205f3fff738LL);
	BLEND_MODE(0x0020ac03ff0f93ffLL);
	BLEND_MODE(0x0020ac04ff0f92ffLL);
	BLEND_MODE(0x0020ac04ff0f93ffLL);
	BLEND_MODE(0x0020ac0aff0f93ffLL);
	BLEND_MODE(0x0020ac60350c937fLL);
	BLEND_MODE(0x0020fe04ff0ff7ffLL);
	BLEND_MODE(0x0020fe05f3fff738LL);
	BLEND_MODE(0x0021246015fc9378LL);
	BLEND_MODE(0x0021a6ac10fc9238LL);
	BLEND_MODE(0x0022aa031f0c93ffLL);
	BLEND_MODE(0x0022aa041f0c93ffLL);
	BLEND_MODE(0x0022ffff1ffcfa38LL);
	BLEND_MODE(0x00242c04ff0f93ffLL);
	BLEND_MODE(0x0025266015fc9378LL);
	BLEND_MODE(0x002527ff1ffc9238LL);
	BLEND_MODE(0x00257e041ffcfdfeLL);
	BLEND_MODE(0x00257e04fffcfd7eLL);
	BLEND_MODE(0x0025a86014fcb738LL);
	BLEND_MODE(0x0025fe0513fcff3fLL);
	BLEND_MODE(0x0025fe6014fcf73bLL);
	BLEND_MODE(0x00261a6035fce578LL);//GraveyardBeam
	BLEND_MODE(0x00262a031f0c93ffLL);//Barinadeexplosion
	BLEND_MODE(0x00262a04130cf37dLL);
	BLEND_MODE(0x00262a041f0c93ffLL);
	BLEND_MODE(0x00262a041f1093ffLL);
	BLEND_MODE(0x00262a041f5893f8LL);
	BLEND_MODE(0x00262a041ffc93f8LL);
	BLEND_MODE(0x00262a60150c937fLL);
	BLEND_MODE(0x00262a60150ce37fLL);//GanondorfSpell/TriforceLightinTempleofTime
	BLEND_MODE(0x00262a60150d157fLL);
	BLEND_MODE(0x00262a601510937fLL);
	BLEND_MODE(0x00262a6016fc9378LL);
	BLEND_MODE(0x00262a60350c937fLL);//Nayru'sLove
	BLEND_MODE(0x00262a60350ce37fLL);
	BLEND_MODE(0x00262a603510937fLL);
	BLEND_MODE(0x00267e031f0cfdffLL);
	BLEND_MODE(0x00267e031ffcfdf8LL);
	BLEND_MODE(0x00267e031ffcfffeLL);//Tentaclealphafix
	BLEND_MODE(0x00267e041f0cfdffLL);
	BLEND_MODE(0x00267e041f10fdffLL);
	BLEND_MODE(0x00267e041ffcfdf8LL);
	BLEND_MODE(0x00267e051ffcf7f8LL);
	BLEND_MODE(0x00267e051ffcfdf8LL);
	BLEND_MODE(0x00267e60350cf37fLL);
	BLEND_MODE(0x002698041f14ffffLL);
	BLEND_MODE(0x0026a0031ffc9378LL);
	BLEND_MODE(0x0026a0031ffc93f9LL);
	BLEND_MODE(0x0026a0041f0c93ffLL);
	BLEND_MODE(0x0026a0041f1093fbLL);
	BLEND_MODE(0x0026a0041f1093ffLL);
	BLEND_MODE(0x0026a0041f1493ffLL);
	BLEND_MODE(0x0026a0041ffc93e0LL);
	BLEND_MODE(0x0026a0041ffc93f8LL);
	BLEND_MODE(0x0026a0041ffc93fcLL);
	BLEND_MODE(0x0026a060150c937fLL);
	BLEND_MODE(0x0026a1ff1ffc9238LL);
	BLEND_MODE(0x0026a1ff1ffc923cLL);
	BLEND_MODE(0x0026e4041ffcfffcLL);
	BLEND_MODE(0x0026ea041f10ffffLL);
	BLEND_MODE(0x002712041f0cffffLL);
	BLEND_MODE(0x002714041f0cffffLL);
	BLEND_MODE(0x00271860350cff7fLL);//Spirit shockwave
	BLEND_MODE(0x002722041f0cffffLL);
	BLEND_MODE(0x00272a60150c937fLL);
	BLEND_MODE(0x00272a603510937fLL);//Barinadeunderside
	BLEND_MODE(0x00272c031f0c93ffLL);
	BLEND_MODE(0x00272c031f1093ffLL);
	BLEND_MODE(0x00272c0415fc93feLL);
	BLEND_MODE(0x00272c041f0c93ffLL);
	BLEND_MODE(0x00272c041f1093ffLL);
	BLEND_MODE(0x00272c041ffc93f8LL);
	BLEND_MODE(0x00272c60150c937fLL);
	BLEND_MODE(0x00272c60150ce37fLL);
	BLEND_MODE(0x00272c601510f37fLL);
	BLEND_MODE(0x00272c6015fc9378LL);
	BLEND_MODE(0x00272c60340c933fLL);//Castle light
	BLEND_MODE(0x00272c603410f33fLL);
	BLEND_MODE(0x00272c60350c937fLL);//SpiritualStonegems
	BLEND_MODE(0x00272c60350ce37fLL);
	BLEND_MODE(0x00272c60350cf37fLL);
	BLEND_MODE(0x00272c603510937fLL);
	BLEND_MODE(0x00272c603510e37fLL);//Din'sFirefireball
	BLEND_MODE(0x00272c603514937fLL);
	BLEND_MODE(0x00272c603514e37fLL);//DeathMountainclouds(Red)
	BLEND_MODE(0x00272c6035a0937fLL);
	BLEND_MODE(0x00272c80350cf37fLL);
	BLEND_MODE(0x00272e041f0c93ffLL);
	BLEND_MODE(0x00273c60f514e37fLL);
	BLEND_MODE(0x00277e0413fcff3fLL);
	BLEND_MODE(0x00277e041f0cf7ffLL);
	BLEND_MODE(0x00277e041ffcfdf8LL);
	BLEND_MODE(0x00277e6035fcf778LL);//Triforce yellow
	BLEND_MODE(0x00277fff1ffcf438LL);
	BLEND_MODE(0x0027fe041ffcfdfeLL);
	BLEND_MODE(0x00309661552efb7dLL);
	BLEND_MODE(0x00309861550eff4fLL);
	BLEND_MODE(0x00309e045ffefdf8LL);//Kirby
	BLEND_MODE(0x0030abff5ffe9238LL);
	BLEND_MODE(0x0030b2045ffefff8LL);//HorsedustatLonLonRanch
	BLEND_MODE(0x0030b26144664924LL);
	BLEND_MODE(0x0030b2615566db6dLL);
	BLEND_MODE(0x0030b3ff5f12da3fLL);
	BLEND_MODE(0x0030b3ff5ffeda38LL);//ZoraSplashes
	BLEND_MODE(0x0030b3fffffefa38LL);
	BLEND_MODE(0x0030ec045fdaedf6LL);
	BLEND_MODE(0x0030ec6155daed76LL);//SageMedallionsandEgg
	BLEND_MODE(0x0030f861fff393c9LL);
	BLEND_MODE(0x0030fe035ffef3f8LL);//Smalljellyfishtentacles
	BLEND_MODE(0x0030fe045f0ef3ffLL);
	BLEND_MODE(0x0030fe045ffef3f8LL);//Unknown(??)
	BLEND_MODE(0x0030fe045ffefbf8LL);
	BLEND_MODE(0x0030fe045ffefdf8LL);
	BLEND_MODE(0x0030fe045ffefdfeLL);
	BLEND_MODE(0x0030fe045ffeff3fLL);
	BLEND_MODE(0x003135ff5f0ada3fLL);
	BLEND_MODE(0x00317fff5ffef438LL);
	BLEND_MODE(0x00322bff5f0e923fLL);
	BLEND_MODE(0x00327e6411fcf87cLL);
	BLEND_MODE(0x00327ecbf0fffc3eLL);
	BLEND_MODE(0x00327feffffff638LL);
	BLEND_MODE(0x00347e04fffcfdfeLL);
	BLEND_MODE(0x00357e6a11fcf67bLL);
	BLEND_MODE(0x00357e6a11fcfc7eLL);
	BLEND_MODE(0x00373c6e117b9fcfLL);
	BLEND_MODE(0x00377e041ffcf3f8LL);
	BLEND_MODE(0x00377e041ffcf7f8LL);
	BLEND_MODE(0x00377e041ffcfdf8LL);
	BLEND_MODE(0x004093ffff0dfe3fLL);
	BLEND_MODE(0x0040b467f0fffe3eLL);
	BLEND_MODE(0x0040fe8155fef379LL);
	BLEND_MODE(0x0040fe8155fef97cLL);
	BLEND_MODE(0x0040fe8155fefd7eLL);
	BLEND_MODE(0x004193ffff0ffe3fLL);
	BLEND_MODE(0x00457fff3ffcfe3fLL);
	BLEND_MODE(0x005094023f15ffffLL);
	BLEND_MODE(0x0050d2a133a5b6dbLL);
	BLEND_MODE(0x0050fe6b20fd7c3dLL);
	BLEND_MODE(0x0050fea144fe7339LL);
	BLEND_MODE(0x00517e023f55ffffLL);
	BLEND_MODE(0x00541aa83335feffLL);
	BLEND_MODE(0x00547ea833fdf2f9LL);
	BLEND_MODE(0x0055a68730fd923eLL);
	BLEND_MODE(0x0055fe041ffcf3f8LL);
	BLEND_MODE(0x005632801ffcfff8LL);
	BLEND_MODE(0x00567eac11fcf279LL);
	BLEND_MODE(0x006093ff3f0dfe3fLL);
	BLEND_MODE(0x0060fe043ffdf3f8LL);
	BLEND_MODE(0x00619ac31137f7fbLL);
	BLEND_MODE(0x0061e6c311cf9fcfLL);
	BLEND_MODE(0x0061fe041ffcfdfeLL);
	BLEND_MODE(0x0061fec311fcf67bLL);
	BLEND_MODE(0x0062fe043f15f9ffLL);
	BLEND_MODE(0x00671603fffcff78LL);
	BLEND_MODE(0x00671604fffcff78LL);
	BLEND_MODE(0x007197fffffcfe38LL);//TailcreaturesinsideJabu-Jabu
	BLEND_MODE(0x0071fee311fcf279LL);
	BLEND_MODE(0x00f09a61501374ffLL);
	BLEND_MODE(0x00f5fa67f50c997fLL);
	BLEND_MODE(0x00f7ffeffffcf67bLL);
	BLEND_MODE(0x00ff97ffff2cfa7dLL);
	BLEND_MODE(0x00ffabffff0d92ffLL);
	BLEND_MODE(0x00ffabfffffc9238LL);
	BLEND_MODE(0x00ffadfffffd9238LL);
	BLEND_MODE(0x00ffd5fffffcf238LL);
	BLEND_MODE(0x00ffe7ffffcd92c9LL);
	BLEND_MODE(0x00ffe7ffffcf9fcfLL);
	BLEND_MODE(0x00ffedffffd996cbLL);
	BLEND_MODE(0x00fffe04f3fcf378LL);
	BLEND_MODE(0x00ffffffff09f63fLL);
	BLEND_MODE(0x00fffffffffcfa7dLL);
	BLEND_MODE(0x00129bfffffdf638LL);//Road Rush64
	default:
		return BlendMode_Generic;   // Basic generic blenmode


#undef BLEND_MODE
	}

	return NULL;
}
