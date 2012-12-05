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

#ifndef DAEDALUS_IO_H_
#define DAEDALUS_IO_H_

#ifdef DAEDALUS_PSP
#include <pspiofilemgr.h>
#endif

namespace IO
{
	namespace File
	{
		bool		Move( const char * p_existing, const char * p_new );
		bool		Delete( const char * p_file );
		bool		Exists( const char * p_path );
#ifdef DAEDALUS_PSP
		int			Stat( const char *p_file, SceIoStat *stat );
#endif

	}
	namespace Directory
	{
		bool		Create( const char * p_path );
		bool		EnsureExists( const char * p_path );
		bool		IsDirectory( const char * p_path );
	}

	namespace Path
	{
		const u32	MAX_PATH_LEN = 260;

		char *				Combine( char * p_dest, const char * p_dir, const char * p_file );
		bool				Append( char * p_path, const char * p_more );
		const char *		FindExtension( const char * p_path );
		const char *		FindFileName( const char * p_path );
		char *				RemoveBackslash( char * p_path );
		bool				RemoveFileSpec( char * p_path );
		void				RemoveExtension( char * p_path );
		bool				AddExtension( char * p_path, const char * p_ext );
#ifdef DAEDALUS_PSP
		int					DeleteRecursive(const char* p_path, const char * p_extension);
#endif
	}

	struct FindDataT
	{
		char	Name[Path::MAX_PATH_LEN+1];
	};

#if defined( DAEDALUS_PSP )
	typedef SceUID FindHandleT;
#elif defined( DAEDALUS_W32 )
	typedef intptr_t FindHandleT;
#elif defined( DAEDALUS_OSX )
	typedef void * FindHandleT;
#else
#error Need to define FindHandleT for this platform
#endif

	bool	FindFileOpen( const char * path, FindHandleT * handle, FindDataT & data );
	bool	FindFileNext( FindHandleT handle, FindDataT & data );
	bool	FindFileClose( FindHandleT handle );
}

#endif

