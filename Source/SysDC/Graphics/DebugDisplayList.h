/*
Copyright (C) 2001,2006 StrmnNrmn

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

#ifndef __DAEDALUS_DEBUGDISPLAYLIST_H__
#define __DAEDALUS_DEBUGDISPLAYLIST_H__


#include <stdarg.h>

//*************************************************************************************
// 
//*************************************************************************************
extern u32 gTesselatorVerticesIn;
extern u32 gTesselatorVerticesOut;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
extern FILE * gDisplayListFile;

inline void __cdecl DL_PF( const char * p_format, ... )
{
	if( gDisplayListFile != NULL )
	{
		va_list va;

		va_start(va, p_format);

		vfprintf( gDisplayListFile, p_format, va );
		fputs( "\n", gDisplayListFile );

		va_end(va);
	}
}

#else

#ifdef __GNUC__
	#define DL_PF(...) do {} while(0)
#else
	#if _MSC_VER >= 1300
		#define DL_PF __noop
	#else
		// avoid warning C4390: ';' : empty controlled statement found; is this the intent?
		#define DL_PF(x) do {} while(0)
		#pragma warning(disable : 4002) 
	#endif
#endif

#endif




#endif // __DAEDALUS_DEBUGDISPLAYLIST_H__
