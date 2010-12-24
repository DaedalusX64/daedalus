/*
Copyright (C) 2007 StrmnNrmn

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
#include "DynaRecProfile.h"

#include "Debug/DebugLog.h"

#include "Core/ROM.h"

#include <map>
#include <vector>
#include <algorithm>

#ifdef DAEDALUS_ENABLE_DYNAREC_PROFILE

namespace DynarecProfile
{

//*************************************************************************************
//
//*************************************************************************************
static std::map<u32,u32>		gFrameLookups;
static u32						gLastFrame;

extern std::map< u32, u32 >		gHotTraceCountMap;


namespace
{
	struct SFragmentCount
	{
		SFragmentCount( u32 count, u32 address )
			:	Count( count )
			,	Address( address )
		{
		}

		u32			Count;
		u32			Address;
	};

	struct SortDecreasingSize
	{
		bool	operator()( const SFragmentCount & a, const SFragmentCount & b ) const
		{
			return a.Count > b.Count;
		}
	};
}

void	CheckForNewFrame()
{
	if( gLastFrame != g_dwNumFrames )
	{
		std::vector< SFragmentCount >		LookupList;
		for(std::map<u32, u32>::const_iterator it = gFrameLookups.begin(); it != gFrameLookups.end(); ++it)
		{
			LookupList.push_back( SFragmentCount( it->second, it->first ) );
		}

		std::sort( LookupList.begin(), LookupList.end(), SortDecreasingSize() );



		for(u32 i = 0; i < LookupList.size(); ++i)
		{
				DAED_LOG( DEBUG_DYNAREC_PROF, "%08x: %d lookups", LookupList[ i ].Address, LookupList[ i ].Count );
		}
		gFrameLookups.clear();
		gLastFrame = g_dwNumFrames;
	}

}

void	LogLookup( u32 address, CFragment * fragment )
{
	CheckForNewFrame();

	DAED_LOG( DEBUG_DYNAREC_CACHE, "LookupFragment( %08x ) -> %s", address, fragment ? "found" : "-----" );
	gFrameLookups[ address ]++;

}


void	LogEnterExit( u32 enter_address, u32 exit_address, u32 instruction_count )
{
	CheckForNewFrame();

	DAED_LOG( DEBUG_DYNAREC_CACHE, "Enter/Exit: %08x -> %08x (executed %d instructions)", enter_address, exit_address, instruction_count );
}



}

#endif
