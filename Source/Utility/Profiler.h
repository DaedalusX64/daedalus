// Profiler.h: interface for the CProfiler class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __PROFILER_H__
#define __PROFILER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef DAEDALUS_ENABLE_PROFILING

#include "Utility/Singleton.h"

struct SProfileItemHandle;

class CProfiler : public CSingleton< CProfiler >  
{
	protected:
		friend class CSingleton< CProfiler >;
		CProfiler();
	public:
		virtual ~CProfiler();

		void					Display();
		void					Update();

		SProfileItemHandle		AddItem( const char * p_str );

		void					Enter( SProfileItemHandle handle );
		void					Exit( SProfileItemHandle handle );

	protected:
		class CProfilerImpl * mpImpl;
};

//*************************************************************************************
//
//*************************************************************************************
struct SProfileItemHandle
{
	explicit SProfileItemHandle( u32 handle )
		:	Handle( handle )
	{
	}

	u32		Handle;
};

//*************************************************************************************
//
//*************************************************************************************
class CAutoProfile
{
public:
	CAutoProfile( SProfileItemHandle handle )
		:	mHandle( handle )
	{
		CProfiler::Get()->Enter( mHandle );
	}

	~CAutoProfile()
	{
		CProfiler::Get()->Exit( mHandle );
	}

private:
	SProfileItemHandle		mHandle;
};
#endif

//*************************************************************************************
//
//*************************************************************************************

#ifdef DAEDALUS_ENABLE_PROFILING

#define DAEDALUS_PROFILE( x )											\
	static	SProfileItemHandle		_profile_item( CProfiler::Get()->AddItem( x ) );	\
	CAutoProfile					_auto_profile( _profile_item );

#else

#define DAEDALUS_PROFILE( x )

#endif

#endif // __PROFILER_H__
