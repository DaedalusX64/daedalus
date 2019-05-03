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

#ifndef HLEGRAPHICS_UCODES_UCODE_DKR_H_
#define HLEGRAPHICS_UCODES_UCODE_DKR_H_

u32 gDKRMatrixAddr = 0;
u32 gDKRVtxCount = 0;
bool gDKRBillBoard = false;

// DKR verts are 10 bytes
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************
void DLParser_DumpVtxInfoDKR(u32 address, u32 v0_idx, u32 num_verts)
{
	if (DLDebug_IsActive())
	{
		u32 psSrc = (u32)(g_pu8RamBase + address);

		for ( u32 idx = v0_idx; idx < v0_idx + num_verts; idx++ )
		{
			f32 x = *(s16*)((psSrc + 0) ^ 2);
			f32 y = *(s16*)((psSrc + 2) ^ 2);
			f32 z = *(s16*)((psSrc + 4) ^ 2);

			//u16 wFlags = gRenderer->GetVtxFlags( idx ); //(u16)psSrc[3^0x1];

			u8 a = *(u8*)((psSrc + 6) ^ 3);	//R
			u8 b = *(u8*)((psSrc + 7) ^ 3);	//G
			u8 c = *(u8*)((psSrc + 8) ^ 3);	//B
			u8 d = *(u8*)((psSrc + 9) ^ 3);	//A

			const v4 & t = gRenderer->GetTransformedVtxPos( idx );
			const v4 & p = gRenderer->GetProjectedVtxPos( idx );
			#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			DL_PF("    #%02d Pos:{% 0.1f,% 0.1f,% 0.1f}->{% 0.1f,% 0.1f,% 0.1f} Proj:{% 6f,% 6f,% 6f,% 6f} RGBA:{%02x%02x%02x%02x}",
				idx, x, y, z, t.x, t.y, t.z, p.x/p.w, p.y/p.w, p.z/p.w, p.w, a, b, c, d );
				#endif

			psSrc+=10;
		}

		/*
		u16 * pwSrc = (u16 *)(g_pu8RamBase + address);
		i = 0;
		for( u32 idx = v0_idx; idx < v0_idx + num_verts; idx++ )
		{
			DL_PF(" #%02d %04x %04x %04x %04x %04x",
				idx, pwSrc[(i + 0) ^ 1],
				pwSrc[(i + 1) ^ 1],
				pwSrc[(i + 2) ^ 1],
				pwSrc[(i + 3) ^ 1],
				pwSrc[(i + 4) ^ 1]);

			i += 5;
		}
		*/

	}
}
#endif

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI0_Vtx_DKR( MicroCodeCommand command )
{
	u32 address		= command.inst.cmd1 + gAuxAddr;
	u32 num_verts   = ((command.inst.cmd0 >> 19) & 0x1F);
	u32 v0_idx		= 0;

	// Increase by one num verts for DKR
	if( g_ROM.GameHacks == DKR ) num_verts++;

	if( command.inst.cmd0 & 0x00010000 )
	{
		if( gDKRBillBoard )
			gDKRVtxCount = 1;
	}
	else
	{
		gDKRVtxCount = 0;
	}

	v0_idx = ((command.inst.cmd0 >> 9) & 0x1F) + gDKRVtxCount;
	#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DL_PF("    Address[0x%08x] v0[%d] Num[%d]", address, v0_idx, num_verts);
	#endif
	gRenderer->SetNewVertexInfoDKR(address, v0_idx, num_verts, gDKRBillBoard);

	gDKRVtxCount += num_verts;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gNumVertices += num_verts;
	DLParser_DumpVtxInfoDKR(address, v0_idx, num_verts);
#endif

}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_DLInMem( MicroCodeCommand command )
{
	gDlistStackPointer++;
	gDlistStack.address[gDlistStackPointer] = command.inst.cmd1;
	gDlistStack.limit = (command.inst.cmd0 >> 16) & 0xFF;
	#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DL_PF("    Address=0x%08x %s", command.inst.cmd1, (command.dlist.param==G_DL_NOPUSH)? "Jump" : (command.dlist.param==G_DL_PUSH)? "Push" : "?");
	DL_PF("    \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
	DL_PF("    ############################################");
	#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_Mtx_DKR( MicroCodeCommand command )
{
	u32 address		= command.inst.cmd1 + RDPSegAddr(gDKRMatrixAddr);
	u32 mtx_command = (command.inst.cmd0 >> 16) & 0x3;
	//u32 length      = (command.inst.cmd0      )& 0xFFFF;

	bool mul = false;

	if (mtx_command == 0)
	{
		//DKR : no mult
		mtx_command = (command.inst.cmd0 >> 22) & 0x3;
	}
	else
	{
		//JFG : mult but only if bit is set
		mul = ((command.inst.cmd0 >> 23) & 0x1);
	}

	// Load matrix from address
	gRenderer->SetDKRMat(address, mul, mtx_command);
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_MoveWord_DKR( MicroCodeCommand command )
{
	switch( command.inst.cmd0 & 0xFF )
	{
	case G_MW_NUMLIGHT:
		gDKRBillBoard = command.inst.cmd1 & 0x1;
		#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		DL_PF("    DKR BillBoard: %d", gDKRBillBoard);
		#endif
		break;

	case G_MW_LIGHTCOL:
		{
		u32 idx = (command.inst.cmd1 >> 6) & 0x3;
		gRenderer->DKRMtxChanged( idx );
		#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		DL_PF("    DKR MtxIdx: %d", idx);
		#endif
		}
		break;

	default:
		DLParser_GBI1_MoveWord( command );
		break;
	}
}
//*****************************************************************************
//
//*****************************************************************************
void DLParser_Set_Addr_DKR( MicroCodeCommand command )
{
	gDKRMatrixAddr  = command.inst.cmd0 & 0x00FFFFFF;
	gAuxAddr		= RDPSegAddr(command.inst.cmd1 & 0x00FFFFFF);
	gDKRVtxCount	= 0;
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_DMA_Tri_DKR( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.inst.cmd1);
	u32 count = (command.inst.cmd0 >> 4) & 0x1F;	//Count should never exceed 16

	TriDKR *tri = (TriDKR*)(g_pu8RamBase + address);

	bool tris_added = false;

	for (u32 i = 0; i < count; i++)
	{
		u32 v0_idx = tri->v0;
		u32 v1_idx = tri->v1;
		u32 v2_idx = tri->v2;

		gRenderer->SetCullMode( !(tri->flag & 0x40), true );

		//if( info & 0x40000000 )
		//{	// no cull
		//	gRenderer->SetCullMode( false, false );
		//}
		//else
		//{
		//	// back culling
		//	gRenderer->SetCullMode( true, true );

		//	//if (RDP_View_Scales_X < 0)
		//	//{   // back culling
		//	//	gRenderer->SetCullMode( true, true );
		//	//}
		//	//else
		//	//{   // front cull
		//	//	gRenderer->SetCullMode( true, false );
		//	//}
		//}
		#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		DL_PF("    Index[%d %d %d] Cull[%s] uv_TexCoord[%0.2f|%0.2f] [%0.2f|%0.2f] [%0.2f|%0.2f]",
			v0_idx, v1_idx, v2_idx, !(tri->flag & 0x40)? "On":"Off",
			(f32)tri->s0/32.0f, (f32)tri->t0/32.0f,
			(f32)tri->s1/32.0f, (f32)tri->t1/32.0f,
			(f32)tri->s2/32.0f, (f32)tri->t2/32.0f);
			#endif

#if 1	//1->Fixes texture scaling, 0->Render as is and get some texture scaling errors
		//
		// This will create problem since some verts will get re-used and over-write new texture coords before previous has been rendered
		// To fix it we copy all verts to a new location where we can have individual texture coordinates for each triangle//Corn
		const u32 new_v0_idx = i * 3 + 32;
		const u32 new_v1_idx = i * 3 + 33;
		const u32 new_v2_idx = i * 3 + 34;

		gRenderer->CopyVtx( v0_idx, new_v0_idx);
		gRenderer->CopyVtx( v1_idx, new_v1_idx);
		gRenderer->CopyVtx( v2_idx, new_v2_idx);

		if( gRenderer->AddTri(new_v0_idx, new_v1_idx, new_v2_idx) )
		{
			tris_added = true;
			// Generate texture coordinates...
			gRenderer->SetVtxTextureCoord( new_v0_idx, tri->s0, tri->t0 );
			gRenderer->SetVtxTextureCoord( new_v1_idx, tri->s1, tri->t1 );
			gRenderer->SetVtxTextureCoord( new_v2_idx, tri->s2, tri->t2 );
		}
#else
		if( gRenderer->AddTri(v0_idx, v1_idx, v2_idx) )
		{
			tris_added = true;
			// Generate texture coordinates...
			gRenderer->SetVtxTextureCoord( v0_idx, tri->s0, tri->t0 );
			gRenderer->SetVtxTextureCoord( v1_idx, tri->s1, tri->t1 );
			gRenderer->SetVtxTextureCoord( v2_idx, tri->s2, tri->t2 );
		}
#endif
		tri++;
	}

	if(tris_added)
	{
		gRenderer->FlushTris();
	}

	gDKRVtxCount = 0;
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Texture_DKR( MicroCodeCommand command )
{
	u32 tile    = command.texture.tile;

	// Seems to use 0x01
	// Force enable texture in DKR Ucode, fixes static texture bug etc
    bool enable = true;
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DL_PF("    Level[%d] Tile[%d] %s", command.texture.level, tile, enable? "enable":"disable");
#endif
	gRenderer->SetTextureTile( tile);
	gRenderer->SetTextureEnable( enable);

	f32 scale_s = f32(command.texture.scaleS)  / (65535.0f * 32.0f);
	f32 scale_t = f32(command.texture.scaleT)  / (65535.0f * 32.0f);
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DL_PF("    ScaleS[%0.4f] ScaleT[%0.4f]", scale_s*32.0f, scale_t*32.0f);
	#endif
	gRenderer->SetTextureScale( scale_s, scale_t );
}

#endif // HLEGRAPHICS_UCODES_UCODE_DKR_H_
