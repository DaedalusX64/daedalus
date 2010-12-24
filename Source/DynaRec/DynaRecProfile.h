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

#ifndef __DAEDALUS_DYNARECPROFILE_H__
#define __DAEDALUS_DYNARECPROFILE_H__

//#define DAEDALUS_ENABLE_DYNAREC_PROFILE

class CFragment;

#ifdef DAEDALUS_ENABLE_DYNAREC_PROFILE
namespace DynarecProfile
{
	void LogLookup( u32 address, CFragment * fragment );
	void LogEnterExit( u32 enter_address, u32 exit_address, u32 instruction_count );
}

#define DYNAREC_PROFILE_LOGLOOKUP( a, f )					DynarecProfile::LogLookup( a, f )
#define DYNAREC_PROFILE_ENTEREXIT( enter, exit, cnt )		DynarecProfile::LogEnterExit( enter, exit, cnt )

#else

#define DYNAREC_PROFILE_LOGLOOKUP( a, f )
#define DYNAREC_PROFILE_ENTEREXIT( enter, exit, cnt )

#endif

#endif // __DAEDALUS_DYNARECPROFILE_H__
