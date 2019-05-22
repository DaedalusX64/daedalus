// Taken from http://svn.ps2dev.org/filedetails.php?repname=psp&path=%2Ftrunk%2Fpspgl%2Fpspgl_vidmem.c&rev=0&sc=0

#include "stdafx.h"
#include "VideoMemoryManager.h"

#include <stdio.h>

#include <pspge.h>

#include "Utility/VolatileMem.h"
#include "Utility/MemoryHeap.h"
#include "Math/MathUtil.h"

const u32 ERAM(3 * 512 * 1024);	//Amount of extra (volatile)RAM to use for textures in addition to VRAM //Corn
//*****************************************************************************
//
//*****************************************************************************
CVideoMemoryManager::~CVideoMemoryManager()
{
}

//*****************************************************************************
//
//*****************************************************************************
class IVideoMemoryManager : public CVideoMemoryManager
{
public:
	IVideoMemoryManager();
	~IVideoMemoryManager();

	virtual bool	Alloc( u32 size, void ** data, bool * isvidmem );
	virtual void	Free(void * ptr);
#ifdef DAEDALUS_DEBUG_MEMORY
	virtual void	DisplayDebugInfo();
#endif
private:
	CMemoryHeap *	mVideoMemoryHeap;
	CMemoryHeap *	mRamMemoryHeap;
};


//*****************************************************************************
//
//*****************************************************************************
template<> bool CSingleton< CVideoMemoryManager >::Create()
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT_Q(mpInstance == nullptr);
#endif
	mpInstance = new IVideoMemoryManager();
	return mpInstance != nullptr;
}

//*****************************************************************************
//
//*****************************************************************************
IVideoMemoryManager::IVideoMemoryManager()
:	mVideoMemoryHeap( CMemoryHeap::Create( MAKE_UNCACHED_PTR( sceGeEdramGetAddr() ), sceGeEdramGetSize() ) )
,	mRamMemoryHeap( CMemoryHeap::Create( MAKE_UNCACHED_PTR( (void*)(((u32)malloc_volatile(ERAM + 0xF) + 0xF) & ~0xF) ), ERAM ) )
//,	mRamMemoryHeap( CMemoryHeap::Create( 1 * 1024 * 1024 ) )
{
	printf( "vram base: %p\n", sceGeEdramGetAddr() );
	printf( "vram size: %d KB\n", sceGeEdramGetSize() / 1024 );
}

//*****************************************************************************
//
//*****************************************************************************
IVideoMemoryManager::~IVideoMemoryManager()
{
	delete mVideoMemoryHeap;
	delete mRamMemoryHeap;
}

//*****************************************************************************
//
//*****************************************************************************
bool IVideoMemoryManager::Alloc( u32 size, void ** data, bool * isvidmem )
{
	void * mem;

	// Ensure that all memory is 16-byte aligned
	size = AlignPow2( size, 16 );

	// Try to alloc fast VRAM
	mem = mVideoMemoryHeap->Alloc( size );
	if( mem != nullptr )
	{
		*data = mem;
		*isvidmem = true;
		return true;
	}
#ifdef DAEDALUS_DEBUG_CONSOLE
	DAEDALUS_ERROR( "Failed to allocate %d bytes of VRAM", size );
#endif
	// Try to alloc normal RAM
	mem = mRamMemoryHeap->Alloc( size );
	if( mem != nullptr )
	{
		*data = mem;
		*isvidmem = false;
		return true;
	}
#ifdef DAEDALUS_DEBUG_CONSOLE
	DAEDALUS_ERROR( "Failed to allocate %d bytes of RAM (risk for BSOD)", size );
#endif
	// It failed, there is no MEMORY left
	*data = nullptr;
	*isvidmem = false;
	return false;
}

//*****************************************************************************
//
//*****************************************************************************
void  IVideoMemoryManager::Free(void * ptr)
{
	if( ptr == nullptr )
	{

	}
	else if( mVideoMemoryHeap->IsFromHeap( ptr ) )
	{
		mVideoMemoryHeap->Free( ptr );
	}
	else if( mRamMemoryHeap->IsFromHeap( ptr ) )
	{
		mRamMemoryHeap->Free( ptr );
	}
	#ifdef DAEDALUS_DEBUG_CONSOLE
	else
	{
		DAEDALUS_ERROR( "Memory is not from any of our heaps" );
	}
	#endif
}

#ifdef DAEDALUS_DEBUG_MEMORY
//*****************************************************************************
//
//*****************************************************************************
void IVideoMemoryManager::DisplayDebugInfo()
{
	printf( "VRAM\n" );
	mVideoMemoryHeap->DisplayDebugInfo();

	printf( "RAM\n" );
	mRamMemoryHeap->DisplayDebugInfo();
}
#endif
