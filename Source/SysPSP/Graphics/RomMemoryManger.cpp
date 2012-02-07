
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

	virtual bool	Alloc( u32 size, void ** data );
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
:	mRomMemoryHeap( CMemoryHeap::Create( 32 * 1024 * 1024 ) )
{
	printf("ha\n");
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
bool IRomMemoryManager::Alloc( u32 size, void ** data )
{
	void * mem;

	mem = mRomMemoryHeap->Alloc( size );
	if( mem != NULL )
	{
		*data = mem;
		return true;
	}
	// It failed, there is no MEMORY left
	*data = NULL;
	return false;
}

//*****************************************************************************
//
//*****************************************************************************
void  IRomMemoryManager::Free(void * ptr)
{
	if( ptr == NULL )
	{

	}
	else if( mRomMemoryHeap->IsFromHeap( ptr ) )
	{
		mRomMemoryHeap->Free( ptr );
	}
	else
	{
		DAEDALUS_ERROR( "Memory is not from any of our heaps" );
	}
}
