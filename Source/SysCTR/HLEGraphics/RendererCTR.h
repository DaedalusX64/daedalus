#pragma once

#include <map>
#include <set>

#include "HLEGraphics/BaseRenderer.h"
#include "SysCTR/HLEGraphics/BlendModes.h"

class CBlendStates;

struct DebugBlendSettings
{
	u32 TexInstall;	//defaults to texture installed
	u32	SetRGB;		//defaults to OFF
	u32	SetA;		//defaults to OFF
	u32	SetRGBA;	//defaults to OFF
	u32	ModRGB;		//defaults to OFF
	u32	ModA;		//defaults to OFF
	u32	ModRGBA;	//defaults to OFF
	u32	SubRGB;		//defaults to OFF
	u32	SubA;		//defaults to OFF
	u32	SubRGBA;	//defaults to OFF
	u32	AOpaque;	//defaults to OFF
	u32	sceENV;		//defaults to OFF
	u32	TXTFUNC;	//defaults to MODULATE_RGB
	u32 ForceRGB;	//defaults to OFF
};

class RendererCTR : public BaseRenderer
{
public:
	RendererCTR();
	~RendererCTR();

	virtual void		RestoreRenderStates();

	virtual void		RenderTriangles(DaedalusVtx * p_vertices, u32 num_vertices, bool disable_zbuffer);

	virtual void		TexRect(u32 tile_idx, const glm::vec2 & xy0, const glm::vec2 & xy1, TexCoord st0, TexCoord st1);
	virtual void		TexRectFlip(u32 tile_idx, const glm::vec2 & xy0, const glm::vec2 & xy1, TexCoord st0, TexCoord st1);
	virtual void		FillRect(const glm::vec2 & xy0, const glm::vec2 & xy1, u32 color);

	virtual void		Draw2DTexture(f32 x0, f32 y0, f32 x1, f32 y1, f32 u0, f32 v0, f32 u1, f32 v1, const std::shared_ptr<CNativeTexture> texture);
	virtual void		Draw2DTextureR(f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3, f32 s, f32 t, const std::shared_ptr<CNativeTexture> texture);
	
	struct SBlendStateEntry
	{
		SBlendStateEntry() : OverrideFunction( NULL ), States( NULL ) {}
		OverrideBlendModeFn			OverrideFunction;
		const CBlendStates *		States;
	};

	SBlendStateEntry	LookupBlendState( u64 mux, bool two_cycles );

private:
	void				RenderUsingCurrentBlendMode(const float* mat_project, DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, bool disable_zbuffer );
	void				RenderUsingRenderSettings( const CBlendStates * states, DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode );
	void                DrawPrimitives(DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, bool has_texture);
	
	// Temporary vertex storage
	DaedalusVtx			mVtx_Save[320];
	
	// BlendMode support
	//
	CBlendStates *		mCopyBlendStates;
	CBlendStates *		mFillBlendStates;

	typedef std::map< u64, SBlendStateEntry > BlendStatesMap;
	BlendStatesMap		mBlendStatesMap;
};

// NB: this is equivalent to gRenderer, but points to the implementation class, for platform-specific functionality.
extern RendererCTR * gRendererCTR;
