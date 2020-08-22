/*

Copyright (C) 2001 NAME

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

#ifndef CORE_DMA_H_
#define CORE_DMA_H_

#include "Base/Types.h"

void DMA_PI_CopyToRDRAM();
void DMA_PI_CopyFromRDRAM();

void DMA_SP_CopyToRDRAM();
void DMA_SP_CopyFromRDRAM();

void DMA_SI_CopyFromDRAM();
void DMA_SI_CopyToDRAM();

bool DMA_HandleTransfer( u8 * p_dst, u32 dst_offset, u32 dst_size, const u8 * p_src, u32 src_offset, u32 src_size, u32 length );
bool DMA_FLASH_CopyToDRAM(u32 dest, u32 StartOffset, u32 len);
bool DMA_FLASH_CopyFromDRAM(u32 dest, u32 len);

extern bool gDMAUsed;


#endif // CORE_DMA_H_
