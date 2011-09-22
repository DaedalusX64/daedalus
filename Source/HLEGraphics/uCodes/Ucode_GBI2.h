/*
Copyright (C) 2009 StrmnNrmn

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

#ifndef UCODE2_H__
#define UCODE2_H__



//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_Vtx( MicroCodeCommand command )
{
    u32 address = RDPSegAddr(command.vtx2.addr);

    u32 vend   = command.vtx2.vend >> 1;
    u32 n      = command.vtx2.n;
    u32 v0	   = vend - n;

	DL_PF( "    Address 0x%08x, vEnd: %d, v0: %d, Num: %d", address, vend, v0, n );

    if ( vend > 64 )
    {
        DL_PF( "    *Warning, attempting to load into invalid vertex positions" );
        DBGConsole_Msg( 0, "DLParser_GBI2_Vtx: Warning, attempting to load into invalid vertex positions: %d -> %d", v0, v0+n );
        return;
    }

    // Check that address is valid...
    if ( (address + (n*16) ) > MAX_RAM_ADDRESS )
    {
        DBGConsole_Msg( 0, "SetNewVertexInfo: Address out of range (0x%08x)", address );
    }
    else
    {
        PSPRenderer::Get()->SetNewVertexInfo( address, v0, n );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
        gNumVertices += n;
        DLParser_DumpVtxInfo( address, v0, n );
#endif
    }
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_Mtx( MicroCodeCommand command )
{
	gAuxAddr = 0;	// For Conker BFD
	u32 address = RDPSegAddr(command.mtx2.addr);

	DL_PF("    Command: %s %s %s Length %d Address 0x%08x",
		command.mtx2.projection ? "Projection" : "ModelView",
		command.mtx2.load ? "Load" : "Mul",
		command.mtx2.nopush == 0 ? "Push" : "No Push",
		command.mtx2.len, address);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (address + 64 > MAX_RAM_ADDRESS)
	{
		DBGConsole_Msg(0, "ZeldaMtx: Address invalid (0x%08x)", address);
		return;
	}
#endif

	// Load matrix from address
	Matrix4x4 mat;
	MatrixFromN64FixedPoint( mat, address );

	if (command.mtx2.projection)
	{
		PSPRenderer::Get()->SetProjection(mat, command.mtx2.nopush==0, command.mtx2.load);
	}
	else
	{
		PSPRenderer::Get()->SetWorldView(mat, command.mtx2.nopush==0, command.mtx2.load);
	}
}
//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_PopMtx( MicroCodeCommand command )
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	u8 mtx_command = (u8)(command.inst.cmd0 & 0xFF);
#endif

	DL_PF("        Command: 0x%02x (%s)", mtx_command, (mtx_command & G_GBI2_MTX_PROJECTION) ? "Projection" : "ModelView");


	PSPRenderer::Get()->PopWorldView();
}
//*****************************************************************************
// 
//*****************************************************************************
/*
void DLParser_GBI2_DL( MicroCodeCommand command )
{
    u32 address = RDPSegAddr(command.dlist.addr);

	DAEDALUS_ASSERT( address < MAX_RAM_ADDRESS, "DL addr out of range (0x%08x)", address );

    DL_PF("    Address=0x%08x Push: 0x%02x", address, command.dlist.param);

	switch (command.dlist.param)
	{
	case G_DL_PUSH:
		DL_PF("    Pushing ZeldaDisplayList 0x%08x", address);
		gDlistStackPointer++;
		gDlistStack[gDlistStackPointer].pc = address;
		gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;

		break;
	case G_DL_NOPUSH:
		DL_PF("    Jumping to ZeldaDisplayList 0x%08x", address);
		if( gDlistStack[gDlistStackPointer].pc == address+8 )	//Is this a loop
		{
			//Hack for Gauntlet Legends
			printf("gggg\n");
			gDlistStack[gDlistStackPointer].pc = address+8;
		}
		else
			gDlistStack[gDlistStackPointer].pc = address;
		gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;
		break;
	}
}
*/
//*****************************************************************************
// Kirby 64, SSB and Cruisn' Exotica use this
//*****************************************************************************
void DLParser_GBI2_DL_Count( MicroCodeCommand command )
{
	u32 address  = RDPSegAddr(command.inst.cmd1);
	//u32 count	 = command.inst.cmd0 & 0xFFFF;

	// For SSB and Kirby, otherwise we'll end up scrapping the pc
	if (address == 0)
	{
		DAEDALUS_ERROR("Invalid DL Count");
		return;
	}

	gDlistStackPointer++;
	gDlistStack[gDlistStackPointer].pc = address;
	gDlistStack[gDlistStackPointer].countdown = ((command.inst.cmd0)&0xFFFF);
}

//***************************************************************************** 
// 
//***************************************************************************** 
void DLParser_GBI2_LoadUCode( MicroCodeCommand command ) 
{
	DL_PF( "	GBI2_LoadUCode (Ignored)" );
}

//*****************************************************************************
//
//*****************************************************************************
//
// Seems to be AND (command.inst.cmd0&0xFFFFFF) OR (command.inst.cmd1&0xFFFFFF)
//
void DLParser_GBI2_GeometryMode( MicroCodeCommand command )
{
    u32 and_bits = (command.inst.cmd0) & 0x00FFFFFF;
    u32 or_bits  = (command.inst.cmd1) & 0x00FFFFFF;

    gGeometryMode &= and_bits;
    gGeometryMode |= or_bits;

	DL_PF("  0x%08x 0x%08x =(x & 0x%08x) | 0x%08x", command.inst.cmd0, command.inst.cmd1, and_bits, or_bits);
	DL_PF("  ZBuffer %s", (gGeometryMode & G_ZELDA_ZBUFFER) ? "On" : "Off");
	DL_PF("  Culling %s", (gGeometryMode & G_ZELDA_CULL_BACK) ? "Back face" : (gGeometryMode & G_ZELDA_CULL_FRONT) ? "Front face" : "Off");
	DL_PF("  Flat Shading %s", (gGeometryMode & G_ZELDA_SHADING_SMOOTH) ? "On" : "Off");
	DL_PF("  Lighting %s", (gGeometryMode & G_ZELDA_LIGHTING) ? "On" : "Off");
	DL_PF("  Texture Gen %s", (gGeometryMode & G_ZELDA_TEXTURE_GEN) ? "On" : "Off");
	DL_PF("  Texture Gen Linear %s", (gGeometryMode & G_ZELDA_TEXTURE_GEN_LINEAR) ? "On" : "Off");
	DL_PF("  Fog %s", (gGeometryMode & G_ZELDA_FOG) ? "On" : "Off");

    PSPRenderer::Get()->SetCullMode( gGeometryMode & G_ZELDA_CULL_FRONT, gGeometryMode & G_ZELDA_CULL_BACK );

	//bool bShade				= (gGeometryMode & G_SHADE)						? true : false;
	//bool bFlatShade         = (gGeometryMode & G_ZELDA_SHADING_SMOOTH)		? true : false;

	bool bFlatShade         = (gGeometryMode & G_ZELDA_TEXTURE_GEN_LINEAR & (g_ROM.GameHacks != TIGERS_HONEY_HUNT))	? true : false;
    PSPRenderer::Get()->SetSmooth( !bFlatShade );
    PSPRenderer::Get()->SetSmoothShade( true );             // Always do this - not sure which bit to use

    PSPRenderer::Get()->SetFogEnable( gGeometryMode & G_ZELDA_FOG );

    PSPRenderer::Get()->SetTextureGen( gGeometryMode & G_ZELDA_TEXTURE_GEN );

	PSPRenderer::Get()->SetTextureGenLin( gGeometryMode & G_ZELDA_TEXTURE_GEN_LINEAR );

    PSPRenderer::Get()->SetLighting( gGeometryMode & G_ZELDA_LIGHTING );

    PSPRenderer::Get()->ZBufferEnable( gGeometryMode & G_ZELDA_ZBUFFER );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_SetOtherModeH( MicroCodeCommand command )
{
    // Mask is constructed slightly differently
    u32 mask		= (u32)((s32)(0x80000000) >> command.othermode.len) >> command.othermode.sft;

    gRDPOtherMode.H = (gRDPOtherMode.H&(~mask)) | command.othermode.data;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	RDP_SetOtherMode( gRDPOtherMode.H, gRDPOtherMode.L );
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_SetOtherModeL( MicroCodeCommand command )
{
	// Mask is constructed slightly differently
	u32 mask		= (u32)((s32)(0x80000000) >> command.othermode.len) >> command.othermode.sft;

	gRDPOtherMode.L = (gRDPOtherMode.L&(~mask)) | command.othermode.data;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	RDP_SetOtherMode( gRDPOtherMode.H, gRDPOtherMode.L );
#endif
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_Texture( MicroCodeCommand command )
{
    gTextureLevel = command.texture.level;
    gTextureTile  = command.texture.tile;

    bool enable = command.texture.enable_gbi2;                        // Seems to use 0x02

	DL_PF("    Level: %d Tile: %d %s", gTextureLevel, gTextureTile, enable ? "enabled":"disabled");
    PSPRenderer::Get()->SetTextureEnable( enable );

	if( !enable )	return;
	
	f32 scale_s = f32(command.texture.scaleS) / (65536.0f * 32.0f);
	f32 scale_t = f32(command.texture.scaleT)  / (65536.0f * 32.0f);

	DL_PF("    ScaleS: %f, ScaleT: %f", scale_s*32.0f, scale_t*32.0f);
	PSPRenderer::Get()->SetTextureScale( scale_s, scale_t );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_Quad( MicroCodeCommand command )
{
    // While the next command pair is Tri2, add vertices
	u32 pc = gDlistStack[gDlistStackPointer].pc;
    u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

	bool tris_added = false;

    do{
        DL_PF("   0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_QUAD");

		// Vertex indices are multiplied by 2
        u32 v0_idx = command.gbi2line3d.v0 >> 1;
        u32 v1_idx = command.gbi2line3d.v1 >> 1;
        u32 v2_idx = command.gbi2line3d.v2 >> 1;

        tris_added |= PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx);

        u32 v3_idx = command.gbi2line3d.v3 >> 1;
        u32 v4_idx = command.gbi2line3d.v4 >> 1;
		u32 v5_idx = command.gbi2line3d.v5 >> 1;

        tris_added |= PSPRenderer::Get()->AddTri(v3_idx, v4_idx, v5_idx);

		//printf("Q 0x%08x: %08x %08x %d\n", pc-8, command.inst.cmd0, command.inst.cmd1, tris_added);

		command.inst.cmd0 = *pCmdBase++;
        command.inst.cmd1 = *pCmdBase++;
        pc += 8;
    }while( command.inst.cmd == G_GBI2_QUAD );

	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
            PSPRenderer::Get()->FlushTris();
    }
}

//*****************************************************************************
//
//*****************************************************************************
// XXX SpiderMan uses this command.
void DLParser_GBI2_Line3D( MicroCodeCommand command )
{
	// While the next command pair is Tri2, add vertices
	u32 pc = gDlistStack[gDlistStackPointer].pc;
    u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

    bool tris_added = false;

    do{
        DL_PF("   0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_LINE3D");

		u32 v0_idx = command.gbi2line3d.v0 >> 1;
        u32 v1_idx = command.gbi2line3d.v1 >> 1;
        u32 v2_idx = command.gbi2line3d.v2 >> 1;

        tris_added |= PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx);

        u32 v3_idx = command.gbi2line3d.v3 >> 1;
        u32 v4_idx = command.gbi2line3d.v4 >> 1;
		u32 v5_idx = command.gbi2line3d.v5 >> 1;

        tris_added |= PSPRenderer::Get()->AddTri(v3_idx, v4_idx, v5_idx);

        command.inst.cmd0 = *pCmdBase++;
        command.inst.cmd1 = *pCmdBase++;
        pc += 8;
    }while( command.inst.cmd == G_GBI2_LINE3D );
	
	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
            PSPRenderer::Get()->FlushTris();
    }
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_Tri1( MicroCodeCommand command )
{
    // While the next command pair is Tri1, add vertices
	u32 pc = gDlistStack[gDlistStackPointer].pc;
    u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

    bool tris_added = false;

    do{
        DL_PF("   0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_TRI1");

		u32 v0_idx = command.gbi2tri1.v0 >> 1;
		u32 v1_idx = command.gbi2tri1.v1 >> 1;
		u32 v2_idx = command.gbi2tri1.v2 >> 1;

        tris_added |= PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx);

        command.inst.cmd0 = *pCmdBase++;
        command.inst.cmd1 = *pCmdBase++;
        pc += 8;
    }while( command.inst.cmd == G_GBI2_TRI1 );

	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
            PSPRenderer::Get()->FlushTris();
    }
}

//*****************************************************************************
// While the next command pair is Tri2, add vertices
//*****************************************************************************
void DLParser_GBI2_Tri2( MicroCodeCommand command )
{
	u32 pc = gDlistStack[gDlistStackPointer].pc;
    u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

    bool tris_added = false;

    do{
        DL_PF("   0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_TRI2");

		// Vertex indices already divided in ucodedef
        u32 v0_idx = command.gbi2tri2.v0;
        u32 v1_idx = command.gbi2tri2.v1;
        u32 v2_idx = command.gbi2tri2.v2;

        tris_added |= PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx);

		u32 v3_idx = command.gbi2tri2.v3;
        u32 v4_idx = command.gbi2tri2.v4;
        u32 v5_idx = command.gbi2tri2.v5;

        tris_added |= PSPRenderer::Get()->AddTri(v3_idx, v4_idx, v5_idx);

        command.inst.cmd0 = *pCmdBase++;
        command.inst.cmd1 = *pCmdBase++;
        pc += 8;
	}while( command.inst.cmd == G_GBI2_TRI2 );

	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
            PSPRenderer::Get()->FlushTris();
    }
}

//*****************************************************************************
//
//*****************************************************************************
/*
void DLParser_GBI2_0x8( MicroCodeCommand command )
{
	if( ((command.inst.cmd0)&0x00FFFFFF) == 0x2F && ((command.inst.cmd1)&0xFF000000) == 0x80000000 )
	{
		// V-Rally 64
		DLParser_S2DEX_ObjLdtxRectR(command);
	}
	else
	{
		DLParser_Nothing(command);
	}
}
*/

#endif // UCODE2_H__
