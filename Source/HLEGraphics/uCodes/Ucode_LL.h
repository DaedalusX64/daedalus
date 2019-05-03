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

#ifndef HLEGRAPHICS_UCODES_UCODE_LL_H_
#define HLEGRAPHICS_UCODES_UCODE_LL_H_

//*****************************************************************************

//IS called Last Legion, but is used for several other games like: Dark Rift, Toukon Road, Toukon Road 2.

//*****************************************************************************
// Only thing I can't figure out why are the characters on those games invisble?
// Dark Rift runs properly without custom microcodes, and has the same symptoms...
// We need Turbo3D ucode support, actually a modified version of it, thanks Gonetz for the info :D

void DLParser_Last_Legion_0x80( MicroCodeCommand command )
{
     gDlistStack.address[gDlistStackPointer] += 16;
     			#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	  DL_PF("    DLParser_RSP_Last_Legion_0x80");
    #endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_Last_Legion_0x00( MicroCodeCommand command )
{

	gDlistStack.address[gDlistStackPointer] += 16;
  			#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DL_PF("    DLParser_RSP_Last_Legion_0x00");
#endif
	if( (command.inst.cmd0) == 0 && (command.inst.cmd1) )
	{
		u32 newaddr = RDPSegAddr((command.inst.cmd1));
		if( newaddr >= MAX_RAM_ADDRESS )
		{
			DLParser_PopDL();
			return;
		}

		u32 pc1 = *(u32 *)(g_pu8RamBase + newaddr+8*1+4);
		u32 pc2 = *(u32 *)(g_pu8RamBase + newaddr+8*4+4);
		pc1 = RDPSegAddr(pc1);
		pc2 = RDPSegAddr(pc2);

		if( pc1 && pc1 != 0xffffff && pc1 < MAX_RAM_ADDRESS)
		{
			// Need to call both DL
			gDlistStackPointer++;
			gDlistStack.address[gDlistStackPointer] = pc1;
			#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			DL_PF("    Address=0x%08x", pc1);
			DL_PF("    \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
			DL_PF("    ############################################");
      #endif
		}

		if( pc2 && pc2 != 0xffffff && pc2 < MAX_RAM_ADDRESS )
		{
			// Need to call both DL
			gDlistStackPointer++;
			gDlistStack.address[gDlistStackPointer] = pc2;
			#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			DL_PF("    Address=0x%08x", pc2);
			DL_PF("    \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
			DL_PF("    ############################################");
      #endif
		}
	}
	else if( (command.inst.cmd1) == 0 )
	{
		DLParser_PopDL();
	}
	else
	{
		DLParser_Nothing( command );
		DLParser_PopDL();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_TexRect_Last_Legion( MicroCodeCommand command )
{
	MicroCodeCommand command2;
	MicroCodeCommand command3;

	//
	// Fetch the next two instructions
	//
	DLParser_FetchNextCommand( &command2 );
	DLParser_FetchNextCommand( &command3 );

	RDP_TexRect tex_rect;
	tex_rect.cmd0 = command.inst.cmd0;
	tex_rect.cmd1 = command.inst.cmd1;

	// Note : The only difference with normal texrect is that these are in a different order!
	tex_rect.cmd2 = command3.inst.cmd1;
	tex_rect.cmd3 = command2.inst.cmd1;

	//Keep integers for as long as possible //Corn
	s32 rect_x0 = tex_rect.x0;
	s32 rect_y0 = tex_rect.y0;
	s32 rect_x1 = tex_rect.x1;
	s32 rect_y1 = tex_rect.y1;

	s16 rect_s0 = tex_rect.s;
	s16 rect_t0 = tex_rect.t;

	s32 rect_dsdx = tex_rect.dsdx;
	s32 rect_dtdy = tex_rect.dtdy;

	//rect_s0 += (((u32)rect_dsdx >> 31) << 5);	//Fixes California Speed
	//rect_t0 += (((u32)rect_dtdy >> 31) << 5);

	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1<<2 to the w/h)
	//
	switch ( gRDPOtherMode.cycle_type )
	{
		case CYCLE_COPY:
			rect_dsdx = rect_dsdx >> 2;	// In copy mode 4 pixels are copied at once.
			// NB! Fall through!
		case CYCLE_FILL:
			rect_x1 += 4;
			rect_y1 += 4;
			break;
		default:
			break;
	}

	s16 rect_s1 = rect_s0 + (rect_dsdx * ( rect_x1 - rect_x0 ) >> 7);
	s16 rect_t1 = rect_t0 + (rect_dtdy * ( rect_y1 - rect_y0 ) >> 7);

	TexCoord st0( rect_s0, rect_t0 );
	TexCoord st1( rect_s1, rect_t1 );
	v2 xy0( tex_rect.x0 / 4.0f, tex_rect.y0 / 4.0f );
	v2 xy1( tex_rect.x1 / 4.0f, tex_rect.y1 / 4.0f );

	gRenderer->TexRect( tex_rect.tile_idx, xy0, xy1, st0, st1 );
}

#endif // HLEGRAPHICS_UCODES_UCODE_LL_H_
