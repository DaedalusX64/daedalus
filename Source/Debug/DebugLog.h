/*
Copyright (C) 2001 StrmnNrmn

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

#pragma once

#ifndef DEBUG_DEBUGLOG_H_
#define DEBUG_DEBUGLOG_H_

#ifdef DAEDALUS_LOG

#include <stdarg.h>

#include "Base/Types.h"

enum EDebugFlags
{
	DEBUG_PI =				 1<<1,
	DEBUG_MI =				 1<<2,
	DEBUG_VI =				 1<<3,

	DEBUG_MEMORY =			 1<<4,
	DEBUG_TLB =				 1<<5,

	DEBUG_INTR =			 1<<6,

	DEBUG_REGS =			 1<<7,

	DEBUG_MEMORY_RDRAM_REG = 1<<8,
	DEBUG_MEMORY_SP_IMEM =	 1<<9,
	DEBUG_MEMORY_SP_REG =	 1<<10,
	DEBUG_MEMORY_MI =		 1<<11,
	DEBUG_MEMORY_VI =		 1<<12,
	DEBUG_MEMORY_AI =		 1<<13,
	DEBUG_MEMORY_RI =		 1<<14,
	DEBUG_MEMORY_SI =		 1<<15,
	DEBUG_MEMORY_PI =		 1<<16,
	DEBUG_MEMORY_PIF =		 1<<17,
	DEBUG_MEMORY_DP =		 1<<18,

	DEBUG_DYNREC =			 1<<19,
	DEBUG_DYNAREC_CACHE =	 1<<20,
	DEBUG_DYNAREC_PROF =	 1<<21,

	DEBUG_FRAME =			 1<<22,
};

//
//	Define a union of the above flags to determine what is logged.
//	The compiler is clever enough to optimise away calls to DPF
//	if a particular flag isn't set.
//
static const u32	DAED_DEBUG_MASK( 0 );

bool		Debug_InitLogging();
void		Debug_FinishLogging();
bool		Debug_GetLoggingEnabled();
void		Debug_SetLoggingEnabled( bool enabled );
void		Debug_Print( const char * format, ... );

#define DAED_CHECK_LOG( flags )			DAED_DEBUG_MASK & (flags)
#define DAED_LOG( flags, ... )		if( DAED_DEBUG_MASK & (flags) ) Debug_Print( __VA_ARGS__ )

#else

#define DAED_CHECK_LOG( flags )			false
#define DAED_LOG( flags, msg, ... )

#endif

#define DPF( flags, ... )		DAED_LOG( flags, __VA_ARGS__ )


#endif // DEBUG_DEBUGLOG_H_


