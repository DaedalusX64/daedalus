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


#ifndef REFCOUNTED_H_
#define REFCOUNTED_H_

class CRefCounted
{
	public:
		CRefCounted()
			:	mRefCount( 0 )
		{
		}

	protected:
		virtual ~CRefCounted()
		{
			DAEDALUS_ASSERT( mRefCount == 0, "Prematurely deleting refcounted object?" );
		}

	public:
		void	AddRef()
		{
			mRefCount++;
		}
		u32		Release()
		{
			DAEDALUS_ASSERT( mRefCount > 0, "RefCount underflow!" );
			mRefCount--;
			u32	ref_count( mRefCount );

			if(ref_count == 0)
			{
				delete this;
			}

			return ref_count;
		}

	private:
		u32			mRefCount;
};

template< typename T >
class CRefPtr
{
public:
	CRefPtr() : mPtr( NULL ) {}
	CRefPtr( T * ptr ) : mPtr( ptr )				  { if( mPtr != NULL ) mPtr->AddRef(); }
	CRefPtr( const CRefPtr & rhs ) : mPtr( rhs.mPtr ) { if( mPtr != NULL ) mPtr->AddRef(); }

	T* operator=( const CRefPtr & rhs )
	{
		if( &rhs != this )
		{
			Assign( rhs.mPtr );
		}
		return mPtr;
	}

	T*  operator=( T * ptr )
	{
		Assign( ptr );
		return mPtr;
	}

	~CRefPtr()
	{
		if( mPtr )
		{
			mPtr->Release();
		}
	}

	operator T *() const
	{
		return mPtr;
	}

	// CComPtr trick
	template <class U>
	class _NoAddRefRelease : public U
	{
		private:
			void	AddRef();
			u32		Release();
	};
	_NoAddRefRelease<T> * operator->() const
	{
		return (_NoAddRefRelease<T>*)mPtr;
	}

private:
	void	Assign( T * ptr )
	{
		if( ptr != NULL ) 		ptr->AddRef();
		if( mPtr )				mPtr->Release();
		mPtr = ptr;
	}

private:
	T *			mPtr;
};

#endif	// REFCOUNTED_H_
