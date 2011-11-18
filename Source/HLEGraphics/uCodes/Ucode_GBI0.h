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

#ifndef UCODE0_H__
#define UCODE0_H__

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI0_Vtx( MicroCodeCommand command )
{
    u32 addr = RDPSegAddr(command.vtx0.addr);
    u32 v0   = command.vtx0.v0;
    u32 n    = command.vtx0.n + 1;

    DL_PF("    Address[0x%08x] v0[%d] Num[%d] Len[0x%04x]", addr, v0, n, command.vtx0.len);

	// Never seen or should happen, but just in case..
	DAEDALUS_ASSERT( (v0 + n) < 80, "Warning, attempting to load into invalid vertex positions");
	DAEDALUS_ASSERT( (addr + (n*16)) < MAX_RAM_ADDRESS, "Address out of range (0x%08x)", addr );

	PSPRenderer::Get()->SetNewVertexInfo( addr, v0, n );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gNumVertices += n;
	DLParser_DumpVtxInfo( addr, v0, n );
#endif

}
//*****************************************************************************
//
//*****************************************************************************
// GE and PD have alot of empty tris...
// Maybe not the best idea to inline nor have an extra jump or and extra branch in Addtri..
inline bool AddTri4( u32 v0, u32 v1, u32 v2 )
{
	//if( (v0 == 0) && (v1 == 0) && (v2 == 0) ) 
	if( (v0 == v1) && (v1 == v2) )
	{
		DL_PF("    Tri: %d,%d,%d (Culled -> Empty?)", v0, v1, v2);
		return false;
	}
	else
	{
		return PSPRenderer::Get()->AddTri(v0, v1, v2);
	}
}

//*****************************************************************************
// It's used by Golden Eye and Perfect Dark
//*****************************************************************************
void DLParser_GBI0_Tri4( MicroCodeCommand command )
{
    // While the next command pair is Tri2, add vertices
	u32 pc = gDlistStack[gDlistStackPointer].pc;

    bool tris_added = false;

    do{
		//DL_PF("    0x%08x: %08x %08x Flag: 0x%02x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, (command.inst.cmd0 >> 16) & 0xFF, "G_GBI1_TRI4");

		//Tri #1
		u32 v0 = command.tri4.v0;
		u32 v1 = command.tri4.v1;
		u32 v2 = command.tri4.v2;

		tris_added |= AddTri4(v0, v1, v2);

		//Tri #2
		u32 v3 = command.tri4.v3;
		u32 v4 = command.tri4.v4;
		u32 v5 = command.tri4.v5;

		tris_added |= AddTri4(v3, v4, v5);

		//Tri #3
		u32 v6 = command.tri4.v6;
		u32 v7 = command.tri4.v7;
		u32 v8 = command.tri4.v8;

		tris_added |= AddTri4(v6, v7, v8);

		//Tri #4
		u32 v9  = command.tri4.v9;
		u32 v10 = command.tri4.v10;
		u32 v11 = command.tri4.v11;

		tris_added |= AddTri4(v9, v10, v11);

		command.inst.cmd0 = *(u32 *)(g_pu8RamBase + pc+0);
		command.inst.cmd1 = *(u32 *)(g_pu8RamBase + pc+4);
		pc += 8;
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
/*
void DLParser_GBI0_Quad( MicroCodeCommand command ) 
{
	DAEDALUS_ERROR("GBI0_Quad : Not supported in ucode0 ? ( Ignored )");
}
*/

#endif // UCODE1_H__
