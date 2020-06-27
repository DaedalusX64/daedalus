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

#ifndef HLEGRAPHICS_UCODES_UCODE_PD_H_
#define HLEGRAPHICS_UCODES_UCODE_PD_H_

//*****************************************************************************
//
//*****************************************************************************
void DLParser_Vtx_PD( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.inst.cmd1);
	u32 v0 =  ((command.inst.cmd0)>>16)&0x0F;
	u32 n  = (((command.inst.cmd0)>>20)&0x0F)+1;
	u32 len = (command.inst.cmd0)&0xFFFF;

	DL_PF("    Address[0x%08x] Len[%d] v0[%d] Num[%d]", address, len, v0, n);
	if (IsVertexInfoValid(address, 12, v0, n))
	{
		gRenderer->SetNewVertexInfoPD( address, v0, n );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
      	gNumVertices += n;
      	DLParser_DumpVtxInfo( address, v0, n );
#endif
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_Set_Vtx_CI_PD( MicroCodeCommand command )
{
	// PD Color index buf address
	gAuxAddr = RDPSegAddr(command.inst.cmd1);
}

#endif // HLEGRAPHICS_UCODES_UCODE_PD_H_
