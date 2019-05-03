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

#undef __GE_NOTHING
//*****************************************************************************
//
//*****************************************************************************


void DLParser_RDPHalf1_GoldenEye( MicroCodeCommand command )
{
	// Check for invalid address
	if ( (command.inst.cmd1)>>24 != 0xce )
		return;

	u32 pc = gDlistStack.address[gDlistStackPointer];		// This points to the next instruction
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
