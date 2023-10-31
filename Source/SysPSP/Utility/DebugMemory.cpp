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


#include "Base/Types.h"

#include <stdio.h>
#include <stdlib.h>

#include "Base/Types.h"

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


#endif //DAEDALUS_SILENT
