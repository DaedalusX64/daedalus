#include <stdlib.h>
#include <assert.h>
#include "PVRRender.h"

PVRRender::PVRRender():
	m_fScreenMultX(2.0f),
	m_fScreenMultY(2.0f),

	m_dwProjectionTop(0),
	m_dwModelViewTop(0),

	m_dwNumStages(0),
	m_dwUVClamp(0),
	m_dwUVFlip(0),

	m_dwAmbientLight(RGBA_MAKE(0XFF, 0XFF, 0XFF, 0XFF)),
	/*KOS
	m_dwTextureFactor(D3DRGBA(255,255,255,255)),
	*/

	m_bCullFront(FALSE),
	m_bCullBack(TRUE),

    /*KOS
	m_dwShadeMode(D3DSHADE_GOURAUD),
	*/

	m_bTextureEnable(FALSE),
	m_bLightingEnable(FALSE),
	m_dwNumLights(0),

	m_bZBuffer(FALSE),
	m_bZUpdate(FALSE),
	m_bZCompare(FALSE),

	m_dwrsZEnable(FALSE),
	m_dwrsZWriteEnable(FALSE),

	m_dwTexturePerspective(FALSE),
	m_bAlphaTestEnable(FALSE),
	m_bAlphaBlendEnable(FALSE),

	m_dwAlpha(0xFF),

	m_dwTileWidth(64),		// Value doesn't really matter, as tex not set
	m_dwTileHeight(64),
	m_fTexWidth(64.0f),		// Value doesn't really matter, as tex not set
	m_fTexHeight(64.0f),
	m_fTexScaleX(1.0f),
	m_fTexScaleY(1.0f),

	m_bTextureGen(FALSE),
	m_bZModeDecal(FALSE),


	m_dwNumVertices(0),
	m_dwNumTRPolyGroups(0),

	m_fFogMult(0.0f),			// * 0 + 0 = 0
	m_fFogOffset(0.0f),
	m_bFogEnable(FALSE),
	m_dwFogColor(0x00FFFFFF),
    m_pSurf( NULL )

{
	//D3DXMATRIX m_mProjection;
	//D3DXMATRIX m_mModelView[10];

	DWORD i;
	for (i = 0; i < 8; i++)
	{
        m_TSSInfo[i].dwFilter = PVR_FILTER_BILINEAR;
        /*KOS
		m_TSSInfo[i].dwColorOp = D3DTOP_MODULATE;
		m_TSSInfo[i].dwColorArg1 = D3DTA_TEXTURE;
		m_TSSInfo[i].dwColorArg2 = D3DTA_DIFFUSE;
		m_TSSInfo[i].dwAlphaOp = D3DTOP_MODULATE;
		m_TSSInfo[i].dwAlphaArg1 = D3DTA_TEXTURE;
		m_TSSInfo[i].dwAlphaArg2 = D3DTA_DIFFUSE;

		m_TSSInfo[i].dwMinFilter = D3DTFN_LINEAR;
		m_TSSInfo[i].dwMagFilter = D3DTFG_LINEAR;

		m_TSSInfo[i].dwAddressUMode = D3DTADDRESS_WRAP;
		m_TSSInfo[i].dwAddressVMode = D3DTADDRESS_WRAP;
		*/
	}
}

PVRRender::~PVRRender()
{
}


HRESULT PVRRender::Initialize()
{
    //glDisable(GL_CULL_FACE);
	return S_OK;
}

void PVRRender::InitBlendMode()
{
}

inline void PVRRender::InitRectVertex(N64VERTEX &v, long x, long y, float tu, float tv, uint32 color)
{
    // Vertex mit Daten f�llen
    v.pos.x = (float)x * m_fScreenMultX;
    v.pos.y = (float)y * m_fScreenMultY;
    v.pos.z = 1.0f;
    v.rhw = 1.0f;
    v.tu = tu;
    v.tv = tv;
    v.dcDiffuse = color;
}

HRESULT PVRRender::TexRect(LONG nX0, LONG nY0, LONG nX1, LONG nY1,
				   float fS0, float fT0, float fS1, float fT1)
{
	// Save ZBuffer state
	BOOL bZEnabled = m_bZBuffer;
	DWORD dwFilter = m_TSSInfo[0].dwFilter;
	
	// Um ein texturiertes Rechteck zu zeichnen, muss m_bTextureEnable true sein
	BOOL bTextureEnable = m_bTextureEnable;
	SetTextureEnable(TRUE);
	ZBufferEnable(FALSE);

	// Goldeneye HACK
	if (nY1 - nY0 < 2) nY1 = nY1+2;

	float fTexULX;
	float fTexLRX;
	float fTexULY;
	float fTexLRY;

	// Scale to Actual texture coords
	// The two cases are to handle the oversized textures hack on voodoos
	if ((float)m_dwTileWidth > (float)m_fTexWidth)
	{
		fTexULX = (fS0 / (float)(m_dwTileWidth-0));
		fTexLRX = (fS1 / (float)(m_dwTileWidth-0));
	}
	else
	{
		fTexULX = (fS0 / (float)(m_fTexWidth-0));
		fTexLRX = (fS1 / (float)(m_fTexWidth-0));
	}

	if ((float)m_dwTileHeight > (float)m_fTexHeight)
	{
		fTexULY = (fT0 / (float)(m_dwTileHeight-0));
		fTexLRY = (fT1 / (float)(m_dwTileHeight-0));
	}
	else
	{
		fTexULY = (fT0 / (float)(m_fTexHeight-0));
		fTexLRY = (fT1 / (float)(m_fTexHeight-0));
	}
	DL_PF("  %f,%f -> %f,%f", fTexULX, fTexULY, fTexLRX, fTexLRY);
    
    // Erstes Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[0], nX0, nY0, fTexULX, fTexULY,RGBA_MAKE(128, 128, 128, 128));
    InitRectVertex(m_ucRectBuffer[1], nX1, nY0, fTexLRX, fTexULY);
    InitRectVertex(m_ucRectBuffer[2], nX0, nY1, fTexULX, fTexLRY);

    // Zweites Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[3], nX1, nY0, fTexLRX, fTexULY);
    InitRectVertex(m_ucRectBuffer[4], nX0, nY1, fTexULX, fTexLRY);
    InitRectVertex(m_ucRectBuffer[5], nX1, nY1, fTexLRX, fTexLRY);
    
    // Rechteck rendern
    FlushN64Tris(m_ucRectBuffer, 6);

	// Restore state
	ZBufferEnable( bZEnabled );
	SetFilter(0, dwFilter);
	SetTextureEnable(bTextureEnable);

	return S_OK;
}

HRESULT PVRRender::TexRectFlip(LONG nX0, LONG nY0, LONG nX1, LONG nY1,
					   float fS0, float fT0, float fS1, float fT1)
{
	// Save ZBuffer state
	BOOL bZEnabled = m_bZBuffer;
    ZBufferEnable(FALSE);
	DWORD dwFilter = m_TSSInfo[0].dwFilter;
    
    // Um ein texturiertes Rechteck zu zeichnen, muss m_bTextureEnable true sein
	BOOL bTextureEnable = m_bTextureEnable;
	SetTextureEnable(TRUE);
	ZBufferEnable(FALSE);

    float tu0 = fS0 / (float)m_dwTileWidth;
	float tv0 = fT0 / (float)m_dwTileHeight;

	float tu1 = fS1 / (float)m_dwTileWidth;
	float tv1 = fT1 / (float)m_dwTileHeight;

	// TODO - In 1/2cycle mode, skip bottom/right edges!?

	const float fTexULX = tu0 * (float)m_dwTileWidth / m_fTexWidth;		// Scale to PowerOf2 coords!
	const float fTexULY = tv0 * (float)m_dwTileHeight / m_fTexHeight;

	const float fTexLRX = tu1 * (float)m_dwTileWidth / m_fTexWidth;
	const float fTexLRY = tv1 * (float)m_dwTileHeight / m_fTexHeight;
    
    // Erstes Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[0], nX0, nY0, fTexULX, fTexULY);
    InitRectVertex(m_ucRectBuffer[1], nX1, nY0, fTexULX, fTexLRY);
    InitRectVertex(m_ucRectBuffer[2], nX0, nY1, fTexLRX, fTexULY);
    
    // Zweites Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[3], nX1, nY0, fTexULX, fTexLRY);
    InitRectVertex(m_ucRectBuffer[4], nX0, nY1, fTexLRX, fTexULY);
    InitRectVertex(m_ucRectBuffer[5], nX1, nY1, fTexLRX, fTexLRY);
    
    // Rechteck rendern
    FlushN64Tris(m_ucRectBuffer, 6);
    
	// Restore state
	ZBufferEnable( bZEnabled );
	SetFilter(0, dwFilter);
	SetTextureEnable(bTextureEnable);

	return S_OK;
}


HRESULT PVRRender::FillRect(LONG nX0, LONG nY0, LONG nX1, LONG nY1, DWORD dwColor)
{
	// Save ZBuffer state
	BOOL bZEnabled = m_bZBuffer;
	DWORD dwFilter = m_TSSInfo[0].dwFilter;

	ZBufferEnable(FALSE);

    // Erstes Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[0], nX0, nY0, 0.0f, 0.0f, dwColor);
    InitRectVertex(m_ucRectBuffer[1], nX1, nY0, 0.0f, 0.0f, dwColor);
    InitRectVertex(m_ucRectBuffer[2], nX0, nY1, 0.0f, 0.0f, dwColor);
    
    // Zweites Dreieck erstellen
    InitRectVertex(m_ucRectBuffer[3], nX1, nY0, 0.0f, 0.0f, dwColor);
    InitRectVertex(m_ucRectBuffer[4], nX0, nY1, 0.0f, 0.0f, dwColor);
    InitRectVertex(m_ucRectBuffer[5], nX1, nY1, 0.0f, 0.0f, dwColor);
    
    // Rechteck rendern
    FlushN64Tris(m_ucRectBuffer, 6);

	// Restore state
	ZBufferEnable( bZEnabled );
    SetFilter(0, dwFilter);

	return S_OK;
}

// Returns TRUE if it thinks the triangle is visible
// Returns FALSE if it is clipped
BOOL PVRRender::TestTri(DWORD dwV0, DWORD dwV1, DWORD dwV2)
{
    // Check vertices are valid!
	if (dwV0 >= 32 || dwV1 >= 32 || dwV2 >= 32)
		return FALSE;

	return TRUE;
}

DWORD PVRRender::LightVert(tVector3 & norm)
{
    float r,g,b,a;
	float fCosT;
	DWORD l;

    /*Original
	D3DXVec3Normalize(&norm, &norm);
	*/
	norm = norm.normalize();

	// Ip = Light intensity
	// Ia = Ambient light intensity
	// Ka = Material ambient reflective property
	// Kd = Material diffuse reflective property
	// L = normalised light direction
	// N = vertex normal

	// I = IaKa + IpKd (N . L)

	// Do ambient
	float Iar = (float)RGBA_GETRED(m_dwAmbientLight);
	float Iag = (float)RGBA_GETGREEN(m_dwAmbientLight);
	float Iab = (float)RGBA_GETBLUE(m_dwAmbientLight);
	//float Iaa = (float)RGBA_GETALPHA(m_dwAmbientLight);

	r = Iar;
	g = Iag;
	b = Iab;
	//a = Iaa;

	for (l = 0; l < m_dwNumLights; l++)
	{
        /*Original
		fCosT = D3DXVec3Dot(&norm, &m_Lights[l].dir);
		*/
		fCosT = norm.dot( &m_Lights[l].dir );

		if (fCosT > 0)
		{
			r += m_Lights[l].col.r * fCosT;
			g += m_Lights[l].col.g * fCosT;
			b += m_Lights[l].col.b * fCosT;
		//	a += m_Lights[l].col.a * fCosT;
		}
	}

	if (r > 255.0f) r = 255.0f;
	if (g > 255.0f) g = 255.0f;
	if (b > 255.0f) b = 255.0f;

	//if (a < 0) a = 0;
	//else if (a > 255.0f) a = 255.0f;
	a = 255.0f;

	return RGBA_MAKE((LONG)r, (LONG)g, (LONG)b, (LONG)a);
}

inline void PVRRender::InitN64Vertex(N64VERTEX & v, DWORD dwV)
{
    // Globale Komponenten eines Vertex initialisieren
    v.pos.x = ( (m_vecProjected[dwV].x+1) * m_nVPWidth/2) + m_nVPLeft;
	v.pos.y = ( (-m_vecProjected[dwV].y+1) * m_nVPHeight/2) + m_nVPTop;
	//v.pos.z = ( m_vecProjected[dwV].z + 1.0f) * 0.5f;
	v.pos.z = m_vecProjected[dwV].z;
	v.rhw = m_fRHW[dwV];
	v.dcDiffuse = m_dwVecCol[dwV];
}

inline void PVRRender::InitTLitVertex(N64VERTEX & v, DWORD dwV)
{
    // Globale Komponenten des Vertex initialisieren
    InitN64Vertex(v, dwV);

	// If the vert is already lit, then there is no normal (and hence we can't generate tex coord)
	// Only scale if not generated automatically
	if (m_bTextureGen && m_bLightingEnable)
	{
		v.tu = m_vecTexture[dwV].x;
		v.tv = m_vecTexture[dwV].y;
	}
	else
	{
		v.tu = (m_vecTexture[dwV].x - m_fTexLeft) / m_fTexWidth;
		v.tv = (m_vecTexture[dwV].y - m_fTexTop) / m_fTexHeight;
		if(m_pSurf->GetWidth() == 128 && m_pSurf->GetHeight() == 16)
		{
            	v.tu = 0.0f;
        		v.tv = 0.0f;
        }
	}
	
	// Bias points by normal
	if (m_bZModeDecal)
        v.rhw += 0.00001f;
}

inline void PVRRender::InitLitVertex(N64VERTEX & v, DWORD dwV)
{
    // Globale Komponenten des Vertex initialisieren
    InitN64Vertex(v, dwV);

    v.tu = 0.0f;
    v.tv = 0.0f;
}

void PVRRender::ClipN64Vertex(N64VERTEX &v, N64VERTEX &v0, DWORD dwV0, N64VERTEX &v1, DWORD dwV1)
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

void PVRRender::ClipN64Vertices(N64VERTEX *v, DWORD dwV[])
{
    // Early out - Sind alle Vertices innerhalb der Clipping-Plane dann abbrechen
    if( m_fDist[dwV[0]] > 0.0f && m_fDist[dwV[1]] > 0.0f && m_fDist[dwV[2]] > 0.0f )
    {
        memcpy(&m_ucVertexBuffer[m_dwNumVertices], v, sizeof(N64VERTEX) * 3);
        m_dwNumVertices += 3;
        return;
    }

    // Zeiger auf den Speicherplatz f�r die geclippten Vertices zwecks Lesbarkeit speichern
    N64VERTEX *tmp = (N64VERTEX *)m_ucClipTmpBuffer;

    // Sutherland-Hodgeman-Algorithmus
    DWORD dwNumVertices = 0;
    for( int i0 = 0; i0 < 3; i0++ )
    {
        // Index f�r den jeweils folgenden Vertex berechnen
        int i1 = (i0 + 1) % 3;

        // Alle F�lle des Sutherland-Hodgeman Algorithmus durchgehen
        if( m_fDist[dwV[i0]] <= 0.0f )
        {
            if( m_fDist[dwV[i1]] > 0.0f )
            {
                // Dieser Vertex ist au�erhalb der ZNearPlane, der n�chste innerhalb
                ClipN64Vertex(tmp[dwNumVertices++], v[i0], dwV[i0], v[i1], dwV[i1]);
                tmp[dwNumVertices++] = v[i1];
            }
        }
        else if( m_fDist[dwV[i1]] <= 0.0f )
        {
            // Dieser Vertex ist innerhalb der ZNearPlane, der n�chste au�erhalb
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


void PVRRender::AddTri(DWORD dwV0, DWORD dwV1, DWORD dwV2)
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
        return;
	
	// Vertices mit dem Sutherland-Hodgeman-Algorithmus an der ZNear-Plane clippen
    DWORD dwV[] = {dwV0, dwV1, dwV2};
	ClipN64Vertices(m_ucClipBuffer, dwV);
}

HRESULT PVRRender::StoreTris(N64VERTEX *v, DWORD count)
{
    // Sicherstellen, dass die TRGROUP noch in den Speicher passt
    assert_msg(!(m_dwNumTRPolyGroups > MAX_TR_GROUPS), "TRGROUP kann nicht mehr gespeichert werden");

    // Anzahl transparenter Vertices ermitteln
    DWORD dwNumTRVertices = 0;
    if(m_dwNumTRPolyGroups > 0)
        dwNumTRVertices = m_pTRPolyGroups[m_dwNumTRPolyGroups - 1].dwFirst + m_pTRPolyGroups[m_dwNumTRPolyGroups - 1].dwCount;
    
    // Sicherstellen, dass die transparenten Vertices noch in den Speicher passen
    assert_msg(!(dwNumTRVertices + count > MAX_TR_VERTS), "Transparenter Vertex kann nicht mehr gespeichert werden");
    
    // Vertices zu den bereits vorhandenen transparenten Vertices hinzuf�gen
    memcpy(&m_pTRVertices[dwNumTRVertices], v, count * sizeof(N64VERTEX));

    // Neue TRGROUP initialisieren
    m_pTRPolyGroups[m_dwNumTRPolyGroups].hdr = hdr;
    m_pTRPolyGroups[m_dwNumTRPolyGroups].dwFirst = dwNumTRVertices;
    m_pTRPolyGroups[m_dwNumTRPolyGroups].dwCount = count;
    m_dwNumTRPolyGroups++;
    
    return S_OK;
}

inline void PVRRender::FlushN64Vertices(pvr_dr_state_t &dr_state, N64VERTEX *v, DWORD count)
{
    // F�r alle Dreiecke ausf�hren
    for( DWORD i = 0; i < count / 3; i++ )
    {
        // Speicheraddresse f�r den neuen Vertex holen
        pvr_vertex_t *vert = pvr_dr_target(dr_state);
        // Vertex mit Daten f�llen
        vert->x = v->pos.x;
        vert->y = v->pos.y;
        vert->z = v->rhw;
        vert->u = v->tu;
        vert->v = v->tv;
        vert->argb = v->dcDiffuse;
        vert->oargb = 0xff000000;
        vert->flags = PVR_CMD_VERTEX;
        // Vertex abschlie�en und n�chsten Vertex holen
        pvr_dr_commit( vert );
        v++;
        
        // Speicheraddresse f�r den neuen Vertex holen
        vert = pvr_dr_target(dr_state);
        // Vertex mit Daten f�llen
        vert->x = v->pos.x;
        vert->y = v->pos.y;
        vert->z = v->rhw;
        vert->u = v->tu;
        vert->v = v->tv;
        vert->argb = v->dcDiffuse;
        vert->oargb = 0xff000000;
        vert->flags = PVR_CMD_VERTEX;
        // Vertex abschlie�en und n�chsten Vertex holen
        pvr_dr_commit( vert );
        v++;
        
        // Speicheraddresse f�r den neuen Vertex holen
        vert = pvr_dr_target(dr_state);
        // Vertex mit Daten f�llen
        vert->x = v->pos.x;
        vert->y = v->pos.y;
        vert->z = v->rhw;
        vert->u = v->tu;
        vert->v = v->tv;
        vert->argb = v->dcDiffuse;
        vert->oargb = 0xff000000;
        vert->flags = PVR_CMD_VERTEX_EOL;
        // Vertex abschlie�en und n�chsten Vertex holen
        pvr_dr_commit( vert );
        v++;
    }
}

HRESULT PVRRender::FlushN64Tris(N64VERTEX *v, DWORD count)
{
    // Early out - Falls alle Vertices geclippt wurden, dann bereits hier abbrechen
    if (count == 0)
        return S_OK;

    InitBlendMode();

    // Polygonheader neu kompilieren
    RecompileHeader();

    // Falls AlphaBlending aktiviert ist, m�ssen die Vertices zwischengespeichert werden
    if(m_bAlphaBlendEnable || m_bAlphaTestEnable)
        return StoreTris(v, count);
        
    // Polygon-Header festlegen
    pvr_prim( &hdr, sizeof(hdr) );

    // DirectRendering-State initialisieren
    pvr_dr_state_t dr_state;
    pvr_dr_init(&dr_state);

    // Vertices rendern
    FlushN64Vertices(dr_state, v, count);
    
    return S_OK;
}

HRESULT PVRRender::FlushTris()
{
    // Dreiecke rendern
    HRESULT status = FlushN64Tris(m_ucVertexBuffer, m_dwNumVertices);
    
    // Anzahl Vertices zur�cksetzen
    m_dwNumVertices = 0;
    
    return status;
}

HRESULT PVRRender::FlushTRTris()
{
    // Early out - Falls keine transparenten Vertices vorhanden sind, dann abbrechen
    if (m_dwNumTRPolyGroups == 0)
        return S_OK;

    // F�r alle Gruppen von transparenten Vertices ausf�hren
    for( int i = 0; i < m_dwNumTRPolyGroups; i++ )
    {
        // Polygon-Header festlegen
        pvr_prim(&m_pTRPolyGroups[i].hdr, sizeof(m_pTRPolyGroups[i].hdr));
        
        // DirectRendering-State initialisieren
        pvr_dr_state_t dr_state;
        pvr_dr_init(&dr_state);
        
        // Vertices rendern
        FlushN64Vertices(dr_state, &m_pTRVertices[m_pTRPolyGroups[i].dwFirst], m_pTRPolyGroups[i].dwCount);
    }
    
    m_dwNumTRPolyGroups = 0;
	return S_OK;
}
