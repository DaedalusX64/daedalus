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

#ifndef GSP_COMMON_H
#define GSP_COMMON_H

#include "../stdafx.h"

#include "../DLParser.h"
#include "../PSPRenderer.h"
#include "../RDPStateManager.h"
#include "../DebugDisplayList.h"
#include "../TextureCache.h"
#include "../Microcode.h"
#include "../UcodeDefs.h"
#include "../Ucode.h"

#include "../Utility/Profiler.h"

#include "../Debug/Dump.h"
#include "../Debug/DBGConsole.h"

#include "../Core/ROM.h"
#include "../Core/CPU.h"

#include "../OSHLE/ultra_gbi.h"

#include "../ConfigOptions.h"



extern u32 gSegments[16];
const  u32 MAX_RAM_ADDRESS = (8*1024*1024);
extern u32 gVertexStride;
extern u32 gTextureLevel;
extern u32 gRDPHalf1;
extern u32 gAmbientLightIdx;

#define RDPSegAddr(seg) 		( (gSegments[((seg)>>24)&0x0F]&0x00ffffff) + ((seg)&0x00FFFFFF) )
//#define SetCommand( cmd, func ) 	gInstructionLookup[ cmd ] = func; gInstructionName[ cmd ] = #cmd;

#define RDP_NOIMPL_WARN(op)             DAEDALUS_DL_ERROR( op )
#define RDP_NOIMPL( op, cmd0, cmd1 )    DAEDALUS_DL_ERROR( "Not Implemented: %s 0x%08x 0x%08x", op, cmd0, cmd1 )

void MatrixFromN64FixedPoint( Matrix4x4 & mat, u32 address );
/*
void DLParser_PushDisplayList( const DList & dl );
void DLParser_CallDisplayList( const DList & dl );*/
void DLParser_PopDL();
void DLParser_InitMicrocode( u32 code_base, u32 code_size, u32 data_base, u32 data_size );
void RDP_MoveMemLight(u32 light_idx, u32 address);


#ifdef DAEDALUS_DEBUG_DISPLAYLIST
extern u32 gNumVertices;
extern u32 gNumDListsCulled;

void DLParser_DumpVtxInfo(u32 address, u32 v0_idx, u32 num_verts);
#endif

#include "gspMacros.h"
#include "gspSprite2D.h"
#include "gspS2DEX.h"
#include "gspCommon.h"

#endif /* GSP_COMMON_H */
