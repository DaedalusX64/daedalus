#include "stdafx.h"
#include "HLEGraphics/BaseRenderer.h"

#include <pspgu.h>

#include "Core/ROM.h"
#include "Graphics/NativeTexture.h"
#include "HLEGraphics/Combiner/BlendConstant.h"
#include "HLEGraphics/Texture.h"
#include "OSHLE/ultra_gbi.h"

// FIXME - surely these should be defined by a system header? Or GU_TRUE etc?
#define GL_TRUE                           1
#define GL_FALSE                          0

BaseRenderer * gRenderer = NULL;

extern void InitBlenderMode( u32 blender );

class RendererPSP : public BaseRenderer
{
	virtual void		RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_mode, bool disable_zbuffer );

	void				RenderUsingRenderSettings( const CBlendStates * states, DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_flags );
};

void RendererPSP::RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_mode, bool disable_zbuffer )
{
	static bool	ZFightingEnabled( false );

	DAEDALUS_PROFILE( "BaseRenderer::RenderUsingCurrentBlendMode" );

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
	if(cycle_mode < CYCLE_COPY)
	{
		gRDPOtherMode.force_bl ? InitBlenderMode( gRDPOtherMode.blender ) : sceGuDisable( GU_BLEND );
	}

	// Initiate Alpha test
	//
	if( (gRDPOtherMode.alpha_compare == G_AC_THRESHOLD) && !gRDPOtherMode.alpha_cvg_sel )
	{
		// G_AC_THRESHOLD || G_AC_DITHER
		sceGuAlphaFunc( (mAlphaThreshold | g_ROM.ALPHA_HACK) ? GU_GEQUAL : GU_GREATER, mAlphaThreshold, 0xff);
		sceGuEnable(GU_ALPHA_TEST);
	}
	// I think this implies that alpha is coming from
	else if (gRDPOtherMode.cvg_x_alpha)
	{
		// Going over 0x70 breaks OOT, but going lesser than that makes lines on games visible...ex: Paper Mario.
		// ALso going over 0x30 breaks the birds in Tarzan :(. Need to find a better way to leverage this.
		sceGuAlphaFunc(GU_GREATER, 0x70, 0xff);
		sceGuEnable(GU_ALPHA_TEST);
	}
	else
	{
		// Use CVG for pixel alpha
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
	if( DebugBlendmode( p_vertices, num_vertices, triangle_mode, render_flags, mMux ) )	return;
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
			if( mpTexture[ g_ROM.T1_HACK ] != NULL )
			{
				const CRefPtr<CNativeTexture> texture( mpTexture[ g_ROM.T1_HACK ]->GetTexture() );

				if(texture != NULL)
				{
					texture->InstallTexture();
					installed_texture = true;
				}
			}
		}

		// If no texture was specified, or if we couldn't load it, clear it out
		if( !installed_texture )
		{
			sceGuDisable( GU_TEXTURE_2D );
		}

		details.ColourAdjuster.Process( p_vertices, num_vertices );

		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
	}
	else if( blend_entry.States != NULL )
	{
		RenderUsingRenderSettings( blend_entry.States, p_vertices, num_vertices, triangle_mode, render_flags );
	}
	else
	{
		// Set default states
		DAEDALUS_ERROR( "Unhandled blend mode" );
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

	static std::vector< DaedalusVtx >	saved_verts;

	if( states->GetNumStates() > 1 )
	{
		saved_verts.resize( num_vertices );
		memcpy( &saved_verts[0], p_vertices, num_vertices * sizeof( DaedalusVtx ) );
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
			memcpy( p_vertices, &saved_verts[0], num_vertices * sizeof( DaedalusVtx ) );
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

		u32 texture_idx( 0 );

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

				if( install_texture1 & mTnL.Flags.Texture && (mTnL.Flags._u32 & (TNL_LIGHT|TNL_TEXGEN)) != (TNL_LIGHT|TNL_TEXGEN) )
				{
					sceGuTexOffset( -mTileTopLeft[ 1 ].x * mTileScale[ 1 ].x, -mTileTopLeft[ 1 ].y * mTileScale[ 1 ].y );
					sceGuTexScale( mTileScale[ 1 ].x, mTileScale[ 1 ].y );
				}
			}
			else
			{
				// NB if install_texture0 and install_texture1 are both set, 0 wins out
				texture_idx = install_texture0 ? 0 : 1;
			}

			if( mpTexture[texture_idx] != NULL )
			{
				CRefPtr<CNativeTexture> texture( mpTexture[ texture_idx ]->GetTexture() );

				if(out.MakeTextureWhite)
				{
					texture = mpTexture[ texture_idx ]->GetRecolouredTexture( c32::White );
				}

				if(texture != NULL)
				{
					texture->InstallTexture();
					installed_texture = true;
				}
			}
		}

		// If no texture was specified, or if we couldn't load it, clear it out
		if( !installed_texture ) sceGuDisable(GU_TEXTURE_2D);

		sceGuTexWrap( mTexWrap[texture_idx].u, mTexWrap[texture_idx].v );

		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
	}
}









bool CreateRenderer()
{
	DAEDALUS_ASSERT_Q(gRenderer == NULL);
	gRenderer = new RendererPSP();
	return true;
}
void DestroyRenderer()
{
	delete gRenderer;
	gRenderer = NULL;
}

