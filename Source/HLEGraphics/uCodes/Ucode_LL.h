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

#ifndef UCODE_LL_H__
#define UCODE_LL_H__

//*****************************************************************************

//IS called Last Legion, but is used for several other games like: Dark Rift, Toukon Road, Toukon Road 2.

//*****************************************************************************
// Only thing I can't figure out why are the characters on those games invisble?
// Dark Rift runs properly without custom microcodes, and has the same symptoms...
// We need Turbo3D ucode support, actually a modified version of it, thanks Gonetz for the info :D

void DLParser_RSP_Last_Legion_0x80( MicroCodeCommand command )
{     
      gDlistStack[gDlistStackPointer].pc += 16;
	  DL_PF("DLParser_RSP_Last_Legion_0x80");
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_RSP_Last_Legion_0x00( MicroCodeCommand command )
{

	gDlistStack[gDlistStackPointer].pc += 16;
	DL_PF("DLParser_RSP_Last_Legion_0x00");

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
			gDlistStack[gDlistStackPointer].pc = pc1;
			gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;
		}

		if( pc2 && pc2 != 0xffffff && pc2 < MAX_RAM_ADDRESS )
		{
			// Need to call both DL
			gDlistStackPointer++;
			gDlistStack[gDlistStackPointer].pc = pc2;
			gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;
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

	// Note : these are in a different order than normal texrect!
	//
	tex_rect.cmd2 = command3.inst.cmd1;
	tex_rect.cmd3 = command2.inst.cmd1;

	v2 d( tex_rect.dsdx / 1024.0f, tex_rect.dtdy / 1024.0f );
	v2 xy0( tex_rect.x0 / 4.0f, tex_rect.y0 / 4.0f );
	v2 xy1;
	v2 uv0( tex_rect.s / 32.0f, tex_rect.t / 32.0f );
	v2 uv1;

	//
	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1.0f to the w/h)
	//
	switch ( gRDPOtherMode.cycle_type )
	{
		case CYCLE_COPY:
			d.x *= 0.25f;	// In copy mode 4 pixels are copied at once.
		case CYCLE_FILL:
			xy1.x = (tex_rect.x1 + 4) * 0.25f;
			xy1.y = (tex_rect.y1 + 4) * 0.25f;
			break;
		default:
			xy1.x = tex_rect.x1 * 0.25f;
			xy1.y = tex_rect.y1 * 0.25f;
			break;
	}

	uv1.x = uv0.x + d.x * ( xy1.x - xy0.x );
	uv1.y = uv0.y + d.y * ( xy1.y - xy0.y );

	PSPRenderer::Get()->TexRect( tex_rect.tile_idx, xy0, xy1, uv0, uv1 );
}

#endif // UCODE_LL_H__
