/*
Copyright (C) 2006 StrmnNrmn

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


#include "Base/Types.h"


#include "DynaRec/DynaRecProfile.h"
#include "DynaRec/FragmentCache.h"
#include "DynaRec/Fragment.h"
#include "DynaRec/IndirectExitMap.h"
#include "Debug/DBGConsole.h"


//

CIndirectExitMap::CIndirectExitMap()
:	mpCache( nullptr )
{
}


//

CIndirectExitMap::~CIndirectExitMap()
{
}


//

CFragment *	CIndirectExitMap::LookupIndirectExit( u32 exit_address )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DAEDALUS_ASSERT( mpCache != nullptr, "Why do we have no cache?" );
	#endif
	CFragment * p( mpCache->LookupFragmentQ( exit_address ) );

	DYNAREC_PROFILE_LOGLOOKUP( exit_address, p );

	return p;
}


//

extern "C"
{

const void *	 IndirectExitMap_Lookup( CIndirectExitMap * p_map, u32 exit_address )
{
	CFragment *	p_fragment( p_map->LookupIndirectExit( exit_address ) );
	if( p_fragment != nullptr )
	{
		return p_fragment->GetEntryTarget().GetTarget();
	}

	return nullptr;
}

}
