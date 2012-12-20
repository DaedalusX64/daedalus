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

#pragma once

struct TLBEntry
{
public:
	u32 pagemask, hi, pfne, pfno;
	u32 mask, g;

private:
	// For speed/convenience
	u32		mask2;			// Mask, Mask/2
	u32		vpnmask, vpn2mask;	// Vpn Mask, VPN/2 Mask
	u32		pfnohi, pfnehi;		// Even/Odd highbits
	u32		checkbit;

	u32		addrcheck;			// vpn2 & vpnmask

	static bool FindTLBEntry( u32 address, u32 * p_idx );

public:
	void UpdateValue(u32 _pagemask, u32 _hi, u32 _pfne, u32 _pfno);
	void Reset();
	static u32 Translate(u32 address, bool& missing);
};

ALIGNED_EXTERN(TLBEntry, g_TLBs[32], CACHE_ALIGN);
