#ifndef PVR_RENDER_H
#define PVR_RENDER_H
#include "StdAfx.h"

#include "core/memory.h"
#include "Debug/MyDebug.h"
#include "Debug/DBGConsole.h"

#include "texture.h"
#include "texturecache.h"
#include "convertImage.h"
#include "nativeTexture.h"
#include "math/vmath.h"
#include "RDP.h"

// Blend-Modes
#define BLEND_NOOP1				0x00000000
#define BLEND_NOOP2				0x00000000

#define BLEND_FOG_ASHADE1		0xc8000000
#define BLEND_FOG_APRIM1		0xc4000000

#define BLEND_PASS1				0x0c080000
#define BLEND_PASS2				0x03020000

#define BLEND_OPA1				0x00440000
#define BLEND_OPA2				0x00110000

#define BLEND_XLU1				0x00400000
#define BLEND_XLU2				0x00100000

#define BLEND_ADD1				0x04400000
#define BLEND_ADD2				0x01100000

#define BLEND_MEM1				0x4c400000		// Mem*0 + Mem*(1-0)?!
#define BLEND_MEM2				0x13100000		// Mem*0 + Mem*(1-0)?!

// Texture stuff
enum ETextureAddressMode
{
	TAM_CLAMP = 0,
	TAM_MIRROR,
	TAM_WRAP,
	TAM_NUM_ADDRESS_MODES,
};

#define DAEDALUS_MATRIX_STACK		60

typedef struct _FiddledVtx
{
	s16 y;
	s16 x;

	s16 flag;
	s16 z;

	s16 tv;
	s16 tu;

	union {
		struct _rgba {
			u8 a;
			u8 b;
			u8 g;
			u8 r;
		} rgba;
		struct _norma {
			s8 na;
			s8 nz;	// b
			s8 ny;	//g
			s8 nx;	//r
		} norma;
	}u0;
} FiddledVtx;

typedef struct _FiddledVtxDKR
{
	s16 y;
	s16 x;

	u8 a;
	u8 b;
	s16 z;

	u8 g;
	u8 r;

} FiddledVtxDKR;

typedef struct tColorF
{
public:
    // Initialisierungskonstruktor
    tColorF(float x, float y, float z, float w)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }
    
    // Standardkonstruktor
    tColorF() {}
    
    // Attribute
    float x, y, z, w;
}tColorF;

ALIGNED_TYPE(struct, DaedalusLight, 16)
{
	tVector3 Direction;		// w compoonent is ignored. Should be normalised
	f32		 Padding0;
	tColorF  Colour;		// Colour, components in range 0..1
};

typedef struct
{
    tVector3    pos;
	float       rhw;
	u32         dcDiffuse;
	float       tu, tv;
}N64VERTEX;

// Definiert, wie viele Vertices transformiert gespeichert werden können
enum { MAX_VERTS = 32 };

// Definiert die Größe des VertexBuffers, der zum Zwischenspeichern transparenter Dreiecke verwendet wird
#define TR_BUFFER_SIZE		(512 * 1024)

static matrix_t m_internal __attribute__((aligned(32)));

class PVRRenderer : public CSingleton< PVRRenderer >
{
public:
	PVRRenderer();
	~PVRRenderer();
	
	// Generic
	void				BeginScene();
	void				EndScene();
	void                Reset();

	// Vertex stuff
	void				SetNewVertexInfo(u32 dwAddress, u32 dwV0, u32 dwNum);	// Assumes dwAddress has already been checked!
	void				SetNewVertexInfoDKR(u32 dwAddress, u32 dwV0, u32 dwNum);	// Assumes dwAddress has already been checked!
	void				SetVtxColor( u32 vert, u32 color );
	void				SetVtxTextureCoord( u32 vert, short tu, short tv );
	void				SetVtxXY( u32 vert, float x, float y );
	
	// Texture stuff
	enum ETextureAddressMode
	{
		TAM_CLAMP = 0,
		TAM_MIRROR,
		TAM_WRAP,
		TAM_NUM_ADDRESS_MODES,
	};

	void				SetTexture( u32 index, CTexture* lpsTexture, s32  nTileLeft, s32 nTileTop, u32 dwTileWidth, u32 dwTileHeight);
	void				SetTextureGen(bool bTextureGen);
	void				SetTextureScale(float fScaleX, float fScaleY);
	void				SetAddressMode( u32 index, ETextureAddressMode mode_u, ETextureAddressMode mode_v );
	
	// Viewport stuff
	void				SetD3DViewport( u32 dwX, u32 dwY, u32 dwWidth, u32 dwHeight );
	void				SetN64Viewport( const tVector3 & scale, const tVector3 & trans ) {}     // Noch nicht implementiert
	void				SetScissor( s32 x0, s32 y0, s32 x1, s32 y1 ) {}                         // Noch nicht implementiert
	
	bool				TestVerts( u32 v0, u32 vn ) const		{ return true; } // Noch nicht implementiert
	u32					GetVtxFlags( u32 i ) const				{ return 0; }
	
	// Matrix stuff
	enum EMatrixLoadStyle
	{
		MATRIX_LOAD = 0,
		MATRIX_MUL = 1
	};

	void				ResetMatrices();
	void				SetProjection(const tMatrix & mat, bool bPush, EMatrixLoadStyle load_style);
	void				SetWorldView(const tMatrix & mat, bool bPush, EMatrixLoadStyle load_style);
	void				PopProjection();
	void				PopWorldView();
	
	// TextRect stuff
	bool				TexRect( const tVector2 & xy0, const tVector2 & xy1, const tVector2 & uv0, const tVector2 & uv1 );
	bool				TexRectFlip( const tVector2 & xy0, const tVector2 & xy1, const tVector2 & uv0, const tVector2 & uv1 );
	bool				FillRect( const tVector2 & xy0, const tVector2 & xy1, u32 color );

	// Alpha stuff
	void                SetAlphaTestEnable(bool bEnable)        { m_bAlphaTestEnable = bEnable; }
    void                SetAlphaBlendEnable(bool bEnable)       { m_bAlphaBlendEnable = bEnable; }
	void                SetAlphaBlendFunc(DWORD src, DWORD dst) { m_dwAlphaBlendSrc = src; m_dwAlphaBlendDst = dst; }

	// Various rendering states
	void				SetTextureEnable( bool bEnable )		{ m_bTextureEnable = bEnable; }
	void				SetPrimitiveColor( u32 colour )			{ m_dwPrimitiveColor = colour; }
	void				SetEnvColor( u32 colour )				{ m_dwEnvColor = colour; }
	void				SetLighting( bool bLighting )			{ m_bLightingEnable = bLighting; }
	void				ZBufferEnable( bool bZBuffer )			{ m_bZBuffer = bZBuffer; }
	
	void				SetFogMult( float fFogMult )			{ m_fFogMult = fFogMult; }
	void				SetFogOffset( float fFogOffset )		{ m_fFogOffset = fFogOffset; }
	void				SetFogEnable( bool bEnable )			{ m_bFogEnable = bEnable; }
	void				SetFogColor( u32 colour )				{ m_dwFogColor = colour; }

	void				SetCullMode( bool bCullFront, bool bCullBack );
	void				SetSmooth( bool bSmooth )				{ mSmooth = bSmooth; }
    void				SetSmoothShade( bool bSmoothShade )		{ mSmoothShade = bSmoothShade; }
	void				SetAlphaRef(u32 dwAlpha)                {}

	inline void InitRectVertex(N64VERTEX &v, float x, float y, float tu, float tv, uint32 color = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f));
    inline void InitN64Vertex(N64VERTEX & v, u32 dwV);
	inline void InitTLitVertex(N64VERTEX & v, u32 dwV);
	inline void InitLitVertex(N64VERTEX & v, u32 dwV);
	void ClipN64Vertex(N64VERTEX &v, N64VERTEX &v0, u32 dwV0, N64VERTEX &v1, u32 dwV1);
	void ClipN64Vertices(N64VERTEX *v, u32 dwV[]);

	tColorF LightVert(tVector3 & norm);


	void InitTUnlitVertices(u32 dwMin, u32 dwMax);
	void InitTLitVertices(u32 dwMin, u32 dwMax);

	void InitUnlitVertices(u32 dwMin, u32 dwMax);
	void InitLitVertices(u32 dwMin, u32 dwMax);

    void InitBlendMode();
    void RecompileHeader();

	// Returns TRUE if ok, FALSE if clipped
	bool TestTri(u32 dwV0, u32 dwV1, u32 dwV2);
	bool AddTri(u32 dwV0, u32 dwV1, u32 dwV2);
	void StoreTris(N64VERTEX *v, u32 count);
	inline void FlushN64Vertices(pvr_dr_state_t &dr_state, N64VERTEX *v, u32 count);
	void FlushN64Tris(N64VERTEX *v, u32 count);
	void FlushTris();

    // Lighting stuff
	void SetLightCol(u32 dwLight, u32 colour);
	void SetAmbientLight(float r, float g, float b);
	void SetLightDirection(u32 dwLight, float x, float y, float z);
	void SetNumLights(u32 dwNumLights)                          { m_dwNumLights = dwNumLights; }


protected:
	float m_fScreenMultX;
	float m_fScreenMultY;
	
    u32	  m_dwProjectionTop;
	u32	  m_dwModelViewTop;

	long m_nVPLeft;
	long m_nVPTop;
	long m_nVPWidth;
	long m_nVPHeight;

	bool m_bTextureEnable;
	bool m_bLightingEnable;

	tColorF mAmbientColour;
	long  m_dwNumLights;

	bool m_bAlphaTestEnable;
	bool m_bAlphaBlendEnable;
	u32 m_dwAlphaBlendSrc;
	u32 m_dwAlphaBlendDst;

	bool m_bZBuffer;
	
	bool m_bCullFront;
	bool m_bCullBack;
	
	bool mSmooth;
	bool mSmoothShade;

	float m_fFogMult;
	float m_fFogOffset;

	bool  m_bFogEnable;
	u32 m_dwFogColor;
	
	u32 m_dwPrimitiveColor;
	u32 m_dwEnvColor;

	// Texutring
	CTexture *			m_lpsTexture;
	DaedalusLight		mLights[8];

	float				m_fTileWidth;		// Float to avoid converts when processing verts
	float				m_fTileHeight;
	float				m_fTexWidth;
	float				m_fTexHeight;		// Float to avoid converts when processing verts

	// Reciprocals for fast mults
	float				m_fInvTileWidth;
	float				m_fInvTileHeight;
	float				m_fInvTexWidth;
	float				m_fInvTexHeight;

	float				m_fTexLeft;
	float				m_fTexTop;
	float				m_fTexScaleX;
	float				m_fTexScaleY;

	bool				m_bTextureGen;			// Generate texture coords?
	u32                 m_dwTexFilter;
	
	u32                 m_dwUVClamp;
	u32                 m_dwUVFlip;
	
	tMatrix m_mProjection[DAEDALUS_MATRIX_STACK];
	tMatrix m_mModelView[DAEDALUS_MATRIX_STACK];

	N64VERTEX   m_ucVertexBuffer[1024];
    N64VERTEX   m_ucRectBuffer[6];
	N64VERTEX   m_ucClipBuffer[3];
	N64VERTEX   m_ucClipTmpBuffer[4];
	u32         m_dwNumVertices;

    tVector3    m_vecProjected[MAX_VERTS];
    tVector3    m_vecUnProjected[MAX_VERTS];
	float		m_fRHW[MAX_VERTS];
	float		m_fDist[MAX_VERTS];
	tVector2    m_vecTexture[MAX_VERTS];
	u32         m_dwVecCol[MAX_VERTS];
    
	// Speichert den VertexBuffer für die tranparenten Vertices
	u8			m_pVertexBuffer[TR_BUFFER_SIZE];
	u8*			m_pVertexBufferPtr;
    
    // Polygon-Header
    pvr_poly_cxt_t  cxt;
    pvr_poly_hdr_t  hdr;
};

#endif
