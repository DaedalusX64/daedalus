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

#include <stdio.h>
#include <stdlib.h>

#include "Utility/DaedalusTypes.h"

//#define DAEDALUS_LOG_ALLOCATIONS

//
//	In public releases, just use the standard memory allocators
//
#ifndef DAEDALUS_SILENT

#if defined( DAEDALUS_LOG_ALLOCATIONS ) || defined( _DEBUG )

#define SAVE_RA( ra )						\
		u32 ra {0};								\
		asm volatile						\
			 (								\
			 "sw $ra, %0\n"					\
			 : "+m"(ra) : : "memory"		\
			 )

#else

#define SAVE_RA( ra )		static const u32 ra( 0 )

#endif

//*****************************************************************************
//
//*****************************************************************************
namespace
{

FILE * GetMemLogFh()
{
	static FILE * gMemLogFh = NULL;
	if( gMemLogFh == NULL )
	{
		gMemLogFh = fopen( "memorylog.txt", "w" );
	}

	if( gMemLogFh != NULL )
	{
		return gMemLogFh;
	}

	return stdout;
}

}

//*****************************************************************************
//
//*****************************************************************************
void * operator new( size_t count )
{
	SAVE_RA( ra );

	void * p_mem( malloc( count ) );
	if(p_mem == NULL)
	{
		char msg[ 1024 ];
		sprintf( msg, "Out of memory (operator new(%d)) - RA is %08x", count, ra );
		DAEDALUS_ERROR( msg );
		printf( "%s\n", msg );
	}

#ifdef DAEDALUS_LOG_ALLOCATIONS
	fprintf( GetMemLogFh(), "Allocating   %8d bytes - %p - RA is %08x\n", count, p_mem, ra );
#endif
	return p_mem;
}

//*****************************************************************************
//
//*****************************************************************************
void * operator new[]( size_t count )
{
	SAVE_RA( ra );

	void * p_mem( malloc( count ) );
	if(p_mem == NULL)
	{
		char msg[ 1024 ];
		sprintf( msg, "Out of memory (operator new[](%d) - RA is %08x", count, ra );
		DAEDALUS_ERROR( msg );
		printf( "%s\n", msg );
	}

#ifdef DAEDALUS_LOG_ALLOCATIONS
	fprintf( GetMemLogFh(), "Allocating[] %8d bytes - %p - RA is %08x\n", count, p_mem, ra );
#endif
	return p_mem;
}

//*****************************************************************************
//
//*****************************************************************************
void operator delete[]( void * p_mem )
{
	#ifdef DAEDALUS_LOG_ALLOCATIONS
	SAVE_RA( ra );
	#endif

	if( p_mem != NULL )
	{
	#ifdef DAEDALUS_LOG_ALLOCATIONS
		fprintf( GetMemLogFh(), "Freeing[] %p - RA is %08x\n", p_mem, ra );
	#endif
		free( p_mem );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void operator delete( void * p_mem )
{
	#ifdef DAEDALUS_LOG_ALLOCATIONS
	SAVE_RA( ra );
	#endif

	if( p_mem != NULL )
	{
	#ifdef DAEDALUS_LOG_ALLOCATIONS
		fprintf( GetMemLogFh(), "Freeing   %p - RA is %08x\n", p_mem, ra );
	#endif
		free( p_mem );
	}
}

#endif //DAEDALUS_SILENT
