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

#include "gspCommon.h"

Matrix4x4 gDKRMatrixes[4];
u32 gDKRCMatrixIndex = 0;
u32 gDKRMatrixAddr = 0;
u32 gDKRVtxAddr = 0;
u32 gDKRVtxCount = 0;
bool gDKRBillBoard = false;

u32 gConkerVtxZAddr = 0;
u32 PDCIAddr = 0;

// DKR verts are extra 4 bytes
//*****************************************************************************
//
//*****************************************************************************
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
void DLParser_DumpVtxInfoDKR(u32 address, u32 v0_idx, u32 num_verts)
{
	if (gDisplayListFile != NULL)
	{
		s16 * psSrc = (s16 *)(g_pu8RamBase + address);

		u32 i = 0;
		for ( u32 idx = v0_idx; idx < v0_idx + num_verts; idx++ )
		{
			f32 x = f32(psSrc[(i + 0) ^ 1]);
			f32 y = f32(psSrc[(i + 1) ^ 1]);
			f32 z = f32(psSrc[(i + 2) ^ 1]);

			//u16 wFlags = PSPRenderer::Get()->GetVtxFlags( idx ); //(u16)psSrc[3^0x1];

			u16 wA = psSrc[(i + 3) ^ 1];
			u16 wB = psSrc[(i + 4) ^ 1];

			u8 a = u8(wA>>8);
			u8 b = u8(wA);
			u8 c = u8(wB>>8);
			u8 d = u8(wB);

			const v4 & t = PSPRenderer::Get()->GetProjectedVtxPos( idx );

			DL_PF(" #%02d Pos: {% 3f,% 3f,% 3f} Extra: %02x %02x %02x %02x (Proj: {% 3f,% 3f,% 3f,% 3f})",
				idx, x, y, z, a, b, c, d, t.x/t.w, t.y/t.w, t.z/t.w, t.w );

			i+=5;
		}

		/*
		u16 * pwSrc = (u16 *)(g_pu8RamBase + address);
		i = 0;
		for( u32 idx = v0_idx; idx < v0_idx + num_verts; idx++ )
		{
			DL_PF(" #%02d %04x %04x %04x %04x %04x",
				idx, pwSrc[(i + 0) ^ 1],
				pwSrc[(i + 1) ^ 1],
				pwSrc[(i + 2) ^ 1],
				pwSrc[(i + 3) ^ 1],
				pwSrc[(i + 4) ^ 1]);
			
			i += 5;
		}
		*/

	}
}
#endif

//*****************************************************************************
//
//*****************************************************************************

void DLParser_GBI0_Vtx_Gemini( MicroCodeCommand command )
{
	//u32 address = RDPSegAddr(command.inst.cmd1);
	u32 address		= command.inst.cmd1 + RDPSegAddr(gDKRVtxAddr);
	u32 v0_idx		= (command.inst.cmd0 >> 9 ) & 0x1F;
	u32 num_verts	= (command.inst.cmd0 >> 19) & 0x1F;


	DL_PF("    Address 0x%08x, v0: %d, Num: %d", address, v0_idx, num_verts);

	if(v0_idx >= 32)
		v0_idx = 31;

	if ((v0_idx + num_verts) > 32)
	{
		DBGConsole_Msg(0, "Warning, attempting to load into invalid vertex positions");
		num_verts = 32 - v0_idx;
	}

	// Check that address is valid...
	if ((address + (num_verts*16)) > MAX_RAM_ADDRESS)
	{
		DBGConsole_Msg(0, "SetNewVertexInfoDKR: Address out of range (0x%08x)", address);
	}
	else
	{
		PSPRenderer::Get()->SetNewVertexInfoDKR(address, v0_idx, num_verts);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		gNumVertices += num_verts;
		DLParser_DumpVtxInfoDKR(address, v0_idx, num_verts);
#endif
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI0_DL_SOTE( MicroCodeCommand command )
{
	// SOTE gets our of pc range, make sure to keep it in range otherwise will crash
	//
    u32 address = RDPSegAddr(command.dlist.addr) & (MAX_RAM_ADDRESS-1);

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
void DLParser_GBI0_Vtx_SOTE( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.inst.cmd1);
	u32 len = (command.inst.cmd0)&0xffff;
	u32 n= ((command.inst.cmd0 >> 4) & 0xfff) / 33 + 1;
	u32 v0 = 0;

	use(len);

	DL_PF("    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", address, v0, n, len);

	if(v0 >= 32)
	{
		v0 = 31;
	}

	if ((v0 + n) > 32)
	{
		DBGConsole_Msg(0, "Warning, attempting to load into invalid vertex positions");
		n = 32 - v0;
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
// BB2k
// DKR
//00229B70: 07020010 000DEFC8 CMD G_DLINMEM  Displaylist at 800DEFC8 (stackp 1, limit 2)
//00229A58: 06000000 800DE520 CMD G_GBI1_DL  Displaylist at 800DE520 (stackp 1, limit 0)
//00229B90: 07070038 00225850 CMD G_DLINMEM  Displaylist at 80225850 (stackp 1, limit 7)

void DLParser_DLInMem( MicroCodeCommand command )
{
	gDlistStackPointer++;
	gDlistStack[gDlistStackPointer].pc = command.inst.cmd1;
	gDlistStack[gDlistStackPointer].countdown = (command.inst.cmd0 >> 16) & 0xFF;

	DL_PF("    DLInMem : Address=0x%08x", command.inst.cmd1);
}

//*****************************************************************************
//
//*****************************************************************************
/*
00229C28: 01400040 002327C0 CMD G_MTX  {Matrix} at 802327C0 ind 1  Load:Mod 
00229BB8: 01400040 00232740 CMD G_MTX  {Matrix} at 80232740 ind 1  Load:Mod 
00229BF0: 01400040 00232780 CMD G_MTX  {Matrix} at 80232780 ind 1  Load:Mod 
00229B28: 01000040 002326C0 CMD G_MTX  {Matrix} at 802326C0  Mul:Mod 
00229B78: 01400040 00232700 CMD G_MTX  {Matrix} at 80232700  Mul:Mod 
*/

// 0x80 seems to be mul
// 0x40 load

void DLParser_Mtx_DKR( MicroCodeCommand command )
{	
	u32 address		= command.inst.cmd1 + RDPSegAddr(gDKRMatrixAddr);
	u32 mtx_command = (command.inst.cmd0 >> 16) & 0xF;
	//u32 length      = (command.inst.cmd0      )& 0xFFFF;

	bool mul = false;

	if (mtx_command == 0)
	{
		//DKR : no mult
		mtx_command = (command.inst.cmd0 >> 22) & 0x3;
	}
	else
	{
		//JFG : mult but only if bit is set
		mul = ((command.inst.cmd0 >> 23) & 0x1);
	}

	// Load matrix from address
	Matrix4x4 mat;
	MatrixFromN64FixedPoint( mat, address );

	if( mul )
	{
		gDKRMatrixes[ mtx_command ] = mat * gDKRMatrixes[0];
	}
	else
	{
		gDKRMatrixes[ mtx_command ] = mat;
	}

	gDKRCMatrixIndex = mtx_command;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DL_PF("    Mtx_DKR: Index %d %s Address 0x%08x\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
			mtx_command, mul ? "Mul" : "Load", address,
			mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
			mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
			mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
			mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
	}
#endif

}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_MoveWord_DKR( MicroCodeCommand command )
{
	u32 num_lights;

	switch ((command.inst.cmd0) & 0xFF)
	{
	case G_MW_NUMLIGHT:
		num_lights = command.inst.cmd1 & 0x7;
		gDKRBillBoard = (command.inst.cmd1 & 0x7) ? true : false;

		DL_PF("    G_MW_NUMLIGHT: Val:%d", num_lights);

		gAmbientLightIdx = num_lights;
		PSPRenderer::Get()->SetNumLights(num_lights);
		break;
	case G_MW_LIGHTCOL:
		//DKR
		gDKRCMatrixIndex = (command.inst.cmd1 >> 6) & 0x7;
		//PSPRenderer::Get()->ResetMatrices();
		break;
	default:
		DLParser_GBI1_MoveWord( command );
		break;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_Set_Addr_DKR( MicroCodeCommand command )
{
	gDKRMatrixAddr = command.inst.cmd0 & 0x00FFFFFF;
	gDKRVtxAddr = command.inst.cmd1 & 0x00FFFFFF;
	gDKRVtxCount=0;
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI0_Vtx_DKR( MicroCodeCommand command )
{
	//u32 address = RDPSegAddr(command.inst.cmd1);
	u32 address = command.inst.cmd1 + RDPSegAddr(gDKRVtxAddr);

	u32 v0_idx =  ((command.inst.cmd0 >> 9) & 0x1F);
	u32 num_verts  = ((command.inst.cmd0 >> 19) & 0x1F) + 1;

	DL_PF("    Address 0x%08x, v0: %d, Num: %d", address, v0_idx, num_verts);

	
	if( command.inst.cmd0 & 0x00010000 )
	{
		if( gDKRBillBoard )
			gDKRVtxCount = 1;
	}
	else
	{
		gDKRVtxCount = 0;
	}

	v0_idx += gDKRVtxCount;
	
	
	if (v0_idx >= 32)
		v0_idx = 31;
	
	if ((v0_idx + num_verts) > 32)
	{
		DL_PF("        Warning, attempting to load into invalid vertex positions");
		DBGConsole_Msg(0, "DLParser_GBI0_Vtx_DKR: Warning, attempting to load into invalid vertex positions");
		num_verts = 32 - v0_idx;
	}

	// Check that address is valid...
	if ((address + (num_verts*16)) > MAX_RAM_ADDRESS)
	{
		DBGConsole_Msg(0, "SetNewVertexInfoDKR: Address out of range (0x%08x)", address);
	}
	else
	{
		PSPRenderer::Get()->SetNewVertexInfoDKR(address, v0_idx, num_verts);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		gNumVertices += num_verts;
		DLParser_DumpVtxInfoDKR(address, v0_idx, num_verts);
#endif

	}
}

//*****************************************************************************
//
//*****************************************************************************
//DKR: 00229BA8: 05710080 001E4AF0 CMD G_DMATRI  Triangles 9 at 801E4AF0
void DLParser_DMA_Tri_DKR( MicroCodeCommand command )
{
	//If bit is set then do backface culling on tris
	//PSPRenderer::Get()->SetCullMode(false, (command.inst.cmd0 & 0x00010000));

	u32 address = RDPSegAddr(command.inst.cmd1);
	u32 count = (command.inst.cmd0 >> 4) & 0xFFF;
	u32 * pData = &g_pu32RamBase[address >> 2];

	bool tris_added = false;

	for (u32 i = 0; i < count; i++)
	{
		DL_PF("    0x%08x: %08x %08x %08x %08x", address + i*16, pData[0], pData[1], pData[2], pData[3]);

		u32 info = pData[ 0 ];

		u32 v0_idx = (info >> 16) & 0x1F;
		u32 v1_idx = (info >>  8) & 0x1F;
		u32 v2_idx = (info      ) & 0x1F;

		if( PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx) )
		{
			tris_added = true;

			//// Generate texture coordinates
			s16 s0( s16(pData[1] >> 16) );
			s16 t0( s16(pData[1] & 0xFFFF) );
			s16 s1( s16(pData[2] >> 16) );
			s16 t1( s16(pData[2] & 0xFFFF) );
			s16 s2( s16(pData[3] >> 16) );
			s16 t2( s16(pData[3] & 0xFFFF) );

			PSPRenderer::Get()->SetVtxTextureCoord( v0_idx, s0, t0 );
			PSPRenderer::Get()->SetVtxTextureCoord( v1_idx, s1, t1 );
			PSPRenderer::Get()->SetVtxTextureCoord( v2_idx, s2, t2 );
		}

		pData += 4;
	}

	if (tris_added)	
	{
		PSPRenderer::Get()->FlushTris();
	}

	gDKRVtxCount = 0;
}

//*****************************************************************************
// The previous way of calculating was based on the assumption that
// there was no "n" field. I didn't realise that the n/length fields shared the
// lower 16 bits (in a 7:9 split).
// u32 length    = (command.inst.cmd0)&0xFFFF;
// u32 num_verts = (length + 1) / 0x210;					// 528
// u32 v0_idx    = ((command.inst.cmd0>>16)&0xFF)/VertexStride;	// /5
//*****************************************************************************
void DLParser_GBI0_Vtx_WRUS( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.inst.cmd1);
	
	u32 v0  = ((command.inst.cmd0 >>16 ) & 0xff) / 5;
	u32 n   =  (command.inst.cmd0 >>9  ) & 0x7f;
	u32 len =  (command.inst.cmd0      ) & 0x1ff;

	use(len);

	DL_PF( "    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", address, v0, n, len );

	if ( (v0 + n) > 32 )
	{
		DL_PF("    *Warning, attempting to load into invalid vertex positions");
		DBGConsole_Msg(0, "DLParser_GBI0_Vtx_WRUS: Warning, attempting to load into invalid vertex positions");
		return;
	}

	PSPRenderer::Get()->SetNewVertexInfo( address, v0, n );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gNumVertices += n;
	DLParser_DumpVtxInfo( address, v0, n );
#endif

}

//*****************************************************************************

//IS called Last Legion, but is used for several other games like: Dark Rift, Toukon Road, Toukon Road 2.

//*****************************************************************************
// Only thing I can't figure out why are the characters on those games invisble?
// Dark Rift runs properly without custom microcodes, and has the same symptoms...
// We need Turbo3D ucode support, actually a modified version of it, thanks Gonetz for the info :D

void DLParser_RSP_Last_Legion_0x80( MicroCodeCommand command )
{     
      gDlistStack[gDlistStackPointer].pc += 16;
	  DL_PF("DLParser_RSP_Last_Legion_0x80");
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_RSP_Last_Legion_0x00( MicroCodeCommand command )
{

	gDlistStack[gDlistStackPointer].pc += 16;
	DL_PF("DLParser_RSP_Last_Legion_0x00");

	if( (command.inst.cmd0) == 0 && (command.inst.cmd1) )
	{
		u32 newaddr = RDPSegAddr((command.inst.cmd1));
		if( newaddr >= MAX_RAM_ADDRESS )
		{
			DLParser_PopDL();
			return;
		}

		u32 pc1 = *(u32 *)(g_pu8RamBase + newaddr+8*1+4);
		u32 pc2 = *(u32 *)(g_pu8RamBase + newaddr+8*4+4);
		pc1 = RDPSegAddr(pc1);
		pc2 = RDPSegAddr(pc2);

		if( pc1 && pc1 != 0xffffff && pc1 < MAX_RAM_ADDRESS)
		{
			// Need to call both DL
			gDlistStackPointer++;
			gDlistStack[gDlistStackPointer].pc = pc1;
			gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;
		}

		if( pc2 && pc2 != 0xffffff && pc2 < MAX_RAM_ADDRESS )
		{
			// Need to call both DL
			gDlistStackPointer++;
			gDlistStack[gDlistStackPointer].pc = pc2;
			gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;
		}
	}
	else if( (command.inst.cmd1) == 0 )
	{
		DLParser_PopDL();
	}
	else
	{
		DLParser_Nothing( command );
		DLParser_PopDL();
	}
}

//*****************************************************************************
//
//*****************************************************************************
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
void DLParser_TexRect_Last_Legion( MicroCodeCommand command )
{
	MicroCodeCommand command2;
	MicroCodeCommand command3;

	//
	// Fetch the next two instructions
	//
	DLParser_FetchNextCommand( &command2 );
	DLParser_FetchNextCommand( &command3 );

	RDP_TexRect tex_rect;
	tex_rect.cmd0 = command.inst.cmd0;
	tex_rect.cmd1 = command.inst.cmd1;

	// Note : these are in a different order than normal texrect!
	//
	tex_rect.cmd2 = command3.inst.cmd1;
	tex_rect.cmd3 = command2.inst.cmd1;

	v2 d( tex_rect.dsdx / 1024.0f, tex_rect.dtdy / 1024.0f );
	v2 xy0( tex_rect.x0 / 4.0f, tex_rect.y0 / 4.0f );
	v2 xy1;
	v2 uv0( tex_rect.s / 32.0f, tex_rect.t / 32.0f );
	v2 uv1;

	//
	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1.0f to the w/h)
	//
	switch ( gRDPOtherMode.cycle_type )
	{
		case CYCLE_COPY:
			d.x *= 0.25f;	// In copy mode 4 pixels are copied at once.
		case CYCLE_FILL:
			xy1.x = (tex_rect.x1 + 4) * 0.25f;
			xy1.y = (tex_rect.y1 + 4) * 0.25f;
			break;
		default:
			xy1.x = tex_rect.x1 * 0.25f;
			xy1.y = tex_rect.y1 * 0.25f;
			break;
	}

	uv1.x = uv0.x + d.x * ( xy1.x - xy0.x );
	uv1.y = uv0.y + d.y * ( xy1.y - xy0.y );

	PSPRenderer::Get()->TexRect( tex_rect.tile_idx, xy0, xy1, uv0, uv1 );
}

#undef __GE_NOTHING
//*****************************************************************************
//
//*****************************************************************************
void DLParser_RDPHalf1_GoldenEye( MicroCodeCommand command )
{
	// Check for invalid address
	if ( (command.inst.cmd1)>>24 != 0xce )	
		return;

	u32 pc = gDlistStack[gDlistStackPointer].pc;		// This points to the next instruction
	u32 * Cmd = (u32 *)(g_pu8RamBase + pc);

	// Indices
	u32 a1 = *Cmd+8*0+4;
	u32 a3 = *Cmd+8*2+4;

	// Unused for now
#ifdef __GE_NOTHING
	u32 a2 = *Cmd+8*1+4;
	u32 a4 = *Cmd+8*3+4;
	u32 a5 = *Cmd+8*4+4;
	u32 a6 = *Cmd+8*5+4;
	u32 a7 = *Cmd+8*6+4;
	u32 a8 = *Cmd+8*7+4;
	u32 a9 = *Cmd+8*8+4;
#endif

	// Note : Color itself is handled elsewhere N.B Blendmode.cpp
	//
	// Coordinates, textures
	s32 x0 = s32(a3>>16)>>24;	// Loads our texture coordinates
	s32 y0 = s32(a1&0xFFFF)/4;	// Loads color coordinates etc
	s32 x1 = 320*100;			// Loads Both screen coordinates and texture coordinates.
	s32 y1 = s32(a1>>16)/4;		// Loads texture etc

	// TIP : f32 x1 can be modified to render the sky differently.
	// Need to check on real hardware to tweak our sky correctly if needed.

	// Loads texrect
	v2 xy0( x0, x0 );
	v2 xy1( x1, x1 );
	v2 uv0( y0 / 40.0f, y0 / 40.0f );
	v2 uv1( y1 / 40.0f, y1 / 40.0f );

	//DL_PF(" Word 1: %u, Word 2: %u, Word 3: %u, Word 4: %u, Word 5: %u, Word 6: %u, Word 7: %u, Word 8: %u, Word 9: %u", a1, a2, a3, a4, a5, a6, a7, a8, a9);
	//DL_PF("    Tile:%d Screen(%f,%f) -> (%f,%f)",				   tile, xy0, xy1, uv0, uv1);
	PSPRenderer::Get()->TexRect( 0, xy0, xy1, uv0, uv1 );

	gDlistStack[gDlistStackPointer].pc += 312;
}

//*****************************************************************************
//
//*****************************************************************************
#if 1	//1->Struct, 0->Old
void DLParser_GBI2_Conker( MicroCodeCommand command )
{
	u32 pc = gDlistStack[gDlistStackPointer].pc;		// This points to the next instruction

    bool tris_added = false;

	do{	//Tri #1
		tris_added |= PSPRenderer::Get()->AddTri(command.conkertri4.v0, command.conkertri4.v1, command.conkertri4.v2);

		//Tri #2
		tris_added |= PSPRenderer::Get()->AddTri(command.conkertri4.v3, command.conkertri4.v4, command.conkertri4.v5);

		//Tri #3
		tris_added |= PSPRenderer::Get()->AddTri(command.conkertri4.v6, command.conkertri4.v7, command.conkertri4.v8);

		//Tri #4
		tris_added |= PSPRenderer::Get()->AddTri((command.conkertri4.v9hi << 2) | command.conkertri4.v9lo, command.conkertri4.v10, command.conkertri4.v11);

		command.inst.cmd0			= *(u32 *)(g_pu8RamBase + pc+0);
		command.inst.cmd1			= *(u32 *)(g_pu8RamBase + pc+4);
		pc += 8;
    }while ( command.conkertri4.cmd == 1 );

	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
            PSPRenderer::Get()->FlushTris();
    }
}

#else
void DLParser_GBI2_Conker( MicroCodeCommand command )
{
	u32 pc = gDlistStack[gDlistStackPointer].pc;		// This points to the next instruction

    bool tris_added = false;

	while ( (command.inst.cmd > 0x0F) && (command.inst.cmd < 0x20) )
    {
		u32 idx[12];

		//Tri #1
		idx[0] = (command.inst.cmd1   )&0x1F;
		idx[1] = (command.inst.cmd1>> 5)&0x1F;
		idx[2] = (command.inst.cmd1>>10)&0x1F;

		tris_added |= PSPRenderer::Get()->AddTri(idx[0], idx[1], idx[2]);

		//Tri #2
		idx[3] = (command.inst.cmd1>>15)&0x1F;
		idx[4] = (command.inst.cmd1>>20)&0x1F;
		idx[5] = (command.inst.cmd1>>25)&0x1F;

		tris_added |= PSPRenderer::Get()->AddTri(idx[3], idx[4], idx[5]);

		//Tri #3
		idx[6] = (command.inst.cmd0    )&0x1F;
		idx[7] = (command.inst.cmd0>> 5)&0x1F;
		idx[8] = (command.inst.cmd0>>10)&0x1F;

		tris_added |= PSPRenderer::Get()->AddTri(idx[6], idx[7], idx[8]);

		//Tri #4
		idx[ 9] = (((command.inst.cmd0>>15)&0x7)<<2)|(command.inst.cmd1>>30);
		idx[10] = (command.inst.cmd0>>18)&0x1F;
		idx[11] = (command.inst.cmd0>>23)&0x1F;

		tris_added |= PSPRenderer::Get()->AddTri(idx[9], idx[10], idx[11]);

		command.inst.cmd0			= *(u32 *)(g_pu8RamBase + pc+0);
		command.inst.cmd1			= *(u32 *)(g_pu8RamBase + pc+4);
		pc += 8;
    }

	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
            PSPRenderer::Get()->FlushTris();
    }
}
#endif

//*****************************************************************************
//
//*****************************************************************************
void RSP_MoveMem_Conker( MicroCodeCommand command )
{
	u32 type = command.inst.cmd0 & 0xFE;
	u32 address = RDPSegAddr(command.inst.cmd1);

	if( type == G_GBI2_MV_MATRIX )
	{
		gConkerVtxZAddr = address;
	}
	else if( type == G_GBI2_MV_LIGHT )
	{
		u32 offset2 = (command.inst.cmd0 >> 5) & 0x3FFF;
		u32 light = 0xFF;

		if( offset2 >= 0x30 )
		{
			light = (offset2 - 0x30)/0x30;
			DL_PF("    Light %d:", light);

			RDP_MoveMemLight(light, address);
		}
		else
		{
			// fix me
			//DBGConsole_Msg(0, "Check me in DLParser_MoveMem_Conker - MoveMem Light");
		}
	}
	else
	{
		DLParser_GBI2_MoveMem( command );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_MoveWord_Conker( MicroCodeCommand command )
{
	u32 type = (command.inst.cmd0 >> 16) & 0xFF;

	if( type != G_MW_NUMLIGHT )
	{
		DLParser_GBI2_MoveWord( command );
	}
	else
	{
		u32 num_lights = command.inst.cmd1 / 48;
		DL_PF("     G_MW_NUMLIGHT: %d", num_lights);

		gAmbientLightIdx = num_lights + 1;
		PSPRenderer::Get()->SetNumLights(num_lights);
	}
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_Vtx_Conker( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.inst.cmd1);
	u32 len    = ((command.inst.cmd0      )& 0xFFF) >> 1;
	u32 n      = ((command.inst.cmd0 >> 12)& 0xFFF);
	u32 v0		= len - n;

	DL_PF("    Vtx: address 0x%08x, len: %d, v0: %d, n: %d", address, len, v0, n);

	PSPRenderer::Get()->SetNewVertexInfoConker( address, v0, n );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
      gNumVertices += n;
      DLParser_DumpVtxInfo( address, v0, n );
#endif

}

// Perfect Dark
//*****************************************************************************
//
//*****************************************************************************
void RSP_Set_Vtx_CI_PD( MicroCodeCommand command )
{
	// Color index buf address
	PDCIAddr = RDPSegAddr(command.inst.cmd1);
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_Vtx_PD( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.inst.cmd1);
	u32 v0 =  ((command.inst.cmd0)>>16)&0x0F;
	u32 n  = (((command.inst.cmd0)>>20)&0x0F)+1;
	u32 len = (command.inst.cmd0)&0xFFFF;

	use(len);

	DL_PF("    Vtx: address 0x%08x, len: %d, v0: %d, n: %d", address, len, v0, n);

	// Doesn't work anyways
	// Todo : Implement proper vertex info for PD
	PSPRenderer::Get()->SetNewVertexInfoPD( address, v0, n );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
      gNumVertices += n;
      DLParser_DumpVtxInfo( address, v0, n );
#endif
}

//*****************************************************************************
//
//*****************************************************************************
/*
void RSP_Tri4_PD( MicroCodeCommand command )
{
	// While the next command pair is Tri2, add vertices
	u32 pc = gDlistStack[gDlistStackPointer].pc;

	bool tris_added = false;

	do {
		for( u32 i=0; i<4; i++)
		{
			u32 v0_idx = (command.inst.cmd1>>(4+(i<<3))) & 0xF;
			u32 v1_idx = (command.inst.cmd1>>(  (i<<3))) & 0xF;
			u32 v2_idx = (command.inst.cmd0>>(  (i<<2))) & 0xF;
			tris_added |= PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx);
		}

		command.inst.cmd0 = *(u32 *)(g_pu8RamBase + pc + 0);
		command.inst.cmd1 = *(u32 *)(g_pu8RamBase + pc + 4);
		pc += 8;

	} while (command.inst.cmd == G_GBI2_TRI2);

	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
            PSPRenderer::Get()->FlushTris();
    }
}
*/
//*****************************************************************************
//
//*****************************************************************************
/*
void DLParser_GBI0_Line3D_SOTE( MicroCodeCommand command )
{
    u32 pc = gDisplayListStack.back().addr;
    u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

    bool tris_added = false;

    while ( command.inst.cmd == G_GBI1_LINE3D )
    {
        u32 v0_idx = ((command.inst.cmd1 >> 24) & 0xFF) / 5;
		u32 v1_idx = ((command.inst.cmd1 >> 16) & 0xFF) / 5;
		u32 v2_idx = ((command.inst.cmd1 >> 8) & 0xFF) / 5;

        tris_added |= PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx);

		u32 v3_idx = ((command.inst.cmd1 >> 24) & 0xFF) / 5;
		u32 v4_idx = ((command.inst.cmd1 >> 8) & 0xFF) / 5;
		u32 v5_idx = (command.inst.cmd1 & 0xFF) / 5;

		tris_added |= PSPRenderer::Get()->AddTri(v3_idx, v4_idx, v5_idx);

		command.inst.cmd0			= *(u32 *)(g_pu8RamBase + pc+0);
		command.inst.cmd1			= *(u32 *)(g_pu8RamBase + pc+4);
		pc += 8;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
        if ( command.inst.cmd == G_GBI1_LINE3D )
        {
                DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, gInstructionName[ command.inst.cmd ]);
        }
#endif			
    }
    gDisplayListStack.back().addr = pc-8;

    if (tris_added)
    {
            PSPRenderer::Get()->FlushTris();
    }
}
*/
//*****************************************************************************
//
//*****************************************************************************
/*
void DLParser_GBI0_Tri1_SOTE( MicroCodeCommand command )
{
    u32 pc = gDisplayListStack.back().addr;
    u32 * pCmdBase = (u32 *)( g_pu8RamBase + pc );

    bool tris_added = false;

    while (command.inst.cmd == G_GBI1_TRI1)
    {

        u32 v0_idx = ((command.inst.cmd1 >> 16) & 0xFF) / 5;
        u32 v1_idx = ((command.inst.cmd1 >> 8) & 0xFF) / 5;
        u32 v2_idx = (command.inst.cmd1 & 0xFF) / 5;

        tris_added |= PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx);

		command.inst.cmd0			= *(u32 *)(g_pu8RamBase + pc+0);
		command.inst.cmd1			= *(u32 *)(g_pu8RamBase + pc+4);
		pc += 8;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
        if ( command.inst.cmd == G_GBI1_TRI1 )
        {
//				DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, gInstructionName[ command.inst.cmd ]);
        }
#endif
    }

    gDisplayListStack.back().addr = pc-8;

    if (tris_added)
    {
		PSPRenderer::Get()->FlushTris();
	}
}
*/
