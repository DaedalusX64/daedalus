/*
Copyright (C) 2001 StrmnNrmn

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

#ifndef __DAEDALUS_D3DRENDER_H__
#define __DAEDALUS_D3DRENDER_H__

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Utility/Singleton.h"
#include "Utility/RefCounted.h"
#include "DaedalusVtx.h"
#include "Graphics/ColourValue.h"
#include "BlendModes.h"
#include "Utility/Preferences.h"

#include <pspgu.h>
#include <set>
#include <map>

class CTexture;
class CNativeTexture;
class CBlendStates;

struct ViewportInfo
{
	f32  ViWidth;
	f32  ViHeight;

	bool Update;
	bool Rumble;
};

struct TextureVtx
{
	v2  t0;
	v3  pos;
};

struct FiddledVtxDKR
{
	s16 y;
	s16 x;

	u8 a;
	u8 b;
	s16 z;

	u8 g;
	u8 r;
};

struct FiddledVtxPD 
{
	s16 y;
	s16	x;

	u8	cidx;
	u8	pad;
	s16 z;

	s16 tv;
	s16 tu;
};

struct FiddledVtx
{
        s16 y;
        s16 x;

        s16 flag;
        s16 z;

        s16 tv;
        s16 tu;

        union
        {
                struct
                {
                        u8 rgba_a;
                        u8 rgba_b;
                        u8 rgba_g;
                        u8 rgba_r;
                };
                struct
                {
                        s8 norm_a;
                        s8 norm_z;      // b
                        s8 norm_y;      // g
                        s8 norm_x;      // r
                };
        };
};
DAEDALUS_STATIC_ASSERT( sizeof(FiddledVtx) == 16 );

struct TextureVtx;

ALIGNED_TYPE(struct, DaedalusLight, 16)
{
	v3		Direction;		// w component is ignored. Should be normalised
	f32		Padding0;
	v4		Colour;			// Colour, components in range 0..1
};

DAEDALUS_STATIC_ASSERT( sizeof( DaedalusLight ) == 32 );

ALIGNED_TYPE(struct, TnLParams, 16)
{
	v4			Ambient;
    float		FogMult;
	float		FogOffset;
	float		TextureScaleX;
	float		TextureScaleY;
};
DAEDALUS_STATIC_ASSERT( sizeof( TnLParams ) == 32 );

// Bits for clipping
// +-+-+-
// xxyyzz

// NB: These are ordered such that the VFPU can generate them easily - make sure you keep the VFPU code up to date if changing these.
#define X_NEG  0x01
#define Y_NEG  0x02
#define Z_NEG  0x04
#define X_POS  0x08
#define Y_POS  0x10
#define Z_POS  0x20

// Test all but Z_NEG (for No Near Plane microcodes)
//const u32 CLIP_TEST_FLAGS( X_POS | X_NEG | Y_POS | Y_NEG | Z_POS );
const u32 CLIP_TEST_FLAGS( X_POS | X_NEG | Y_POS | Y_NEG | Z_POS | Z_NEG );

//*****************************************************************************
//
//*****************************************************************************
class PSPRenderer : public CSingleton< PSPRenderer >
{
protected:
	friend class CSingleton< PSPRenderer >;
	PSPRenderer();

public:
	~PSPRenderer();

	void				BeginScene();
	void				EndScene();

	void				SetVIScales();
	void				Reset();

	// Verious rendering states
	enum ETnLModeFlags
	{
		TNL_LIGHT		= 1 << 0,
		TNL_TEXTURE		= 1 << 1,
		TNL_TEXGEN		= 1 << 2,
		TNL_TEXGENLIN	= 1 << 3,
		TNL_FOG			= 1 << 4,
	};

	inline void			SetTextureEnable(bool enable)			{ if( enable ) mTnLModeFlags |= TNL_TEXTURE; else mTnLModeFlags &= ~TNL_TEXTURE; }
	inline void			SetLighting(bool enable)				{ if( enable ) mTnLModeFlags |= TNL_LIGHT;	 else mTnLModeFlags &= ~TNL_LIGHT; }
	inline void			SetTextureGen(bool enable)				{ if( enable ) mTnLModeFlags |= TNL_TEXGEN;  else mTnLModeFlags &= ~TNL_TEXGEN; }
	inline void			SetTextureGenLin(bool enable)			{ if( enable ) mTnLModeFlags |= TNL_TEXGENLIN;  else mTnLModeFlags &= ~TNL_TEXGENLIN; }

	// PrimDepth will replace the z value if depth_source=1 (z range 32767-0 while PSP depthbuffer range 0-65535)//Corn
	inline void			SetPrimitiveDepth( u32 z )				{ mPrimDepth = (f32)( ( ( 32767 - z ) << 1) + 1 ); }
	inline void			SetPrimitiveColour( c32 colour )		{ mPrimitiveColour = colour; }
	inline void			SetEnvColour( c32 colour )				{ mEnvColour = colour; }
	inline void			ZBufferEnable(bool bZBuffer)			{ mZBuffer = bZBuffer; }

	inline void			SetNumLights(u32 num)					{ mNumLights = num; }
	void				SetLightCol(u32 light, u32 colour);
	void				SetLightDirection(u32 l, float x, float y, float z);
	inline void			SetAmbientLight( const v4 & colour )	{ mTnLParams.Ambient = colour; }

	inline void			SetMux( u64 mux )						{ mMux = mux; }
	inline void			SetSmooth( bool bSmooth )				{ mSmooth = bSmooth; }
	inline void			SetSmoothShade( bool bSmoothShade )		{ mSmoothShade = bSmoothShade; }
	inline void			SetAlphaRef(u32 alpha)					{ mAlphaThreshold = alpha; }
	inline void			SetCullMode(bool bFront, bool bBack)	{ mCull = bFront | bBack; if( bBack ) mCullMode = GU_CCW; else mCullMode = GU_CW; }

	// Texture stuff
	inline void			SetTextureScale(float fScaleX, float fScaleY)	{ mTnLParams.TextureScaleX = fScaleX; mTnLParams.TextureScaleY = fScaleY; }
	void                Draw2DTexture(float, float, float, float, float, float, float, float);

	// Viewport stuff
	void				SetN64Viewport( const v3 & scale, const v3 & trans );
	void				SetScissor( u32 x0, u32 y0, u32 x1, u32 y1 );

	// Fog stuff
	inline void			SetFogEnable(bool Enable)				{ if(Enable & gFogEnabled) sceGuEnable(GU_FOG); else sceGuDisable(GU_FOG); }
	inline void			SetFogMinMax(float fMin, float fMax)	{ sceGuFog(fMin, fMax, mFogColour.GetColour()); }
	inline void			SetFogColour( c32 colour )				{ mFogColour = colour; }
	// Unused.. will remove soon
	inline void			SetFogMult( float fFogMult )			{ mTnLParams.FogMult = fFogMult; }
	inline void			SetFogOffset( float fFogOffset )		{ mTnLParams.FogOffset = fFogOffset; }


	// Matrix stuff
	enum EMatrixLoadStyle
	{
		MATRIX_LOAD,
		MATRIX_MUL,
	};

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	void				PrintActive();
#endif
	void				ResetMatrices();
	void				SetProjection(const Matrix4x4 & mat, bool bPush, bool bReplace);
	void				SetWorldView(const Matrix4x4 & mat, bool bPush, bool bReplace);
	inline void			PopProjection() {if (mProjectionTop > 0) --mProjectionTop;	mWorldProjectValid = false;}
	inline void			PopWorldView()	{if (mModelViewTop > 0)	 --mModelViewTop;	mWorldProjectValid = false;}
	void				InsertMatrix(u32 w0, u32 w1);
	void				ForceMatrix(const Matrix4x4 & mat);

	// Vertex stuff	
	void				SetNewVertexInfo(u32 address, u32 v0, u32 n);	// Assumes dwAddress has already been checked!	
	void				SetNewVertexInfoConker(u32 address, u32 v0, u32 n);	// For conker..	
	void				SetNewVertexInfoDKR(u32 address, u32 v0, u32 n);	// Assumes dwAddress has already been checked!	
	void				SetNewVertexInfoPD(u32 address, u32 v0, u32 n);	// Assumes dwAddress has already been checked!	
	void				ModifyVertexInfo(u32 whered, u32 vert, u32 val);
	void				SetVtxColor( u32 vert, c32 color );
	inline void			SetVtxTextureCoord( u32 vert, short tu, short tv ) {mVtxProjected[vert].Texture.x = (f32)tu * (1.0f / 32.0f); mVtxProjected[vert].Texture.y = (f32)tv * (1.0f / 32.0f);}
	inline void			SetVtxXY( u32 vert, float x, float y );
	void				SetVtxZ( u32 vert, float z );

	// TextRect stuff
	void				TexRect( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 );
	void				TexRectFlip( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 );
	void				FillRect( const v2 & xy0, const v2 & xy1, u32 color );
		
	// Returns true if triangle visible, false otherwise
	bool				AddTri(u32 v0, u32 v1, u32 v2);

	// Render our current triangle list to screen
	void				FlushTris();
	
	// Returns true if bounding volume is visible within NDC box, false if culled
	inline bool			TestVerts( u32 v0, u32 vn ) const		{ u32 f=mVtxProjected[v0].ClipFlags; for( u32 i=v0+1; i<=vn; i++ ) f&=mVtxProjected[i].ClipFlags; return (f&CLIP_TEST_FLAGS)==0; }
	inline s32			GetVtxDepth( u32 i ) const				{ return (s32)mVtxProjected[ i ].ProjectedPos.z; }
	inline v4			GetTransformedVtxPos( u32 i ) const		{ return mVtxProjected[ i ].TransformedPos; }
	inline v4			GetProjectedVtxPos( u32 i ) const		{ return mVtxProjected[ i ].ProjectedPos; }
	inline u32			GetVtxFlags( u32 i ) const				{ return mVtxProjected[ i ].ClipFlags; }

	// Rendering stats
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	inline u32			GetNumTrisRendered() const				{ return m_dwNumTrisRendered; }
	inline u32			GetNumTrisClipped() const				{ return m_dwNumTrisClipped; }
	inline u32			GetNumRect() const						{ return m_dwNumRect; }
#endif

	// Debugging
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	void					SetRecordCombinerStates( bool enable )	{ mRecordCombinerStates = enable; }					// Sets whether combiner states will be recorded for the subsequent frames
	const std::set<u64> &	GetRecordedCombinerStates() const		{ return mRecordedCombinerStates; }

	bool				IsCombinerStateDefault( u64 state ) const	{ return IsInexactDefault( LookupOverrideBlendModeInexact( state ) ); }
	bool				IsCombinerStateForced( u64 state ) const	{ return LookupOverrideBlendModeForced( state ) != NULL; }
	//bool				IsCombinerStateUnhandled( u64 state ) const	{ return mUnhandledCombinerStates.find( state ) != mUnhandledCombinerStates.end(); }
	bool				IsCombinerStateDisabled( u64 state ) const	{ return mDisabledCombinerStates.find( state ) != mDisabledCombinerStates.end(); }
	void				DisableCombinerState( u64 state )			{ mDisabledCombinerStates.insert( state ); }
	void				EnableCombinerState( u64 state )			{ mDisabledCombinerStates.erase( state ); }

	void				ToggleDisableCombinerState( u64 state )		{ if( IsCombinerStateDisabled( state )) { EnableCombinerState(state); } else { DisableCombinerState( state ); mNastyTexture = false; } }
	void				ToggleNastyTexture( bool enable )			{ mNastyTexture = ( enable =! mNastyTexture ); }
#endif

	struct SBlendStateEntry
	{
		SBlendStateEntry() : OverrideFunction( NULL ), States( NULL ) {}
		OverrideBlendModeFn			OverrideFunction;
		const CBlendStates *		States;
	};

	SBlendStateEntry	LookupBlendState( u64 mux, bool two_cycles );

private:
	inline void			EnableTexturing( u32 tile_idx );
	void				EnableTexturing( u32 index, u32 tile_idx );

	void				RestoreRenderStates();
	
	void				SetPSPViewport( s32 x, s32 y, u32 w, u32 h );
	void				UpdateViewport();

	v2					ConvertN64ToPsp( const v2 & n64_coords ) const;

	enum ERenderMode
	{
		RM_RENDER_2D,
		RM_RENDER_3D,
	};

	void				RenderUsingRenderSettings( const CBlendStates * states, DaedalusVtx * p_vertices, u32 num_vertices, u32 render_flags );
	void				RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, ERenderMode mode, bool disable_zbuffer );
// Old code, kept for reference
#ifdef DAEDALUS_IS_LEGACY
	void 				TestVFPUVerts( u32 v0, u32 num, const FiddledVtx * verts, const Matrix4x4 & mat_world );
	template< bool FogEnable, int TextureMode >
	void ProcessVerts( u32 v0, u32 num, const FiddledVtx * verts, const Matrix4x4 & mat_world );
#endif

	void				PrepareTrisClipped( DaedalusVtx ** p_p_vertices, u32 * p_num_vertices ) const;
	void				PrepareTrisUnclipped( DaedalusVtx ** p_p_vertices, u32 * p_num_vertices ) const;

	v4					LightVert( const v3 & norm ) const;

	void				RenderTriangleList( const DaedalusVtx * p_verts, u32 num_verts, bool disable_zbuffer );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST	
	enum EPlaceholderTextureType
	{
		PTT_WHITE = 0,
		PTT_SELECTED,
		PTT_MISSING,
	};

	void				SelectPlaceholderTexture( EPlaceholderTextureType type );
	bool				DebugBlendmode( DaedalusVtx * p_vertices, u32 num_vertices, u32 render_flags, u64 mux );
	void				DebugMux( const CBlendStates * states, DaedalusVtx * p_vertices, u32 num_vertices, u32 render_flags, u64 mux);
#endif
private:
	enum { MAX_VERTS = 80 };		// F3DLP.Rej supports up to 80 verts!

	TnLParams			mTnLParams;

	v2					mN64ToPSPScale;
	v2					mN64ToPSPTranslate;

	v3					mVpScale;
	v3					mVpTrans;

	u64					mMux;

	u32					mTnLModeFlags;

	u32					mNumLights;

	bool				mZBuffer;

	bool				mCull;
	int					mCullMode;
	
	u32					mAlphaThreshold;

	bool				mSmooth;
	bool				mSmoothShade;

	f32					mPrimDepth;

	c32					mFogColour;
	c32					mPrimitiveColour;
	c32					mEnvColour;

	DaedalusLight		mLights[16];	//Conker uses more than 8

	// Texturing
	static const u32 NUM_N64_TEXTURES = 2;

	CRefPtr<CTexture>	mpTexture[ NUM_N64_TEXTURES ];
	v2					mTileTopLeft[ NUM_N64_TEXTURES ];
	v2					mTileScale[ NUM_N64_TEXTURES ];
	
	//Max is 18 according to the manual //Corn
	static const u32 MATRIX_STACK_SIZE = 18; 

	inline Matrix4x4 &	GetWorldProject() const;

	mutable Matrix4x4	mWorldProject;
	Matrix4x4			mProjectionStack[MATRIX_STACK_SIZE];
	Matrix4x4			mModelViewStack[MATRIX_STACK_SIZE];
	Matrix4x4			mInvProjection;
	u32					mProjectionTop;
	u32					mModelViewTop;
	mutable bool		mWorldProjectValid;
	bool				mProjisNew;
	bool				mWPmodified;
		
	static const u32 	MAX_VERTICES = 500;	
	u16					m_swIndexBuffer[MAX_VERTICES];
	u32					m_dwNumIndices;

	// Processed vertices waiting for output...
	DaedalusVtx4		mVtxProjected[MAX_VERTS];			// Transformed and projected vertices (suitable for clipping etc)
	u32					mVtxClipFlagsUnion;					// Bitwise OR of all the vertex flags added to the current batch. If this is 0, we can trivially accept everything without clipping

	//
	//	BlendMode support
	//
	CBlendStates *		mCopyBlendStates;
	CBlendStates *		mFillBlendStates;
	typedef std::map< u64, SBlendStateEntry > BlendStatesMap;
	BlendStatesMap		mBlendStatesMap;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	//
	// Stats
	//
	u32					m_dwNumTrisRendered;
	u32					m_dwNumTrisClipped;
	u32					m_dwNumRect;

	// Debugging
	bool				mNastyTexture;
	bool				mRecordCombinerStates;
	std::set< u64 >		mRecordedCombinerStates;
	std::set< u64 >		mDisabledCombinerStates;
	std::set< u64 >		mUnhandledCombinerStates;
#endif
};

#endif // __DAEDALUS_D3DRENDER_H__
