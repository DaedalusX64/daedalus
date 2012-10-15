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

#ifndef UCODE_SOTE_H__
#define UCODE_SOTE_H__

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI0_Vtx_SOTE( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.inst.cmd1);
	u32 n		= ((command.inst.cmd0 >> 4) & 0xfff) / 33 + 1;
	u32 v0		= 0;

	DL_PF("    Address[0x%08x] v0[%d] Num[%d]", address, v0, n);

	DAEDALUS_ASSERT( n < 32, "Warning, attempting to load into invalid vertex positions" );

	PSPRenderer::Get()->SetNewVertexInfo( address, v0, n );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gNumVertices += n;
	DLParser_DumpVtxInfo( address, v0, n );
#endif
}
//
// SOTE gets out of range, make sure to keep it in range otherwise will crash/gfx will b incorrect
//
//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI0_DL_SOTE( MicroCodeCommand command )
{
    u32 address = RDPSegAddr(command.dlist.addr) & (MAX_RAM_ADDRESS-1); // Keep addr in range..

	DAEDALUS_ASSERT( address < MAX_RAM_ADDRESS, "DL addr out of range (0x%08x)", address );

    DL_PF("    Address=0x%08x Push: 0x%02x", address, command.dlist.param);

	if( command.dlist.param == G_DL_PUSH )
		gDlistStackPointer++;

	gDlistStack.address[gDlistStackPointer] = address;
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetTImg_SOTE( MicroCodeCommand command )
{
	g_TI.Format		= command.img.fmt;
	g_TI.Size		= command.img.siz;
	g_TI.Width		= command.img.width + 1;
	g_TI.Address	= RDPSegAddr(command.img.addr) & (MAX_RAM_ADDRESS-1); // Keep addr in range..
	//g_TI.bpl		= g_TI.Width << g_TI.Size >> 1;
}

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



#endif // UCODE_SOTE_H__
