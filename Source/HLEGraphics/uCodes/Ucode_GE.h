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

#ifndef HLEGRAPHICS_UCODES_UCODE_GE_H_
#define HLEGRAPHICS_UCODES_UCODE_GE_H_

//*****************************************************************************
//
//*****************************************************************************
// GE and PD have alot of empty tris...
inline bool AddTri4( u32 v0, u32 v1, u32 v2 )
{
	if( v0 == v1 )
	{
		DL_PF("    Tris: v0:%d, v1:%d, v2:%d (Culled -> Empty)",v0, v1, v2);
		return false;
	}

	return gRenderer->AddTri(v0, v1, v2);
}

//*****************************************************************************
//
//*****************************************************************************
//FIX ME: See TriX implementation in GlideN64
void DLParser_Tri4_GE( MicroCodeCommand command )
{
	// While the next command pair is Tri4, add vertices
	u32 pc = gDlistStack.address[gDlistStackPointer];
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

	bool tris_added = false;
	do
	{
		//DL_PF("    0x%08x: %08x %08x Flag: 0x%02x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, (command.inst.cmd0 >> 16) & 0xFF, "G_GBI1_TRI4");

		//Tri #1
		u32 v0 = command.tri4.v0;
		u32 v1 = command.tri4.v1;
		u32 v2 = command.tri4.v2;

		tris_added |= gRenderer->AddTri(v0, v1, v2);

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

		command.inst.cmd0 = *pCmdBase++;
		command.inst.cmd1 = *pCmdBase++;
		pc += 8;
	} while ( command.inst.cmd == G_GE_Tri4 );

	gDlistStack.address[gDlistStackPointer] = pc-8;

	if (tris_added)
	{
		gRenderer->FlushTris();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_RDPHalf1_GE( MicroCodeCommand command )
{
	// Check for invalid address
	if ( (command.inst.cmd1)>>24 != 0xce )
		return;

	u32 pc = gDlistStack.address[gDlistStackPointer];		// This points to the next instruction
	u32 * Cmd = (u32 *)(g_pu8RamBase + pc);

	// Indices
	u32 a1 = *Cmd+8*0+4;
	u32 a3 = *Cmd+8*2+4;

	// Note : Color itself is handled elsewhere N.B Blendmode.cpp
	//
	// Coordinates, textures
	s32 x0 = s32(a3>>16)>>24;
	//s32 s0 = s32(a1&0xFFFF)/4;
	s32 y0 = 320*100;
	s32 t0 = s32(a1>>16)/4;

	// TIP : f32 x1 can be modified to render the sky differently.
	// Need to check on real hardware to tweak our sky correctly if needed.

	v2 xy0( (f32)x0, (f32)x0 );
	v2 xy1( (f32)y0, (f32)y0 );
	TexCoord uv0( 0.f, 0.f );
	TexCoord uv1( t0 / 40.0f, t0 / 40.0f );

	//DL_PF(" Word 1: %u, Word 2: %u, Word 3: %u, Word 4: %u, Word 5: %u, Word 6: %u, Word 7: %u, Word 8: %u, Word 9: %u", a1, a2, a3, a4, a5, a6, a7, a8, a9);
	//DL_PF("    Tile:%d Screen(%f,%f) -> (%f,%f)",				   tile, xy0, xy1, uv0, uv1);
	gRenderer->TexRect( 0, xy0, xy1, uv0, uv1 );

	gDlistStack.address[gDlistStackPointer] += 312;
}


#endif // HLEGRAPHICS_UCODES_UCODE_GE_H_
