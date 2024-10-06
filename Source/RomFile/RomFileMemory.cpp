/*
Copyright (C) 2012 StrmnNrmn

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

#include "RomFile/RomFileMemory.h"
#include "Utility/MemoryHeap.h"

#include <stdlib.h>
#include <iostream> 

extern bool PSP_IS_SLIM;

CROMFileMemory::~CROMFileMemory() {}


class IROMFileMemory : public CROMFileMemory
{
public:
	IROMFileMemory();
	~IROMFileMemory();

//	virtual	bool	IsAvailable();
	virtual void *	Alloc( u32 size );
	virtual void	Free(void * ptr);

private:
#ifdef DAEDALUS_PSP
	CMemoryHeap *	mRomMemoryHeap;
#endif
};



template<> bool CSingleton< CROMFileMemory >::Create()
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT_Q(mpInstance == nullptr);
	#endif

	mpInstance = std::make_shared<IROMFileMemory>();
	return mpInstance != NULL;
}


IROMFileMemory::IROMFileMemory()
{
#ifdef DAEDALUS_PSP
	//
	// Allocate large memory heap for SLIM+ (32Mb) Used for ROM Buffer and ROM Cache
	// Otherwise allocate small memory heap for PHAT (2Mb) Used for ROM cache only
	//
	if( PSP_IS_SLIM )
	{
		mRomMemoryHeap = CMemoryHeap::Create( 16 * 1024 * 1024 );
	}
	else
	{
		mRomMemoryHeap = CMemoryHeap::Create( 4 * 1024 * 1024 );
	}
#endif
// #ifdef DAEDALUS_POSIX
// 	mRomMemoryHeap = CMemoryHeap::Create(21 * 1024 * 1024);
// #endif
}


IROMFileMemory::~IROMFileMemory()
{
#ifdef DAEDALUS_PSP
	delete mRomMemoryHeap;
#endif
}


/*
bool IROMFileMemory::IsAvailable()
{
	DAEDALUS_ASSERT( mRomMemoryHeap != NULL, "This heap isn't available" );

	return mRomMemoryHeap != NULL;
}
*/

void * IROMFileMemory::Alloc( u32 size )
{
	std::cout << "Allocating Memory" << std::endl;
#ifdef DAEDALUS_PSP
	return mRomMemoryHeap->Alloc( size );
#else
	return malloc( size );
#endif
}


void  IROMFileMemory::Free(void * ptr)
{
#ifdef DAEDALUS_PSP
	mRomMemoryHeap->Free( ptr );
#else
std::cout << "Freeing Memory" << std::endl;
	free( ptr );
#endif
}
