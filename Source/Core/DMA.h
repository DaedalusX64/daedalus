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

#ifndef DMA_H_
#define DMA_H_


void DMA_PI_CopyToRDRAM();
void DMA_PI_CopyFromRDRAM();

void DMA_SP_CopyToRDRAM();
void DMA_SP_CopyFromRDRAM();

void DMA_SI_CopyFromDRAM();
void DMA_SI_CopyToDRAM();

bool DMA_HandleTransfer( u8 * p_dst, u32 dst_offset, u32 dst_size, const u8 * p_src, u32 src_offset, u32 src_size, u32 length );
bool DMA_FLASH_CopyToDRAM(u32 dest, u32 StartOffset, u32 len);
bool DMA_FLASH_CopyFromDRAM(u32 dest, u32 len);

extern u32 s_nNumDmaTransfers;			// Incremented on every Cart->RDRAM Xfer
extern u32 s_nTotalDmaTransferSize;		// Total size of every Cart->RDRAM Xfer
extern u32 s_nNumSPTransfers;			// Incremented on every RDRAM->SPMem Xfer
extern u32 s_nTotalSPTransferSize;		// Total size of every RDRAM->SPMem Xfer

extern bool gDMAUsed;


#endif	// DMA_H_
