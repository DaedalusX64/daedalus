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

	DL_PF( "    Address[0x%08x] vEnd[%d] v0[%d] Num[%d]", address, vend, v0, n );

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
	u32 address = RDPSegAddr(command.mtx2.addr);

	DL_PF("    Command: %s %s %s Length %d Address 0x%08x",
		command.mtx2.projection ? "Projection" : "ModelView",
		command.mtx2.load ? "Load" : "Mul",
		command.mtx2.nopush == 0 ? "Push" : "No Push",
		command.mtx2.len, address);

	// Load matrix from address
	if (command.mtx2.projection)
	{
		PSPRenderer::Get()->SetProjection(address, command.mtx2.nopush==0, command.mtx2.load);
	}
	else
	{
		PSPRenderer::Get()->SetWorldView(address, command.mtx2.nopush==0, command.mtx2.load);
	}
}

//*****************************************************************************
//
//*****************************************************************************
//0016A710: DB020000 00000018 CMD Zelda_MOVEWORD  Mem[2][00]=00000018 Lightnum=0
//001889F0: DB020000 00000030 CMD Zelda_MOVEWORD  Mem[2][00]=00000030 Lightnum=2
void DLParser_GBI2_MoveWord( MicroCodeCommand command )
{

	switch (command.mw2.type)
	{
	case G_MW_MATRIX:
		{
			DL_PF("    G_MW_MATRIX(2)");
			PSPRenderer::Get()->InsertMatrix(command.inst.cmd0, command.inst.cmd1);
		}
		break;

	case G_MW_NUMLIGHT:
		{
			// Lightnum
			// command->cmd1:
			// 0x18 = 24 = 0 lights
			// 0x30 = 48 = 2 lights

			u32 num_lights = command.mw2.value / 24;
			DL_PF("    G_MW_NUMLIGHT: %d", num_lights);

			gAmbientLightIdx = num_lights;
			PSPRenderer::Get()->SetNumLights(num_lights);
		}
		break;
/*
	case G_MW_CLIP:	// Seems to be unused?
		{
			DL_PF("     G_MW_CLIP");
		}
		break;
*/
	case G_MW_SEGMENT:
		{
			u32 segment = command.mw2.offset >> 2;
			u32 address	= command.mw2.value;

			DL_PF( "    G_MW_SEGMENT Segment[%d] = 0x%08x", segment, address );

			gSegments[segment] = address;
		}
		break;

	case G_MW_FOG: // WIP, only works for a few games
		{
			f32 a = command.mw2.value >> 16;
			f32 b = command.mw2.value & 0xFFFF;

			//f32 min = b - a;
			//f32 max = b + a;
			//min = min * (1.0f / 16.0f);
			//max = max * (1.0f / 4.0f);
			f32 min = a / 256.0f;
			f32 max = b / 6.0f;

			//DL_PF(" G_MW_FOG. Mult = 0x%04x (%f), Off = 0x%04x (%f)", wMult, 255.0f * fMult, wOff, 255.0f * fOff );

			PSPRenderer::Get()->SetFogMinMax(min, max);

			//printf("1Fog %.0f | %.0f || %.0f | %.0f\n", min, max, a, b);
		}
		break;

	case G_MW_LIGHTCOL:
		{
			u32 light_idx = command.mw2.offset / 0x18;
			u32 field_offset = (command.mw2.offset & 0x7);

			DL_PF("    G_MW_LIGHTCOL/0x%08x: 0x%08x", command.mw2.offset, command.mw2.value);

			if (field_offset == 0)
			{
				// Light col, not the copy
				if (light_idx == gAmbientLightIdx)
				{
					v3 col( N64COL_GETR_F(command.mw2.value), N64COL_GETG_F(command.mw2.value), N64COL_GETB_F(command.mw2.value) );
					PSPRenderer::Get()->SetAmbientLight( col );
				}
				else
				{
					PSPRenderer::Get()->SetLightCol(light_idx, command.mw2.value);
				}
			}
		}
		break;
/*
	case G_MW_PERSPNORM:
		DL_PF("     G_MW_PERSPNORM 0x%04x", (s16)command.inst.cmd1);
		break;

	case G_MW_POINTS:
		DL_PF("     G_MW_POINTS : Ignored");
		break;
*/
	default:
		{
			DL_PF("    Ignored!!");

		}
		break;
	}
}

//*****************************************************************************
//
//*****************************************************************************
/*

001889F8: DC08060A 80188708 CMD Zelda_MOVEMEM  Movemem[0806] <- 80188708
!light 0 color 0.12 0.16 0.35 dir 0.01 0.00 0.00 0.00 (2 lights) [ 1E285A00 1E285A00 01000000 00000000 ]
data(00188708): 1E285A00 1E285A00 01000000 00000000 
00188A00: DC08090A 80188718 CMD Zelda_MOVEMEM  Movemem[0809] <- 80188718
!light 1 color 0.23 0.25 0.30 dir 0.01 0.00 0.00 0.00 (2 lights) [ 3C404E00 3C404E00 01000000 00000000 ]
data(00188718): 3C404E00 3C404E00 01000000 00000000 
00188A08: DC080C0A 80188700 CMD Zelda_MOVEMEM  Movemem[080C] <- 80188700
!light 2 color 0.17 0.16 0.26 dir 0.23 0.31 0.70 0.00 (2 lights) [ 2C294300 2C294300 1E285A00 1E285A00 ]
*/
/*
ZeldaMoveMem: 0xdc080008 0x801984d8
SetScissor: x0=416 y0=72 x1=563 y1=312 mode=0
// Mtx
ZeldaMoveWord:0xdb0e0000 0x00000041 Ignored
ZeldaMoveMem: 0xdc08000a 0x80198538
ZeldaMoveMem: 0xdc08030a 0x80198548

ZeldeMoveMem: Unknown Type. 0xdc08000a 0x80198518
ZeldeMoveMem: Unknown Type. 0xdc08030a 0x80198528
ZeldeMoveMem: Unknown Type. 0xdc08000a 0x80198538
ZeldeMoveMem: Unknown Type. 0xdc08030a 0x80198548
ZeldeMoveMem: Unknown Type. 0xdc08000a 0x80198518
ZeldeMoveMem: Unknown Type. 0xdc08030a 0x80198528
ZeldeMoveMem: Unknown Type. 0xdc08000a 0x80198538
ZeldeMoveMem: Unknown Type. 0xdc08030a 0x80198548


0xa4001120: <0x0c000487> JAL       0x121c        Seg2Addr(t8)				dram
0xa4001124: <0x332100fe> ANDI      at = t9 & 0x00fe
0xa4001128: <0x937309c1> LBU       s3 <- 0x09c1(k1)							len
0xa400112c: <0x943402f0> LHU       s4 <- 0x02f0(at)							dmem
0xa4001130: <0x00191142> SRL       v0 = t9 >> 0x0005
0xa4001134: <0x959f0336> LHU       ra <- 0x0336(t4)
0xa4001138: <0x080007f6> J         0x1fd8        SpMemXfer
0xa400113c: <0x0282a020> ADD       s4 = s4 + v0								dmem

ZeldaMoveMem: 0xdc08000a 0x8010e830 Type: 0a Len: 08 Off: 4000
ZeldaMoveMem: 0xdc08030a 0x8010e840 Type: 0a Len: 08 Off: 4018
// Light
ZeldaMoveMem: 0xdc08060a 0x800ff368 Type: 0a Len: 08 Off: 4030
ZeldaMoveMem: 0xdc08090a 0x800ff360 Type: 0a Len: 08 Off: 4048
//VP
ZeldaMoveMem: 0xdc080008 0x8010e3c0 Type: 08 Len: 08 Off: 4000

*/

//*****************************************************************************
//
//*****************************************************************************

void DLParser_GBI2_MoveMem( MicroCodeCommand command )
{

	u32 address	 = RDPSegAddr(command.inst.cmd1);
	//u32 offset = (command.inst.cmd0 >> 8) & 0xFFFF;
	u32 type	 = (command.inst.cmd0     ) & 0xFE;
	//u32 length  = (command.inst.cmd0 >> 16) & 0xFF;

	switch (type)
	{
	case G_GBI2_MV_VIEWPORT:
		{
			RDP_MoveMemViewport( address );
		}
		break;

	case G_GBI2_MV_LIGHT:
		{
			u32 offset2 = (command.inst.cmd0 >> 5) & 0x3FFF;

		switch (offset2)
		{
		case 0x00:
		case 0x18:
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			{
				s8 * pcBase = g_ps8RamBase + address;
				DL_PF("    G_MV_LOOKAT %f %f %f", f32(pcBase[8 ^ 0x3]), f32(pcBase[9 ^ 0x3]), f32(pcBase[10 ^ 0x3]));
			}
#endif
			break;
		default:		//0x30/48/60
			{
				u32 light_idx = (offset2 - 0x30)/0x18;
				//DL_PF("    Light %d:", light_idx);
				RDP_MoveMemLight(light_idx, address);
			}
			break;
		}
		break;

		}
		break;

	case G_GBI2_MV_MATRIX:
		{
			DL_PF("    Force Matrix(2): addr=%08X", address);
			// Rayman 2, Donald Duck, Tarzan, all wrestling games use this
			PSPRenderer::Get()->ForceMatrix( address );
		}
		break;
/*
	case G_GBI2_MVO_L0:
	case G_GBI2_MVO_L1:
	case G_GBI2_MVO_L2:
	case G_GBI2_MVO_L3:
	case G_GBI2_MVO_L4:
	case G_GBI2_MVO_L5:
	case G_GBI2_MVO_L6:
	case G_GBI2_MVO_L7:
		DL_PF("MoveMem Light(2)");
		break;
	case G_GBI2_MV_POINT:
		DL_PF("MoveMem Point(2)");
		break;

	case G_GBI2_MVO_LOOKATY:
		DL_PF("MoveMem LOOKATY(2)");
		break;
*/

	case 0x00:
	case 0x02:
		{
			// Ucode for Evangelion.v64
			// 0 ObjMtx
			// 2 SubMtx
			DLParser_S2DEX_ObjMoveMem( command );
		}
		break;

	default:
		DL_PF("    GBI2 MoveMem Type: Unknown");
		DBGConsole_Msg(0, "GBI2 MoveMem: Unknown Type. 0x%08x 0x%08x", command.inst.cmd0, command.inst.cmd1);
		break;
	}
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

	DL_PF("    Address=0x%08x %s", address, (command.dlist.param==G_DL_NOPUSH)? "Jump" : (command.dlist.param==G_DL_PUSH)? "Push" : "?");
	DL_PF("    \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
	DL_PF("    ############################################");
}

//***************************************************************************** 
// 
//***************************************************************************** 
void DLParser_GBI2_LoadUCode( MicroCodeCommand command ) 
{
	DL_PF( "    GBI2_LoadUCode (Ignored)" );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_GeometryMode( MicroCodeCommand command )
{
	gGeometryMode._u32 &= command.inst.arg0;
	gGeometryMode._u32 |= command.inst.arg1;

	DL_PF("    0x%08x 0x%08x =(x & 0x%08x) | 0x%08x", command.inst.cmd0, command.inst.cmd1, command.inst.arg0, command.inst.arg1);
	DL_PF("    ZBuffer %s", (gGeometryMode.GBI2_Zbuffer) ? "On" : "Off");
	DL_PF("    Culling %s", (gGeometryMode.GBI2_CullBack) ? "Back face" : (gGeometryMode.GBI2_CullFront) ? "Front face" : "Off");
	DL_PF("    Flat Shading %s", (gGeometryMode.GBI2_ShadingSmooth) ? "On" : "Off");
	DL_PF("    Lighting %s", (gGeometryMode.GBI2_Lighting) ? "On" : "Off");
	DL_PF("    Texture Gen %s", (gGeometryMode.GBI2_TexGen) ? "On" : "Off");
	DL_PF("    Texture Gen Linear %s", (gGeometryMode.GBI2_TexGenLin) ? "On" : "Off");
	DL_PF("    Fog %s", (gGeometryMode.GBI2_Fog) ? "On" : "Off");

	TnLPSP TnLMode;

	TnLMode.Light		= gGeometryMode.GBI2_Lighting;
	TnLMode.Texture		= 0;	//Force this to false
	TnLMode.TexGen		= gGeometryMode.GBI2_TexGen;
	TnLMode.TexGenLin	= gGeometryMode.GBI2_TexGenLin;
	TnLMode.Fog			= gGeometryMode.GBI2_Fog;
	TnLMode.Shade		= !(gGeometryMode.GBI2_TexGenLin & (g_ROM.GameHacks != TIGERS_HONEY_HUNT));
	TnLMode.Zbuffer		= gGeometryMode.GBI2_Zbuffer;
	TnLMode.TriCull		= gGeometryMode.GBI2_CullFront | gGeometryMode.GBI2_CullBack;
	TnLMode.CullBack	= gGeometryMode.GBI2_CullBack;

	PSPRenderer::Get()->SetTnLMode( TnLMode._u32 );
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_SetOtherModeH( MicroCodeCommand command )
{
    // Mask is constructed slightly differently
    const u32 mask = (u32)((s32)(0x80000000) >> command.othermode.len) >> command.othermode.sft;

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
	const u32 mask = (u32)((s32)(0x80000000) >> command.othermode.len) >> command.othermode.sft;

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
	 
	u32 tile    = command.texture.tile;
    bool enable = command.texture.enable_gbi2;   // Seems to use 0x02                    

	DL_PF("    Level[%d] Tile[%d] %s", command.texture.level, tile, enable ? "enable":"disable");

	PSPRenderer::Get()->SetTextureTile( tile );
    PSPRenderer::Get()->SetTextureEnable( enable );
	
	f32 scale_s = f32(command.texture.scaleS) / (65535.0f * 32.0f);
	f32 scale_t = f32(command.texture.scaleT)  / (65535.0f * 32.0f);

	DL_PF("    ScaleS[%0.4f], ScaleT[%0.4f]", scale_s*32.0f, scale_t*32.0f);
	PSPRenderer::Get()->SetTextureScale( scale_s, scale_t );
}

//*****************************************************************************
//
//*****************************************************************************

void DLParser_GBI2_DMA_IO( MicroCodeCommand command )
{
	DL_UNIMPLEMENTED_ERROR( "G_DMA_IO" );
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
        //DL_PF("    0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_QUAD");

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
// XXX SpiderMan uses this command.DLParser_GBI2_Tri2
void DLParser_GBI2_Line3D( MicroCodeCommand command )
{
	// While the next command pair is Tri2, add vertices
	u32 pc = gDlistStack[gDlistStackPointer].pc;
    u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

    bool tris_added = false;

    do{
        //DL_PF("    0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_LINE3D");

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
        //DL_PF("    0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_TRI1");

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
        //DL_PF("    0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, "G_GBI2_TRI2");

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
	if( (command.inst.arg0 == 0x2F && ((command.inst.cmd1)&0xFF000000) == 0x80000000 )
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
