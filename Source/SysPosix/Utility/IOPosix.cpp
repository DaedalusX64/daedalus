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
#include "System/IO.h"
#include <dirent.h>
#include <filesystem>


#ifdef DAEDALUS_CTR
const std::filesystem::path baseDir = std::filesystem::current_path() / "3ds" / "DaedalusX64";
#else
const std::filesystem::path baseDir = std::filesystem::current_path();
#endif


namespace IO
{




	bool	FindFileOpen( const char * path, FindHandleT * handle, FindDataT & data )
	{
		DIR * d = opendir( path );
		if( d != NULL )
		{
			// To support findfirstfile() API we must return the first result immediately
			if( FindFileNext( d, data ) )
			{
				*handle = d;
				return true;
			}

			// Clean up
			closedir( d );
		}

		return false;
	}

	bool	FindFileNext( FindHandleT handle, FindDataT & data )
	{
		DAEDALUS_ASSERT( handle != NULL, "Cannot search with invalid directory handle" );

		while (dirent * ep = readdir( static_cast< DIR * >( handle ) ) )
		{
			// Ignore hidden files (and '.' and '..')
			if (ep->d_name[0] == '.')
				continue;

			IO::Path::Assign( data.Name, ep->d_name );
			return true;
		}

		return false;
	}

	bool	FindFileClose( FindHandleT handle )
	{
		DAEDALUS_ASSERT( handle != NULL, "Trying to close an invalid directory handle" );

		return closedir( static_cast< DIR * >( handle ) ) >= 0;
	}
}
