// Profiler.cpp: implementation of the CProfiler class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Profiler.h"

#ifdef DAEDALUS_ENABLE_PROFILING

#include "Debug/DBGConsole.h"
#include "Utility/Timing.h"
#include "Utility/Hash.h"

#include <vector>
#include <string>
#include <stack>
#include <set>
#include <map>
#include <algorithm>

namespace
{

u64	GetNow()
{
	u64 now;
	NTiming::GetPreciseTime( &now );
	return now;
}
}

//*************************************************************************************
//
//*************************************************************************************
class CProfileItem
{
	public:
		typedef std::vector< CProfileItem * > ItemVector;

		CProfileItem( const char * p_str )
			:	mName( p_str )
		{
		}

		inline const char *			GetName() const				{ return mName.c_str(); }

	private:
		std::string					mName;
};

inline u32 ComputeCallstackHash( const std::vector< CProfileItem * > & items )
{
	if( items.empty() )
		return 0;

	return murmur2_hash( reinterpret_cast< const u8 * >( &items[0] ), items.size() * sizeof( CProfileItem * ), 0 );
}

class CProfileCallstack
{
public:
	CProfileCallstack( const CProfileCallstack * parent, const std::vector< CProfileItem * > & items )
		:	mParent( parent )
		,	mItems( items )
		,	mHash( ComputeCallstackHash( items ) )
		,	mTotalTime( 0 )
		,	mStartTime( 0 )
		,	mHitCount( 0 )
	{
	}


	CProfileItem *	GetBack() const	{ return mItems.empty() ? NULL : mItems.back(); }
	u32		GetDepth() const		{ return mItems.size(); }
	bool	Empty() const			{ return mItems.empty(); }
	void	Clear()					{ return mItems.clear(); }
	u32		GetHash() const			{ return mHash; }

	s32		Compare( const CProfileCallstack & rhs ) const
	{
		u32 i;
		for( i = 0; i < mItems.size() && i < rhs.mItems.size(); ++i )
		{
			s32		compare( stricmp( mItems[ i ]->GetName(), rhs.mItems[ i ]->GetName() ) );
			if( compare != 0 )
			{
				return compare;
			}
		}

		// 
		//	If both stacks are equal to this point and there are no more entries,
		//	they must be identical.
		//
		if( i >= mItems.size() && i >= rhs.mItems.size() )
			return 0;

		return i >= mItems.size() ? 1 : -1;
	}

	inline	const CProfileCallstack *	GetParent() const	{ return mParent; }
			void				Reset();
	inline	void				StartTiming();
	inline	void				StopTiming( u64 now );

	inline	u64					GetTotalTime() const		{ return mTotalTime; }
	inline	u32					GetHitCount() const			{ return mHitCount; }

private:
	const CProfileCallstack *		mParent;
	std::vector< CProfileItem * >	mItems;
	u32								mHash;
	u64								mTotalTime;				// The total time accumulated since the last Reset
	u64								mStartTime;				// The time when StartTiming() was called
	u32								mHitCount;				// The total number of times StartTiming() has been called since the last Reset()
};


//*************************************************************************************
//
//*************************************************************************************
class CProfilerImpl
{
	public:
		CProfilerImpl();
		~CProfilerImpl();

		void					Display();
		void					Update();

		SProfileItemHandle		AddItem( const char * p_str );

		inline void				Enter( SProfileItemHandle handle );
		inline void				Exit( SProfileItemHandle handle );

	private:
		CProfileCallstack *		GetActiveStats();


	private:
		typedef std::vector< CProfileItem * >			ProfileItemList;
		typedef std::stack< CProfileItem * >			ProfileItemStack;

		std::vector< CProfileItem * >	mActiveItems;
		std::vector< CProfileCallstack * >	mActiveCallstacks;

		typedef std::map< u32, CProfileCallstack * >	CallstackStatsMap;
		CallstackStatsMap		mCallstackStatsMap;

		f32						mFrequencyInv;
		ProfileItemList			mItems;
};


//*************************************************************************************
//
//*************************************************************************************
CProfilerImpl::CProfilerImpl()
{
	u64	frequency;
	NTiming::GetPreciseFrequency( &frequency );
	mFrequencyInv = 1.0f / float( frequency );

	SProfileItemHandle	root_item( AddItem( "<root>" ) );
	Enter( root_item );
}

//*************************************************************************************
//
//*************************************************************************************
CProfilerImpl::~CProfilerImpl()
{
	for( ProfileItemList::iterator it = mItems.begin(); it != mItems.end(); ++it )
	{
		CProfileItem * p_item( *it );
		delete p_item;
	}
}

//*************************************************************************************
//
//*************************************************************************************
SProfileItemHandle		CProfilerImpl::AddItem( const char * p_str )
{
	// Create a new item
	SProfileItemHandle	handle( mItems.size() );
	CProfileItem *		p_item( new CProfileItem( p_str ) );

	mItems.push_back( p_item );

	return handle;
}

//*************************************************************************************
//
//*************************************************************************************
void	CProfilerImpl::Display()
{
}

namespace
{
	void Pad( char * str, u32 length )
	{
		u32 actLen = strlen( str );

		if (actLen > length)
		{
			str[length] = '\0';
			str[length - 1 ] = '.';
			str[length - 2 ] = '.';
			str[length - 3 ] = '.';
		}
		else
		{
			char * end( str +  actLen);

			while( u32(end - str) < length )
			{
				*end++ = ' ';
			}
			*end = '\0';
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
struct SortByCallstack
{
	bool operator()( const CProfileCallstack * a, const CProfileCallstack * b )
	{
		return a->Compare( *b ) > 0;
	}
};


void	CProfilerImpl::Update()
{
	DAEDALUS_ASSERT( mActiveCallstacks.size() == mActiveItems.size(), "Why are there different numbers of callstacks/items?" );

	u64			now( GetNow() );
	for( u32 i = 0; i < mActiveCallstacks.size(); ++i )
	{
		mActiveCallstacks[ i ]->StopTiming( now );
	}

	u64			total_root_time( 0 );
	if( mActiveCallstacks.size() > 0 ) 
	{
		total_root_time = mActiveCallstacks[ 0 ]->GetTotalTime();
	}

	const char * const TERMINAL_SAVE_CURSOR			= "\033[s";
//	const char * const TERMINAL_RESTORE_CURSOR		= "\033[u";
//	const char * const TERMINAL_TOP_LEFT			= "\033[2A\033[2K";
	const char * const TERMINAL_TOP_LEFT			= "\033[H";
	const char * const TERMINAL_ERASE_TO_EOL		= "\033[K";
//	const char * const TERMINAL_ERASE_TO_EOS		= "\033[J";

	printf( TERMINAL_SAVE_CURSOR );
	printf( TERMINAL_TOP_LEFT );

	std::vector< const CProfileCallstack * >	active_callstacks;
	for( CallstackStatsMap::const_iterator it = mCallstackStatsMap.begin(); it != mCallstackStatsMap.end(); ++it )
	{
		const CProfileCallstack *	callstack( it->second );
		if( callstack->GetHitCount() > 0 )
		{
			active_callstacks.push_back( callstack );
		}
	}

	std::sort( active_callstacks.begin(), active_callstacks.end(), SortByCallstack() );

	//       0         1         2         3         4         5         6         7         8
	//       012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
	printf( " Function                                         Time us  Parent Overall  Hits\n" );

	for( u32 i = 0; i < active_callstacks.size(); ++i )
	{
		const CProfileCallstack *	callstack( active_callstacks[ i ] );

		u64					parent_time( 0 );
		const CProfileCallstack *	parent( callstack->GetParent() );
		if( parent != NULL )
		{
			parent_time = parent->GetTotalTime();
		}

		u32		hit_count( callstack->GetHitCount() );

		// Display details on this item
		u32		depth( callstack->GetDepth() );
		u32		total_us( u32( callstack->GetTotalTime() * 1000.0f * 1000.0f * mFrequencyInv ) );

		f32		percent_parent_time( 0 );
		f32		percent_total_time( 0 );

		if( parent_time != 0 )
		{
			percent_parent_time = 100.0f * f32( callstack->GetTotalTime() ) / f32( parent_time );
		}
		if( total_root_time != 0 )
		{
			percent_total_time = 100.0f * f32( callstack->GetTotalTime() ) / f32( total_root_time );
		}

		char line[ 1024 ];
		sprintf( line, "\033[2K%x%*s%s" , depth, depth, "", callstack->GetBack()->GetName() );
		Pad( line, 54 );
		printf( "%s %6.2f %6.1f%% %6.1f%% %5d%s\n", line, (f32)total_us / 1000.0f, percent_parent_time, percent_total_time, hit_count, TERMINAL_ERASE_TO_EOL );
		//DBGConsole_Msg( 0, "%*s %s %d,%03dms (%d calls)", depth, "", p_item->GetName(), total_us / 1000, total_us % 1000, hit_count );
	}

	printf( "<*>");
	//printf( "\033[2K----------------\n%s\n", TERMINAL_ERASE_TO_EOS );
	//printf( TERMINAL_RESTORE_CURSOR );
	fflush( stdout );

	for( CallstackStatsMap::iterator it = mCallstackStatsMap.begin(); it != mCallstackStatsMap.end(); ++it )
	{
		CProfileCallstack *	callstack( it->second );
		callstack->Reset();
	}

	for( u32 i = 0; i < mActiveCallstacks.size(); ++i )
	{
		mActiveCallstacks[ i ]->StartTiming();
	}
}

//*************************************************************************************
//
//*************************************************************************************
CProfileCallstack *	CProfilerImpl::GetActiveStats()
{
	CProfileCallstack *				callstack;
	u32								callstack_hash( ComputeCallstackHash( mActiveItems ) );
	CallstackStatsMap::iterator	it( mCallstackStatsMap.find( callstack_hash ) );
	if( it == mCallstackStatsMap.end() )
	{
		CProfileCallstack *	parent_callstack( mActiveCallstacks.empty() ? NULL : mActiveCallstacks.back() );

		callstack = new CProfileCallstack( parent_callstack, mActiveItems );
		mCallstackStatsMap[ callstack_hash ] = callstack;
	}
	else
	{
		callstack = it->second;
	}

	return callstack;
}

//*************************************************************************************
// Start profiling for an item
//*************************************************************************************
void	CProfilerImpl::Enter( SProfileItemHandle handle )
{
	DAEDALUS_ASSERT( handle.Handle < mItems.size(), "Invalid handle!" );
	DAEDALUS_ASSERT( mActiveCallstacks.size() == mActiveItems.size(), "Why are there different numbers of callstacks/items?" );

	CProfileItem *			item( mItems[ handle.Handle ] );

	mActiveItems.push_back( item );

	CProfileCallstack *		callstack( GetActiveStats() );

	mActiveCallstacks.push_back( callstack );

	callstack->StartTiming();

	DAEDALUS_ASSERT( mActiveCallstacks.size() == mActiveItems.size(), "Why are there different numbers of callstacks/items?" );
}

//*************************************************************************************
// Stop profiling for an item
//*************************************************************************************
void	CProfilerImpl::Exit( SProfileItemHandle handle )
{
	DAEDALUS_ASSERT( handle.Handle < mItems.size(), "Invalid handle!" );
	DAEDALUS_ASSERT( mActiveCallstacks.size() == mActiveItems.size(), "Why are there different numbers of callstacks/items?" );

	u64					now( GetNow() );
	CProfileItem *		item( mItems[ handle.Handle ] );

	if( !mActiveItems.empty() )
	{
		if( item == mActiveItems.back() )
		{
			CProfileCallstack *		callstack( mActiveCallstacks.back() );
			mActiveItems.pop_back();
			mActiveCallstacks.pop_back();
			
			callstack->StopTiming( now );
		}
		else
		{
			DAEDALUS_ERROR( "Popping the wrong item" );
		}
	}
	else
	{
		DAEDALUS_ERROR( "Item stack underflow" );
	}

	DAEDALUS_ASSERT( mActiveCallstacks.size() == mActiveItems.size(), "Why are there different numbers of callstacks/items?" );
}

//*************************************************************************************
//
//*************************************************************************************
void	CProfileCallstack::Reset()
{
	mTotalTime = 0;
	mStartTime = 0;
	mHitCount = 0;
}

//*************************************************************************************
//
//*************************************************************************************
void	CProfileCallstack::StartTiming()
{
	DAEDALUS_ASSERT( mStartTime == 0, "Already timing - recursive?" );

	mHitCount++;
	mStartTime = GetNow();
}

//*************************************************************************************
//
//*************************************************************************************
void CProfileCallstack::StopTiming( u64 now )
{
	DAEDALUS_ASSERT( mStartTime != 0, "We're not currently timing" );

	if( mStartTime != 0 )
	{
		mTotalTime += now - mStartTime;
		mStartTime = 0;
	}
}

//*************************************************************************************
//
//*************************************************************************************
CProfiler::CProfiler()
:	mpImpl( new CProfilerImpl( ) )
{
}

//*************************************************************************************
//
//*************************************************************************************
CProfiler::~CProfiler()
{
	delete mpImpl;
}

//*************************************************************************************
//
//*************************************************************************************
template<> bool CSingleton< CProfiler >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);
	
	mpInstance = new CProfiler();

	return true;
}

//*************************************************************************************
//
//*************************************************************************************
SProfileItemHandle		CProfiler::AddItem( const char * p_str )
{
	return mpImpl->AddItem( p_str );
}

//*************************************************************************************
//
//*************************************************************************************
void	CProfiler::Enter( SProfileItemHandle handle )
{
	mpImpl->Enter( handle );
}

//*************************************************************************************
//
//*************************************************************************************
void	CProfiler::Exit( SProfileItemHandle handle )
{
	mpImpl->Exit( handle );
}

//*************************************************************************************
//
//*************************************************************************************
void CProfiler::Update()
{
	mpImpl->Update();
}

//*************************************************************************************
//
//*************************************************************************************
void CProfiler::Display()
{
	mpImpl->Display();
}

#endif
