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
// Trix implementation borrowed from GlideN64
//*****************************************************************************
void DLParser_Trix_GE( MicroCodeCommand command )
{
	// While the next command pair is TriX, add vertices
	u32 pc = gDlistStack.address[gDlistStackPointer];
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

	bool tris_added = false;
	do
	{
		//DL_PF("    0x%08x: %08x %08x Flag: 0x%02x %-10s", pc-8, command.inst.cmd0, command.inst.cmd1, (command.inst.cmd0 >> 16) & 0xFF, "G_GBI1_TRI4");
		u32 cmd0 = command.inst.cmd0;
		u32 cmd1 = command.inst.cmd1;
		while(cmd1 != 0) 
		{
			u32 v0 = cmd1 & 0xf;
			cmd1 >>= 4;

			u32 v1 = cmd1 & 0xf;
			cmd1 >>= 4;

			u32 v2 = cmd0 & 0xf;
			cmd0 >>= 4;

			tris_added |= gRenderer->AddTri(v0, v1, v2);
		}
		command.inst.cmd0 = *pCmdBase++;
		command.inst.cmd1 = *pCmdBase++;
		pc += 8;
	} while ( command.inst.cmd == G_GE_TriX );

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
	if ( (command.inst.cmd1 >> 24) != G_RDP_TRI_SHADE_TXTR )
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
