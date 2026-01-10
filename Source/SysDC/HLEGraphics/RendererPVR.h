#ifndef PVR_RENDER_H
#define PVR_RENDER_H

#include "main.h"
#include "SurfaceHandler.h"
#include "DBGConsole.h"
#include "Memory.h"
#include "math/vector/vector2.h"
#include "math/vector/vector3.h"
#include "math/vector/vector4.h"
#include "math/matrix/matrix.h"
#include "math/plane/plane.h"
#include "RDP_GFX.h"

#define CLIPPING 0

#define DAEDALUS_MATRIX_STACK		60

	typedef struct _FiddledVtx
	{
		short y;
		short x;

		short flag;
		short z;

		short tv;
		short tu;

		union {
			struct _rgba {
				BYTE a;
				BYTE b;
				BYTE g;
				BYTE r;
			} rgba;
			struct _norma {
				char na;
				char nz;	// b
				char ny;	//g
				char nx;	//r
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

typedef struct tagTSSInfo
{
	DWORD dwColorOp;
	DWORD dwColorArg1;
	DWORD dwColorArg2;
	DWORD dwAlphaOp;
	DWORD dwAlphaArg1;
	DWORD dwAlphaArg2;

	DWORD dwFilter;

	DWORD dwAddressUMode;
	DWORD dwAddressVMode;

} TSSInfo;

enum
{
	D3DRENDER_LOAD = 0,
	D3DRENDER_MUL = 1
};

typedef struct D3DXCOLOR {
    float r;
    float g;
    float b;
    float a;
} D3DXCOLOR;

typedef struct
{
	tVector3 dir;		// Should be normalised
	D3DXCOLOR col;

} DaedalusLight;

typedef struct
{
    tVector3    pos;
	float       rhw;
	DWORD       dcDiffuse;
	float       tu, tv;
}N64VERTEX;

// Definiert Obergrenzen f�r transparente Vertices
#define MAX_TR_VERTS 4096
#define MAX_TR_GROUPS 256
typedef struct
{
    // Speichert den Polygon-Header
    pvr_poly_hdr_t  hdr;
    
    // Speichert den Start-Index im VertexBuffer
    DWORD           dwFirst;
    
    // Speichert die Anzahl an Vertices im VertexBuffer
    DWORD           dwCount;
}TRGROUP;

static matrix_t m_internal __attribute__((aligned(32)));

class PVRRender
{
public:
	PVRRender();
	~PVRRender();

	HRESULT Initialize();


	inline void SetScreenMult(float fMultX, float fMultY)
	{
		m_fScreenMultX = fMultX;
		m_fScreenMultY = fMultY;
	}

	inline void SetTextureEnable(BOOL bEnable)
	{
		m_bTextureEnable = bEnable;
	}


	inline void SetFilter(DWORD dwStage, DWORD dwMinFilter)
	{
		m_TSSInfo[dwStage].dwFilter = dwMinFilter;
	}

	inline void SetAddressU(DWORD dwStage, DWORD dwMode)
	{
        // Bei D3DTADDRESS_WRAP darf weder m_dwUVClamp noch m_dwUVFlip gesetzt sein
        if(dwMode == D3DTADDRESS_WRAP)
        {
            m_dwUVClamp = m_dwUVClamp &(~PVR_UVCLAMP_U);
            m_dwUVFlip = m_dwUVFlip &(~PVR_UVFLIP_U);
        }
        
        // Bei D3DTADDRESS_MIRROR darf nur m_dwUVFlip gesetzt sein
        if(dwMode == D3DTADDRESS_MIRROR)
        {
            m_dwUVClamp = m_dwUVClamp &(~PVR_UVCLAMP_U);
            m_dwUVFlip = m_dwUVFlip | PVR_UVFLIP_U;
        }
        
        // Bei D3DTADDRESS_CLAMP darf nur m_dwUVClamp gesetzt sein
        if(dwMode == D3DTADDRESS_CLAMP)
        {
            m_dwUVClamp = m_dwUVClamp | PVR_UVCLAMP_U;
            m_dwUVFlip = m_dwUVFlip &(~PVR_UVFLIP_U);
        }
	}
	inline void SetAddressV(DWORD dwStage, DWORD dwMode)
	{
        // Bei D3DTADDRESS_WRAP darf weder m_dwUVClamp noch m_dwUVFlip gesetzt sein
        if(dwMode == D3DTADDRESS_WRAP)
        {
            m_dwUVClamp = m_dwUVClamp &(~PVR_UVCLAMP_V);
            m_dwUVFlip = m_dwUVFlip &(~PVR_UVFLIP_V);
        }

        // Bei D3DTADDRESS_MIRROR darf nur m_dwUVFlip gesetzt sein
        if(dwMode == D3DTADDRESS_MIRROR)
        {
            m_dwUVClamp = m_dwUVClamp &(~PVR_UVCLAMP_V);
            m_dwUVFlip = m_dwUVFlip | PVR_UVFLIP_V;
        }

        // Bei D3DTADDRESS_CLAMP darf nur m_dwUVClamp gesetzt sein
        if(dwMode == D3DTADDRESS_CLAMP)
        {
            m_dwUVClamp = m_dwUVClamp | PVR_UVCLAMP_V;
            m_dwUVFlip = m_dwUVFlip &(~PVR_UVFLIP_V);
        }
	}

	inline void SetNumStages(DWORD dwMaxStage)
	{
	}

	inline void SetAmbientLight(DWORD dwLight)
	{
        m_dwAmbientLight = dwLight;
	}

	inline void SetPrimitiveColor(DWORD dwCol)
	{
        m_dwPrimitiveColor = dwCol;
	}


	inline void SetEnvColor(DWORD dwCol)
	{
        m_dwEnvColor = dwCol;
	}

	inline void SetTextureFactor(DWORD dwCol)
	{
	}


	inline void SetCullMode(BOOL bCullFront, BOOL bCullBack)
	{
        m_bCullFront = bCullFront;
		m_bCullBack = bCullBack;
	}

	inline void SetShadeMode(DWORD dwShadeMode)
	{
	}

	inline void SetLighting(BOOL bLighting)
	{
        m_bLightingEnable = bLighting;
	}



	inline void ZBufferEnable(BOOL bZBuffer)
	{
        if (m_bZBuffer != bZBuffer)
		{
			m_bZBuffer = bZBuffer;
			if (bZBuffer)
			{
				SetD3DRSZEnable( m_bZCompare );
				SetD3DRSZWriteEnable( m_bZUpdate );
			}
			else
			{
				SetD3DRSZEnable( false );
				SetD3DRSZWriteEnable( false );
			}
		}
	}

    inline void SetZCompare(BOOL bZCompare)
	{
        if (m_bZCompare != bZCompare)
		{
			m_bZCompare = bZCompare;

			if (m_bZBuffer)
				SetD3DRSZEnable( bZCompare );
		}
	}

	inline void SetZUpdate(BOOL bZUpdate)
	{
        if (m_bZUpdate != bZUpdate)
		{
			m_bZUpdate = bZUpdate;

			// Only update if ZBuffer is enabled
			if (m_bZBuffer)
				SetD3DRSZWriteEnable( bZUpdate );
		}
	}

	inline void SetTexturePerspective(DWORD dwTexturePerspective)
	{
	}

	inline void SetAlphaTestEnable(BOOL bAlphaTestEnable)
	{
		m_bAlphaTestEnable = bAlphaTestEnable;
	}
	
	inline void SetAlphaBlendEnable(BOOL bAlphaBlendEnable)
	{
		m_bAlphaBlendEnable = bAlphaBlendEnable;
	}
	
	inline void SetAlphaBlendFunc(DWORD src, DWORD dst)
	{
        m_dwAlphaBlendSrc = src;
        m_dwAlphaBlendDst = dst;
	}

	inline void SetTexture(SurfaceHandler *pSurf,
						   LONG  nTileLeft, LONG nTileTop,
						   DWORD dwTileWidth, DWORD dwTileHeight)
	{
        if( m_pSurf != pSurf )
        {
            m_pSurf = pSurf;

            m_dwTileWidth = dwTileWidth;
			m_dwTileHeight = dwTileHeight;

            m_fTexLeft = (float)nTileLeft;
			m_fTexTop  = (float)nTileTop;
			
			if( m_pSurf != NULL )
			{
                m_fTexWidth  = (float)m_pSurf->GetWidth();
				m_fTexHeight = (float)m_pSurf->GetHeight();
            }
            else
            {
                m_fTexWidth = (float)m_dwTileWidth;
				m_fTexHeight = (float)m_dwTileHeight;
            }
        }
	}

	// Generate texture coords?
	inline void SetTextureGen(BOOL bTextureGen)
	{
        m_bTextureGen = bTextureGen;
	}

	inline void SetTextureScale(float fScaleX, float fScaleY)
	{
        m_fTexScaleX = fScaleX;
		m_fTexScaleY = fScaleY;
	}

	inline void SetAlphaRef(DWORD dwAlpha)
	{

	}
	void SetCycleType(DWORD dwCycleType)
	{
        m_dwCycleType = dwCycleType;
    }
    DWORD GetCycleType()
    {
        return m_dwCycleType;
    }

	void SetMux(DWORD dwMux0, DWORD dwMux1)
	{
        m_Mux = (((u64)dwMux0) << 32) | (u64)dwMux1;
	}


	inline void SetFogMultOff(float fFogMult, float fFogOffset)
	{
        m_fFogMult = fFogMult;
		m_fFogOffset = fFogOffset;
		DL_PF("SetFogMultOff %f %f", fFogMult, fFogOffset);
	}

	inline void SetFogEnable(BOOL bEnable)
	{
        m_bFogEnable = bEnable;
	}

	inline void SetFogColor(DWORD dwCol)
	{
        m_dwFogColor = dwCol;
	}

	inline void SetZmodeDecal(BOOL bZModeDecal)
	{
        m_bZModeDecal = bZModeDecal;
	}

    inline void RecompileHeader()
    {
        // Evtl. Alpha-Blending behandeln
        DWORD list = PVR_LIST_OP_POLY;
        if(m_bAlphaBlendEnable || m_bAlphaTestEnable)
            list = PVR_LIST_TR_POLY;

        // Zwischen Polygonheader mit Textur und Polygonheader ohne Textur unterscheiden
        if( m_bTextureEnable && m_pSurf != NULL )
            pvr_poly_cxt_txr(&cxt, list, PVR_TXRFMT_ARGB4444, m_pSurf->GetCorrectedWidth(), m_pSurf->GetCorrectedHeight(), m_pSurf->GetSurface(), m_TSSInfo[0].dwFilter);
        else
			pvr_poly_cxt_col(&cxt, list);

        // Culling immer deaktivieren
        cxt.gen.culling = PVR_CULLING_NONE;
        
        // Textur-Adressierung festlegen
        cxt.txr.uv_clamp = m_dwUVClamp;
        cxt.txr.uv_flip = m_dwUVFlip;
        
        // Alpha-Blending-Mode festlegen
        if( m_bAlphaBlendEnable )
        {
            cxt.blend.src = m_dwAlphaBlendSrc;
            cxt.blend.dst = m_dwAlphaBlendDst;
        }
        
        // ZBuffer festlegen
        if( !m_dwrsZWriteEnable )
            cxt.depth.write = PVR_DEPTHWRITE_DISABLE;
        if( !m_dwrsZEnable )
        {
            cxt.depth.comparison = PVR_DEPTHCMP_ALWAYS;
            cxt.depth.write = PVR_DEPTHWRITE_DISABLE;
        }
        
        // Header kompilieren
		pvr_poly_compile(&hdr, &cxt);
    }

	// Assumes dwAddress has already been checked!
	// Don't inline - it's too big with the transform macros
	void SetNewVertexInfo(DWORD dwAddress, DWORD dwV0, DWORD dwNum)
	{
        LONG nFogR, nFogG, nFogB, nFogA;
		LONG nR, nG, nB, nA;

		if (m_bFogEnable)
		{
			nFogR = RGBA_GETRED(m_dwFogColor);
			nFogG = RGBA_GETGREEN(m_dwFogColor);
			nFogB = RGBA_GETBLUE(m_dwFogColor);
			nFogA = RGBA_GETALPHA(m_dwFogColor);
		}


		FiddledVtx * pVtxBase = (FiddledVtx*)(g_pu8RamBase + dwAddress);

        // Projektionsmatrix in die interne Hardware-Matrix des Dreamcast kopieren
        tMatrix matWorldProject = m_mModelView[m_dwModelViewTop] * m_mProjection[m_dwProjectionTop];
        memcpy(&m_internal, &matWorldProject, sizeof(matrix_t));
        mat_load(&m_internal);

        m_pNearPlane.a = matWorldProject.m[0][3] + matWorldProject.m[0][2];
        m_pNearPlane.b = matWorldProject.m[1][3] + matWorldProject.m[1][2];
        m_pNearPlane.c = matWorldProject.m[2][3] + matWorldProject.m[2][2];
        m_pNearPlane.d = matWorldProject.m[3][3] + matWorldProject.m[3][2];
        
		DWORD i;
		for (i = dwV0; i < dwV0 + dwNum; i++)
		{
			DWORD dwFlags;

			FiddledVtx & vert = pVtxBase[i - dwV0];
			
			m_vecUnProjected[i].x = (float)vert.x;
			m_vecUnProjected[i].y = (float)vert.y;
			m_vecUnProjected[i].z = (float)vert.z;
			
			m_fDist[i] = m_pNearPlane.distanceFrom(m_vecUnProjected[i]);
			
			m_vecProjected[i].x = (float)vert.x;
			m_vecProjected[i].y = (float)vert.y;
			m_vecProjected[i].z = (float)vert.z;
			m_fRHW[i] = 1.0f;
			
			mat_trans_single4(m_vecProjected[i].x, m_vecProjected[i].y, m_vecProjected[i].z, m_fRHW[i]);

			if (m_bLightingEnable)
			{
				tVector3 mn;		// modelnorm
				DWORD dwCol;
				tMatrix matWorld = m_mModelView[m_dwModelViewTop];

				mn.x = (float)vert.u0.norma.nx;
				mn.y = (float)vert.u0.norma.ny;
				mn.z = (float)vert.u0.norma.nz;

                /*Original
				Vec3TransformNormal(mn, matWorld);
				*/
				mn = mn.transformNormal( matWorld );
				
				m_vecTransformedNormal[i] = mn;

				// Do lighting
				dwCol = LightVert(mn);

				nR = RGBA_GETRED(dwCol);
				nG = RGBA_GETGREEN(dwCol);
				nB = RGBA_GETBLUE(dwCol);
				nA = RGBA_GETALPHA(dwCol);
			}
			else
			{
				nR = vert.u0.rgba.r;
				nG = vert.u0.rgba.g;
				nB = vert.u0.rgba.b;
				nA = vert.u0.rgba.a;
			}

			// Assign true vert colour after lighting/fogging
			m_dwVecCol[i] = RGBA_MAKE(nR, nG, nB, nA);

			if (m_bTextureEnable)
			{
				// Update texture coords n.b. need to divide tu/tv by bogus scale on addition to buffer
				tVector2 & t = m_vecTexture[i];

				// If the vert is already lit, then there is no normal (and hence we
				// can't generate tex coord)
				if (m_bTextureGen && m_bLightingEnable)
				{
                    
					tMatrix & matWV = m_mModelView[m_dwModelViewTop];
					tVector3 & norm = m_vecTransformedNormal[i];

					// Assign the spheremap's texture coordinates
					t.x = (0.5f * ( 1.0f + ( norm.x*matWV.m[0][0] +
						                     norm.y*matWV.m[1][0] +
											 norm.z*matWV.m[2][0] ) ));

					t.y = (0.5f * ( 1.0f - ( norm.x*matWV.m[0][1] +
						                     norm.y*matWV.m[1][1] +
											 norm.z*matWV.m[2][1] ) ));
				}
				else
				{
					t.x = (float)vert.tu * m_fTexScaleX;
					t.y = (float)vert.tv * m_fTexScaleY;
				}
			}
		}
	}


	// Assumes dwAddress has already been checked!
	// Don't inline - it's too big with the transform macros
	// DKR seems to use longer vert info
	void SetNewVertexInfoDKR(DWORD dwAddress, DWORD dwV0, DWORD dwNum)
	{
	}


	void SetVtxTextureCoord(DWORD dwV, short tu, short tv)
	{
        m_vecTexture[dwV].x = (float)tu * m_fTexScaleX;
		m_vecTexture[dwV].y = (float)tv * m_fTexScaleY;
	}

	HRESULT TexRect(LONG nX0, LONG nY0, LONG nX1, LONG nY1, float fS0, float fT0, float fS1, float fT1);
	HRESULT TexRectFlip(LONG nX0, LONG nY0, LONG nX1, LONG nY1, float fS0, float fT0, float fS1, float fT1);
	HRESULT FillRect(LONG nX0, LONG nY0, LONG nX1, LONG nY1, DWORD dwColor);



	inline void SetProjection(const tMatrix & mat, BOOL bPush, LONG nLoadReplace)
	{
        if (bPush)
		{
			if (m_dwProjectionTop >= (DAEDALUS_MATRIX_STACK-1))
				DBGConsole_Msg(0, "Pushing past proj stack limits!");
			else
				m_dwProjectionTop++;

			if (nLoadReplace == D3DRENDER_LOAD)
				// Load projection matrix
				m_mProjection[m_dwProjectionTop] = mat;
			else
				m_mProjection[m_dwProjectionTop] = (tMatrix)mat * m_mProjection[m_dwProjectionTop-1];

		}
		else
		{
			if (nLoadReplace == D3DRENDER_LOAD)
				// Load projection matrix
				m_mProjection[m_dwProjectionTop] = mat;
			else
				m_mProjection[m_dwProjectionTop] = (tMatrix)mat * m_mProjection[m_dwProjectionTop];

		}
	}

	inline void SetWorldView(const tMatrix & mat, BOOL bPush, LONG nLoadReplace)
	{
        // ModelView
		if (bPush)
		{
			if (m_dwModelViewTop >= (DAEDALUS_MATRIX_STACK-1))
				DBGConsole_Msg(0, "Pushing past modelview stack limits!");
			else
				m_dwModelViewTop++;

			// We should store the current projection matrix...
			if (nLoadReplace == D3DRENDER_LOAD)
			{
				// Load projection matrix
				m_mModelView[m_dwModelViewTop] = mat;
			}
			else			// Multiply projection matrix
			{
				m_mModelView[m_dwModelViewTop] = (tMatrix)mat * m_mModelView[m_dwModelViewTop-1];
			}
		}
		else	// NoPush
		{
			if (nLoadReplace == D3DRENDER_LOAD)
			{
				// Load projection matrix
				m_mModelView[m_dwModelViewTop] = mat;
            }
			else
			{
				// Multiply projection matrix
				m_mModelView[m_dwModelViewTop] = (tMatrix)mat * m_mModelView[m_dwModelViewTop];
			}

		}
	}

	inline void PopProjection()
	{
        if (m_dwProjectionTop > 0)
			m_dwProjectionTop--;
	}

	inline void PopWorldView()
	{
        if (m_dwModelViewTop > 0)
			m_dwModelViewTop--;
	}

	void SetViewport(LONG nLeft, LONG nTop, LONG nRight, LONG nBottom)
	{
        m_nVPLeft   = (LONG)(nLeft * m_fScreenMultX);
		m_nVPTop    = (LONG)(nTop  * m_fScreenMultY);
		m_nVPWidth  = (LONG)((nRight - nLeft) * m_fScreenMultX);
		m_nVPHeight = (LONG)((nBottom - nTop) * m_fScreenMultY);
	}

	// Returns TRUE if ok, FALSE if clipped
	BOOL TestTri(DWORD dwV0, DWORD dwV1, DWORD dwV2);
	void AddTri(DWORD dwV0, DWORD dwV1, DWORD dwV2);
	HRESULT StoreTris(N64VERTEX *v, DWORD count);
	inline void FlushN64Vertices(pvr_dr_state_t &dr_state, N64VERTEX *v, DWORD count);
	HRESULT FlushTRTris();
	HRESULT FlushN64Tris(N64VERTEX *v, DWORD count);
	HRESULT FlushTris();

	void SetLightCol(DWORD dwLight, DWORD dwCol)
	{
        m_Lights[dwLight].col.r = (float)((dwCol >> 24)&0xFF);
		m_Lights[dwLight].col.g = (float)((dwCol >> 16)&0xFF);
		m_Lights[dwLight].col.b = (float)((dwCol >>  8)&0xFF);
		m_Lights[dwLight].col.a = 255.0f;	// Ignore light alpha
	}

	void SetLightDirection(DWORD dwLight, float x, float y, float z)
	{
        // Invert the x/y/z
		m_Lights[dwLight].dir.x = x;
		m_Lights[dwLight].dir.y = y;
		m_Lights[dwLight].dir.z = z;
        /*Original
		D3DXVec3Normalize(&m_Lights[dwLight].dir,
						  &m_Lights[dwLight].dir);
        */
        m_Lights[dwLight].dir = m_Lights[dwLight].dir.normalize();
	}

	void SetNumLights(DWORD dwNumLights)
	{
        m_dwNumLights = dwNumLights;
	}


protected:
    TSSInfo m_TSSInfo[8];
    
	float m_fScreenMultX;
	float m_fScreenMultY;

	LONG m_nVPLeft;
	LONG m_nVPTop;
	LONG m_nVPWidth;
	LONG m_nVPHeight;

	BOOL m_bTextureEnable;
	BOOL m_bLightingEnable;

	DWORD m_dwNumStages;
	DWORD m_dwUVClamp;
	DWORD m_dwUVFlip;

	DWORD m_dwAmbientLight;
	LONG  m_dwNumLights;
	DWORD m_dwTextureFactor;

	DWORD m_dwCycleType;
	uint64 m_Mux;

	DWORD m_dwShadeMode;
	DWORD m_dwTexturePerspective;
	BOOL m_bAlphaTestEnable;
	BOOL m_bAlphaBlendEnable;
	DWORD m_dwAlphaBlendSrc;
	DWORD m_dwAlphaBlendDst;

	BOOL m_bZBuffer;
	BOOL m_bZUpdate;
	BOOL m_bZCompare;

	DWORD m_dwrsZEnable;
	DWORD m_dwrsZWriteEnable;

	BOOL m_bCullFront;
	BOOL m_bCullBack;


	DWORD m_dwAlpha;

	float m_fFogMult;
	float m_fFogOffset;

	BOOL  m_bFogEnable;
	DWORD m_dwFogColor;
	DWORD m_dwPrimitiveColor;
	DWORD m_dwEnvColor;

	// Texutring
	SurfaceHandler *m_pSurf;
	DaedalusLight m_Lights[8];
	
	DWORD m_dwTileWidth;
	DWORD m_dwTileHeight;
	float m_fTexWidth;
	float m_fTexHeight;		// Float to avoid converts when processing verts
	float m_fTexLeft;
	float m_fTexTop;		//
	float m_fTexScaleX;
	float m_fTexScaleY;

	BOOL m_bTextureGen;			// Generate texture coords?
	BOOL m_bZModeDecal;			// Decal stuff?
	
	tMatrix m_mProjection[DAEDALUS_MATRIX_STACK];
	tMatrix m_mModelView[DAEDALUS_MATRIX_STACK];

	DWORD	  m_dwProjectionTop;
	DWORD	  m_dwModelViewTop;

	N64VERTEX m_ucVertexBuffer[1024];
    N64VERTEX m_ucRectBuffer[6];
	N64VERTEX m_ucClipBuffer[3];
	N64VERTEX m_ucClipTmpBuffer[4];
	DWORD     m_dwNumVertices;

public:			// For DL_PF
    tVector3    m_vecProjected[32];
    tVector3    m_vecUnProjected[32];
    tPlane      m_pNearPlane;
	float		m_fRHW[32];
	float		m_fDist[32];
	tVector3    m_vecTransformedNormal[32];
	tVector2    m_vecTexture[32];
	DWORD		m_dwVecCol[32];
    TRGROUP     m_pTRPolyGroups[MAX_TR_GROUPS];
	DWORD       m_dwNumTRPolyGroups;
    N64VERTEX   m_pTRVertices[MAX_TR_VERTS];
    
    // Polygon-Header
    pvr_poly_cxt_t  cxt;
    pvr_poly_hdr_t  hdr;

	// Reset for a new frame
	inline void Reset()
	{
        // Matrizzen zur�cksetzen
        ResetMatrices();
        
        // Polygonspeicher zur�cksetzen
		m_dwNumVertices = 0;
		m_dwNumTRPolyGroups = 0;
		
		// RenderState zur�cksetzen
		m_bAlphaBlendEnable = FALSE;
		m_bAlphaTestEnable = FALSE;
		m_pSurf = NULL;
	}

protected:

    inline void InitRectVertex(N64VERTEX & v, long x, long y, float tu, float tv, DWORD color = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f));
    inline void InitN64Vertex(N64VERTEX & v, DWORD dwV);
	inline void InitTLitVertex(N64VERTEX & v, DWORD dwV);
	inline void InitLitVertex(N64VERTEX & v, DWORD dwV);
	void ClipN64Vertex(N64VERTEX &v, N64VERTEX &v0, DWORD dwV0, N64VERTEX &v1, DWORD dwV1);
	void ClipN64Vertices(N64VERTEX *v, DWORD dwV[]);

	DWORD LightVert(tVector3 & norm);


	//BOOL FindCombineMode(u64 mux, SetCombineInfo & sci);
	void InitBlendMode();

	HRESULT InitTUnlitVertices(DWORD dwMin, DWORD dwMax);
	HRESULT InitTLitVertices(DWORD dwMin, DWORD dwMax);

	HRESULT InitUnlitVertices(DWORD dwMin, DWORD dwMax);
	HRESULT InitLitVertices(DWORD dwMin, DWORD dwMax);


	inline void SetColorOp(DWORD dwStage, DWORD dwOp)
	{
        /*KOS
        if (m_TSSInfo[dwStage].dwColorOp != dwOp)
		{
			m_TSSInfo[dwStage].dwColorOp = dwOp;
			m_pD3DDev->SetTextureStageState( dwStage, D3DTSS_COLOROP, dwOp );
		}*/
	}
	inline void SetColorArg1(DWORD dwStage, DWORD dwArg1)
	{
        /*KOS
        if (m_TSSInfo[dwStage].dwColorArg1 != dwArg1 && dwArg1 != ~0)
		{
			m_TSSInfo[dwStage].dwColorArg1 = dwArg1;
			m_pD3DDev->SetTextureStageState( dwStage, D3DTSS_COLORARG1, dwArg1 );
		}*/
	}
	inline void SetColorArg2(DWORD dwStage, DWORD dwArg2)
	{
        /*KOS
        if (m_TSSInfo[dwStage].dwColorArg2 != dwArg2 && dwArg2 != ~0)
		{
			m_TSSInfo[dwStage].dwColorArg2 = dwArg2;
			m_pD3DDev->SetTextureStageState( dwStage, D3DTSS_COLORARG2, dwArg2 );
		}*/
	}

	inline void SetAlphaOp(DWORD dwStage, DWORD dwOp)
	{
        /*KOS
        if (m_TSSInfo[dwStage].dwAlphaOp != dwOp)
		{
			m_TSSInfo[dwStage].dwAlphaOp = dwOp;
			m_pD3DDev->SetTextureStageState( dwStage, D3DTSS_ALPHAOP, dwOp );
		}*/
	}

	inline void SetAlphaArg1(DWORD dwStage, DWORD dwArg1)
	{
        /*KOS
        if (m_TSSInfo[dwStage].dwAlphaArg1 != dwArg1 && dwArg1 != ~0)
		{
			m_TSSInfo[dwStage].dwAlphaArg1 = dwArg1;
			m_pD3DDev->SetTextureStageState( dwStage, D3DTSS_ALPHAARG1, dwArg1 );
		}
		*/
	}

	inline void SetAlphaArg2(DWORD dwStage, DWORD dwArg2)
	{
        /*KOS
        if (m_TSSInfo[dwStage].dwAlphaArg2 != dwArg2 && dwArg2 != ~0)
		{
			m_TSSInfo[dwStage].dwAlphaArg2 = dwArg2;
			m_pD3DDev->SetTextureStageState( dwStage, D3DTSS_ALPHAARG2, dwArg2 );
		}*/
	}

	inline void SetD3DRSZEnable(bool dwrsZEnable)
	{
        m_dwrsZEnable = dwrsZEnable;
	}
	inline void SetD3DRSZWriteEnable(bool dwrsZWriteEnable)
	{
		m_dwrsZWriteEnable = dwrsZWriteEnable;
	}

public:
	// Init matrix stack to identity matrices
	void ResetMatrices()
	{
        tMatrix mat;

		m_dwProjectionTop = 0;
		m_dwModelViewTop = 0;
		m_mProjection[0] = mat;
		m_mModelView[0] = mat;
	}

	void PrintActive()
	{
	}

};

#endif
