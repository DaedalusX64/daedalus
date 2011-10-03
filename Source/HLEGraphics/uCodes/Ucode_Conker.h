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

#ifndef UCODE_CONKER_H__
#define UCODE_CONKER_H__


//*****************************************************************************
//
//*****************************************************************************
void RSP_Vtx_Conker( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.inst.cmd1);
	u32 len    = ((command.inst.cmd0      )& 0xFFF) >> 1;
	u32 n      = ((command.inst.cmd0 >> 12)& 0xFFF);
	u32 v0		= len - n;

	DL_PF("    Vtx: address 0x%08x, len: %d, v0: %d, n: %d", address, len, v0, n);

	PSPRenderer::Get()->SetNewVertexInfoConker( address, v0, n );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
      gNumVertices += n;
      DLParser_DumpVtxInfo( address, v0, n );
#endif

}

//*****************************************************************************
//
//*****************************************************************************
#if 1	//1->Struct, 0->Old
void DLParser_GBI2_Conker( MicroCodeCommand command )
{
	u32 pc = gDlistStack[gDlistStackPointer].pc;		// This points to the next instruction

    bool tris_added = false;

	do{	//Tri #1
		tris_added |= PSPRenderer::Get()->AddTri(command.conkertri4.v0, command.conkertri4.v1, command.conkertri4.v2);

		//Tri #2
		tris_added |= PSPRenderer::Get()->AddTri(command.conkertri4.v3, command.conkertri4.v4, command.conkertri4.v5);

		//Tri #3
		tris_added |= PSPRenderer::Get()->AddTri(command.conkertri4.v6, command.conkertri4.v7, command.conkertri4.v8);

		//Tri #4
		tris_added |= PSPRenderer::Get()->AddTri((command.conkertri4.v9hi << 2) | command.conkertri4.v9lo, command.conkertri4.v10, command.conkertri4.v11);

		command.inst.cmd0			= *(u32 *)(g_pu8RamBase + pc+0);
		command.inst.cmd1			= *(u32 *)(g_pu8RamBase + pc+4);
		pc += 8;
    }while ( command.conkertri4.cmd == 1 );

	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
            PSPRenderer::Get()->FlushTris();
    }
}

#else
void DLParser_GBI2_Conker( MicroCodeCommand command )
{
	u32 pc = gDlistStack[gDlistStackPointer].pc;		// This points to the next instruction

    bool tris_added = false;

	while ( (command.inst.cmd > 0x0F) && (command.inst.cmd < 0x20) )
    {
		u32 idx[12];

		//Tri #1
		idx[0] = (command.inst.cmd1   )&0x1F;
		idx[1] = (command.inst.cmd1>> 5)&0x1F;
		idx[2] = (command.inst.cmd1>>10)&0x1F;

		tris_added |= PSPRenderer::Get()->AddTri(idx[0], idx[1], idx[2]);

		//Tri #2
		idx[3] = (command.inst.cmd1>>15)&0x1F;
		idx[4] = (command.inst.cmd1>>20)&0x1F;
		idx[5] = (command.inst.cmd1>>25)&0x1F;

		tris_added |= PSPRenderer::Get()->AddTri(idx[3], idx[4], idx[5]);

		//Tri #3
		idx[6] = (command.inst.cmd0    )&0x1F;
		idx[7] = (command.inst.cmd0>> 5)&0x1F;
		idx[8] = (command.inst.cmd0>>10)&0x1F;

		tris_added |= PSPRenderer::Get()->AddTri(idx[6], idx[7], idx[8]);

		//Tri #4
		idx[ 9] = (((command.inst.cmd0>>15)&0x7)<<2)|(command.inst.cmd1>>30);
		idx[10] = (command.inst.cmd0>>18)&0x1F;
		idx[11] = (command.inst.cmd0>>23)&0x1F;

		tris_added |= PSPRenderer::Get()->AddTri(idx[9], idx[10], idx[11]);

		command.inst.cmd0			= *(u32 *)(g_pu8RamBase + pc+0);
		command.inst.cmd1			= *(u32 *)(g_pu8RamBase + pc+4);
		pc += 8;
    }

	gDlistStack[gDlistStackPointer].pc = pc-8;

    if (tris_added)
    {
            PSPRenderer::Get()->FlushTris();
    }
}
#endif

//*****************************************************************************
//
//*****************************************************************************
void RSP_MoveMem_Conker( MicroCodeCommand command )
{
	u32 type = command.inst.cmd0 & 0xFE;
	u32 address = RDPSegAddr(command.inst.cmd1);

	switch ( type )
	{
	case G_GBI2_MV_MATRIX:	//Get address to Light Normals
		{
			gAuxAddr = (u32)g_pu8RamBase + address;		//Conker VtxZ address	
		}
		break;
	case G_GBI2_MV_LIGHT:
		{
			u32 offset2 = (command.inst.cmd0 >> 5) & 0x3FFF;

			if( offset2 >= 0x30 )
			{
				u32 light = (offset2 - 0x30)/0x30;
				DL_PF("    Light %d:", light);
				RDP_MoveMemLight(light, address);
			}
			else
			{
				// fix me
				//DBGConsole_Msg(0, "Check me in DLParser_MoveMem_Conker - MoveMem Light");
			}
		}
		break;
	default:
		DLParser_GBI2_MoveMem( command );
		break;
	}
}


//*****************************************************************************
//
//*****************************************************************************
//f32 gCoord_Mod[16];

void RSP_MoveWord_Conker( MicroCodeCommand command )
{
#if 1
	switch (command.mw2.type)
	{
	case G_MW_NUMLIGHT:
		{
			u32 num_lights = command.inst.cmd1 / 48;
			DL_PF("     G_MW_NUMLIGHT: %d", num_lights);

			gAmbientLightIdx = num_lights;
			PSPRenderer::Get()->SetNumLights(num_lights);
		}
		break;
		
	case G_MW_SEGMENT:
		{
			u32 segment = command.mw2.offset >> 2;
			u32 address	= command.mw2.value;

			DL_PF( "      G_MW_SEGMENT Segment[%d] = 0x%08x", segment, address );

			gSegments[segment] = address;
		}
		break;
		
	/*
	case G_MW_CLIP:
		//if (offset == 0x04)
		//{
		//	rdp.clip_ratio = sqrt((float)rdp.cmd1);
		//	rdp.update |= UPDATE_VIEWPORT;
		//}
		DL_PF("     G_MoveWord_Conker: CLIP");
		break;
		
	case G_MW_FOG:
		//rdp.fog_multiplier = (short)(rdp.cmd1 >> 16);
		//rdp.fog_offset = (short)(rdp.cmd1 & 0x0000FFFF);
		DL_PF("     G_MoveWord_Conker: Fog");
		break;
		
	case G_MW_POINTS:
		DL_PF("     G_MoveWord_Conker: forcemtx");
		break;
		
	case G_MW_PERSPNORM:
		DL_PF("     G_MoveWord_Conker: perspnorm");
		break;
		
	case 0x10:  // moveword coord mod
		{
			DL_PF("     G_MoveWord_Conker: coord mod");

			if ( command.inst.cmd0 & 8 ) return;

			u32 idx = (command.inst.cmd0 >> 1) & 3;
			u32 pos = command.inst.cmd0 & 0x30;

			if (pos == 0)
			{
				gCoord_Mod[0+idx] = (s16)(command.inst.cmd1 >> 16);
				gCoord_Mod[1+idx] = (s16)(command.inst.cmd1 & 0xFFFF);
			}
			else if (pos == 0x10)
			{
				gCoord_Mod[4+idx] = (command.inst.cmd1 >> 16) / 65536.0f;
				gCoord_Mod[5+idx] = (command.inst.cmd1 & 0xFFFF) / 65536.0f;
				gCoord_Mod[12+idx] = gCoord_Mod[0+idx] + gCoord_Mod[4+idx];
				gCoord_Mod[13+idx] = gCoord_Mod[1+idx] + gCoord_Mod[5+idx];
				
			}
			else if (pos == 0x20)
			{
				gCoord_Mod[8+idx] = (s16)(command.inst.cmd1 >> 16);
				gCoord_Mod[9+idx] = (s16)(command.inst.cmd1 & 0xFFFF);
			}
		}
		break;
		*/
	default:
		DL_PF("     G_MoveWord_Conker: Unknown");
  }

#else
	u32 type = (command.inst.cmd0 >> 16) & 0xFF;

	if( type != G_MW_NUMLIGHT )
	{
		DLParser_GBI2_MoveWord( command );
	}
	else
	{
		u32 num_lights = command.inst.cmd1 / 48;
		DL_PF("     G_MW_NUMLIGHT: %d", num_lights);

		gAmbientLightIdx = num_lights + 1;
		PSPRenderer::Get()->SetNumLights(num_lights);
	}
#endif
}



#endif // UCODE_CONKER_H__
