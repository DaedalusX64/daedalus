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

#include "stdafx.h"
#include "Utility/IO.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>

namespace IO
{
	const char kPathSeparator = '/';

	namespace File
	{
		bool	Move( const char * p_existing, const char * p_new )
		{
			return rename( p_existing, p_new ) >= 0;
		}

		bool	Delete( const char * p_file )
		{
			return remove( p_file ) == 0;
		}

		bool	Exists( const char * p_path )
		{
			FILE * fh = fopen( p_path, "rb" );
			if ( fh )
			{
				fclose( fh );
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	namespace Directory
	{
		bool	Create( const char * p_path )
		{
			return mkdir( p_path, 0777 ) == 0;
		}

		bool	EnsureExists( const char * p_path )
		{
			if ( IsDirectory(p_path) )
				return true;

			// Make sure parent exists,
			IO::Filename	p_path_parent;
			IO::Path::Assign( p_path_parent, p_path );
			IO::Path::RemoveBackslash( p_path_parent );
			if( IO::Path::RemoveFileSpec( p_path_parent ) )
			{
				//	Recursively create parents. Need to be careful of stack overflow
				if( !EnsureExists( p_path_parent ) )
					return false;
			}

			return Create( p_path );
		}

		bool	IsDirectory( const char * p_path )
		{
			struct stat		s;

			if(stat( p_path, &s ) == 0)
			{
				if(s.st_mode & S_IFDIR)
				{
					return true;
				}
			}

			return false;
		}
	}

	namespace Path
	{
		char *	Combine( char * p_dest, const char * p_dir, const char * p_file )
		{
			strcpy( p_dest, p_dir );
			Append( p_dest, p_file );
			return p_dest;
		}

		bool	Append( char * p_path, const char * p_more )
		{
			u32		len( strlen(p_path) );

			if(len > 0)
			{
				if(p_path[len-1] != kPathSeparator)
				{
					p_path[len] = kPathSeparator;
					p_path[len+1] = '\0';
					len++;
				}
			}
			strcat( p_path, p_more );
			return true;
		}

		const char *  FindExtension( const char * p_path )
		{
			return strrchr( p_path, '.' );
		}

		const char *	FindFileName( const char * p_path )
		{
			char * p_last_slash = strrchr( p_path, kPathSeparator );
			if ( p_last_slash )
			{
				return p_last_slash + 1;
			}
			else
			{
				return NULL;
			}
		}

		char *	RemoveBackslash( char * p_path )
		{
			u32 len = strlen( p_path );
			if ( len > 0 )
			{
				char * p_last_char( &p_path[ len - 1 ] );
				if ( *p_last_char == kPathSeparator )
				{
					*p_last_char = '\0';
				}
				return p_last_char;
			}
			return NULL;
		}

		bool	RemoveFileSpec( char * p_path )
		{
			char * last_slash = strrchr( p_path, kPathSeparator );
			if ( last_slash )
			{
				*last_slash = '\0';
				return true;
			}
			return false;
		}

		void	RemoveExtension( char * p_path )
		{
			char * ext = const_cast< char * >( FindExtension( p_path ) );
			if ( ext )
			{
				*ext = '\0';
			}
		}

		void	AddExtension( char * p_path, const char * p_ext )
		{
			strcat( p_path, p_ext );
		}
	}




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


