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

#ifndef GSP_MACROS_H
#define GSP_MACROS_H


//*****************************************************************************
// Macros
//*****************************************************************************

#define G_ZELDA_ZBUFFER				G_ZBUFFER	// Guess 
#define G_ZELDA_CULL_BACK			0x00000200  /*G_CULL_FRONT */ 
#define G_ZELDA_CULL_FRONT			0x00000400  /*G_CULL_BACK  */
#define G_ZELDA_FOG					G_FOG 
#define G_ZELDA_LIGHTING			G_LIGHTING 
#define G_ZELDA_TEXTURE_GEN			G_TEXTURE_GEN 
#define G_ZELDA_TEXTURE_GEN_LINEAR	G_TEXTURE_GEN_LINEAR 
#define G_ZELDA_SHADING_SMOOTH		0x00200000

//***************************************************************************** 
//
//***************************************************************************** 

UcodeFunc( DLParser_GBI1_CullDL );
UcodeFunc( DLParser_GBI2_CullDL );
UcodeFunc( DLParser_GBI1_DL );
UcodeFunc( DLParser_GBI2_DL );
UcodeFunc( DLParser_GBI1_EndDL );
UcodeFunc( DLParser_GBI2_EndDL );
UcodeFunc( DLParser_GBI1_BranchZ );
UcodeFunc( DLParser_GBI1_LoadUCode );

UcodeFunc( DLParser_GBI1_SetGeometryMode );
UcodeFunc( DLParser_GBI1_ClearGeometryMode );
UcodeFunc( DLParser_GBI2_GeometryMode );
UcodeFunc( DLParser_GBI1_SetOtherModeL );
UcodeFunc( DLParser_GBI1_SetOtherModeH );
UcodeFunc( DLParser_GBI2_SetOtherModeL );
UcodeFunc( DLParser_GBI2_SetOtherModeH );

UcodeFunc( DLParser_GBI1_Texture );
UcodeFunc( DLParser_GBI2_Texture );

UcodeFunc( DLParser_GBI0_Vtx );
UcodeFunc( DLParser_GBI1_Vtx );
UcodeFunc( DLParser_GBI2_Vtx );
UcodeFunc( DLParser_GBI1_ModifyVtx );

UcodeFunc( DLParser_GBI1_Mtx );
UcodeFunc( DLParser_GBI2_Mtx );
UcodeFunc( DLParser_GBI1_PopMtx );
UcodeFunc( DLParser_GBI2_PopMtx );

UcodeFunc( DLParser_GBI0_Tri4 );
UcodeFunc( DLParser_GBI0_Quad );

UcodeFunc( DLParser_GBI2_Quad );
UcodeFunc( DLParser_GBI2_Line3D );
UcodeFunc( DLParser_GBI2_Tri1 );
UcodeFunc( DLParser_GBI2_Tri2 );

UcodeFunc( DLParser_GBI1_Tri1 );
UcodeFunc( DLParser_GBI1_Tri2 );
UcodeFunc( DLParser_GBI1_Line3D );

//*****************************************************************************
// New GBI2 ucodes
//*****************************************************************************
UcodeFunc( DLParser_GBI2_DL_Count );
//UcodeFunc( DLParser_GBI2_0x8 );



#endif /* GSP_MACROS_H */
