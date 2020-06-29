/*
Copyright (C) 2020 StrmnNrmn

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

#ifndef HLEGRAPHICS_UCODES_UCODE_BETA_H_
#define HLEGRAPHICS_UCODES_UCODE_BETA_H_

// WRUS and SOTE use this early version of the Fast3D ucode

//*****************************************************************************
// The previous way of calculating was based on the assumption that
// there was no "n" field. I didn't realise that the n/length fields shared the
// lower 16 bits (in a 7:9 split).
// u32 length    = (command.inst.cmd0)&0xFFFF;
// u32 num_verts = (length + 1) / 0x210;					// 528
// u32 v0_idx    = ((command.inst.cmd0>>16)&0xFF)/VertexStride;	// /5
//*****************************************************************************
void DLParser_GBI0_Vtx_Beta( MicroCodeCommand command )
{
	u32 addr = RDPSegAddr(command.inst.cmd1);
	u32 v0  = ((command.inst.cmd0 >>16 ) & 0xff) / 5;
	u32 n   =  (command.inst.cmd0 >>9  ) & 0x7f;
	u32 len =  (command.inst.cmd0      ) & 0x1ff;

	DL_PF( "    Address[0x%08x] v0[%d] Num[%d] Len[0x%04x]", addr, v0, n, len );
	if (IsVertexInfoValid(addr, 16, v0, n))
	{
		gRenderer->SetNewVertexInfo( addr, v0, n );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		gNumVertices += n;
		DLParser_DumpVtxInfo( addr, v0, n );
#endif
	}
}

//*****************************************************************************

//*****************************************************************************
void DLParser_GBI0_Tri2_Beta( MicroCodeCommand command )
{
	DLParser_GBI1_Tri2_T< 5 >(command);
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI0_Tri1_Beta( MicroCodeCommand command )
{
	DLParser_GBI1_Tri1_T< 5 >(command);
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI0_Line3D_Beta( MicroCodeCommand command )
{
	DLParser_GBI1_Line3D_T< 5 >(command);
}

#endif // HLEGRAPHICS_UCODES_UCODE_BETA_H_
