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

#ifndef DAEDALUS_DEBUGDISPLAYLIST_H_
#define DAEDALUS_DEBUGDISPLAYLIST_H_

//*************************************************************************************
//
//*************************************************************************************
#ifdef DAEDALUS_DEBUG_DISPLAYLIST

extern FILE * gDisplayListFile;

#define DL_PF( ... )									\
{														\
	if( gDisplayListFile != NULL )						\
	{													\
		fprintf( gDisplayListFile, __VA_ARGS__ );		\
		fputs( "\n", gDisplayListFile );				\
	}													\
}

#else

#define DL_PF(...)		do {} while(0)

#endif



//*************************************************************************************
//	Provide some special assert macros to allow display list debugging
//*************************************************************************************
#if defined(DAEDALUS_DEBUG_DISPLAYLIST) && defined( DAEDALUS_ENABLE_ASSERTS )

extern bool gDebugDisplayList;

//
//	Assert a condition is valid
//
#define DAEDALUS_DL_ASSERT( e, ... )										\
{																			\
	static bool ignore = false;												\
	if ( !(e) && !ignore )													\
	{																		\
		DL_PF( __VA_ARGS__ );												\
		EAssertResult i = DaedalusAssert( #e,  __FILE__, __LINE__, __VA_ARGS__  );	\
		if ( i == AR_BREAK )												\
		{																	\
			gDebugDisplayList = true;										\
		}																	\
		else if ( i == AR_IGNORE )											\
		{																	\
			ignore = true;	/* Ignore throughout session */					\
		}																	\
	}																		\
}

//
// Use this to assert unconditionally - e.g. for unhandled cases
//
#define DAEDALUS_DL_ERROR( ... )											\
{																			\
	static bool ignore = false;												\
	if ( !ignore )															\
	{																		\
		DL_PF( __VA_ARGS__ );												\
		EAssertResult i = DaedalusAssert( "", __FILE__, __LINE__, __VA_ARGS__ );		\
		if ( i == AR_BREAK )												\
		{																	\
			gDebugDisplayList = true;										\
		}																	\
		else if ( i == AR_IGNORE )											\
		{																	\
			ignore = true;	/* Ignore throughout session */					\
		}																	\
	}																		\
}

#else

#define DAEDALUS_DL_ASSERT( e, ... )		DAEDALUS_ASSERT( e, __VA_ARGS__ )
#define DAEDALUS_DL_ERROR( ... )			DAEDALUS_ERROR( __VA_ARGS__ )

#endif

#endif // DAEDALUS_DEBUGDISPLAYLIST_H_
