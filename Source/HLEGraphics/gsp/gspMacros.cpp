/*
Copyright (C) 2009 StrmnNrmn
Copyright (C) 2003-2009 Rice1964

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

//
//	gSP Macros
//

#include "gspCommon.h"


u32 gGeometryMode = 0;
//*****************************************************************************
// gSPVertex( Vtx *v, u32 n, u32 v0 )
//*****************************************************************************
void DLParser_GBI0_Vtx( MicroCodeCommand command )
{
    u32 address = RDPSegAddr(command.vtx0.addr);

    u32 len = command.vtx0.len;
    u32 v0  = command.vtx0.v0;
    u32 n   = command.vtx0.n + 1;

    use(len);

    DL_PF("    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", address, v0, n, len);

    //Crash doom
	if ((v0 + n) > 80)
    {
        DL_PF("        Warning, attempting to load into invalid vertex positions");
        DBGConsole_Msg(0, "DLParser_GBI0_Vtx: Warning, attempting to load into invalid vertex positions");
		n = 32 - v0;
    }

    // Check that address is valid... mario golf/tennis
    if ( (address + (n*16)) > MAX_RAM_ADDRESS )
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
// The previous way of calculating was based on the assumption that
// there was no "n" field. I didn't realise that the n/length fields shared the
// lower 16 bits (in a 6:10 split).
// u32 length    = (command.inst.cmd0)&0xFFFF;
// u32 num_verts = (length + 1) / 0x210;                        // 528
// u32 v0_idx    = ((command.inst.cmd0>>16)&0xFF)/gVertexStride;      // /5
//*****************************************************************************
void DLParser_GBI1_Vtx( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.vtx1.addr);

    //u32 length    = (command.inst.cmd0)&0xFFFF;
    //u32 num_verts = (length + 1) / 0x410;
    //u32 v0_idx    = ((command.inst.cmd0>>16)&0x3f)/2;

    u32 len = command.vtx1.len;
    u32 v0  = command.vtx1.v0;
    u32 n   = command.vtx1.n;

    use(len);

    DL_PF("    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", address, v0, n, len);

    if ( address > MAX_RAM_ADDRESS )
    {
        DL_PF("     Address out of range - ignoring load");
        return;
    }

    if ( (v0 + n) > 64 )
    {
        DL_PF("        Warning, attempting to load into invalid vertex positions");
        DBGConsole_Msg( 0, "        DLParser_GBI1_Vtx: Warning, attempting to load into invalid vertex positions" );
        return;
    }

    PSPRenderer::Get()->SetNewVertexInfo( address, v0, n );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
    gNumVertices += n;
    DLParser_DumpVtxInfo( address, v0, n );
#endif
}

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
// gspModifyVertex( s32 vtx, u32 where, u32 val )
//*****************************************************************************
void DLParser_GBI1_ModifyVtx( MicroCodeCommand command )
{
	u32 offset =  command.modifyvtx.offset;
	u32 vert   = command.modifyvtx.vtx;
	u32 value  = command.modifyvtx.value;

	DAEDALUS_ASSERT( offset, " ModifyVtx : Can't handle" );

	// Cures crash after swinging in Mario Golf
	if( vert > 80 )
	{
		DAEDALUS_ERROR("ModifyVtx: Invalid vertex number: %d", vert);
		return;
	}
	
	PSPRenderer::Get()->ModifyVertexInfo( offset, vert, value );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Mtx( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.mtx1.addr);

	DL_PF("    Command: %s %s %s Length %d Address 0x%08x",
		command.mtx1.projection == 1 ? "Projection" : "ModelView",
		command.mtx1.load == 1 ? "Load" : "Mul",
		command.mtx1.push == 1 ? "Push" : "NoPush",
		command.mtx1.len, address);

	// Load matrix from address
	Matrix4x4 mat;
	MatrixFromN64FixedPoint( mat, address );

	if (command.mtx1.projection)
	{
		PSPRenderer::Get()->SetProjection(mat, command.mtx1.push, command.mtx1.load);
	}
	else
	{
		PSPRenderer::Get()->SetWorldView(mat, command.mtx1.push, command.mtx1.load);
	}
}

extern u32 ConkerVtxZAddr;
//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_Mtx( MicroCodeCommand command )
{
	ConkerVtxZAddr = 0;	// For Conker BFD
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
void DLParser_GBI1_PopMtx( MicroCodeCommand command )
{
	DL_PF("    Command: (%s)",	command.popmtx.projection ? "Projection" : "ModelView");

	// Do any of the other bits do anything?
	// So far only Extreme-G seems to Push/Pop projection matrices

	if (command.popmtx.projection)
	{
		PSPRenderer::Get()->PopProjection();
	}
	else
	{
		PSPRenderer::Get()->PopWorldView();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_PopMtx( MicroCodeCommand command )
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	u8 mtx_command = (u8)(command.inst.cmd0 & 0xFF);

	DL_PF("        Command: 0x%02x (%s)", mtx_command, (mtx_command & G_GBI2_MTX_PROJECTION) ? "Projection" : "ModelView");
#endif

	PSPRenderer::Get()->PopWorldView();
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_CullDL( MicroCodeCommand command )
{
	u32 first = command.culldl.first;
	u32 last = command.culldl.end;

	DL_PF(" Culling using verts %d to %d\n", first, last);

	if( last < first ) return;
	if( PSPRenderer::Get()->TestVerts( first, last ) )
	{
		DL_PF(" Display list is visible, returning");
		return;
	}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	++gNumDListsCulled;
#endif

	DL_PF(" No vertices were visible, culling rest of display list");

	DLParser_PopDL();
}

//*****************************************************************************
// 
//*****************************************************************************
void DLParser_GBI1_DL( MicroCodeCommand command )
{
    u32 address = RDPSegAddr(command.dlist.addr);

	DAEDALUS_ASSERT( address < MAX_RAM_ADDRESS, "DL addr out of range (0x%08x)", address );

    DL_PF("    Address=0x%08x Push: 0x%02x", address, command.dlist.param);

	if( command.dlist.param == G_DL_PUSH )
		gDlistStackPointer++;

	gDlistStack[gDlistStackPointer].pc = address;
	gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;
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
//
//*****************************************************************************
void DLParser_GBI1_EndDL( MicroCodeCommand command )
{
	DLParser_PopDL();
}

//*****************************************************************************
//
//*****************************************************************************
// Kirby 64, SSB and Cruisn' Exotica use this
//
void DLParser_GBI2_DL_Count( MicroCodeCommand command )
{
	//DAEDALUS_ERROR("DL_COUNT");

	// This cmd is likely to execute number of ucode at the given address
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
// When the depth is less than the z value provided, branch to given address
//*****************************************************************************
void DLParser_GBI1_BranchZ( MicroCodeCommand command )
{
	//Always branching will usually just waste a bit of fillrate (PSP got plenty)
	//Games seem not to bother if we branch less than Z

	u32 vtx		 = command.branchz.vtx;
	
	//Works in Aerogauge (skips rendering ship shadows and exaust plumes from afar)
	//Fails in OOT : Death Mountain and MM : Outside of Clock Town
	// Seems are Z axis is inverted... Might be tricky to get it right on the PSP

	f32 vtxdepth = 65536.0f * (1.0f - PSPRenderer::Get()->GetProjectedVtxPos(vtx).z / PSPRenderer::Get()->GetProjectedVtxPos(vtx).w);

	s32 zval = (s32)( command.branchz.value & 0x7FFF );

	//printf("%0.0f %d\n", vtxdepth, zval);

	if( (g_ROM.GameHacks != AEROGAUGE) || (vtxdepth >= zval) )
	{					
		u32 pc = gDlistStack[gDlistStackPointer].pc;
		u32 dl = *(u32 *)(g_pu8RamBase + pc-12);
		u32 address = RDPSegAddr(dl);

		DL_PF("BranchZ to DisplayList 0x%08x", address);

		gDlistStack[gDlistStackPointer].pc = address;
		gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;
	}
}

//***************************************************************************** 
// 
//***************************************************************************** 
// AST, Yoshi's World, Scooby Doo
//
void DLParser_GBI1_LoadUCode( MicroCodeCommand command ) 
{ 
	u32 code_base = (command.inst.cmd1 & 0x1fffffff);
    u32 code_size = 0x1000; 
    u32 data_base = gRDPHalf1 & 0x1fffffff;         // Preceeding RDP_HALF1 sets this up
    u32 data_size = (command.inst.cmd0 & 0xFFFF) + 1; // set into range otherwise can go loco (SSV) as -1358952448.. which misses our expected 4096 size...

	DLParser_InitMicrocode( code_base, code_size, data_base, data_size ); 
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
inline void DLParser_InitGeometryMode()
{
	// CULL_BACK has priority, Fixes Mortal Kombat 4
	bool bCullFront         = (gGeometryMode & G_CULL_FRONT)		? true : false;
	bool bCullBack          = (gGeometryMode & G_CULL_BACK)			? true : false;
	PSPRenderer::Get()->SetCullMode(bCullFront, bCullBack);

	bool bShade				= (gGeometryMode & G_SHADE)				? true : false;
	PSPRenderer::Get()->SetSmooth( bShade );

	bool bShadeSmooth       = (gGeometryMode & G_SHADING_SMOOTH)	? true : false;
	PSPRenderer::Get()->SetSmoothShade( bShadeSmooth );

	bool bFog				= (gGeometryMode & G_FOG)				? true : false;
	PSPRenderer::Get()->SetFogEnable( bFog );

	bool bTextureGen        = (gGeometryMode & G_TEXTURE_GEN)		? true : false;
	PSPRenderer::Get()->SetTextureGen(bTextureGen);

	bool bLighting			= (gGeometryMode & G_LIGHTING)			? true : false;
	PSPRenderer::Get()->SetLighting( bLighting );

	bool bZBuffer           = (gGeometryMode & G_ZBUFFER)			? true : false;
	PSPRenderer::Get()->ZBufferEnable( bZBuffer );
}

//***************************************************************************** 
// 
//*****************************************************************************
void DLParser_GBI1_ClearGeometryMode( MicroCodeCommand command )
{
    u32 mask = (command.inst.cmd1);
    
    gGeometryMode &= ~mask;

    DLParser_InitGeometryMode();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
    if (gDisplayListFile != NULL)
    {
            DL_PF("    Mask=0x%08x", mask);
            if (mask & G_ZBUFFER)                           DL_PF("  Disabling ZBuffer");
            if (mask & G_TEXTURE_ENABLE)                    DL_PF("  Disabling Texture");
            if (mask & G_SHADE)                             DL_PF("  Disabling Shade");
            if (mask & G_SHADING_SMOOTH)                    DL_PF("  Disabling Smooth Shading");
            if (mask & G_CULL_FRONT)                        DL_PF("  Disabling Front Culling");
            if (mask & G_CULL_BACK)                         DL_PF("  Disabling Back Culling");
            if (mask & G_FOG)                               DL_PF("  Disabling Fog");
            if (mask & G_LIGHTING)                          DL_PF("  Disabling Lighting");
            if (mask & G_TEXTURE_GEN)                       DL_PF("  Disabling Texture Gen");
            if (mask & G_TEXTURE_GEN_LINEAR)                DL_PF("  Disabling Texture Gen Linear");
            if (mask & G_LOD)                               DL_PF("  Disabling LOD (no impl)");
    }
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_SetGeometryMode(  MicroCodeCommand command  )
{
    u32 mask = command.inst.cmd1;

    gGeometryMode |= mask;

    DLParser_InitGeometryMode();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
    if (gDisplayListFile != NULL)
    {
            DL_PF("    Mask=0x%08x", mask);
            if (mask & G_ZBUFFER)                           DL_PF("  Enabling ZBuffer");
            if (mask & G_TEXTURE_ENABLE)                    DL_PF("  Enabling Texture");
            if (mask & G_SHADE)                             DL_PF("  Enabling Shade");
            if (mask & G_SHADING_SMOOTH)                    DL_PF("  Enabling Smooth Shading");
            if (mask & G_CULL_FRONT)                        DL_PF("  Enabling Front Culling");
            if (mask & G_CULL_BACK)                         DL_PF("  Enabling Back Culling");
            if (mask & G_FOG)                               DL_PF("  Enabling Fog");
            if (mask & G_LIGHTING)                          DL_PF("  Enabling Lighting");
            if (mask & G_TEXTURE_GEN)                       DL_PF("  Enabling Texture Gen");
            if (mask & G_TEXTURE_GEN_LINEAR)                DL_PF("  Enabling Texture Gen Linear");
            if (mask & G_LOD)                               DL_PF("  Enabling LOD (no impl)");
    }
#endif
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

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
    if (gDisplayListFile != NULL)
    {
            DL_PF("    0x%08x 0x%08x =(x & 0x%08x) | 0x%08x", command.inst.cmd0, command.inst.cmd1, and_bits, or_bits);

            if ((~and_bits) & G_ZELDA_ZBUFFER)						DL_PF("  Disabling ZBuffer");
            if ((~and_bits) & G_ZELDA_SHADING_SMOOTH)				DL_PF("  Disabling Flat Shading");
            if ((~and_bits) & G_ZELDA_CULL_FRONT)                   DL_PF("  Disabling Front Culling");
            if ((~and_bits) & G_ZELDA_CULL_BACK)                    DL_PF("  Disabling Back Culling");
            if ((~and_bits) & G_ZELDA_FOG)							DL_PF("  Disabling Fog");
            if ((~and_bits) & G_ZELDA_LIGHTING)						DL_PF("  Disabling Lighting");
            if ((~and_bits) & G_ZELDA_TEXTURE_GEN)                  DL_PF("  Disabling Texture Gen");
			if ((~and_bits) & G_ZELDA_TEXTURE_GEN_LINEAR)			DL_PF("  Enabling Texture Gen Linear");

            if (or_bits & G_ZELDA_ZBUFFER)							DL_PF("  Enabling ZBuffer");
            if (or_bits & G_ZELDA_SHADING_SMOOTH)					DL_PF("  Enabling Flat Shading");
            if (or_bits & G_ZELDA_CULL_FRONT)						DL_PF("  Enabling Front Culling");
            if (or_bits & G_ZELDA_CULL_BACK)						DL_PF("  Enabling Back Culling");
            if (or_bits & G_ZELDA_FOG)								DL_PF("  Enabling Fog");
            if (or_bits & G_ZELDA_LIGHTING)							DL_PF("  Enabling Lighting");
            if (or_bits & G_ZELDA_TEXTURE_GEN)						DL_PF("  Enabling Texture Gen");
			if (or_bits & G_ZELDA_TEXTURE_GEN_LINEAR)               DL_PF("  Enabling Texture Gen Linear");
    }
#endif

    gGeometryMode &= and_bits;
    gGeometryMode |= or_bits;

    bool bCullFront         = (gGeometryMode & G_ZELDA_CULL_FRONT)			? true : false;
    bool bCullBack          = (gGeometryMode & G_ZELDA_CULL_BACK)			? true : false;
    PSPRenderer::Get()->SetCullMode(bCullFront, bCullBack);

//  bool bShade				= (gGeometryMode & G_SHADE)						? true : false;
//  bool bFlatShade         = (gGeometryMode & G_ZELDA_SHADING_SMOOTH)		? true : false;

	bool bFlatShade         = (gGeometryMode & G_ZELDA_TEXTURE_GEN_LINEAR)	? true : false;
    if (g_ROM.GameHacks == TIGERS_HONEY_HUNT) bFlatShade = false;	// Hack for Tiger Honey Hunt
    PSPRenderer::Get()->SetSmooth( !bFlatShade );

    bool bFog				= (gGeometryMode & G_ZELDA_FOG)					? true : false;
    PSPRenderer::Get()->SetFogEnable( bFog );

	bool bTextureGen        = (gGeometryMode & G_ZELDA_TEXTURE_GEN)			? true : false;
    PSPRenderer::Get()->SetTextureGen(bTextureGen);

    bool bLighting			= (gGeometryMode & G_ZELDA_LIGHTING)			? true : false;
    PSPRenderer::Get()->SetLighting( bLighting );

	bool bZBuffer           = (gGeometryMode & G_ZELDA_ZBUFFER)				? true : false;
    PSPRenderer::Get()->ZBufferEnable( bZBuffer );

    PSPRenderer::Get()->SetSmoothShade( true );             // Always do this - not sure which bit to use
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_SetOtherModeL( MicroCodeCommand command )
{
    u32 mask		= ((1 << command.othermode.len) - 1) << command.othermode.sft;

    gRDPOtherMode.L = (gRDPOtherMode.L&(~mask)) | command.othermode.data;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	RDP_SetOtherMode( gRDPOtherMode.H, gRDPOtherMode.L );
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_SetOtherModeH( MicroCodeCommand command )
{
    u32 mask		= ((1 << command.othermode.len) - 1) << command.othermode.sft;

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
void DLParser_GBI1_Texture( MicroCodeCommand command )
{

    gTextureLevel = command.texture.level;
    gTextureTile  = command.texture.tile;

    bool enable = command.texture.enable_gbi0;                        // Seems to use 0x01
	if( enable )
	{
		f32 scale_s = f32(command.texture.scaleS) * (1.0f / (65536.0f * 32.0f));
		f32 scale_t = f32(command.texture.scaleT) * (1.0f / (65536.0f * 32.0f));

		DL_PF("    ScaleS: %f, ScaleT: %f", scale_s*32.0f, scale_t*32.0f);
		PSPRenderer::Get()->SetTextureScale( scale_s, scale_t );
	}

	DL_PF("    Level: %d Tile: %d %s", gTextureLevel, gTextureTile, enable ? "enabled":"disabled");
	PSPRenderer::Get()->SetTextureEnable( enable );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_Texture( MicroCodeCommand command )
{
    gTextureLevel = command.texture.level;
    gTextureTile  = command.texture.tile;


    bool enable = command.texture.enable_gbi2;                        // Seems to use 0x02
	if( enable )
	{
		f32 scale_s = f32(command.texture.scaleS) * (1.0f / (65536.0f * 32.0f));
		f32 scale_t = f32(command.texture.scaleT) * (1.0f / (65536.0f * 32.0f));

		DL_PF("    ScaleS: %f, ScaleT: %f", scale_s*32.0f, scale_t*32.0f);
		PSPRenderer::Get()->SetTextureScale( scale_s, scale_t );
	}

	DL_PF("    Level: %d Tile: %d %s", gTextureLevel, gTextureTile, enable ? "enabled":"disabled");
    PSPRenderer::Get()->SetTextureEnable( enable );
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

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
        if ( command.inst.cmd == G_GBI2_QUAD )
        {
                //DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, gInstructionName[ command.inst.cmd ]);
                DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_QUAD");
        }
#endif
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

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
        if ( command.inst.cmd == G_GBI2_LINE3D )
        {
                //DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, gInstructionName[ command.inst.cmd ]);
                DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_LINE3D");
        }
#endif
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
        //u32 flags = (command.inst.cmd1>>24)&0xFF;
        u32 v0_idx = command.gbi2tri1.v0 >> 1;
		u32 v1_idx = command.gbi2tri1.v1 >> 1;
		u32 v2_idx = command.gbi2tri1.v2 >> 1;

        tris_added |= PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx);

        command.inst.cmd0 = *pCmdBase++;
        command.inst.cmd1 = *pCmdBase++;
        pc += 8;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
        if ( command.inst.cmd == G_GBI2_TRI1 )
        {
                //DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, gInstructionName[ command.inst.cmd ]);
                DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_TRI1");
        }
#endif			
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
		// Vertex indices are exact !
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

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
        if ( command.inst.cmd == G_GBI2_TRI2 )
        {
                //DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, gInstructionName[ command.inst.cmd ]);
                DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_TRI2");
        }
#endif
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
void DLParser_GBI1_Tri2( MicroCodeCommand command )
{
    // While the next command pair is Tri2, add vertices
	u32 pc = gDlistStack[gDlistStackPointer].pc;
    u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

    bool tris_added = false;

    do{
		// Vertex indices are multiplied by 10 for GBI0, by 2 for GBI1
		u32 v0_idx = command.gbi1tri2.v0 / gVertexStride;
		u32 v1_idx = command.gbi1tri2.v1 / gVertexStride;
		u32 v2_idx = command.gbi1tri2.v2 / gVertexStride;

		tris_added |= PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx);

		u32 v3_idx = command.gbi1tri2.v3 / gVertexStride;
		u32 v4_idx = command.gbi1tri2.v4 / gVertexStride;
		u32 v5_idx = command.gbi1tri2.v5 / gVertexStride;

		tris_added |= PSPRenderer::Get()->AddTri(v3_idx, v4_idx, v5_idx);

		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		pc += 8;

	#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		if ( command.inst.cmd == G_GBI1_TRI2 )
		{
	//		DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, gInstructionName[ command.inst.cmd ]);
		}
#endif
    }while( command.inst.cmd == G_GBI1_TRI2 );

	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
            PSPRenderer::Get()->FlushTris();
    }
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Line3D( MicroCodeCommand command )
{
    // While the next command pair is Tri1, add vertices
	u32 pc = gDlistStack[gDlistStackPointer].pc;
    u32 * pCmdBase = (u32 *)( g_pu8RamBase + pc );

    bool tris_added = false;

	if( command.gbi1line3d.v3 == 0 )
	{
		// This removes the tris that cover the screen in Flying Dragon
		// Actually this wrong, we should support line3D properly here..
		DAEDALUS_ERROR("Flying Dragon Hack -- Skipping Line3D");
		return;
	}

	do{
		u32 v0_idx   = command.gbi1line3d.v0 / gVertexStride;
		u32 v1_idx   = command.gbi1line3d.v1 / gVertexStride;
		u32 v2_idx   = command.gbi1line3d.v2 / gVertexStride;
		u32 v3_idx   = command.gbi1line3d.v3 / gVertexStride;

		tris_added |= PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx);
		tris_added |= PSPRenderer::Get()->AddTri(v2_idx, v3_idx, v0_idx);

		command.inst.cmd0 = *pCmdBase++;
		command.inst.cmd1 = *pCmdBase++;
		pc += 8;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		if ( command.inst.cmd == G_GBI1_LINE3D )
		{
//			DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, gInstructionName[ command.inst.cmd ]);
		}
#endif
	}while( command.inst.cmd == G_GBI1_LINE3D );

	gDlistStack[gDlistStackPointer].pc = pc-8;

	if (tris_added)
	{
			PSPRenderer::Get()->FlushTris();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Tri1( MicroCodeCommand command )
{
    //DAEDALUS_PROFILE( "DLParser_GBI1_Tri1_T" );
    // While the next command pair is Tri1, add vertices
	u32 pc = gDlistStack[gDlistStackPointer].pc;
    u32 * pCmdBase = (u32 *)( g_pu8RamBase + pc );

    bool tris_added = false;

    do{
        //u32 flags = (command.inst.cmd1>>24)&0xFF;
        // Vertex indices are multiplied by 10 for Mario64, by 2 for MarioKart
        u32 v0_idx = command.gbi1tri1.v0 / gVertexStride;
        u32 v1_idx = command.gbi1tri1.v1 / gVertexStride;
        u32 v2_idx = command.gbi1tri1.v2 / gVertexStride;

        tris_added |= PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx);

        command.inst.cmd0= *pCmdBase++;
        command.inst.cmd1= *pCmdBase++;
        pc += 8;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
        if ( command.inst.cmd == G_GBI1_TRI1 )
        {
//				DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, gInstructionName[ command.inst.cmd ]);
        }
#endif
    }while( command.inst.cmd == G_GBI1_TRI1 );

	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
		PSPRenderer::Get()->FlushTris();
	}
}

//*****************************************************************************
// It's used by Golden Eye
//*****************************************************************************
void DLParser_GBI0_Tri4( MicroCodeCommand command )
{
	//DAEDALUS_ERROR("GBI0_Tri4 ");
    // While the next command pair is Tri2, add vertices
	u32 pc = gDlistStack[gDlistStackPointer].pc;

    bool tris_added = false;

    do{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
        u32 flags = (command.inst.cmd0 >> 16) & 0xFF;
		DL_PF("    GBI0 Tri4: 0x%08x 0x%08x Flag: 0x%02x", command.inst.cmd0, command.inst.cmd1, flags);
#endif
		//Tri #1
		u32 v0 = command.tri4.v0;
		u32 v1 = command.tri4.v1;
		u32 v2 = command.tri4.v2;

		tris_added |= PSPRenderer::Get()->AddTri(v0, v1, v2);

		//Tri #2
		u32 v3 = command.tri4.v3;
		u32 v4 = command.tri4.v4;
		u32 v5 = command.tri4.v5;

		tris_added |= PSPRenderer::Get()->AddTri(v3, v4, v5);

		//Tri #3
		u32 v6 = command.tri4.v6;
		u32 v7 = command.tri4.v7;
		u32 v8 = command.tri4.v8;

		tris_added |= PSPRenderer::Get()->AddTri(v6, v7, v8);

		//Tri #4
		u32 v9  = command.tri4.v9;
		u32 v10 = command.tri4.v10;
		u32 v11 = command.tri4.v11;

		tris_added |= PSPRenderer::Get()->AddTri(v9, v10, v11);

		command.inst.cmd0			= *(u32 *)(g_pu8RamBase + pc+0);
		command.inst.cmd1			= *(u32 *)(g_pu8RamBase + pc+4);
		pc += 8;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
        if ( command.inst.cmd == G_GBI1_TRI2 )
        {
//			DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, gInstructionName[ command.inst.cmd ]);
        }
#endif
    }while( command.inst.cmd == G_GBI1_TRI2 );

	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
		PSPRenderer::Get()->FlushTris();
	}
}

//*****************************************************************************
// Actually line3d, not supported I think.
//*****************************************************************************
void DLParser_GBI0_Quad( MicroCodeCommand command ) 
{
	DAEDALUS_ERROR("GBI0_Quad : Line3D not supported in ucode0 ? ( Ignored )");
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