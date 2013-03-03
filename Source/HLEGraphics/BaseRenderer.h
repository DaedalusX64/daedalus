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

#ifndef BASERENDERER_H__
#define BASERENDERER_H__

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Utility/RefCounted.h"
#include "DaedalusVtx.h"
#include "Graphics/ColourValue.h"
#include "Utility/Preferences.h"
#include "RDP.h"

#include <pspgu.h>

class CTexture;
class CNativeTexture;

struct ViewportInfo
{
	u32  ViWidth;
	u32  ViHeight;

	bool Update;
	bool Rumble;
};

struct TextureVtx
{
	v2  t0;
	v3  pos;
};
/*
struct LineVtx
{
	c32	colour;
	v3  pos;
};
*/
//Cant be used for DKR since pointer start as odd and even addresses //Corn
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

        union
        {
			s16 flag;
            struct
            {
				s8 normz;
				u8 pad;
            };
        };
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

ALIGNED_TYPE(struct, DaedalusLight, 16)
{
	v3		Direction;		// w component is ignored. Should be normalised
	f32		Padding0;
	v4		Colour;			// Colour, components in range 0..1
};

DAEDALUS_STATIC_ASSERT( sizeof( DaedalusLight ) == 32 );

ALIGNED_TYPE(struct, TnLParams, 16)
{
	TnLPSP			Flags;
	u32				NumLights;
	float			TextureScaleX;
	float			TextureScaleY;
	DaedalusLight	Lights[16];	//Conker uses more than 8
};
//DAEDALUS_STATIC_ASSERT( sizeof( TnLParams ) == 32 );

// Bits for clipping
// +-+-+-
// xxyyzz
// NB: These are ordered such that the VFPU can generate them easily - make sure you keep the VFPU code up to date if changing these.
#define X_NEG  0x01	//left
#define Y_NEG  0x02	//bottom
#define Z_NEG  0x04	//far
#define X_POS  0x08	//right
#define Y_POS  0x10	//top
#define Z_POS  0x20	//near
#define CLIP_TEST_FLAGS ( X_POS | X_NEG | Y_POS | Y_NEG | Z_POS | Z_NEG )

enum CycleType
{
	CYCLE_1CYCLE = 0,		// Please keep in this order - matches RDP
	CYCLE_2CYCLE,
	CYCLE_COPY,
	CYCLE_FILL,
};

//*****************************************************************************
//
//*****************************************************************************
class BaseRenderer
{
public:
	BaseRenderer();
	virtual ~BaseRenderer();

	void				BeginScene();
	void				EndScene();

	void				SetVIScales();
	void				Reset();

	// Various rendering states
	inline void			SetTnLMode(u32 mode)					{ mTnL.Flags._u32 = (mTnL.Flags._u32 & TNL_TEXTURE) | mode; if(gFogEnabled) (mTnL.Flags.Fog)? sceGuEnable(GU_FOG) : sceGuDisable(GU_FOG); sceGuShadeModel( mTnL.Flags.Shade ? GU_SMOOTH : GU_FLAT ); }
	inline void			SetTextureEnable(bool enable)			{ mTnL.Flags.Texture = enable; }
	inline void			SetTextureTile(u32 tile)				{ mTextureTile = tile; }
	inline u32			GetTextureTile() const					{ return mTextureTile; }
	inline void			SetCullMode(bool enable, bool mode)		{ mTnL.Flags.TriCull = enable; mTnL.Flags.CullBack = mode; }

	// Fog stuff
	inline void			SetFogMinMax(float fMin, float fMax)	{ sceGuFog(fMin, fMax, mFogColour.GetColour()); }
	inline void			SetFogColour( c32 colour )				{ mFogColour = colour; }

	// PrimDepth will replace the z value if depth_source=1 (z range 32767-0 while PSP depthbuffer range 0-65535)//Corn
	inline void			SetPrimitiveDepth( u32 z )				{ mPrimDepth = (f32)( ( ( 32767 - z ) << 1) + 1 ); }
	inline void			SetPrimitiveColour( c32 colour )		{ mPrimitiveColour = colour; }
	inline c32			GetPrimitiveColour()					{ return mPrimitiveColour; }
	inline void			SetEnvColour( c32 colour )				{ mEnvColour = colour; }

	inline void			SetNumLights(u32 num)					{ mTnL.NumLights = num; }
	inline void			SetLightCol(u32 l, f32 r, f32 g, f32 b) { mTnL.Lights[l].Colour.x= r/255.0f; mTnL.Lights[l].Colour.y= g/255.0f; mTnL.Lights[l].Colour.z= b/255.0f; mTnL.Lights[l].Colour.w= 1.0f; }
	inline void			SetLightDirection(u32 l, f32 x, f32 y, f32 z)			{ v3 n(x, y, z); n.Normalise(); mTnL.Lights[l].Direction.x=n.x; mTnL.Lights[l].Direction.y=n.y; mTnL.Lights[l].Direction.z=n.z; mTnL.Lights[l].Padding0=0.0f; }

	inline void			SetMux( u64 mux )						{ mMux = mux; }
	inline void			SetAlphaRef(u32 alpha)					{ mAlphaThreshold = alpha; }

	// Texture stuff
	inline void			SetTextureScale(float fScaleX, float fScaleY)	{ mTnL.TextureScaleX = fScaleX; mTnL.TextureScaleY = fScaleY; }
	virtual void		Draw2DTextureBlit(f32 x, f32 y, f32 width, f32 height, f32 u0, f32 v0, f32 u1, f32 v1, CNativeTexture * texture) = 0;
	virtual void		Draw2DTexture(f32 frameX, f32 frameY, f32 frameW, f32 frameH, f32 imageX, f32 imageY, f32 imageW, f32 imageH) = 0;
	virtual void		Draw2DTextureR(f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3, f32 s, f32 t) = 0;


	// Viewport stuff
	void				SetN64Viewport( const v2 & scale, const v2 & trans );
	void				SetScissor( u32 x0, u32 y0, u32 x1, u32 y1 );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	void				PrintActive();
#endif
	void				ResetMatrices();
	void				SetProjectionDKR(const u32 address, bool mul, u32 idx);
	void				SetProjection(const u32 address, bool bPush, bool bReplace);
	void				SetWorldView(const u32 address, bool bPush, bool bReplace);
	inline void			PopProjection() {if (mProjectionTop > 0) --mProjectionTop;	mWorldProjectValid = false;}
	inline void			PopWorldView()	{if (mModelViewTop > 0)	 --mModelViewTop;	mWorldProjectValid = false;}
	void				InsertMatrix(u32 w0, u32 w1);
	void				ForceMatrix(const u32 address);
	inline void			DKRMtxChanged( u32 idx )	{mWPmodified = true; mDKRMatIdx = idx;}

	// Vertex stuff
	void				SetNewVertexInfo(u32 address, u32 v0, u32 n);	// Assumes dwAddress has already been checked!
	void				SetNewVertexInfoConker(u32 address, u32 v0, u32 n);	// For conker..
	void				SetNewVertexInfoDKR(u32 address, u32 v0, u32 n);	// Assumes dwAddress has already been checked!
	void				SetNewVertexInfoPD(u32 address, u32 v0, u32 n);	// Assumes dwAddress has already been checked!
	void				ModifyVertexInfo(u32 whered, u32 vert, u32 val);
	void				SetVtxColor( u32 vert, c32 color );
	inline void			SetVtxTextureCoord( u32 vert, s16 tu, s16 tv ) { mVtxProjected[vert].Texture.x = (f32)tu * (1.0f / 32.0f); mVtxProjected[vert].Texture.y = (f32)tv * (1.0f / 32.0f); }
	inline void			SetVtxXY( u32 vert, float x, float y );
	void				SetVtxZ( u32 vert, float z );
	inline void			CopyVtx( u32 vert_src, u32 vert_dst ) { mVtxProjected[vert_dst] = mVtxProjected[vert_src]; }

	// TextRect stuff
	void				TexRect( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 );
	void				TexRectFlip( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 );
	void				FillRect( const v2 & xy0, const v2 & xy1, u32 color );

	// Returns true if triangle visible, false otherwise
	bool				AddTri(u32 v0, u32 v1, u32 v2);

	// Render our current triangle list to screen
	void				FlushTris();
	//void				Line3D( u32 v0, u32 v1, u32 width );

	// Returns true if bounding volume is visible within NDC box, false if culled
	inline bool			TestVerts( u32 v0, u32 vn ) const		{ u32 f=mVtxProjected[v0].ClipFlags; for( u32 i=v0+1; i<=vn; i++ ) f&=mVtxProjected[i].ClipFlags; return f==0; }
	inline s32			GetVtxDepth( u32 i ) const				{ return (s32)mVtxProjected[ i ].ProjectedPos.z; }
	inline v4			GetTransformedVtxPos( u32 i ) const		{ return mVtxProjected[ i ].TransformedPos; }
	inline v4			GetProjectedVtxPos( u32 i ) const		{ return mVtxProjected[ i ].ProjectedPos; }
	inline u32			GetVtxFlags( u32 i ) const				{ return mVtxProjected[ i ].ClipFlags; }

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	// Rendering stats
	inline u32			GetNumTrisRendered() const				{ return mNumTrisRendered; }
	inline u32			GetNumTrisClipped() const				{ return mNumTrisClipped; }
	inline u32			GetNumRect() const						{ return mNumRect; }


	virtual void 		ResetDebugState()						{}
#endif

protected:
	inline void			EnableTexturing( u32 tile_idx );
	void				EnableTexturing( u32 index, u32 tile_idx );

	void				RestoreRenderStates();

	void				SetPSPViewport( s32 x, s32 y, u32 w, u32 h );
	void				UpdateViewport();

	void				ConvertN64ToPsp( const v2 & n64_coords, v2 & answ ) const;

	virtual void		RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_mode, bool disable_zbuffer ) = 0;

	// Old code, kept for reference
#ifdef DAEDALUS_IS_LEGACY
	void 				TestVFPUVerts( u32 v0, u32 num, const FiddledVtx * verts, const Matrix4x4 & mat_world );
	template< bool FogEnable, int TextureMode >
	void ProcessVerts( u32 v0, u32 num, const FiddledVtx * verts, const Matrix4x4 & mat_world );
#endif

	void				PrepareTrisClipped( DaedalusVtx ** p_p_vertices, u32 * p_num_vertices ) const;
	void				PrepareTrisUnclipped( DaedalusVtx ** p_p_vertices, u32 * p_num_vertices ) const;

	v4					LightVert( const v3 & norm ) const;

protected:
	enum { MAX_VERTS = 80 };		// F3DLP.Rej supports up to 80 verts!

	TnLParams			mTnL;

	v2					mN64ToPSPScale;
	v2					mN64ToPSPTranslate;

	v2					mVpScale;
	v2					mVpTrans;

	u64					mMux;

	u32					mTextureTile;

	u32					mAlphaThreshold;

	f32					mPrimDepth;

	c32					mFogColour;
	c32					mPrimitiveColour;
	c32					mEnvColour;

	// Texturing
	struct TextureWrap
	{
		u32	u;
		u32 v;
	};
	static const u32 NUM_N64_TEXTURES = 2;

	CRefPtr<CTexture>	mpTexture[ NUM_N64_TEXTURES ];
	v2					mTileTopLeft[ NUM_N64_TEXTURES ];
	v2					mTileScale[ NUM_N64_TEXTURES ];
	TextureWrap			mTexWrap[ NUM_N64_TEXTURES ];

	//Max is 18 according to the manual //Corn
	static const u32 MATRIX_STACK_SIZE = 20;

	inline Matrix4x4 &	GetWorldProject();

	mutable Matrix4x4	mWorldProject;
	Matrix4x4			mProjectionStack[MATRIX_STACK_SIZE];
	Matrix4x4			mModelViewStack[MATRIX_STACK_SIZE];
	Matrix4x4			mInvProjection;
	u32					mProjectionTop;
	u32					mModelViewTop;
	mutable bool		mWorldProjectValid;
	bool				mReloadProj;
	bool				mWPmodified;
	u32					mDKRMatIdx;

	static const u32 	MAX_VERTICES = 320;	//we need at least 80 verts * 3 = 240? But Flying Dragon uses more than 256 //Corn
	u16					m_swIndexBuffer[MAX_VERTICES];
	u32					mNumIndices;

	// Processed vertices waiting for output...
	DaedalusVtx4		mVtxProjected[MAX_VERTS];			// Transformed and projected vertices (suitable for clipping etc)
	u32					mVtxClipFlagsUnion;					// Bitwise OR of all the vertex flags added to the current batch. If this is 0, we can trivially accept everything without clipping


#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	//
	// Stats
	//
	u32					mNumTrisRendered;
	u32					mNumTrisClipped;
	u32					mNumRect;

	// Debugging
	bool				mNastyTexture;
#endif
};

bool CreateRenderer();
void DestroyRenderer();
extern BaseRenderer * gRenderer;

#endif // BASERENDERER_H__
