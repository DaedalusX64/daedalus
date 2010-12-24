/*
Copyright (C) 2005 StrmnNrmn

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


#ifndef VIDEOMEMORYMANAGER_H_
#define VIDEOMEMORYMANAGER_H_

#include "Utility/Singleton.h"

class CVideoMemoryManager : public CSingleton< CVideoMemoryManager >
{
public:
	virtual ~CVideoMemoryManager();

	virtual bool			Alloc( u32 size, void ** data, bool * isvidmem ) = 0;
	virtual void			Free( void * ptr ) = 0;
#ifdef DAEDALUS_DEBUG_MEMORY
	virtual void			DisplayDebugInfo() = 0;
#endif
};


#endif // VIDEOMEMORYMANAGER_H_
