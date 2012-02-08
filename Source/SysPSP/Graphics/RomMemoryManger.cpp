
#include "stdafx.h"
#include "RomMemoryManger.h"

#include "Utility/MemoryHeap.h"

extern bool PSP_IS_SLIM;
//*****************************************************************************
//
//*****************************************************************************
CRomMemoryManager::~CRomMemoryManager()
{
}

//*****************************************************************************
//
//*****************************************************************************
class IRomMemoryManager : public CRomMemoryManager
{
public:
	IRomMemoryManager();
	~IRomMemoryManager();

	virtual	bool	IsAvailable();
	virtual void *	Alloc( u32 size );
	virtual void	Free(void * ptr);

private:
	CMemoryHeap *	mRomMemoryHeap;
};


//*****************************************************************************
//
//*****************************************************************************
template<> bool CSingleton< CRomMemoryManager >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IRomMemoryManager();
	return mpInstance != NULL;
}

//*****************************************************************************
//
//*****************************************************************************
IRomMemoryManager::IRomMemoryManager()
{
	if(PSP_IS_SLIM)
	{
		mRomMemoryHeap = CMemoryHeap::Create( 32 * 1024 * 1024 );
	}
	else
	{
		mRomMemoryHeap = NULL;
	}
}

//*****************************************************************************
//
//*****************************************************************************
IRomMemoryManager::~IRomMemoryManager()
{
	delete mRomMemoryHeap;
}

//*****************************************************************************
//
//*****************************************************************************
bool IRomMemoryManager::IsAvailable()
{
	return mRomMemoryHeap != NULL;
}

//*****************************************************************************
//
//*****************************************************************************
void * IRomMemoryManager::Alloc( u32 size )
{
	return mRomMemoryHeap->Alloc( size );
}

//*****************************************************************************
//
//*****************************************************************************
void  IRomMemoryManager::Free(void * ptr)
{
	if( ptr == NULL )	
		return;

	if( mRomMemoryHeap->IsFromHeap( ptr ) )
	{
		mRomMemoryHeap->Free( ptr );
	}
	else
	{
		DAEDALUS_ERROR( "Memory is not from any of our heaps" );
	}
}
