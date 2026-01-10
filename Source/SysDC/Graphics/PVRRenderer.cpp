#include <stdlib.h>
#include <assert.h>
#include "ultra_gbi.h"
#include "PVRRenderer.h"

enum CycleType
{
	CYCLE_1CYCLE = 0,		// Please keep in this order - matches RDP
	CYCLE_2CYCLE,
	CYCLE_COPY,
	CYCLE_FILL,
};

PVRRenderer::PVRRenderer():
	m_fScreenMultX(2.0f),
	m_fScreenMultY(2.0f),

	m_dwProjectionTop(0),
	m_dwModelViewTop(0),

	m_bTextureEnable(FALSE),
	m_bLightingEnable(FALSE),

	m_dwNumLights(0),

	m_bAlphaTestEnable(FALSE),
	m_bAlphaBlendEnable(FALSE),

	m_bZBuffer(FALSE),

	m_bCullFront(FALSE),
	m_bCullBack(TRUE),

	mSmooth(true),
	mSmoothShade(true),
	
	m_fFogMult(0.0f),			// * 0 + 0 = 0
	m_fFogOffset(0.0f),
	m_bFogEnable(FALSE),
	m_dwFogColor(0x00FFFFFF),
	
    m_lpsTexture( NULL ),

    m_fTileWidth(32.0f),
	m_fTileHeight(32.0f),
	m_fTexWidth(32.0f),
	m_fTexHeight(32.0f),
	m_fInvTileWidth(1/32.0f),
	m_fInvTileHeight(1/32.0f),
	m_fInvTexWidth(1/32.0f),
	m_fInvTexHeight(1/32.0f),

	m_bTextureGen(FALSE),
	m_dwTexFilter(PVR_FILTER_BILINEAR),
	
	m_dwUVClamp(0),
	m_dwUVFlip(0),

	m_dwNumVertices(0),
	m_pVertexBufferPtr(&m_pVertexBuffer[0])
{
    mAmbientColour = tColorF( 1.0f, 1.0f, 1.0f, 1.0f );
}

PVRRenderer::~PVRRenderer()
{
}

//*****************************************************************************
// Creator function for singleton
//*****************************************************************************
template<> bool CSingleton< PVRRenderer >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new PVRRenderer();
	if (!mpInstance)
	{
		return false;
	}

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void PVRRenderer::BeginScene()
{
    // Szene beginnen
    pvr_wait_ready();
    pvr_scene_begin();

	// VertexBuffer für transparente Vertices zurücksetzen
	m_pVertexBufferPtr = &m_pVertexBuffer[0];

    // Liste mit nicht-transparenten Polygonen beginnen
    pvr_list_begin( PVR_LIST_OP_POLY );
}

//*****************************************************************************
//
//*****************************************************************************
void PVRRenderer::EndScene()
{
    // Liste mit nicht-transparenten Polygonen abschließen
    pvr_list_finish();

	// Liste mit transparenten Polygonen rendern
    pvr_list_begin(PVR_LIST_TR_POLY);
	sq_cpy((void *)PVR_TA_INPUT, (void *)m_pVertexBuffer, m_pVertexBufferPtr - m_pVertexBuffer);
    pvr_list_finish();
    
    // Szene abschließen
    pvr_scene_finish();
}

void PVRRenderer::Reset()
{
    // Matrizzen zurücksetzen
    ResetMatrices();

    // Polygonspeicher zurücksetzen
	m_dwNumVertices = 0;
	m_pVertexBufferPtr = &m_pVertexBuffer[0];

	// RenderState zurücksetzen
	m_bAlphaBlendEnable = FALSE;
	m_bAlphaTestEnable = FALSE;
}

//*****************************************************************************
//
//*****************************************************************************
void PVRRenderer::SetTexture( u32 index,
							CTexture* lpsTexture,
							s32  nTileLeft, s32 nTileTop,
							u32 dwTileWidth, u32 dwTileHeight )
{
    // Nur Texturen für die erste Texture-Stage annehmen
    if(index != 0)
        return;
        
    //lpsTexture = NULL; - For wireframe
	if (m_lpsTexture != lpsTexture)
	{
		// Dereference the previous texture
		if ( m_lpsTexture )
		{
			m_lpsTexture->Release();
			m_lpsTexture = NULL;
		}

		m_lpsTexture = lpsTexture;

		if ( m_lpsTexture )
		{
			m_lpsTexture->AddRef();

			m_fTileWidth = (float)dwTileWidth;
			m_fTileHeight = (float)dwTileHeight;

			// Update the bogus scale parameters
			m_fTexLeft = (float)nTileLeft;
			m_fTexTop  = (float)nTileTop;

			m_fTexWidth = (float)m_lpsTexture->GetWidth();
			m_fTexHeight = (float)m_lpsTexture->GetHeight();

			m_fInvTileWidth = 1.0f / m_fTileWidth;
			m_fInvTileHeight = 1.0f / m_fTileHeight;
			m_fInvTexWidth = 1.0f / m_fTexWidth;
			m_fInvTexHeight = 1.0f / m_fTexHeight;
		}
	}
}

void PVRRenderer::SetTextureScale(float fScaleX, float fScaleY)
{
    m_fTexScaleX = fScaleX;
	m_fTexScaleY = fScaleY;
}

// Generate texture coords?
void PVRRenderer::SetTextureGen(bool bTextureGen)
{
    m_bTextureGen = bTextureGen;
}

void PVRRenderer::SetAddressMode( u32 index, ETextureAddressMode mode_u, ETextureAddressMode mode_v )
{
    // Bei TAM_WRAP darf weder m_dwUVClamp noch m_dwUVFlip gesetzt sein
    if(mode_u == TAM_WRAP)
    {
        m_dwUVClamp = m_dwUVClamp &(~PVR_UVCLAMP_U);
        m_dwUVFlip = m_dwUVFlip &(~PVR_UVFLIP_U);
    }

    // Bei TAM_MIRROR darf nur m_dwUVFlip gesetzt sein
    if(mode_u == TAM_MIRROR)
    {
        m_dwUVClamp = m_dwUVClamp &(~PVR_UVCLAMP_U);
        m_dwUVFlip = m_dwUVFlip | PVR_UVFLIP_U;
    }

    // Bei TAM_CLAMP darf nur m_dwUVClamp gesetzt sein
    if(mode_u == TAM_CLAMP)
    {
        m_dwUVClamp = m_dwUVClamp | PVR_UVCLAMP_U;
        m_dwUVFlip = m_dwUVFlip &(~PVR_UVFLIP_U);
    }
    
    // Bei TAM_WRAP darf weder m_dwUVClamp noch m_dwUVFlip gesetzt sein
    if(mode_v == TAM_WRAP)
    {
        m_dwUVClamp = m_dwUVClamp &(~PVR_UVCLAMP_V);
        m_dwUVFlip = m_dwUVFlip &(~PVR_UVFLIP_V);
    }

    // Bei TAM_MIRROR darf nur m_dwUVFlip gesetzt sein
    if(mode_v == TAM_MIRROR)
    {
        m_dwUVClamp = m_dwUVClamp &(~PVR_UVCLAMP_V);
        m_dwUVFlip = m_dwUVFlip | PVR_UVFLIP_V;
    }

    // Bei TAM_CLAMP darf nur m_dwUVClamp gesetzt sein
    if(mode_v == TAM_CLAMP)
    {
        m_dwUVClamp = m_dwUVClamp | PVR_UVCLAMP_V;
        m_dwUVFlip = m_dwUVFlip &(~PVR_UVFLIP_V);
    }
}

//*****************************************************************************
//
//*****************************************************************************
void PVRRenderer::SetCullMode( bool bCullFront, bool bCullBack )
{
	m_bCullFront = bCullFront;
	m_bCullBack = bCullBack;
}

void PVRRenderer::SetD3DViewport( u32 dwX, u32 dwY, u32 dwWidth, u32 dwHeight )
{
    m_fScreenMultX = (float)dwWidth  / 320.0f;
	m_fScreenMultY = (float)dwHeight / 240.0f;

    m_nVPLeft   = dwX;
	m_nVPTop    = dwY;
	m_nVPWidth  = dwWidth;
	m_nVPHeight = dwHeight;
}

// Init matrix stack to identity matrices
void PVRRenderer::ResetMatrices()
{
    tMatrix mat;

	m_dwProjectionTop = 0;
	m_dwModelViewTop = 0;
	m_mProjection[0] = mat;
	m_mModelView[0] = mat;
}

void PVRRenderer::SetProjection(const tMatrix & mat, bool bPush, EMatrixLoadStyle load_style)
{
    if (bPush)
	{
		if (m_dwProjectionTop >= (DAEDALUS_MATRIX_STACK-1))
			DBGConsole_Msg(0, "Pushing past proj stack limits!");
		else
			m_dwProjectionTop++;

		if (load_style == MATRIX_LOAD)
			// Load projection matrix
			m_mProjection[m_dwProjectionTop] = mat;
		else
			m_mProjection[m_dwProjectionTop] = (tMatrix)mat * m_mProjection[m_dwProjectionTop-1];

	}
	else
	{
		if (load_style == MATRIX_LOAD)
			// Load projection matrix
			m_mProjection[m_dwProjectionTop] = mat;
		else
			m_mProjection[m_dwProjectionTop] = (tMatrix)mat * m_mProjection[m_dwProjectionTop];

	}
}
	
void PVRRenderer::SetWorldView(const tMatrix & mat, bool bPush, EMatrixLoadStyle load_style)
{
    // ModelView
	if (bPush)
	{
		if (m_dwModelViewTop >= (DAEDALUS_MATRIX_STACK-1))
			DBGConsole_Msg(0, "Pushing past modelview stack limits!");
		else
			m_dwModelViewTop++;

		// We should store the current projection matrix...
		if (load_style == MATRIX_LOAD)
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
		if (load_style == MATRIX_LOAD)
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

void PVRRenderer::PopProjection()
{
    if (m_dwProjectionTop > 0)
		m_dwProjectionTop--;
}

void PVRRenderer::PopWorldView()
{
    if (m_dwModelViewTop > 0)
		m_dwModelViewTop--;
}

//*****************************************************************************
//
//*****************************************************************************
void PVRRenderer::SetAmbientLight( float r, float g, float b )
{
	mAmbientColour.x = r;
	mAmbientColour.y = g;
	mAmbientColour.z = b;
	//mAmbientColour.w = 1.0f;
}

//*****************************************************************************
//
//*****************************************************************************
void PVRRenderer::SetLightCol(u32 dwLight, u32 colour)
{
	mLights[dwLight].Colour.x = (float)((colour >> 24)&0xFF) / 255.0f;
	mLights[dwLight].Colour.y = (float)((colour >> 16)&0xFF) / 255.0f;
	mLights[dwLight].Colour.z = (float)((colour >>  8)&0xFF) / 255.0f;
	mLights[dwLight].Colour.w = 1.0f;	// Ignore light alpha
}

//*****************************************************************************
//
//*****************************************************************************
void PVRRenderer::SetLightDirection(u32 l, float x, float y, float z)
{
	tVector3	normal( x, y, z );
	normal = normal.normalize();

	mLights[l].Direction.x = normal.x;
	mLights[l].Direction.y = normal.y;
	mLights[l].Direction.z = normal.z;
	mLights[l].Padding0 = 0.0f;
}

tColorF PVRRenderer::LightVert(tVector3 & norm)
{
	// Do ambient
	tColorF	result( mAmbientColour );

	for ( s32 l = 0; l < m_dwNumLights; l++ )
	{
		f32 fCosT = norm.dot( &mLights[l].Direction );
		if (fCosT > 0)
		{
			result.x += mLights[l].Colour.x * fCosT;
			result.y += mLights[l].Colour.y * fCosT;
			result.z += mLights[l].Colour.z * fCosT;
		//	result.w += mLights[l].Colour.w * fCosT;
		}
	}

	if( result.x > 1.0f ) result.x = 1.0f;
	if( result.y > 1.0f ) result.y = 1.0f;
	if( result.z > 1.0f ) result.z = 1.0f;
	if( result.w > 1.0f ) result.w = 1.0f;

	return result;
}


void PVRRenderer::InitBlendMode()
{
	/*KOS
    switch ( gRDPOtherMode.cycle_type )
	{
		case CYCLE_COPY:			InitCycleCopy();	break;
		case CYCLE_FILL:			InitCycleFill();	break;
		case CYCLE_1CYCLE:			InitCycle1();		break;
		case CYCLE_2CYCLE:			InitCycle2();		break;
	}*/

	switch ( gRDPOtherMode.text_filt )
	{
		case 0:			//G_TF_POINT:	// 0
			m_dwTexFilter = PVR_FILTER_NEAREST;
			break;
		case 2:			//G_TF_BILERP:	// 2
			m_dwTexFilter = PVR_FILTER_BILINEAR;
			break;
		case 3:			//G_TF_AVERAGE:	// 3?
			m_dwTexFilter = PVR_FILTER_BILINEAR;
			break;
	}

    //
	// I can't think why the hand in mario's menu screen is rendered with an opaque rendermode,
	// and no alpha threshold. We set the alpha reference to 1 to ensure that the transparent pixels
	// don't get rendered. I hope this doesn't fuck anything else up.
	//
	if ( gRDPOtherMode.alpha_compare == 0 )
	{
		if ( gRDPOtherMode.cvg_x_alpha )
			SetAlphaTestEnable(true);
		else
			SetAlphaTestEnable(false);
	}
	else
	{
		// G_AC_THRESHOLD || G_AC_DITHER
		SetAlphaTestEnable(true);
	}

    // Blend-Modes einlesen
	u32 blendmode_1 = u32( gRDPOtherMode._u32[0] & 0xcccc0000 );
	u32 blendmode_2 = u32( gRDPOtherMode._u32[0] & 0x33330000 );

    // Alpha-Blending festlegen
	if ( gRDPOtherMode.cycle_type == CYCLE_FILL )
	{
        SetAlphaBlendEnable(false);
	}
	else if ( blendmode_1 == 0 && blendmode_2 == 0 )
	{
        SetAlphaBlendFunc(PVR_BLEND_SRCALPHA, PVR_BLEND_INVSRCALPHA);
        SetAlphaBlendEnable(true);
	}
	else if ( blendmode_1 == BLEND_FOG_ASHADE1 && blendmode_2 == BLEND_OPA2 )
	{
		// c811
        SetAlphaBlendEnable(false);
	}
	else if ( blendmode_1 == BLEND_FOG_ASHADE1 && blendmode_2 == BLEND_XLU2 )
	{
		// c810
		SetAlphaBlendFunc(PVR_BLEND_SRCALPHA, PVR_BLEND_INVSRCALPHA);
        SetAlphaBlendEnable(true);
	}
	else if ( blendmode_1 == BLEND_XLU1 && blendmode_2 == BLEND_XLU2 )
	{
		// 0x0050
		SetAlphaBlendFunc(PVR_BLEND_SRCALPHA, PVR_BLEND_INVSRCALPHA);
        SetAlphaBlendEnable(true);
	}
	else if ( blendmode_1 == BLEND_XLU1 && blendmode_2 == BLEND_NOOP2 )
	{
		// 0x0040
		SetAlphaBlendFunc(PVR_BLEND_SRCALPHA, PVR_BLEND_INVSRCALPHA);
        SetAlphaBlendEnable(true);
	}
	else if ( blendmode_1 == BLEND_XLU1 && blendmode_2 == BLEND_ADD2 )
	{
		// 0x005n
		// XXXX
		/*KOS
		SetAlphaBlendFunc(PVR_BLEND_SRCCOLOR, PVR_BLEND_DSTCOLOR);
        SetAlphaBlendEnable(true);
        */
        SetAlphaBlendEnable(false);
	}
	else if ( blendmode_1 == BLEND_OPA1 && blendmode_2 == BLEND_OPA2 )
	{
		// 0x0055
		// Would have thoughts this was SRCALPHA/DSTALPHA, but apparently not
        SetAlphaBlendEnable(false);
	}
	else if ( blendmode_1 == BLEND_OPA1 && blendmode_2 == BLEND_NOOP2 )
	{
		// 0x0044
        SetAlphaBlendEnable(false);
	}
	else if ( blendmode_1 == BLEND_PASS1 && blendmode_2 == BLEND_PASS2 )
	{
		// 0x0f0a
        SetAlphaBlendEnable(false);
	}
	else if ( blendmode_1 == BLEND_PASS1 && blendmode_2 == BLEND_XLU2 )
	{
		// 0x0c18
		SetAlphaBlendFunc(PVR_BLEND_SRCALPHA, PVR_BLEND_INVSRCALPHA);
        SetAlphaBlendEnable(true);
	}
	else if ( blendmode_1 == BLEND_PASS1 && blendmode_2 == BLEND_OPA2 )
	{
		// 0x0c19
        SetAlphaBlendEnable(false);
	}
	else if ( blendmode_1 == BLEND_MEM1 && blendmode_2 == BLEND_MEM2 )
	{
		// 0x5f50
        SetAlphaBlendEnable(false);
	}
	else
	{
		SetAlphaBlendFunc(PVR_BLEND_SRCALPHA, PVR_BLEND_INVSRCALPHA);
        SetAlphaBlendEnable(true);
	}
}

void PVRRenderer::RecompileHeader()
{
    // Alpha-Blending bzw. Alpha-Test-Status aktualisieren
    InitBlendMode();
    
    // Evtl. Alpha-Blending behandeln
    u32 list = PVR_LIST_OP_POLY;
    if(m_bAlphaBlendEnable || m_bAlphaTestEnable)
        list = PVR_LIST_TR_POLY;

    // Zwischen Polygonheader mit Textur und Polygonheader ohne Textur unterscheiden
    if( m_bTextureEnable && m_lpsTexture != NULL )
        pvr_poly_cxt_txr(&cxt, list, PVR_TXRFMT_ARGB4444, m_lpsTexture->GetWidth(), m_lpsTexture->GetHeight(), m_lpsTexture->GetTexture()->GetData(), m_dwTexFilter);
    else
		pvr_poly_cxt_col(&cxt, list);
		
    // Backface-Culling aktivieren
	if(m_bCullFront)
        cxt.gen.culling = PVR_CULLING_CCW;
	else if( m_bCullBack )
        cxt.gen.culling = PVR_CULLING_CW;
	else
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

    // Only update if ZBuffer is enabled
	if (m_bZBuffer)
	{
		if(!gRDPOtherMode.z_cmp)
            cxt.depth.comparison = PVR_DEPTHCMP_ALWAYS;
        if(!gRDPOtherMode.z_upd)
            cxt.depth.write = PVR_DEPTHWRITE_DISABLE;
	}
	else
	{
        // ZBuffer vollständig deaktivieren
        cxt.depth.comparison = PVR_DEPTHCMP_ALWAYS;
        cxt.depth.write = PVR_DEPTHWRITE_DISABLE;
	}
	
	// Shading festlegen
	cxt.txr.filter = mSmooth ? PVR_SHADE_GOURAUD : PVR_SHADE_FLAT;

    // Header kompilieren
	pvr_poly_compile(&hdr, &cxt);
}

bool PVRRenderer::TexRect( const tVector2 & xy0, const tVector2 & xy1, const tVector2 & uv0, const tVector2 & uv1 )
{
    // TODO - In 1/2cycle mode, skip bottom/right edges!?
	tVector2 screen0;
	tVector2 screen1;
	tVector2 tex_uv0( uv0 );
	tVector2 tex_uv1( uv1 );

	// Scale to Actual texture coords
	// The two cases are to handle the oversized textures hack on voodoos
	float width_scale = (m_fTileWidth > m_fTexWidth) ? m_fInvTileWidth : m_fInvTexWidth;
	float height_scale = (m_fTileHeight > m_fTexHeight) ? m_fInvTileHeight : m_fInvTexHeight;

	tex_uv0.x *= width_scale;
	tex_uv1.x *= width_scale;

	tex_uv0.y *= height_scale;
	tex_uv1.y *= height_scale;

	// Save ZBuffer state
	BOOL bZEnabled = m_bZBuffer;
    ZBufferEnable(FALSE);

	// Um ein texturiertes Rechteck zu zeichnen, muss m_bTextureEnable true sein
	BOOL bTextureEnable = m_bTextureEnable;
	SetTextureEnable(TRUE);

	//
	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1.0f to the w/h)
	//
	tVector2 edge( 0, 0 );

	switch ( gRDPOtherMode.cycle_type )
	{
		case CYCLE_COPY:			edge.x += 1.0f; edge.y += 1.0f;	break;
		case CYCLE_FILL:			edge.x += 1.0f; edge.y += 1.0f;	break;
		case CYCLE_1CYCLE:											break;
		case CYCLE_2CYCLE:											break;
	}

	// Erstes Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[0], xy0.x,          xy0.y,          tex_uv0.x, tex_uv0.y);
    InitRectVertex(m_ucRectBuffer[1], xy0.x,          xy1.y + edge.y, tex_uv0.x, tex_uv1.y);
    InitRectVertex(m_ucRectBuffer[2], xy1.x + edge.x, xy0.y,          tex_uv1.x, tex_uv0.y);

    // Zweites Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[3], xy1.x + edge.x, xy0.y,          tex_uv1.x, tex_uv0.y);
    InitRectVertex(m_ucRectBuffer[4], xy0.x,          xy1.y + edge.y, tex_uv0.x, tex_uv1.y);
    InitRectVertex(m_ucRectBuffer[5], xy1.x + edge.x, xy1.y + edge.y, tex_uv1.x, tex_uv1.y);

    // Rechteck rendern
    FlushN64Tris(m_ucRectBuffer, 6);

    // Restore state
	ZBufferEnable( bZEnabled );
	SetTextureEnable(bTextureEnable);

    return true;
}

bool PVRRenderer::TexRectFlip( const tVector2 & xy0, const tVector2 & xy1, const tVector2 & uv0, const tVector2 & uv1 )
{
    // TODO - In 1/2cycle mode, skip bottom/right edges!?
	tVector2 screen0;
	tVector2 screen1;
	tVector2 tex_uv0( uv0 );
	tVector2 tex_uv1( uv1 );
	
	float width_scale = (m_fTileWidth > m_fTexWidth) ? m_fInvTileWidth : m_fInvTexWidth;
	float height_scale = (m_fTileHeight > m_fTexHeight) ? m_fInvTileHeight : m_fInvTexHeight;

	tex_uv0.x *= width_scale;
	tex_uv1.x *= width_scale;

	tex_uv0.y *= height_scale;
	tex_uv1.y *= height_scale;
	
	// Save ZBuffer state
	BOOL bZEnabled = m_bZBuffer;
    ZBufferEnable(FALSE);
	
	// Um ein texturiertes Rechteck zu zeichnen, muss m_bTextureEnable true sein
	BOOL bTextureEnable = m_bTextureEnable;
	SetTextureEnable(TRUE);

	//
	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1.0f to the w/h)
	//
	tVector2 edge( 0, 0 );

	switch ( gRDPOtherMode.cycle_type )
	{
		case CYCLE_COPY:			edge.x += 1.0f; edge.y += 1.0f;	break;
		case CYCLE_FILL:			edge.x += 1.0f; edge.y += 1.0f;	break;
		case CYCLE_1CYCLE:											break;
		case CYCLE_2CYCLE:											break;
	}

	// Erstes Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[0], xy0.x,          xy0.y,          tex_uv0.x, tex_uv0.y);
    InitRectVertex(m_ucRectBuffer[1], xy0.x,          xy1.y + edge.y, tex_uv1.x, tex_uv0.y);
    InitRectVertex(m_ucRectBuffer[2], xy1.x + edge.x, xy0.y,          tex_uv0.x, tex_uv1.y);

    // Zweites Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[3], xy1.x + edge.x, xy0.y,          tex_uv0.x, tex_uv1.y);
    InitRectVertex(m_ucRectBuffer[4], xy0.x,          xy1.y + edge.y, tex_uv1.x, tex_uv0.y);
    InitRectVertex(m_ucRectBuffer[5], xy1.x + edge.x, xy1.y + edge.y, tex_uv1.x, tex_uv1.y);

    // Rechteck rendern
    FlushN64Tris(m_ucRectBuffer, 6);
    
    // Restore state
	ZBufferEnable( bZEnabled );
	SetTextureEnable(bTextureEnable);

    return true;
}

bool PVRRenderer::FillRect( const tVector2 & xy0, const tVector2 & xy1, u32 color )
{
	tVector2 screen0;
	tVector2 screen1;

	if ( (gRDPOtherMode._u32[0] & 0xffff0000) == 0x5f500000 )
	{
		// this blend mode is mem*0 + mem*1, so we don't need to render it... Very odd!
		return true;
	}

	// Save ZBuffer state
	BOOL bZEnabled = m_bZBuffer;

	ZBufferEnable(FALSE);

    // Erstes Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[0], xy0.x, xy0.y, 0.0f, 0.0f, color);
    InitRectVertex(m_ucRectBuffer[1], xy0.x, xy1.y, 0.0f, 0.0f, color);
    InitRectVertex(m_ucRectBuffer[2], xy1.x, xy0.y, 0.0f, 0.0f, color);

    // Zweites Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[3], xy1.x, xy0.y, 0.0f, 0.0f, color);
    InitRectVertex(m_ucRectBuffer[4], xy0.x, xy1.y, 0.0f, 0.0f, color);
    InitRectVertex(m_ucRectBuffer[5], xy1.x, xy1.y, 0.0f, 0.0f, color);

    // Rechteck rendern
    FlushN64Tris(m_ucRectBuffer, 6);

	// Restore state
	ZBufferEnable( bZEnabled );

    return true;
}

inline void PVRRenderer::InitRectVertex(N64VERTEX &v, float x, float y, float tu, float tv, uint32 color)
{
    // Vertex mit Daten füllen
    v.pos.x = x * m_fScreenMultX;
    v.pos.y = y * m_fScreenMultY;
    v.pos.z = 1.0f;
    v.rhw   = 1.0f;
    v.tu    = tu;
    v.tv    = tv;
    v.dcDiffuse = color;
}

// Assumes dwAddress has already been checked!
// Don't inline - it's too big with the transform macros
void PVRRenderer::SetNewVertexInfo(u32 dwAddress, u32 dwV0, u32 dwNum)
{
    long nFogR, nFogG, nFogB, nFogA;

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

    // Clipping-Plane erstellen
    tPlane pZNear( matWorldProject.m[0][3] + matWorldProject.m[0][2],
                   matWorldProject.m[1][3] + matWorldProject.m[1][2],
                   matWorldProject.m[2][3] + matWorldProject.m[2][2],
                   matWorldProject.m[3][3] + matWorldProject.m[3][2] );

	u32 i;
	for (i = dwV0; i < dwV0 + dwNum; i++)
	{
		u32 dwFlags;

		FiddledVtx & vert = pVtxBase[i - dwV0];

		m_vecUnProjected[i].x = (float)vert.x;
		m_vecUnProjected[i].y = (float)vert.y;
		m_vecUnProjected[i].z = (float)vert.z;

		m_fDist[i] = pZNear.distanceFrom(m_vecUnProjected[i]);

		m_vecProjected[i].x = (float)vert.x;
		m_vecProjected[i].y = (float)vert.y;
		m_vecProjected[i].z = (float)vert.z;
		m_fRHW[i] = 1.0f;

		mat_trans_single4(m_vecProjected[i].x, m_vecProjected[i].y, m_vecProjected[i].z, m_fRHW[i]);

        tVector3 vecTransformedNormal;		// Used only when m_bLightingEnable
		if (m_bLightingEnable)
		{
			tMatrix matWorld = m_mModelView[m_dwModelViewTop];

            // Normale transformieren
            tVector3 model_normal((float)vert.u0.norma.nx, (float)vert.u0.norma.ny, (float)vert.u0.norma.nz);
			vecTransformedNormal = model_normal.transformNormal( matWorld );
			vecTransformedNormal = vecTransformedNormal.normalize();

			// Do lighting
			tColorF col = LightVert(vecTransformedNormal);
			m_dwVecCol[i] = RGBA_MAKE((u8)(col.x * 255.0f), (u8)(col.y * 255.0f), (u8)(col.z * 255.0f), (u8)(col.w * 255.0f));
		}
		else
		{
            // Vertex-Farbe direkt zuweisen
            m_dwVecCol[i] = RGBA_MAKE(vert.u0.rgba.r, vert.u0.rgba.g, vert.u0.rgba.b, vert.u0.rgba.a);
		}

		if (m_bTextureEnable)
		{
			// Update texture coords n.b. need to divide tu/tv by bogus scale on addition to buffer
			tVector2 & t = m_vecTexture[i];

			// If the vert is already lit, then there is no normal (and hence we
			// can't generate tex coord)
			if (m_bTextureGen && m_bLightingEnable)
			{

				tMatrix & matWV = m_mModelView[m_dwModelViewTop];
				tVector3 & norm = vecTransformedNormal;

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
void PVRRenderer::SetNewVertexInfoDKR(u32 dwAddress, u32 dwV0, u32 dwNum)
{
    // Diddy Kong Racing funktioniert sowieso nicht (vielleicht jetzt doch?)
}

void PVRRenderer::SetVtxColor( u32 vert, u32 color )
{
    if(vert < MAX_VERTS)
        m_dwVecCol[vert] = color;
}
void PVRRenderer::SetVtxTextureCoord( u32 vert, short tu, short tv )
{
    if(vert < MAX_VERTS)
    {
        m_vecTexture[vert].x = (float)tu / 32.0f;
		m_vecTexture[vert].y = (float)tv / 32.0f;
    }
}
void PVRRenderer::SetVtxXY( u32 vert, float x, float y )
{
    if(vert < MAX_VERTS)
    {
        m_vecProjected[vert].x = x;
        m_vecProjected[vert].y = y;
    }
}


// Returns TRUE if it thinks the triangle is visible
// Returns FALSE if it is clipped
bool PVRRenderer::TestTri(u32 dwV0, u32 dwV1, u32 dwV2)
{
    // Check vertices are valid!
	if (dwV0 >= 32 || dwV1 >= 32 || dwV2 >= 32)
		return FALSE;

	return TRUE;
}

inline void PVRRenderer::InitN64Vertex(N64VERTEX & v, u32 dwV)
{
    // Globale Komponenten eines Vertex initialisieren
    v.pos.x = ( (m_vecProjected[dwV].x+1) * m_nVPWidth/2) + m_nVPLeft;
	v.pos.y = ( (-m_vecProjected[dwV].y+1) * m_nVPHeight/2) + m_nVPTop;
	//v.pos.z = ( m_vecProjected[dwV].z + 1.0f) * 0.5f;
	v.pos.z = m_vecProjected[dwV].z;
	v.rhw = m_fRHW[dwV];
	v.dcDiffuse = m_dwVecCol[dwV];
}

inline void PVRRenderer::InitTLitVertex(N64VERTEX & v, u32 dwV)
{
    // Globale Komponenten des Vertex initialisieren
    InitN64Vertex(v, dwV);

    // Textur-Koordinaten unverändert übernehmen (werden später noch angepasst)
	v.tu = m_vecTexture[dwV].x;
	v.tv = m_vecTexture[dwV].y;

	// Bias points by normal
	if (IsZModeDecal())
        v.rhw += 0.00001f;
}

inline void PVRRenderer::InitLitVertex(N64VERTEX & v, u32 dwV)
{
    // Globale Komponenten des Vertex initialisieren
    InitN64Vertex(v, dwV);

    v.tu = 0.0f;
    v.tv = 0.0f;
}

void PVRRenderer::ClipN64Vertex(N64VERTEX &v, N64VERTEX &v0, u32 dwV0, N64VERTEX &v1, u32 dwV1)
{
    // Zeiger auf die untransformierten Positionsdaten zwecks Lesbarkeit speichern
    tVector3 *uv0 = &m_vecUnProjected[dwV0];
    tVector3 *uv1 = &m_vecUnProjected[dwV1];
    
    // Faktor zum Interpolieren berechnen
    float t = m_fDist[dwV0] / (m_fDist[dwV0] - m_fDist[dwV1]);

    // Position linear interpolieren
    v.pos.x = uv0->x + t * (uv1->x - uv0->x);
    v.pos.y = uv0->y + t * (uv1->y - uv0->y);
    v.pos.z = uv0->z + t * (uv1->z - uv0->z);
    v.rhw = 1.0f;
    
    mat_trans_single4(v.pos.x, v.pos.y, v.pos.z, v.rhw);
    
    v.pos.x = ( (v.pos.x+1) * m_nVPWidth/2) + m_nVPLeft;
	v.pos.y = ( (-v.pos.y+1) * m_nVPHeight/2) + m_nVPTop;
	//v.pos.z = (v.pos.z + 1.0f) * 0.5f;

    // Textur-Koordinate interpolieren
    v.tu = v0.tu + t * (v1.tu - v0.tu);
    v.tv = v0.tv + t * (v1.tv - v0.tv);

    // Farbe linear interpolieren
    v.dcDiffuse = RGBA_MAKE((BYTE)(RGBA_GETRED(v0.dcDiffuse) + t * (int)(RGBA_GETRED(v1.dcDiffuse) - RGBA_GETRED(v0.dcDiffuse))),
                            (BYTE)(RGBA_GETGREEN(v0.dcDiffuse) + t * (int)(RGBA_GETGREEN(v1.dcDiffuse) - RGBA_GETGREEN(v0.dcDiffuse))),
                            (BYTE)(RGBA_GETBLUE(v0.dcDiffuse) + t * (int)(RGBA_GETBLUE(v1.dcDiffuse) - RGBA_GETBLUE(v0.dcDiffuse))),
                            (BYTE)(RGBA_GETALPHA(v0.dcDiffuse) + t * (int)(RGBA_GETALPHA(v1.dcDiffuse) - RGBA_GETALPHA(v0.dcDiffuse))));
}

void PVRRenderer::ClipN64Vertices(N64VERTEX *v, u32 dwV[])
{
    // Early out - Sind alle Vertices innerhalb der Clipping-Plane dann abbrechen
    if( m_fDist[dwV[0]] > 0.0f && m_fDist[dwV[1]] > 0.0f && m_fDist[dwV[2]] > 0.0f )
    {
        memcpy(&m_ucVertexBuffer[m_dwNumVertices], v, sizeof(N64VERTEX) * 3);
        m_dwNumVertices += 3;
        return;
    }

    // Zeiger auf den Speicherplatz für die geclippten Vertices zwecks Lesbarkeit speichern
    N64VERTEX *tmp = (N64VERTEX *)m_ucClipTmpBuffer;

    // Sutherland-Hodgeman-Algorithmus
    u32 dwNumVertices = 0;
    for( int i0 = 0; i0 < 3; i0++ )
    {
        // Index für den jeweils folgenden Vertex berechnen
        int i1 = (i0 + 1) % 3;

        // Alle Fälle des Sutherland-Hodgeman Algorithmus durchgehen
        if( m_fDist[dwV[i0]] <= 0.0f )
        {
            if( m_fDist[dwV[i1]] > 0.0f )
            {
                // Dieser Vertex ist außerhalb der ZNearPlane, der nächste innerhalb
                ClipN64Vertex(tmp[dwNumVertices++], v[i0], dwV[i0], v[i1], dwV[i1]);
                tmp[dwNumVertices++] = v[i1];
            }
        }
        else if( m_fDist[dwV[i1]] <= 0.0f )
        {
            // Dieser Vertex ist innerhalb der ZNearPlane, der nächste außerhalb
            ClipN64Vertex(tmp[dwNumVertices++], v[i0], dwV[i0], v[i1], dwV[i1]);
        }
        else
        {
            // Beide Vertices sind innerhalb der ZNearPlane
            tmp[dwNumVertices++] = v[i1];
        }
    }
    
    // Aus den geclippten Daten wieder Dreiecke erstellen
    memcpy(&m_ucVertexBuffer[m_dwNumVertices], tmp, sizeof(N64VERTEX) * 3);
    m_dwNumVertices += 3;
    if(dwNumVertices == 4)
    {
        m_ucVertexBuffer[m_dwNumVertices++] = tmp[0];
        m_ucVertexBuffer[m_dwNumVertices++] = tmp[2];
        m_ucVertexBuffer[m_dwNumVertices++] = tmp[3];
    }
}


bool PVRRenderer::AddTri(u32 dwV0, u32 dwV1, u32 dwV2)
{
    if (m_bTextureEnable)
	{
		// Add to textured, lit buffer
		InitTLitVertex(m_ucClipBuffer[0], dwV0);
		InitTLitVertex(m_ucClipBuffer[1], dwV1);
		InitTLitVertex(m_ucClipBuffer[2], dwV2);
	}
	else
	{
		// No textures...
		// Untextured, lit
		InitLitVertex(m_ucClipBuffer[0], dwV0);
		InitLitVertex(m_ucClipBuffer[1], dwV1);
		InitLitVertex(m_ucClipBuffer[2], dwV2);
	}
	
	// Early out - Ist kein Vertex des Dreiecks auf dem Bildschirm dann abbrechen
    if( m_fDist[dwV0] <= 0.0f && m_fDist[dwV1] <= 0.0f && m_fDist[dwV2] <= 0.0f )
        return false;
	
	// Vertices mit dem Sutherland-Hodgeman-Algorithmus an der ZNear-Plane clippen
    u32 dwV[] = {dwV0, dwV1, dwV2};
	ClipN64Vertices(m_ucClipBuffer, dwV);
	return true;
}

void PVRRenderer::StoreTris(N64VERTEX *v, u32 count)
{
    // Sicherstellen, dass die Vertices noch in den VertexBuffer passen
    assert_msg(((m_pVertexBufferPtr - m_pVertexBuffer) + sizeof(pvr_poly_hdr_t) + count * 3 * sizeof(pvr_vertex_t)) < TR_BUFFER_SIZE, "Transparente Vertices können nicht mehr gespeichert werden");

	// Polygon-Header im VertexBuffer speichern
	pvr_poly_hdr_t *hdrPtr = (pvr_poly_hdr_t *)m_pVertexBufferPtr;
	*hdrPtr = hdr;
	hdrPtr++;	

	// Zeiger auf den VertexBuffer speichern
	pvr_vertex_t *vert = (pvr_vertex_t *)hdrPtr;

	// Für alle Dreiecke ausführen
    for( u32 i = 0; i < count / 3; i++ )
    {
        // Vertex mit Daten füllen
        vert->x = v->pos.x;
        vert->y = v->pos.y;
        vert->z = v->rhw;
        vert->u = v->tu;
        vert->v = v->tv;
        vert->argb = v->dcDiffuse;
        vert->oargb = 0xff000000;
        vert->flags = PVR_CMD_VERTEX;
        
		// Mit dem nächsten Vertex fortfahren
        vert++;
        v++;
        
        // Vertex mit Daten füllen
        vert->x = v->pos.x;
        vert->y = v->pos.y;
        vert->z = v->rhw;
        vert->u = v->tu;
        vert->v = v->tv;
        vert->argb = v->dcDiffuse;
        vert->oargb = 0xff000000;
        vert->flags = PVR_CMD_VERTEX;
        
		// Mit dem nächsten Vertex fortfahren
        vert++;
        v++;
        
        // Vertex mit Daten füllen
        vert->x = v->pos.x;
        vert->y = v->pos.y;
        vert->z = v->rhw;
        vert->u = v->tu;
        vert->v = v->tv;
        vert->argb = v->dcDiffuse;
        vert->oargb = 0xff000000;
        vert->flags = PVR_CMD_VERTEX_EOL;
        
		// Mit dem nächsten Vertex fortfahren
        vert++;
        v++;
    }

	// Zeiger auf die aktuelle VertexBuffer-Position aktualisieren
	m_pVertexBufferPtr = (uint8 *)vert;
}

inline void PVRRenderer::FlushN64Vertices(pvr_dr_state_t &dr_state, N64VERTEX *v, u32 count)
{
    // Für alle Dreiecke ausführen
    for( u32 i = 0; i < count / 3; i++ )
    {
        // Speicheraddresse für den neuen Vertex holen
        pvr_vertex_t *vert = pvr_dr_target(dr_state);
        // Vertex mit Daten füllen
        vert->x = v->pos.x;
        vert->y = v->pos.y;
        vert->z = v->rhw;
        vert->u = v->tu;
        vert->v = v->tv;
        vert->argb = v->dcDiffuse;
        vert->oargb = 0xff000000;
        vert->flags = PVR_CMD_VERTEX;
        // Vertex abschließen und nächsten Vertex holen
        pvr_dr_commit( vert );
        v++;
        
        // Speicheraddresse für den neuen Vertex holen
        vert = pvr_dr_target(dr_state);
        // Vertex mit Daten füllen
        vert->x = v->pos.x;
        vert->y = v->pos.y;
        vert->z = v->rhw;
        vert->u = v->tu;
        vert->v = v->tv;
        vert->argb = v->dcDiffuse;
        vert->oargb = 0xff000000;
        vert->flags = PVR_CMD_VERTEX;
        // Vertex abschließen und nächsten Vertex holen
        pvr_dr_commit( vert );
        v++;
        
        // Speicheraddresse für den neuen Vertex holen
        vert = pvr_dr_target(dr_state);
        // Vertex mit Daten füllen
        vert->x = v->pos.x;
        vert->y = v->pos.y;
        vert->z = v->rhw;
        vert->u = v->tu;
        vert->v = v->tv;
        vert->argb = v->dcDiffuse;
        vert->oargb = 0xff000000;
        vert->flags = PVR_CMD_VERTEX_EOL;
        // Vertex abschließen und nächsten Vertex holen
        pvr_dr_commit( vert );
        v++;
    }
}

void PVRRenderer::FlushN64Tris(N64VERTEX *v, u32 count)
{
    // Early out - Falls alle Vertices geclippt wurden, dann bereits hier abbrechen
    if (count == 0)
        return;

    // Polygonheader neu kompilieren
    RecompileHeader();

    // Falls AlphaBlending aktiviert ist, müssen die Vertices zwischengespeichert werden
    if(m_bAlphaBlendEnable || m_bAlphaTestEnable)
        return StoreTris(v, count);
        
    // Polygon-Header festlegen
    pvr_prim( &hdr, sizeof(hdr) );

    // DirectRendering-State initialisieren
    pvr_dr_state_t dr_state;
    pvr_dr_init(dr_state);

    // Vertices rendern
    FlushN64Vertices(dr_state, v, count);
}

void PVRRenderer::FlushTris()
{
	// Only scale if not generated automatically
	if (m_bTextureEnable && !( m_bTextureGen && m_bLightingEnable ) )
	{
		for ( u32 v = 0; v < m_dwNumVertices; v++ )
		{
			m_ucVertexBuffer[v].tu = (m_ucVertexBuffer[v].tu - m_fTexLeft) * m_fInvTexWidth;
			m_ucVertexBuffer[v].tv = (m_ucVertexBuffer[v].tv - m_fTexTop) * m_fInvTexHeight;
		}
	}
	
    // Dreiecke rendern
    FlushN64Tris(m_ucVertexBuffer, m_dwNumVertices);
    
    // Anzahl Vertices zurücksetzen
    m_dwNumVertices = 0;
}