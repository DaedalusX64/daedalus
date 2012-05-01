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

#ifndef UCODE_DKR_H__
#define UCODE_DKR_H__

u32 gDKRCMatrixIndex = 0;
u32 gDKRMatrixAddr = 0;
u32 gDKRAddr = 0;
u32 gDKRVtxCount = 0;
bool gDKRBillBoard = false;

// DKR verts are extra 4 bytes
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************
void DLParser_DumpVtxInfoDKR(u32 address, u32 v0_idx, u32 num_verts)
{
	if (gDisplayListFile != NULL)
	{
		u32 psSrc = (u32)(g_pu8RamBase + address);

		for ( u32 idx = v0_idx; idx < v0_idx + num_verts; idx++ )
		{
			f32 x = *(s16*)((psSrc + 0) ^ 2);
			f32 y = *(s16*)((psSrc + 2) ^ 2);
			f32 z = *(s16*)((psSrc + 4) ^ 2);

			//u16 wFlags = PSPRenderer::Get()->GetVtxFlags( idx ); //(u16)psSrc[3^0x1];

			u8 a = *(u8*)((psSrc + 6) ^ 3);	//R
			u8 b = *(u8*)((psSrc + 7) ^ 3);	//G
			u8 c = *(u8*)((psSrc + 8) ^ 3);	//B
			u8 d = *(u8*)((psSrc + 9) ^ 3);	//A

			const v4 & t = PSPRenderer::Get()->GetTransformedVtxPos( idx );
			const v4 & p = PSPRenderer::Get()->GetProjectedVtxPos( idx );

			DL_PF("    #%02d Pos:{% 0.1f,% 0.1f,% 0.1f}->{% 0.1f,% 0.1f,% 0.1f} Proj:{% 6f,% 6f,% 6f,% 6f} RGBA:{%02x%02x%02x%02x}",
				idx, x, y, z, t.x, t.y, t.z, p.x/p.w, p.y/p.w, p.z/p.w, p.w, a, b, c, d );

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
	u32 address		= command.inst.cmd1 + gDKRAddr;
	u32 num_verts   = ((command.inst.cmd0 >> 19) & 0x1F);

	// Increase by one num verts for DKR
	if( g_ROM.GameHacks == DKR )
		num_verts++;

	if( command.inst.cmd0 & 0x00010000 )
	{
		if( gDKRBillBoard )
			gDKRVtxCount = 1;
	}
	else
	{
		gDKRVtxCount = 0;
	}

	u32 v0_idx = ((command.inst.cmd0 >> 9) & 0x1F) + gDKRVtxCount;

	DL_PF("    Address[0x%08x] v0[%d] Num[%d]", address, v0_idx, num_verts);

	DAEDALUS_ASSERT( v0_idx < 32, "DKR : v0 out of bound! %d" );

	PSPRenderer::Get()->SetNewVertexInfoDKR(address, v0_idx, num_verts);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gNumVertices += num_verts;
	DLParser_DumpVtxInfoDKR(address, v0_idx, num_verts);
#endif

}

//*****************************************************************************
//
//*****************************************************************************
// BB2k
// DKR
//00229B70: 07020010 000DEFC8 CMD G_DLINMEM  Displaylist at 800DEFC8 (stackp 1, limit 2)
//00229A58: 06000000 800DE520 CMD G_GBI1_DL  Displaylist at 800DE520 (stackp 1, limit 0)
//00229B90: 07070038 00225850 CMD G_DLINMEM  Displaylist at 80225850 (stackp 1, limit 7)

void DLParser_DLInMem( MicroCodeCommand command )
{
	gDlistStackPointer++;
	gDlistStack[gDlistStackPointer].pc = command.inst.cmd1;
	gDlistStack[gDlistStackPointer].countdown = (command.inst.cmd0 >> 16) & 0xFF;

	DL_PF("    Address=0x%08x %s", command.inst.cmd1, (command.dlist.param==G_DL_NOPUSH)? "Jump" : (command.dlist.param==G_DL_PUSH)? "Push" : "?");
	DL_PF("    \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
	DL_PF("    ############################################");
}

//*****************************************************************************
//
//*****************************************************************************
/*
00229C28: 01400040 002327C0 CMD G_MTX  {Matrix} at 802327C0 ind 1  Load:Mod 
00229BB8: 01400040 00232740 CMD G_MTX  {Matrix} at 80232740 ind 1  Load:Mod 
00229BF0: 01400040 00232780 CMD G_MTX  {Matrix} at 80232780 ind 1  Load:Mod 
00229B28: 01000040 002326C0 CMD G_MTX  {Matrix} at 802326C0  Mul:Mod 
00229B78: 01400040 00232700 CMD G_MTX  {Matrix} at 80232700  Mul:Mod 
*/

// 0x80 seems to be mul
// 0x40 load

void DLParser_Mtx_DKR( MicroCodeCommand command )
{	
	u32 address		= command.inst.cmd1 + RDPSegAddr(gDKRMatrixAddr);
	u32 mtx_command = (command.inst.cmd0 >> 16) & 0xF;
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

	if( mul )
	{
		Matrix4x4 & mat( *PSPRenderer::Get()->DKRMtxPtr( 4 ) );	//Borrow unused matrix from stack which is aligned to 16bytes
		PSPRenderer::Get()->MatrixFromN64FixedPoint( mat, address );
	#ifdef DAEDALUS_PSP_USE_VFPU
		matrixMultiplyAligned( PSPRenderer::Get()->DKRMtxPtr( mtx_command ), &mat, PSPRenderer::Get()->DKRMtxPtr( 0 ) );
	#else
		*PSPRenderer::Get()->DKRMtxPtr( mtx_command ) = mat * *PSPRenderer::Get()->DKRMtxPtr( 0 );
	#endif
	}
	else
	{
		PSPRenderer::Get()->MatrixFromN64FixedPoint( *PSPRenderer::Get()->DKRMtxPtr( mtx_command ), address );
	}

	gDKRCMatrixIndex = mtx_command;

	PSPRenderer::Get()->Mtxchanged();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	const Matrix4x4 & mtx( *PSPRenderer::Get()->DKRMtxPtr( mtx_command ) );
	DL_PF("    Mtx_DKR: Index %d %s Address 0x%08x\n"
			"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
			mtx_command, mul ? "Mul" : "Load", address,
			mtx.m[0][0], mtx.m[0][1], mtx.m[0][2], mtx.m[0][3],
			mtx.m[1][0], mtx.m[1][1], mtx.m[1][2], mtx.m[1][3],
			mtx.m[2][0], mtx.m[2][1], mtx.m[2][2], mtx.m[2][3],
			mtx.m[3][0], mtx.m[3][1], mtx.m[3][2], mtx.m[3][3]);
#endif
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
		DL_PF("    DKR BillBoard: %d", gDKRBillBoard);
		break;

	case G_MW_LIGHTCOL:
		gDKRCMatrixIndex = (command.inst.cmd1 >> 6) & 0x7;
		PSPRenderer::Get()->Mtxchanged();
		DL_PF("    DKR MtxIndx: %d", gDKRCMatrixIndex);
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
	gDKRAddr		= RDPSegAddr(command.inst.cmd1 & 0x00FFFFFF);
	gDKRVtxCount	= 0;
}

//*****************************************************************************
//
//*****************************************************************************
//DKR: 00229BA8: 05710080 001E4AF0 CMD G_DMATRI  Triangles 9 at 801E4AF0
void DLParser_DMA_Tri_DKR( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.inst.cmd1);
	u32 count = (command.inst.cmd0 >> 4) & 0xFFF;

	// Unlike normal tris ucodes this has the tris info in rdram
	TriDKR *tri = (TriDKR*)&g_pu32RamBase[ address >> 2 ];

	DAEDALUS_ASSERT( count < 16, "DKR to many triangles, indexing outside mVtxProjected array" );

	bool tris_added = false;

	for (u32 i = 0; i < count; i++)
	{
		u32 v0_idx = tri->v0;
		u32 v1_idx = tri->v1;
		u32 v2_idx = tri->v2;

		PSPRenderer::Get()->SetCullMode( !(tri->flag & 0x40), true );

		//if( info & 0x40000000 )
		//{	// no cull
		//	PSPRenderer::Get()->SetCullMode( false, false );
		//}
		//else
		//{
		//	// back culling
		//	PSPRenderer::Get()->SetCullMode( true, true );

		//	//if (RDP_View_Scales_X < 0)
		//	//{   // back culling
		//	//	PSPRenderer::Get()->SetCullMode( true, true );
		//	//}
		//	//else
		//	//{   // front cull
		//	//	PSPRenderer::Get()->SetCullMode( true, false );
		//	//}
		//}
	
		DL_PF("    Index[%d %d %d] Cull[%s] uv_TexCoord[%0.2f|%0.2f] [%0.2f|%0.2f] [%0.2f|%0.2f]",
			v0_idx, v1_idx, v2_idx, !(tri->flag & 0x40)? "On":"Off",
			(f32)tri->s0/32.0f, (f32)tri->t0/32.0f,
			(f32)tri->s1/32.0f, (f32)tri->t1/32.0f,
			(f32)tri->s2/32.0f, (f32)tri->t2/32.0f);

#if 1	//1->Fixes texture scaling, 0->Render as is and get some texture scaling errors
		//
		// This will create problem since some verts will get re-used and over-write new texture coords
		// To fix it we copy all verts to a new location where we can have individual texture coordinates //Corn
		PSPRenderer::Get()->CopyVtx( v0_idx, i*3+32);
		PSPRenderer::Get()->CopyVtx( v1_idx, i*3+33);
		PSPRenderer::Get()->CopyVtx( v2_idx, i*3+34);

		if( PSPRenderer::Get()->AddTri(i*3+32, i*3+33, i*3+34) )
		{
			tris_added = true;
			// Generate texture coordinates...
			PSPRenderer::Get()->SetVtxTextureCoord( i*3+32, tri->s0, tri->t0 );
			PSPRenderer::Get()->SetVtxTextureCoord( i*3+33, tri->s1, tri->t1 );
			PSPRenderer::Get()->SetVtxTextureCoord( i*3+34, tri->s2, tri->t2 );
		}
#else
		if( PSPRenderer::Get()->AddTri(v0_idx, v1_idx, v2_idx) )
		{
			tris_added = true;
			// Generate texture coordinates...
			PSPRenderer::Get()->SetVtxTextureCoord( v0_idx, tri->s0, tri->t0 );
			PSPRenderer::Get()->SetVtxTextureCoord( v1_idx, tri->s1, tri->t1 );
			PSPRenderer::Get()->SetVtxTextureCoord( v2_idx, tri->s2, tri->t2 );
		}
#endif
		tri++;
	}

	if(tris_added)	
	{
		PSPRenderer::Get()->FlushTris();
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
	
	DL_PF("    Level[%d] Tile[%d] %s", command.texture.level, tile, enable? "enable":"disable");

	PSPRenderer::Get()->SetTextureTile( tile);
	PSPRenderer::Get()->SetTextureEnable( enable);

	f32 scale_s = f32(command.texture.scaleS)  / (65535.0f * 32.0f);
	f32 scale_t = f32(command.texture.scaleT)  / (65535.0f * 32.0f);

	DL_PF("    ScaleS[%0.4f] ScaleT[%0.4f]", scale_s*32.0f, scale_t*32.0f);
	PSPRenderer::Get()->SetTextureScale( scale_s, scale_t );
}

#endif // UCODE_DKR_H__
