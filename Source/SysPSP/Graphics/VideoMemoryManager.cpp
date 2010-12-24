// Taken from http://svn.ps2dev.org/filedetails.php?repname=psp&path=%2Ftrunk%2Fpspgl%2Fpspgl_vidmem.c&rev=0&sc=0

#include "stdafx.h"
#include "VideoMemoryManager.h"

#include "Utility/MemoryHeap.h"

#include <pspge.h>

extern bool PSP_IS_SLIM;

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
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IVideoMemoryManager();
	return mpInstance != NULL;
}

//*****************************************************************************
//
//*****************************************************************************
IVideoMemoryManager::IVideoMemoryManager()
:	mVideoMemoryHeap( CMemoryHeap::Create( MAKE_UNCACHED_PTR( sceGeEdramGetAddr() ), sceGeEdramGetSize() ) )
//,	mRamMemoryHeap( CMemoryHeap::Create( 1 * 1024 * 1024 ) )
{
	printf( "vram base: %p\n", sceGeEdramGetAddr() );
	printf( "vram size: %d KB\n", sceGeEdramGetSize() / 1024 );
	  	 
	if (PSP_IS_SLIM) 	 
		 mRamMemoryHeap = CMemoryHeap::Create( 4 * 1024 * 1024 ); 	 
	else 	 
		 mRamMemoryHeap = CMemoryHeap::Create( 1 * 1024 * 1024 );
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

	mem = mVideoMemoryHeap->Alloc( size );
	if( mem != NULL )
	{
		*data = mem;
		*isvidmem = true;
		return true;
	}

	DAEDALUS_ERROR( "Video Buffer Full (allocating of %d bytes RAM)", size );

	mem = mRamMemoryHeap->Alloc( size );
	if( mem != NULL )
	{
		*data = mem;
		*isvidmem = false;
		return true;
	}

	DAEDALUS_ERROR( "Video Buffer Full (allocation of %d bytes failed)", size );

	*data = NULL;
	*isvidmem = false;
	return false;
}

//*****************************************************************************
//
//*****************************************************************************
void  IVideoMemoryManager::Free(void * ptr)
{
	if( ptr == NULL )
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
	else
	{
		DAEDALUS_ERROR( "Memory is not from any of our heaps" );
	}
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