#include "stdafx.h"
#include "HLEGraphics/BaseRenderer.h"

#include "Core/ROM.h"
#include "Graphics/NativeTexture.h"
#include "HLEGraphics/Texture.h"
#include "OSHLE/ultra_gbi.h"

BaseRenderer * gRenderer = NULL;

class RendererOSX : public BaseRenderer
{
	virtual void		RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_mode, bool disable_zbuffer );
};



extern void InitBlenderMode( u32 blender );
void RendererOSX::RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_mode, bool disable_zbuffer )
{
	static bool	ZFightingEnabled = false;

	DAEDALUS_PROFILE( "RendererOSX::RenderUsingCurrentBlendMode" );

	if ( disable_zbuffer )
	{
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}
	else
	{
		// Fixes Zfighting issues we have on the PSP.
		if( gRDPOtherMode.zmode == 3 )
		{
			if( !ZFightingEnabled )
			{
				ZFightingEnabled = true;
				//FIXME
				//glDepthRange(65535 / 65536.f, 80 / 65536.f);
			}
		}
		else if( ZFightingEnabled )
		{
			ZFightingEnabled = false;
			//FIXME
			//glDepthRange(65535 / 65536.f, 0 / 65536.f);
		}

		// Enable or Disable ZBuffer test
		if ( (mTnL.Flags.Zbuffer & gRDPOtherMode.z_cmp) | gRDPOtherMode.z_upd )
		{
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}

		glDepthMask(gRDPOtherMode.z_upd ? GL_TRUE : GL_FALSE);
	}


	u32 cycle_mode = gRDPOtherMode.cycle_type;

	// Initiate Blender
	//
	if(cycle_mode < CYCLE_COPY)
	{
		gRDPOtherMode.force_bl ? InitBlenderMode( gRDPOtherMode.blender ) : glDisable( GL_BLEND );
	}

	// Initiate Alpha test
	//
	if( (gRDPOtherMode.alpha_compare == G_AC_THRESHOLD) && !gRDPOtherMode.alpha_cvg_sel )
	{
		// G_AC_THRESHOLD || G_AC_DITHER
		glAlphaFunc( (mAlphaThreshold | g_ROM.ALPHA_HACK) ? GL_GEQUAL : GL_GREATER, (float)mAlphaThreshold / 255.f);
		glEnable(GL_ALPHA_TEST);
	}
	else if (gRDPOtherMode.cvg_x_alpha)
	{
		// Going over 0x70 brakes OOT, but going lesser than that makes lines on games visible...ex: Paper Mario.
		// ALso going over 0x30 breaks the birds in Tarzan :(. Need to find a better way to leverage this.
		glAlphaFunc(GL_GREATER, (float)0x70 / 255.f);
		glEnable(GL_ALPHA_TEST);
	}
	else
	{
		// Use CVG for pixel alpha
        glDisable(GL_ALPHA_TEST);
	}

	extern void GLRenderer_SetMux(u64 mux, u32 cycle_type, const c32 & prim_col, const c32 & env_col);
	GLRenderer_SetMux(mMux, cycle_mode, mPrimitiveColour, mEnvColour);


	u32 render_flags( GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | render_mode );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	// Used for Blend Explorer, or Nasty texture
	//
	if( DebugBlendmode( p_vertices, num_vertices, triangle_mode, render_flags, mMux ) )	return;
#endif

	// FIXME - figure out from mux
	bool install_textures[2] = { true, false };

	for (u32 i = 0; i < 2; ++i)
	{
		if (!install_textures[i])
			continue;

		if (mpTexture[i] != NULL)
		{
			CRefPtr<CNativeTexture> texture = mpTexture[i]->GetTexture();
			if (texture != NULL)
			{
				texture->InstallTexture();

				// FIXME(strmnnrmn) Set tex offset/scale here, not in BaseRenderer

				// Initiate Texture Filter
				//
				// G_TF_AVERAGE : 1, G_TF_BILERP : 2 (linear)
				// G_TF_POINT   : 0 (nearest)
				//
				if( (gRDPOtherMode.text_filt != G_TF_POINT) | (gGlobalPreferences.ForceLinearFilter) )
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GU_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GU_NEAREST);
				}

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mTexWrap[i].u);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mTexWrap[i].v);
			}
		}
	}

	int prim = 0;
	switch (triangle_mode)
	{
	case GU_SPRITES:
	//	printf( "Draw SPRITES\n" );
		break;
	case GU_TRIANGLES:
		prim = GL_TRIANGLES;
		break;
	case GU_TRIANGLE_STRIP:
		printf( "Draw TRIANGLE_STRIP\n" );
		break;
	case GU_TRIANGLE_FAN:
		printf( "Draw TRIANGLE_FAN\n" );
		break;
	}

	if (prim != 0)
	{
		void GLRenderer_RenderDaedalusVtx(int prim, const DaedalusVtx * vertices, int count);
		GLRenderer_RenderDaedalusVtx( prim, p_vertices, num_vertices );
	}
}


bool CreateRenderer()
{
	DAEDALUS_ASSERT_Q(gRenderer == NULL);
	gRenderer = new RendererOSX();
	return true;
}
void DestroyRenderer()
{
	delete gRenderer;
	gRenderer = NULL;
}
