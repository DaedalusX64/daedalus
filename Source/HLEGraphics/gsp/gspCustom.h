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

#ifndef GSP_CUSTOM_H
#define GSP_CUSTOM_H

//*****************************************************************************
// Custom
//*****************************************************************************
UcodeFunc( DLParser_GBI0_DL_SOTE );
UcodeFunc( DLParser_GBI0_Vtx_SOTE );
//UcodeFunc( DLParser_GBI0_Line3D_SOTE );
//UcodeFunc( DLParser_GBI0_Tri1_SOTE );
UcodeFunc( DLParser_RSP_Last_Legion_0x80 );
UcodeFunc( DLParser_RSP_Last_Legion_0x00 );
UcodeFunc( DLParser_TexRect_Last_Legion );
UcodeFunc( DLParser_RDPHalf1_GoldenEye );
UcodeFunc( DLParser_DLInMem );
UcodeFunc( DLParser_Mtx_DKR );
UcodeFunc( DLParser_MoveWord_DKR );
UcodeFunc( DLParser_Set_Addr_DKR );
UcodeFunc( DLParser_GBI0_Vtx_DKR );
UcodeFunc( DLParser_GBI0_Vtx_WRUS );
UcodeFunc( DLParser_DMA_Tri_DKR );
UcodeFunc( DLParser_GBI0_Vtx_Gemini );
UcodeFunc( DLParser_GBI2_Conker );
UcodeFunc( RSP_MoveMem_Conker );
UcodeFunc( RSP_MoveWord_Conker );
UcodeFunc( RSP_Vtx_Conker );
UcodeFunc( RSP_Set_Vtx_CI_PD );
UcodeFunc( RSP_Vtx_PD );
//UcodeFunc( RSP_Tri4_PD );

//*****************************************************************************
//
//*****************************************************************************

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
void DLParser_DumpVtxInfoDKR(u32 address, u32 v0_idx, u32 num_verts);
#endif

#endif /* GSP_CUSTOM_H */
