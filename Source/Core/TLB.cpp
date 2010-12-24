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

#include "stdafx.h"

#include "TLB.h"
#include "CPU.h"
#include "Debug/DebugLog.h"
#include "Debug/DBGConsole.h"

#include "OSHLE/ultra_R4300.h"

ALIGNED_GLOBAL(TLBEntry, g_TLBs[32], CACHE_ALIGN);
//u32 TLBEntry::LastMatched = 0;

void TLBEntry::UpdateValue(u32 _pagemask, u32 _hi, u32 _pfno, u32 _pfne)
{
	// From the R4300i Instruction manual:
	// The G bit of the TLB is written with the logical AND of the G bits of the EntryLo0 and EntryLo1 regs
	// The TLB entry is loaded with the contents of the EntryHi and EntryLo regs.

	// TLB[INDEX] <- PageMask || (EntryHi AND NOT PageMask) || EntryLo1 || EntryLo0
	DPF( DEBUG_TLB, "PAGEMASK: 0x%08x ENTRYHI: 0x%08x. ENTRYLO1: 0x%08x. ENTRYLO0: 0x%08x", _pagemask, _hi, _pfno, _pfne);

	pagemask = _pagemask;
	hi = _hi;
	pfne = _pfne;
	pfno = _pfno;

	g = pfne & pfno & TLBLO_G;

	// Build the masks:
	mask     =  pagemask | (~TLBHI_VPN2MASK);
	mask2    =  mask>>1;
	vpnmask  = ~mask;
	vpn2mask =  vpnmask>>1;

	addrcheck= hi & vpnmask;

	pfnehi = ((pfne<<TLBLO_PFNSHIFT) & vpn2mask);
	pfnohi = ((pfno<<TLBLO_PFNSHIFT) & vpn2mask);

	switch (pagemask)
	{
	case TLBPGMASK_4K:	// 4k
		DPF(DEBUG_TLB, "       4k Pagesize");
		checkbit = 0x00001000;   // bit 12
		break;
	case TLBPGMASK_16K: //  16k pagesize
		DPF(DEBUG_TLB, "       16k Pagesize");
		checkbit = 0x00004000;   // bit 14
		break;
	case TLBPGMASK_64K: //  64k pagesize
		DPF(DEBUG_TLB, "       64k Pagesize");
		checkbit = 0x00010000;   // bit 16
		break;
	case TLBPGMASK_256K: // 256k pagesize
		DPF(DEBUG_TLB, "       256k Pagesize");
		checkbit = 0x00040000;   // bit 18
		break;
	case TLBPGMASK_1M: //   1M pagesize
		DPF(DEBUG_TLB, "       1M Pagesize");
		checkbit = 0x00100000;   // bit 20
		break;
	case TLBPGMASK_4M: //   4M pagesize
		DPF(DEBUG_TLB, "       4M Pagesize");
		checkbit = 0x00400000;   // bit 22
		break;
	case TLBPGMASK_16M: //  16M pagesize
		DPF(DEBUG_TLB, "       16M Pagesize");
		checkbit = 0x01000000;   // bit 24
		break;
	default: // should not happen!
		DPF(DEBUG_TLB, "       Unknown Pagesize");
		checkbit = 0;
		break;
	}
}

void TLBEntry::Reset()
{
	UpdateValue(0x00000000, 0x80000000, 0, 0);		
}

//*****************************************************************************
//
//*****************************************************************************
inline bool	TLBEntry::FindTLBEntry( u32 address, u32 * p_idx )
{
	static u32 i = 0;

	for ( u32 count = 0; count < 32; count++ )
	{
		// Hack to check most recently reference entry first
		// This gives some speedup if the matched address is near
		// the end of the tlb array (32 entries)
		i = (count + i) & 0x1F;

		struct TLBEntry & tlb = g_TLBs[i];

		// Check that the VPN numbers match
		if ((address & tlb.vpnmask) == tlb.addrcheck)
		{
			if (!tlb.g)
			{
				if ( (tlb.hi & TLBHI_PIDMASK) !=
					 (gCPUState.CPUControl[C0_ENTRYHI]._u32_0 & TLBHI_PIDMASK) )
				{
					// Entries ASID must match.
					continue;
				}
			}

			*p_idx = i;

			return true;
		}
	}

	return false;
}

//*****************************************************************************
//
//*****************************************************************************
u32 TLBEntry::Translate(u32 address, bool& missing)
{
	u32 iMatched;

	missing = !FindTLBEntry( address, &iMatched );
	if (!missing)
	{
		struct TLBEntry & tlb = g_TLBs[iMatched];

		bool	valid;
		//bool	dirty;	// Seems unused -Salvy
		u32		physical_addr;

		// Check for odd/even entry
		if (address & tlb.checkbit)
		{
			//dirty = (tlb.pfno & TLBLO_D) != 0;
			valid = (tlb.pfno & TLBLO_V) != 0;
			physical_addr = tlb.pfnohi | (address & tlb.mask2);
		}
		else
		{
			//dirty = (tlb.pfne & TLBLO_D) != 0;
			valid = (tlb.pfne & TLBLO_V) != 0;
			physical_addr = tlb.pfnehi | (address & tlb.mask2);
		}

		if ( valid )
		{
			return physical_addr;
		}
		else
		{
			// Throw TLB Invalid exception
			return 0;
		}
	}
	else
	{
		// TLBRefill

		// No valid TLB entry - throw TLB Refill Exception
		return 0;
	}
}
