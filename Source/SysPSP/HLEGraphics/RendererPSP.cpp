#include "stdafx.h"
#include "RendererPSP.h"

#include <pspgu.h>

#include "Combiner/BlendConstant.h"
#include "Combiner/CombinerTree.h"
#include "Combiner/RenderSettings.h"
#include "Core/ROM.h"
#include "Debug/Dump.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/NativeTexture.h"
#include "HLEGraphics/CachedTexture.h"
#include "HLEGraphics/DLDebug.h"
#include "HLEGraphics/RDPStateManager.h"
#include "HLEGraphics/TextureCache.h"
#include "Math/MathUtil.h"
#include "OSHLE/ultra_gbi.h"
#include "Utility/IO.h"
#include "Utility/Profiler.h"


//Draw normal filled triangles
#define DRAW_MODE GU_TRIANGLES
//Draw lines
//Also enable clean scene in advanced menu //Corn
//#define DRAW_MODE GU_LINE_STRIP

// FIXME - surely these should be defined by a system header? Or GU_TRUE etc?
#define GL_TRUE                           1
#define GL_FALSE                          0

BaseRenderer * gRenderer    = NULL;
RendererPSP  * gRendererPSP = NULL;

extern void InitBlenderMode( u32 blender );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST

// General blender used for Blend Explorer when debuging Dlists //Corn
DebugBlendSettings gDBlend;

static const u32 kPlaceholderTextureWidth  = 16;
static const u32 kPlaceholderTextureHeight = 16;
static const u32 kPlaceholderSize = kPlaceholderTextureWidth * kPlaceholderTextureHeight;

ALIGNED_GLOBAL(u32,       gWhiteTexture[kPlaceholderSize], DATA_ALIGN);
ALIGNED_GLOBAL(u32, gPlaceholderTexture[kPlaceholderSize], DATA_ALIGN);
ALIGNED_GLOBAL(u32,    gSelectedTexture[kPlaceholderSize], DATA_ALIGN);


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
	switch( gDBlend.ForceRGB ) \
	{ \
		case 1: details.ColourAdjuster.SetRGB( c32::White ); break; \
		case 2: details.ColourAdjuster.SetRGB( c32::Black ); break; \
		case 3: details.ColourAdjuster.SetRGB( c32::Red ); break; \
		case 4: details.ColourAdjuster.SetRGB( c32::Green ); break; \
		case 5: details.ColourAdjuster.SetRGB( c32::Blue ); break; \
		case 6: details.ColourAdjuster.SetRGB( c32::Magenta ); break; \
		case 7: details.ColourAdjuster.SetRGB( c32::Gold ); break; \
	} \
	switch( gDBlend.SetRGB ) \
	{ \
		case 1: details.ColourAdjuster.SetRGB( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.SetRGB( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.SetRGB( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.SetRGB( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	switch( gDBlend.SetA ) \
	{ \
		case 1: details.ColourAdjuster.SetA( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.SetA( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.SetA( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.SetA( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	switch( gDBlend.SetRGBA ) \
	{ \
		case 1: details.ColourAdjuster.SetRGBA( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.SetRGBA( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.SetRGBA( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.SetRGBA( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	switch( gDBlend.ModRGB ) \
	{ \
		case 1: details.ColourAdjuster.ModulateRGB( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.ModulateRGB( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.ModulateRGB( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.ModulateRGB( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	switch( gDBlend.ModA ) \
	{ \
		case 1: details.ColourAdjuster.ModulateA( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.ModulateA( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.ModulateA( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.ModulateA( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	switch( gDBlend.ModRGBA ) \
	{ \
		case 1: details.ColourAdjuster.ModulateRGBA( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.ModulateRGBA( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.ModulateRGBA( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.ModulateRGBA( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	switch( gDBlend.SubRGB ) \
	{ \
		case 1: details.ColourAdjuster.SubtractRGB( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.SubtractRGB( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.SubtractRGB( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.SubtractRGB( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	switch( gDBlend.SubA ) \
	{ \
		case 1: details.ColourAdjuster.SubtractA( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.SubtractA( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.SubtractA( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.SubtractA( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	switch( gDBlend.SubRGBA ) \
	{ \
		case 1: details.ColourAdjuster.SubtractRGBA( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.SubtractRGBA( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.SubtractRGBA( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.SubtractRGBA( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	if( gDBlend.AOpaque ) details.ColourAdjuster.SetAOpaque(); \
	switch( gDBlend.sceENV ) \
	{ \
		case 1: sceGuTexEnvColor( details.EnvColour.GetColour() ); break; \
		case 2: sceGuTexEnvColor( details.PrimColour.GetColour() ); break; \
	} \
	details.InstallTexture = gDBlend.TexInstall; \
	sceGuTexFunc( PSPtxtFunc[ (gDBlend.TXTFUNC >> 1) % 6 ], PSPtxtA[ gDBlend.TXTFUNC & 1 ] ); \
}

#endif // DAEDALUS_DEBUG_DISPLAYLIST

RendererPSP::RendererPSP()
{
	//
	//	Set up RGB = T0, A = T0
	//
	mCopyBlendStates = new CBlendStates;
	{
		CAlphaRenderSettings *	alpha_settings( new CAlphaRenderSettings( "Copy" ) );
		CRenderSettingsModulate *	colour_settings( new CRenderSettingsModulate( "Copy" ) );

		alpha_settings->AddTermTexel0();
		colour_settings->AddTermTexel0();

		mCopyBlendStates->SetAlphaSettings( alpha_settings );
		mCopyBlendStates->AddColourSettings( colour_settings );
	}

	//
	//	Set up RGB = Diffuse, A = Diffuse
	//
	mFillBlendStates = new CBlendStates;
	{
		CAlphaRenderSettings *	alpha_settings( new CAlphaRenderSettings( "Fill" ) );
		CRenderSettingsModulate *	colour_settings( new CRenderSettingsModulate( "Fill" ) );

		alpha_settings->AddTermConstant( new CBlendConstantExpressionValue( BC_SHADE ) );
		colour_settings->AddTermConstant(  new CBlendConstantExpressionValue( BC_SHADE ) );

		mFillBlendStates->SetAlphaSettings( alpha_settings );
		mFillBlendStates->AddColourSettings( colour_settings );
	}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	memset( gWhiteTexture, 0xff, sizeof(gWhiteTexture) );

	memset( &gDBlend.TexInstall, 0, sizeof(gDBlend) );
	gDBlend.TexInstall = 1;

	u32	texel_idx = 0;
	const u32	COL_MAGENTA = c32::Magenta.GetColour();
	const u32	COL_GREEN   = c32::Green.GetColour();
	const u32	COL_BLACK   = c32::Black.GetColour();
	for(u32 y = 0; y < kPlaceholderTextureHeight; ++y)
	{
		for(u32 x = 0; x < kPlaceholderTextureWidth; ++x)
		{
			gPlaceholderTexture[ texel_idx ] = ((x&1) == (y&1)) ? COL_MAGENTA : COL_BLACK;
			gSelectedTexture[ texel_idx ]    = ((x&1) == (y&1)) ? COL_GREEN   : COL_BLACK;

			texel_idx++;
		}
	}
#endif
}

RendererPSP::~RendererPSP()
{
	delete mFillBlendStates;
	delete mCopyBlendStates;
}

void RendererPSP::RestoreRenderStates()
{
	// Initialise the device to our default state

	// No fog
	sceGuDisable(GU_FOG);

	// We do our own culling
	sceGuDisable(GU_CULL_FACE);

	// But clip our tris please (looks better in far field see Aerogauge)
	sceGuEnable(GU_CLIP_PLANES);
	//sceGuDisable(GU_CLIP_PLANES);

	//u32 width, height;
	//CGraphicsContext::Get()->GetScreenSize(&width, &height);

	//This was breaking Glover's sky and Rocket Robot's right/left sides of the screen when in un/scaled mode
	//I think the problem was that GetScreenSize for PSP only returns 480x240, we should get the current screen size?
	//Or get scissor.left/right/top/bottom, then pass them trough ConvertN64ToScreen and then use those to set PSP scissors
	//sceGuScissor(0,0, width,height);

	sceGuEnable(GU_SCISSOR_TEST);

	// We do our own lighting
	sceGuDisable(GU_LIGHTING);

	sceGuAlphaFunc(GU_GEQUAL, 0x04, 0xff );
	sceGuEnable(GU_ALPHA_TEST);

	sceGuDisable( GU_BLEND );

	// Default is ZBuffer disabled
	sceGuDepthMask(GL_TRUE);	// GL_TRUE to disable z-writes
	sceGuDepthFunc(GU_GEQUAL);		// GEQUAL?
	sceGuDisable(GU_DEPTH_TEST);

	// Initialise all the renderstate to our defaults.
	sceGuShadeModel(GU_SMOOTH);

	sceGuTexEnvColor( c32::White.GetColour() );
	sceGuTexOffset(0.0f,0.0f);

	//sceGuFog(near,far,mFogColour);
	// Texturing stuff
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
	//sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexWrap(GU_REPEAT,GU_REPEAT);

	//sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &gMatrixIdentity ) );
	sceGuSetMatrix( GU_VIEW, reinterpret_cast< const ScePspFMatrix4 * >( &gMatrixIdentity ) );
	sceGuSetMatrix( GU_MODEL, reinterpret_cast< const ScePspFMatrix4 * >( &gMatrixIdentity ) );
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
void RendererPSP::ResetDebugState()
{
	BaseRenderer::ResetDebugState();
	mRecordedCombinerStates.clear();
}
#endif

RendererPSP::SBlendStateEntry RendererPSP::LookupBlendState( u64 mux, bool two_cycles )
{
	#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DAEDALUS_PROFILE( "RendererPSP::LookupBlendState" );
	mRecordedCombinerStates.insert( mux );
#endif

	REG64 key;
	key._u64 = mux;

	// Top 8 bits are never set - use the very top one to differentiate between 1/2 cycles
	key._u32_1 |= (two_cycles << 31);

	BlendStatesMap::const_iterator	it( mBlendStatesMap.find( key._u64 ) );
	if( it != mBlendStatesMap.end() )
	{
		return it->second;
	}

	// Blendmodes with Inexact blends either get an Override blend or a Default blend (GU_TFX_MODULATE)
	// If its not an Inexact blend then we check if we need to Force a blend mode none the less// Salvy
	//
	SBlendStateEntry entry;
	CCombinerTree tree( mux, two_cycles );
	entry.States = tree.GetBlendStates();

	if( entry.States->IsInexact() )
	{
		entry.OverrideFunction = LookupOverrideBlendModeInexact( mux );
	}
	else
	{
		// This is for non-inexact blends, errg hacks and such to be more precise
		entry.OverrideFunction = LookupOverrideBlendModeForced( mux );
	}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	printf( "Adding %08x%08x - %d cycles - %s\n", u32(mux>>32), u32(mux), two_cycles ? 2 : 1, entry.States->IsInexact() ?  IsCombinerStateDefault(mux) ? "Inexact(Default)" : "Inexact(Override)" : entry.OverrideFunction==NULL ? "Auto" : "Forced");
#endif

	//Add blend mode to the Blend States Map
	mBlendStatesMap[ key._u64 ] = entry;

	return entry;
}

void RendererPSP::RenderTriangles( DaedalusVtx * p_vertices, u32 num_vertices, bool disable_zbuffer )
{
	if( mTnL.Flags.Texture )
	{
		UpdateTileSnapshots( mTextureTile );

		const CNativeTexture * texture = mBoundTexture[0];

		if( texture && (mTnL.Flags._u32 & (TNL_LIGHT|TNL_TEXGEN)) != (TNL_LIGHT|TNL_TEXGEN) )
		{
			float scale_x = texture->GetScaleX();
			float scale_y = texture->GetScaleY();

			// Hack to fix the sun in Zelda OOT/MM
			if( g_ROM.ZELDA_HACK && (gRDPOtherMode.L == 0x0c184241) )	 //&& ti.GetFormat() == G_IM_FMT_I && (ti.GetWidth() == 64)
			{
				scale_x *= 0.5f;
				scale_y *= 0.5f;
			}
			sceGuTexOffset( -mTileTopLeft[ 0 ].s * scale_x / 4.f,
							-mTileTopLeft[ 0 ].t * scale_y / 4.f );
			sceGuTexScale( scale_x, scale_y );
		}
		else
		{
			sceGuTexOffset( 0.0f, 0.0f );
			sceGuTexScale( 1.0f, 1.0f );
		}
	}

	RenderUsingCurrentBlendMode( p_vertices, num_vertices, DRAW_MODE, GU_TRANSFORM_3D, disable_zbuffer );
}

inline void RendererPSP::RenderFog( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_flags )
{
	//This will render a second pass on triangles that are fog enabled to blend in the fog color as a function of depth(alpha) //Corn
	//
	//if( gRDPOtherMode.c1_m1a==3 || gRDPOtherMode.c1_m2a==3 || gRDPOtherMode.c2_m1a==3 || gRDPOtherMode.c2_m2a==3 )
	{
		//sceGuShadeModel(GU_SMOOTH);
		sceGuDepthFunc(GU_EQUAL);	//Make sure to only blend on pixels that has been rendered on first pass //Corn
		sceGuDepthMask(GL_TRUE);	//GL_TRUE to disable z-writes, no need to write to zbuffer for second pass //Corn
		sceGuEnable(GU_BLEND);
		sceGuDisable(GU_TEXTURE_2D);	//Blend triangle without a texture
		sceGuDisable(GU_ALPHA_TEST);
		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

		u32 FogColor = mFogColour.GetColour();

		//Copy fog color to vertices
		for(u32 i = 0 ; i < num_vertices ; i++)
		{
			u32 alpha = p_vertices[i].Colour.GetColour() & 0xFF000000;
			p_vertices[i].Colour = (c32)(alpha | FogColor);
		}

		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );

		sceGuDepthFunc(GU_GEQUAL);	//Restore default depth function
	}
}

void RendererPSP::RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_mode, bool disable_zbuffer )
{
	static bool	ZFightingEnabled( false );

	DAEDALUS_PROFILE( "RendererPSP::RenderUsingCurrentBlendMode" );

	if ( disable_zbuffer )
	{
		sceGuDisable(GU_DEPTH_TEST);
		sceGuDepthMask( GL_TRUE );	// GL_TRUE to disable z-writes
	}
	else
	{
		// Fixes Zfighting issues we have on the PSP.
		if( gRDPOtherMode.zmode == 3 )
		{
			if( !ZFightingEnabled )
			{
				ZFightingEnabled = true;
				sceGuDepthRange(65535,80);
			}
		}
		else if( ZFightingEnabled )
		{
			ZFightingEnabled = false;
			sceGuDepthRange(65535,0);
		}

		// Enable or Disable ZBuffer test
		if ( (mTnL.Flags.Zbuffer & gRDPOtherMode.z_cmp) | gRDPOtherMode.z_upd )
		{
			sceGuEnable(GU_DEPTH_TEST);
		}
		else
		{
			sceGuDisable(GU_DEPTH_TEST);
		}

		// GL_TRUE to disable z-writes
		sceGuDepthMask( gRDPOtherMode.z_upd ? GL_FALSE : GL_TRUE );
	}

	// Initiate Texture Filter
	//
	// G_TF_AVERAGE : 1, G_TF_BILERP : 2 (linear)
	// G_TF_POINT   : 0 (nearest)
	//
	if( (gRDPOtherMode.text_filt != G_TF_POINT) | (gGlobalPreferences.ForceLinearFilter) )
	{
		sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	}
	else
	{
		sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	}

	u32 cycle_mode = gRDPOtherMode.cycle_type;

	// Initiate Blender
	//
	if(cycle_mode < CYCLE_COPY && gRDPOtherMode.force_bl)
	{
		InitBlenderMode(gRDPOtherMode.blender);
	}
	else
	{
		sceGuDisable( GU_BLEND );
	}

	// Initiate Alpha test
	//
	if( (gRDPOtherMode.alpha_compare == G_AC_THRESHOLD) && !gRDPOtherMode.alpha_cvg_sel )
	{
		u8 alpha_threshold = mBlendColour.GetA();
		sceGuAlphaFunc( (alpha_threshold | g_ROM.ALPHA_HACK) ? GU_GEQUAL : GU_GREATER, alpha_threshold, 0xff);
		sceGuEnable(GU_ALPHA_TEST);
	}
	else if (gRDPOtherMode.cvg_x_alpha)
	{
		// Going over 0x70 breaks OOT, but going lesser than that makes lines on games visible...ex: Paper Mario.
		// Also going over 0x30 breaks the birds in Tarzan :(. Need to find a better way to leverage this.
		sceGuAlphaFunc(GU_GREATER, 0x70, 0xff);
		sceGuEnable(GU_ALPHA_TEST);
	}
	else
	{
		sceGuDisable(GU_ALPHA_TEST);
	}

	SBlendStateEntry		blend_entry;

	switch ( cycle_mode )
	{
		case CYCLE_COPY:		blend_entry.States = mCopyBlendStates; break;
		case CYCLE_FILL:		blend_entry.States = mFillBlendStates; break;
		case CYCLE_1CYCLE:		blend_entry = LookupBlendState( mMux, false ); break;
		case CYCLE_2CYCLE:		blend_entry = LookupBlendState( mMux, true ); break;
	}

	u32 render_flags( GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | render_mode );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	// Used for Blend Explorer, or Nasty texture
	//
	if( DebugBlendmode( p_vertices, num_vertices, triangle_mode, render_flags, mMux ) )
		return;
#endif

	// This check is for inexact blends which were handled either by a custom blendmode or auto blendmode thing
	//
	if( blend_entry.OverrideFunction != NULL )
	{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		// Used for dumping mux and highlight inexact blend
		//
		DebugMux( blend_entry.States, p_vertices, num_vertices, triangle_mode, render_flags, mMux );
#endif

		// Local vars for now
		SBlendModeDetails details;

		details.EnvColour = mEnvColour;
		details.PrimColour = mPrimitiveColour;
		details.InstallTexture = true;
		details.ColourAdjuster.Reset();

		blend_entry.OverrideFunction( details );

		bool installed_texture( false );

		if( details.InstallTexture )
		{
			u32 texture_idx = g_ROM.T1_HACK ? 1 : 0;

			if( mBoundTexture[ texture_idx ] )
			{
				mBoundTexture[ texture_idx ]->InstallTexture();

				sceGuTexWrap( mTexWrap[ texture_idx ].u, mTexWrap[ texture_idx ].v );

				installed_texture = true;
			}
		}

		// If no texture was specified, or if we couldn't load it, clear it out
		if( !installed_texture )
		{
			sceGuDisable( GU_TEXTURE_2D );
		}

		if ( mTnL.Flags.Fog )
		{
			DaedalusVtx * p_FogVtx = static_cast<DaedalusVtx *>(sceGuGetMemory(num_vertices * sizeof(DaedalusVtx)));
			memcpy( p_FogVtx, p_vertices, num_vertices * sizeof( DaedalusVtx ) );
			details.ColourAdjuster.Process( p_vertices, num_vertices );
			sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
			RenderFog( p_FogVtx, num_vertices, triangle_mode, render_flags );
		}
		else
		{
			details.ColourAdjuster.Process( p_vertices, num_vertices );
			sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
		}
	}
	else if( blend_entry.States != NULL )
	{
		RenderUsingRenderSettings( blend_entry.States, p_vertices, num_vertices, triangle_mode, render_flags );
	}
	else
	{
		#ifdef DAEDALUS_DEBUG_CONSOLE
		// Set default states
		DAEDALUS_ERROR( "Unhandled blend mode" );
		#endif
		sceGuDisable( GU_TEXTURE_2D );
		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
	}
}

void RendererPSP::RenderUsingRenderSettings( const CBlendStates * states, DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_flags)
{
	DAEDALUS_PROFILE( "RendererPSP::RenderUsingRenderSettings" );

	const CAlphaRenderSettings *	alpha_settings( states->GetAlphaSettings() );

	SRenderState	state;

	state.Vertices = p_vertices;
	state.NumVertices = num_vertices;
	state.PrimitiveColour = mPrimitiveColour;
	state.EnvironmentColour = mEnvColour;

	//Avoid copying vertices twice if we already save a copy to render fog //Corn
	DaedalusVtx * p_FogVtx( mVtx_Save );
	if( mTnL.Flags.Fog )
	{
		p_FogVtx = static_cast<DaedalusVtx *>(sceGuGetMemory(num_vertices * sizeof(DaedalusVtx)));
		memcpy( p_FogVtx, p_vertices, num_vertices * sizeof( DaedalusVtx ) );
	}
	else if( states->GetNumStates() > 1 )
	{
		memcpy( mVtx_Save, p_vertices, num_vertices * sizeof( DaedalusVtx ) );
	}

	for( u32 i = 0; i < states->GetNumStates(); ++i )
	{
		const CRenderSettings *		settings( states->GetColourSettings( i ) );

		bool install_texture0( settings->UsesTexture0() || alpha_settings->UsesTexture0() );
		bool install_texture1( settings->UsesTexture1() || alpha_settings->UsesTexture1() );

		SRenderStateOut out;

		memset( &out, 0, sizeof( out ) );

		settings->Apply( install_texture0 || install_texture1, state, out );
		alpha_settings->Apply( install_texture0 || install_texture1, state, out );

		// TODO: this nobbles the existing diffuse colour on each pass. Need to use a second buffer...
		if( i > 0 )
		{
			memcpy( p_vertices, p_FogVtx, num_vertices * sizeof( DaedalusVtx ) );
		}

		if(out.VertexExpressionRGB != NULL)
		{
			out.VertexExpressionRGB->ApplyExpressionRGB( state );
		}
		if(out.VertexExpressionA != NULL)
		{
			out.VertexExpressionA->ApplyExpressionAlpha( state );
		}

		bool installed_texture = false;

		u32 texture_idx = 0;

		if(install_texture0 || install_texture1)
		{
			u32	tfx( GU_TFX_MODULATE );
			switch( out.BlendMode )
			{
			case PBM_MODULATE:		tfx = GU_TFX_MODULATE; break;
			case PBM_REPLACE:		tfx = GU_TFX_REPLACE; break;
			case PBM_BLEND:			tfx = GU_TFX_BLEND; sceGuTexEnvColor( out.TextureFactor.GetColour() ); break;
			}

			sceGuTexFunc( tfx, out.BlendAlphaMode == PBAM_RGB ? GU_TCC_RGB :  GU_TCC_RGBA );

			if( g_ROM.T1_HACK )
			{
				// NB if install_texture0 and install_texture1 are both set, 1 wins out
				texture_idx = install_texture1;

				const CNativeTexture * texture1 = mBoundTexture[ 1 ];

				if( install_texture1 && texture1 && mTnL.Flags.Texture && (mTnL.Flags._u32 & (TNL_LIGHT|TNL_TEXGEN)) != (TNL_LIGHT|TNL_TEXGEN) )
				{
					float scale_x = texture1->GetScaleX();
					float scale_y = texture1->GetScaleY();

					sceGuTexOffset( -mTileTopLeft[ 1 ].s * scale_x / 4.f,
									-mTileTopLeft[ 1 ].t * scale_y / 4.f );
					sceGuTexScale( scale_x, scale_y );
				}
			}
			else
			{
				// NB if install_texture0 and install_texture1 are both set, 0 wins out
				texture_idx = install_texture0 ? 0 : 1;
			}

			CRefPtr<CNativeTexture> texture;

			if(out.MakeTextureWhite)
			{
				TextureInfo white_ti = mBoundTextureInfo[ texture_idx ];
				white_ti.SetWhite(true);
				texture = CTextureCache::Get()->GetOrCreateTexture( white_ti );
			}
			else
			{
				texture = mBoundTexture[ texture_idx ];
			}

			if(texture != NULL)
			{
				texture->InstallTexture();
				installed_texture = true;
			}
		}

		// If no texture was specified, or if we couldn't load it, clear it out
		if( !installed_texture ) sceGuDisable(GU_TEXTURE_2D);

		sceGuTexWrap( mTexWrap[texture_idx].u, mTexWrap[texture_idx].v );

		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );

		if ( mTnL.Flags.Fog )
		{
			RenderFog( p_FogVtx, num_vertices, triangle_mode, render_flags );
		}
	}
}

void RendererPSP::TexRect( u32 tile_idx, const v2 & xy0, const v2 & xy1, TexCoord st0, TexCoord st1 )
{
	mTnL.Flags.Fog = 0;	//For now we force fog off for textrect, normally it should be fogged when depth_source is set //Corn

	UpdateTileSnapshots( tile_idx );

	// NB: we have to do this after UpdateTileSnapshot, as it set up mTileTopLeft etc.
	PrepareTexRectUVs(&st0, &st1);

	// Convert fixed point uvs back to floating point format.
	// NB: would be nice to pass these as s16 ints, and use GU_TEXTURE_16BIT
	v2 uv0( (float)st0.s / 32.f, (float)st0.t / 32.f );
	v2 uv1( (float)st1.s / 32.f, (float)st1.t / 32.f );

	v2 screen0;
	v2 screen1;
	if( gGlobalPreferences.ViewportType == VT_FULLSCREEN_HD )
	{
		screen0.x = roundf( roundf( HD_SCALE * xy0.x ) * mN64ToScreenScale.x + 59 );	//59 in translate is an ugly hack that only work on 480x272 display//Corn
		screen0.y = roundf( roundf( xy0.y )            * mN64ToScreenScale.y + mN64ToScreenTranslate.y );

		screen1.x = roundf( roundf( HD_SCALE * xy1.x ) * mN64ToScreenScale.x + 59 ); //59 in translate is an ugly hack that only work on 480x272 display//Corn
		screen1.y = roundf( roundf( xy1.y )            * mN64ToScreenScale.y + mN64ToScreenTranslate.y );
	}
	else
	{
		ConvertN64ToScreen( xy0, screen0 );
		ConvertN64ToScreen( xy1, screen1 );
	}

	DL_PF( "    Screen:  %.1f,%.1f -> %.1f,%.1f", screen0.x, screen0.y, screen1.x, screen1.y );
	DL_PF( "    Texture: %.1f,%.1f -> %.1f,%.1f", uv0.x, uv0.y, uv1.x, uv1.y );

	const f32 depth = gRDPOtherMode.depth_source ? mPrimDepth : 0.0f;

#if 1	//1->SPRITE, 0->STRIP
	DaedalusVtx * p_vertices = static_cast<DaedalusVtx *>(sceGuGetMemory(2 * sizeof(DaedalusVtx)));

	p_vertices[0].Position.x = screen0.x;
	p_vertices[0].Position.y = screen0.y;
	p_vertices[0].Position.z = depth;
	p_vertices[0].Colour = c32(0xffffffff);
	p_vertices[0].Texture.x = uv0.x;
	p_vertices[0].Texture.y = uv0.y;

	p_vertices[1].Position.x = screen1.x;
	p_vertices[1].Position.y = screen1.y;
	p_vertices[1].Position.z = depth;
	p_vertices[1].Colour = c32(0xffffffff);
	p_vertices[1].Texture.x = uv1.x;
	p_vertices[1].Texture.y = uv1.y;

	RenderUsingCurrentBlendMode( p_vertices, 2, GU_SPRITES, GU_TRANSFORM_2D, gRDPOtherMode.depth_source ? false : true );
#else
	//	To be used with TRIANGLE_STRIP, which requires 40% less verts than TRIANGLE
	//	For reference for future ports and if SPRITES( which uses %60 less verts than TRIANGLE) causes issues
	DaedalusVtx * p_vertices = static_cast<DaedalusVtx *>(sceGuGetMemory(4 * sizeof(DaedalusVtx)));

	p_vertices[0].Position.x = screen0.x;
	p_vertices[0].Position.y = screen0.y;
	p_vertices[0].Position.z = depth;
	p_vertices[0].Colour = c32(0xffffffff);
	p_vertices[0].Texture.x = uv0.x;
	p_vertices[0].Texture.y = uv0.y;

	p_vertices[1].Position.x = screen1.x;
	p_vertices[1].Position.y = screen0.y;
	p_vertices[1].Position.z = depth;
	p_vertices[1].Colour = c32(0xffffffff);
	p_vertices[1].Texture.x = uv1.x;
	p_vertices[1].Texture.y = uv0.y;

	p_vertices[2].Position.x = screen0.x;
	p_vertices[2].Position.y = screen1.y;
	p_vertices[2].Position.z = depth;
	p_vertices[2].Colour = c32(0xffffffff);
	p_vertices[2].Texture.x = uv0.x;
	p_vertices[2].Texture.y = uv1.y;

	p_vertices[3].Position.x = screen1.x;
	p_vertices[3].Position.y = screen1.y;
	p_vertices[3].Position.z = depth;
	p_vertices[3].Colour = c32(0xffffffff);
	p_vertices[3].Texture.x = uv1.x;
	p_vertices[3].Texture.y = uv1.y;

	RenderUsingCurrentBlendMode( p_vertices, 4, GU_TRIANGLE_STRIP, GU_TRANSFORM_2D, gRDPOtherMode.depth_source ? false : true );
#endif

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	++mNumRect;
#endif
}

void RendererPSP::TexRectFlip( u32 tile_idx, const v2 & xy0, const v2 & xy1, TexCoord st0, TexCoord st1 )
{
	mTnL.Flags.Fog = 0;	//For now we force fog off for textrect, normally it should be fogged when depth_source is set //Corn

	UpdateTileSnapshots( tile_idx );

	// NB: we have to do this after UpdateTileSnapshot, as it set up mTileTopLeft etc.
	PrepareTexRectUVs(&st0, &st1);

	// Convert fixed point uvs back to floating point format.
	// NB: would be nice to pass these as s16 ints, and use GU_TEXTURE_16BIT
	v2 uv0( (float)st0.s / 32.f, (float)st0.t / 32.f );
	v2 uv1( (float)st1.s / 32.f, (float)st1.t / 32.f );

	v2 screen0;
	v2 screen1;
	// FIXME(strmnnrmn): why is VT_FULLSCREEN_HD code in TexRect() not also done here?
	ConvertN64ToScreen( xy0, screen0 );
	ConvertN64ToScreen( xy1, screen1 );

	DL_PF( "    Screen:  %.1f,%.1f -> %.1f,%.1f", screen0.x, screen0.y, screen1.x, screen1.y );
	DL_PF( "    Texture: %.1f,%.1f -> %.1f,%.1f", uv0.x, uv0.y, uv1.x, uv1.y );

	DaedalusVtx * p_vertices = static_cast<DaedalusVtx *>(sceGuGetMemory(4 * sizeof(DaedalusVtx)));

	p_vertices[0].Position.x = screen0.x;
	p_vertices[0].Position.y = screen0.y;
	p_vertices[0].Position.z = 0.0f;
	p_vertices[0].Colour = c32(0xffffffff);
	p_vertices[0].Texture.x = uv0.x;
	p_vertices[0].Texture.y = uv0.y;

	p_vertices[1].Position.x = screen1.x;
	p_vertices[1].Position.y = screen0.y;
	p_vertices[1].Position.z = 0.0f;
	p_vertices[1].Colour = c32(0xffffffff);
	p_vertices[1].Texture.x = uv0.x;
	p_vertices[1].Texture.y = uv1.y;

	p_vertices[2].Position.x = screen0.x;
	p_vertices[2].Position.y = screen1.y;
	p_vertices[2].Position.z = 0.0f;
	p_vertices[2].Colour = c32(0xffffffff);
	p_vertices[2].Texture.x = uv1.x;
	p_vertices[2].Texture.y = uv0.y;

	p_vertices[3].Position.x = screen1.x;
	p_vertices[3].Position.y = screen1.y;
	p_vertices[3].Position.z = 0.0f;
	p_vertices[3].Colour = c32(0xffffffff);
	p_vertices[3].Texture.x = uv1.x;
	p_vertices[3].Texture.y = uv1.y;

	// FIXME(strmnnrmn): shouldn't this pass gRDPOtherMode.depth_source ? false : true for the disable_zbuffer arg, as TextRect()?
	RenderUsingCurrentBlendMode( p_vertices, 4, GU_TRIANGLE_STRIP, GU_TRANSFORM_2D, true );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	++mNumRect;
#endif
}

void RendererPSP::FillRect( const v2 & xy0, const v2 & xy1, u32 color )
{
/*
	if ( (gRDPOtherMode._u64 & 0xffff0000) == 0x5f500000 )	//Used by Wave Racer
	{
		// this blend mode is mem*0 + mem*1, so we don't need to render it... Very odd!
		DAEDALUS_ERROR("	mem*0 + mem*1 - skipped");
		return;
	}
*/
	// This if for C&C - It might break other stuff (I'm not sure if we should allow alpha or not..)
	//color |= 0xff000000;

	v2 screen0;
	v2 screen1;
	ConvertN64ToScreen( xy0, screen0 );
	ConvertN64ToScreen( xy1, screen1 );

	DL_PF( "    Screen:  %.1f,%.1f -> %.1f,%.1f", screen0.x, screen0.y, screen1.x, screen1.y );

	DaedalusVtx * p_vertices = static_cast<DaedalusVtx *>(sceGuGetMemory(2 * sizeof(DaedalusVtx)));

	// No need for Texture.x/y as we don't do any texturing for fillrect
	p_vertices[0].Position.x = screen0.x;
	p_vertices[0].Position.y = screen0.y;
	p_vertices[0].Position.z = 0.0f;
	p_vertices[0].Colour = c32(color);
	//p_vertices[0].Texture.x = 0.0f;
	//p_vertices[0].Texture.y = 0.0f;

	p_vertices[1].Position.x = screen1.x;
	p_vertices[1].Position.y = screen1.y;
	p_vertices[1].Position.z = 0.0f;
	p_vertices[1].Colour = c32(color);
	//p_vertices[1].Texture.x = 1.0f;
	//p_vertices[1].Texture.y = 0.0f;

	// FIXME(strmnnrmn): shouldn't this pass gRDPOtherMode.depth_source ? false : true for the disable_zbuffer arg, as TexRect()?
	RenderUsingCurrentBlendMode( p_vertices, 2, GU_SPRITES, GU_TRANSFORM_2D, true );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	++mNumRect;
#endif
}

void RendererPSP::Draw2DTexture(f32 x0, f32 y0, f32 x1, f32 y1,
								f32 u0, f32 v0, f32 u1, f32 v1,
								const CNativeTexture * texture)
{
	DAEDALUS_PROFILE( "RendererPSP::Draw2DTexture" );
	TextureVtx *p_verts = (TextureVtx*)sceGuGetMemory(4*sizeof(TextureVtx));

	// Enable or Disable ZBuffer test
	if ( (mTnL.Flags.Zbuffer & gRDPOtherMode.z_cmp) | gRDPOtherMode.z_upd )
	{
		sceGuEnable(GU_DEPTH_TEST);
	}
	else
	{
		sceGuDisable(GU_DEPTH_TEST);
	}

	// GL_TRUE to disable z-writes
	sceGuDepthMask( gRDPOtherMode.z_upd ? GL_FALSE : GL_TRUE );
	//sceGuShadeModel(GU_FLAT);

	//ToDO: Set alpha/blend states according RenderUsingCurrentBlendMode?
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);

	sceGuEnable(GU_BLEND);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);

	// Handle large images (width > 512) with blitting, since the PSP HW can't handle
	// Handling height > 512 doesn't work well? Ignore for now.
	if( u1 >= 512 )
	{

		Draw2DTextureBlit( x0, y0, x1, y1, u0, v0, u1, v1, texture );
		return;
	}

	p_verts[0].pos.x = N64ToScreenX(x0);
	p_verts[0].pos.y = N64ToScreenY(y0);
	p_verts[0].pos.z = 0.0f;
	p_verts[0].t0.x  = u0;
	p_verts[0].t0.y  = v0;

	p_verts[1].pos.x = N64ToScreenX(x1);
	p_verts[1].pos.y = N64ToScreenY(y0);
	p_verts[1].pos.z = 0.0f;
	p_verts[1].t0.x  = u1;
	p_verts[1].t0.y  = v0;

	p_verts[2].pos.x = N64ToScreenX(x0);
	p_verts[2].pos.y = N64ToScreenY(y1);
	p_verts[2].pos.z = 0.0f;
	p_verts[2].t0.x  = u0;
	p_verts[2].t0.y  = v1;

	p_verts[3].pos.x = N64ToScreenX(x1);
	p_verts[3].pos.y = N64ToScreenY(y1);
	p_verts[3].pos.z = 0.0f;
	p_verts[3].t0.x  = u1;
	p_verts[3].t0.y  = v1;

	sceGuDrawArray( GU_TRIANGLE_STRIP, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 4, 0, p_verts );
}

void RendererPSP::Draw2DTextureR(f32 x0, f32 y0, f32 x1, f32 y1,
								 f32 x2, f32 y2, f32 x3, f32 y3,
								 f32 s, f32 t)	// With Rotation
{
	DAEDALUS_PROFILE( "RendererPSP::Draw2DTextureR" );
	TextureVtx *p_verts = (TextureVtx*)sceGuGetMemory(4*sizeof(TextureVtx));

	// Enable or Disable ZBuffer test
	if ( (mTnL.Flags.Zbuffer & gRDPOtherMode.z_cmp) | gRDPOtherMode.z_upd )
	{
		sceGuEnable(GU_DEPTH_TEST);
	}
	else
	{
		sceGuDisable(GU_DEPTH_TEST);
	}

	// GL_TRUE to disable z-writes
	sceGuDepthMask( gRDPOtherMode.z_upd ? GL_FALSE : GL_TRUE );
	//sceGuShadeModel(GU_FLAT);

	//ToDO: Set alpha/blend states according RenderUsingCurrentBlendMode?
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);

	sceGuEnable(GU_BLEND);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);

	p_verts[0].pos.x = N64ToScreenX(x0);
	p_verts[0].pos.y = N64ToScreenY(y0);
	p_verts[0].pos.z = 0.0f;
	p_verts[0].t0.x  = 0.0f;
	p_verts[0].t0.y  = 0.0f;

	p_verts[1].pos.x = N64ToScreenX(x1);
	p_verts[1].pos.y = N64ToScreenY(y1);
	p_verts[1].pos.z = 0.0f;
	p_verts[1].t0.x  = s;
	p_verts[1].t0.y  = 0.0f;

	p_verts[2].pos.x = N64ToScreenX(x2);
	p_verts[2].pos.y = N64ToScreenY(y2);
	p_verts[2].pos.z = 0.0f;
	p_verts[2].t0.x  = s;
	p_verts[2].t0.y  = t;

	p_verts[3].pos.x = N64ToScreenX(x3);
	p_verts[3].pos.y = N64ToScreenY(y3);
	p_verts[3].pos.z = 0.0f;
	p_verts[3].t0.x  = 0.0f;
	p_verts[3].t0.y  = t;

	sceGuDrawArray( GU_TRIANGLE_FAN, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 4, 0, p_verts );
}

// The following blitting code was taken from The TriEngine.
// See http://www.assembla.com/code/openTRI for more information.
void RendererPSP::Draw2DTextureBlit(f32 x, f32 y, f32 width, f32 height,
									f32 u0, f32 v0, f32 u1, f32 v1,
									const CNativeTexture * texture)
{
	if (!texture)
	{
		DAEDALUS_ERROR("No texture in Draw2DTextureBlit");
		return;
	}

	f32 cur_v = v0;
	f32 cur_y = y;
	f32 v_end = v1;
	f32 y_end = height;
	f32 vslice = 512.f;
	f32 ystep = (height/(v1-v0) * vslice);
	f32 vstep = ((v1-v0) > 0 ? vslice : -vslice);

	f32 x_end = width;
	f32 uslice = 64.f;
	//f32 ustep = (u1-u0)/width * xslice;
	f32 xstep = (width/(u1-u0) * uslice);
	f32 ustep = ((u1-u0) > 0 ? uslice : -uslice);

	const u8* data = static_cast<const u8*>(texture->GetData());

	for ( ; cur_y < y_end; cur_y+=ystep, cur_v+=vstep )
	{
		f32 cur_u = u0;
		f32 cur_x = x;
		f32 u_end = u1;

		f32 poly_height = ((cur_y+ystep) > y_end) ? (y_end-cur_y) : ystep;
		f32 source_height = vstep;

		// support negative vsteps
		if ((vstep > 0) && (cur_v+vstep > v_end))
		{
			source_height = (v_end-cur_v);
		}
		else if ((vstep < 0) && (cur_v+vstep < v_end))
		{
			source_height = (cur_v-v_end);
		}

		const u8* udata = data;
		// blit maximizing the use of the texture-cache
		for( ; cur_x < x_end; cur_x+=xstep, cur_u+=ustep )
		{
			// support large images (width > 512)
			if (cur_u>512.f || cur_u+ustep>512.f)
			{
				s32 off = (ustep>0) ? ((int)cur_u & ~31) : ((int)(cur_u+ustep) & ~31);

				udata += off * GetBitsPerPixel( texture->GetFormat() );
				cur_u -= off;
				u_end -= off;
				sceGuTexImage(0, Min<u32>(512,texture->GetCorrectedWidth()), Min<u32>(512,texture->GetCorrectedHeight()), texture->GetBlockWidth(), udata);
			}
			TextureVtx *p_verts = (TextureVtx*)sceGuGetMemory(2*sizeof(TextureVtx));

			//f32 poly_width = ((cur_x+xstep) > x_end) ? (x_end-cur_x) : xstep;
			f32 poly_width = xstep;
			f32 source_width = ustep;

			// support negative usteps
			if ((ustep > 0) && (cur_u+ustep > u_end))
			{
				source_width = (u_end-cur_u);
			}
			else if ((ustep < 0) && (cur_u+ustep < u_end))
			{
				source_width = (cur_u-u_end);
			}

			p_verts[0].t0.x = cur_u;
			p_verts[0].t0.y = cur_v;
			p_verts[0].pos.x = N64ToScreenX(cur_x);
			p_verts[0].pos.y = N64ToScreenY(cur_y);
			p_verts[0].pos.z = 0;

			p_verts[1].t0.x = cur_u + source_width;
			p_verts[1].t0.y = cur_v + source_height;
			p_verts[1].pos.x = N64ToScreenX(cur_x + poly_width);
			p_verts[1].pos.y = N64ToScreenY(cur_y + poly_height);
			p_verts[1].pos.z = 0;

			sceGuDrawArray( GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, p_verts );
		}
	}
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
void RendererPSP::SelectPlaceholderTexture( EPlaceholderTextureType type )
{
	switch( type )
	{
	case PTT_WHITE:			sceGuTexImage(0, kPlaceholderTextureWidth, kPlaceholderTextureHeight, kPlaceholderTextureWidth, gWhiteTexture);       break;
	case PTT_SELECTED:		sceGuTexImage(0, kPlaceholderTextureWidth, kPlaceholderTextureHeight, kPlaceholderTextureWidth, gSelectedTexture);    break;
	case PTT_MISSING:		sceGuTexImage(0, kPlaceholderTextureWidth, kPlaceholderTextureHeight, kPlaceholderTextureWidth, gPlaceholderTexture); break;
	default:
		DAEDALUS_ERROR( "Unhandled type" );
		break;
	}
}
#endif // DAEDALUS_DEBUG_DISPLAYLIST

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
// Used for Blend Explorer, or Nasty texture
bool RendererPSP::DebugBlendmode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_flags, u64 mux )
{
	if( IsCombinerStateDisabled( mux ) )
	{
		if( mNastyTexture )
		{
			// Use the nasty placeholder texture
			//
			sceGuEnable(GU_TEXTURE_2D);
			SelectPlaceholderTexture( PTT_SELECTED );
			sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
			sceGuTexMode(GU_PSM_8888,0,0,GL_TRUE);		// maxmips/a2/swizzle = 0
			sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
		}
		else
		{
			//Allow Blend Explorer
			//
			SBlendModeDetails details;

			details.InstallTexture = true;
			details.EnvColour      = mEnvColour;
			details.PrimColour     = mPrimitiveColour;
			details.ColourAdjuster.Reset();

			//Insert the Blend Explorer
			BLEND_MODE_MAKER

			bool installed_texture = false;

			if( details.InstallTexture )
			{
				if( mBoundTexture[0] != NULL )
				{
					mBoundTexture[0]->InstallTexture();
					installed_texture = true;
				}
			}

			// If no texture was specified, or if we couldn't load it, clear it out
			if( !installed_texture ) sceGuDisable( GU_TEXTURE_2D );

			details.ColourAdjuster.Process( p_vertices, num_vertices );
			sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
		}

		return true;
	}

	return false;
}
#endif // DAEDALUS_DEBUG_DISPLAYLIST


#ifdef DAEDALUS_DEBUG_DISPLAYLIST
void RendererPSP::DebugMux( const CBlendStates * states, DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_flags, u64 mux)
{
	// Only dump missing_mux when we awant to search for inexact blends aka HighlightInexactBlendModes is enabled.
	// Otherwise will dump lotsa of missing_mux even though is not needed since was handled correctly by auto blendmode thing - Salvy
	//
	if (gGlobalPreferences.HighlightInexactBlendModes && states->IsInexact())
	{
		if (mUnhandledCombinerStates.find( mux ) == mUnhandledCombinerStates.end())
		{
			IO::Filename filepath;
			Dump_GetDumpDirectory(filepath, g_ROM.settings.GameName.c_str());
			IO::Path::Append(filepath, "missing_mux.txt");

			FILE * fh = fopen(filepath, mUnhandledCombinerStates.empty() ? "w" : "a");
			if (fh != NULL)
			{
				DLDebug_PrintMux( fh, mux );
				fclose(fh);
			}

			mUnhandledCombinerStates.insert( mux );
		}

		sceGuEnable( GU_TEXTURE_2D );
		sceGuTexMode( GU_PSM_8888, 0, 0, GL_TRUE );		// maxmips/a2/swizzle = 0

		// Use the nasty placeholder texture
		SelectPlaceholderTexture( PTT_MISSING );
		sceGuTexFunc( GU_TFX_REPLACE, GU_TCC_RGBA );
		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
	}
}

#endif // DAEDALUS_DEBUG_DISPLAYLIST





bool CreateRenderer()
{
	DAEDALUS_ASSERT_Q(gRenderer == NULL);
	gRendererPSP = new RendererPSP();
	gRenderer    = gRendererPSP;
	return true;
}
void DestroyRenderer()
{
	delete gRendererPSP;
	gRendererPSP = NULL;
	gRenderer    = NULL;
}
