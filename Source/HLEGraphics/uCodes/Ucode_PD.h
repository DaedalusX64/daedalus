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

#ifndef UCODE_PD_H__
#define UCODE_PD_H__

//*****************************************************************************
//
//*****************************************************************************
void DLParser_Vtx_PD( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.inst.cmd1);
	u32 v0 =  ((command.inst.cmd0)>>16)&0x0F;
	u32 n  = (((command.inst.cmd0)>>20)&0x0F)+1;
	u32 len = (command.inst.cmd0)&0xFFFF;

	use(len);

	DL_PF("    Address[0x%08x] Len[%d] v0[%d] Num[%d]", address, len, v0, n);

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
void DLParser_Set_Vtx_CI_PD( MicroCodeCommand command )
{
	// PD Color index buf address
	gAuxAddr = (u32)g_pu8RamBase + RDPSegAddr(command.inst.cmd1);
}

//*****************************************************************************
//
//*****************************************************************************
/*
void DLParser_Tri4_PD( MicroCodeCommand command )
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

#endif // UCODE_PD_H__
