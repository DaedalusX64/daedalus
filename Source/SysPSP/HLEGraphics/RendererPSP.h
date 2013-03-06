
#ifndef RENDERERPSP_H__
#define RENDERERPSP_H__

#include <map>
#include <set>

#include "HLEGraphics/BaseRenderer.h"
#include "HLEGraphics/BlendModes.h"

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

class RendererPSP : public BaseRenderer
{
public:
	RendererPSP();
	~RendererPSP();

	virtual void		RestoreRenderStates();

	virtual void		RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, ERenderMode render_mode, bool disable_zbuffer );

	virtual void		TexRect( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 );
	virtual void		TexRectFlip( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 );
	virtual void		FillRect( const v2 & xy0, const v2 & xy1, u32 color );

	virtual void		Draw2DTextureBlit(f32 x, f32 y, f32 width, f32 height, f32 u0, f32 v0, f32 u1, f32 v1, CNativeTexture * texture);
	virtual void		Draw2DTexture(f32 frameX, f32 frameY, f32 frameW, f32 frameH, f32 imageX, f32 imageY, f32 imageW, f32 imageH);
	virtual void		Draw2DTextureR(f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3, f32 s, f32 t);

	struct SBlendStateEntry
	{
		SBlendStateEntry() : OverrideFunction( NULL ), States( NULL ) {}
		OverrideBlendModeFn			OverrideFunction;
		const CBlendStates *		States;
	};

	SBlendStateEntry	LookupBlendState( u64 mux, bool two_cycles );

private:
	void				RenderUsingRenderSettings( const CBlendStates * states, DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_flags );

private:
	// BlendMode support
	//
	CBlendStates *		mCopyBlendStates;
	CBlendStates *		mFillBlendStates;

	typedef std::map< u64, SBlendStateEntry > BlendStatesMap;
	BlendStatesMap		mBlendStatesMap;


	// Functions and members related to the DisplayListDebugger.
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
public:
	virtual void 		ResetDebugState();

	bool				IsCombinerStateDefault( u64 state ) const	{ return IsInexactDefault( LookupOverrideBlendModeInexact( state ) ); }
	bool				IsCombinerStateForced( u64 state ) const	{ return LookupOverrideBlendModeForced( state ) != NULL; }
	//bool				IsCombinerStateUnhandled( u64 state ) const	{ return mUnhandledCombinerStates.find( state ) != mUnhandledCombinerStates.end(); }

	bool				IsCombinerStateDisabled( u64 state ) const	{ return mDisabledCombinerStates.find( state ) != mDisabledCombinerStates.end(); }
	void				DisableCombinerState( u64 state )			{ mDisabledCombinerStates.insert( state ); }
	void				EnableCombinerState( u64 state )			{ mDisabledCombinerStates.erase( state ); }
	void				ToggleDisableCombinerState( u64 state )		{ if( IsCombinerStateDisabled( state )) { EnableCombinerState(state); } else { DisableCombinerState( state ); mNastyTexture = false; } }
	void				ToggleNastyTexture( bool enable )			{ mNastyTexture = ( enable =! mNastyTexture ); }

	void					SetRecordCombinerStates( bool enable )	{ mRecordCombinerStates = enable; }					// Sets whether combiner states will be recorded for the subsequent frames
	const std::set<u64> &	GetRecordedCombinerStates() const		{ return mRecordedCombinerStates; }

private:
	enum EPlaceholderTextureType
	{
		PTT_WHITE = 0,
		PTT_SELECTED,
		PTT_MISSING,
	};

	void				SelectPlaceholderTexture( EPlaceholderTextureType type );
	bool				DebugBlendmode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_flags, u64 mux );
	void				DebugMux( const CBlendStates * states, DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_flags, u64 mux);

private:
	bool				mRecordCombinerStates;		// FIXME(strmnnrmn): this variable never seems to be read from?.
	std::set< u64 >		mRecordedCombinerStates;

	std::set< u64 >		mDisabledCombinerStates;

	std::set< u64 >		mUnhandledCombinerStates;
#endif
};

// NB: this is equivalent to gRenderer, but points to the implementation class, for platform-specific functionality.
extern RendererPSP * gRendererPSP;

#endif // RENDERERPSP_H__
