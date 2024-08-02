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

#ifndef UTILITY_IO_H_
#define UTILITY_IO_H_

#include "Base/Types.h"

#include <filesystem>

namespace IO
{
	namespace Path
	{
		const u32	kMaxPathLen = 260;

		#ifdef DAEDALUS_BATCH_TEST
		const char *		FindExtension( const char * p_path );
		const char *		FindFileName( const char * p_path );
		#endif
	}

	typedef char Filename[IO::Path::kMaxPathLen+1];

	#ifdef DAEDALUS_CTR
	struct FindDataT
	{
		Filename	Name;
	};

// This is also pretty redundant, as the IO file is pretty much deprecated
	using FindHandleT = void *;

	bool	FindFileOpen( const char * path, FindHandleT * handle, FindDataT & data );
	bool	FindFileNext( FindHandleT handle, FindDataT & data );
	bool	FindFileClose( FindHandleT handle );
	#endif
}

#endif // UTILITY_IO_H_
