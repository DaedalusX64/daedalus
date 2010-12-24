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

#ifndef MEMORYPOOL_H_
#define MEMORYPOOL_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
//	XXXX Work in progress - don't use!
//
template< typename T >	
class CMemoryPool
{
	enum { MaxItems = 2500 };
	enum { Align = 16 };
	//enum { ItemSize = sizeof( T ) };//(sizeof( T ) + (Align-1)) & ~(Align-1) };
	u32 ItemSize;

	struct Chunk
	{
		Chunk * Next;
	};

	//struct _A{ DAEDALUS_STATIC_ASSERT( sizeof( Chunk ) == 4 ); };
	//struct _B{ DAEDALUS_STATIC_ASSERT( ItemSize >= sizeof( T ) ); };
	//struct _C{ DAEDALUS_STATIC_ASSERT( ItemSize >= sizeof( Chunk ) ); };

public:
	CMemoryPool()
		:	ItemSize( (sizeof( T ) + (Align-1)) & ~(Align-1) )
		,	mMemory( new u8[ MaxItems * ItemSize ] )
	{

		Chunk * first( NULL );

		// Add chunks in reverse order
		for( s32 i = MaxItems - 1; i >= 0; --i )
		{
			Chunk *		chunk( reinterpret_cast< Chunk *>( &mMemory[ i * ItemSize ] ) );

			chunk->Next = first;
			first = chunk;
		}

		mFirstFree = first;
	}

	~CMemoryPool()
	{
		delete [] mMemory;
	}

	inline T *	Allocate()
	{
		static bool said= false;
		if( !said )
		{
		printf( "Item size: %d->%d, MaxItems %d\n", sizeof( T ),  ItemSize, MaxItems );
			said = true;
		}
		Chunk * chunk( mFirstFree );
		if( chunk != NULL )
		{
			mFirstFree = chunk->Next;
		#ifdef _DEBUG
			memset( chunk, 0xcd, ItemSize );
		#endif
		}

		return reinterpret_cast< T * >( chunk );
	}

	inline void	Deallocate( void * ptr )
	{
		DAEDALUS_ASSERT( ptr == NULL || ptr >= mMemory && ptr < &mMemory[MaxItems * ItemSize], "Trying to free block outside of our chunk" );

		if( ptr != NULL )
		{
			Chunk *		chunk( reinterpret_cast< Chunk * >( ptr ) );

		#ifdef _DEBUG
			memset( chunk, 0xdd, ItemSize );
		#endif

			chunk->Next = mFirstFree;
			mFirstFree = chunk;
		}
	}

private:
	u8 *		mMemory;
	Chunk *		mFirstFree;
};


//
//	stl compatible allocator
//

//
//	Specialisation for <void>, avoiding void references
//
template <typename T> class CMemoryPoolAllocator;
template <> class CMemoryPoolAllocator<void>
{
public:
	typedef void *			pointer;
	typedef const void *	const_pointer;
	typedef void			value_type;
	template < class U > 	struct rebind { typedef CMemoryPoolAllocator<U> other; };
}; 

template< typename T >
class CMemoryPoolAllocator
{
public:
	typedef size_t		size_type;
	typedef ptrdiff_t	difference_type;
	typedef T *			pointer;
	typedef const T *	const_pointer;
	typedef T &			reference;
	typedef const T &	const_reference;
	typedef T			value_type;

	template< class U > struct rebind { typedef CMemoryPoolAllocator< U > other; };

	CMemoryPoolAllocator() {}
	CMemoryPoolAllocator( const CMemoryPoolAllocator< T > & ) {}
	template< typename U > CMemoryPoolAllocator( const CMemoryPoolAllocator< U > & ) {}

	size_type		max_size() const throw()			{ return 1; }

	pointer			address( reference x ) const		{ return &x; }
	const_pointer	address( const_reference x ) const	{ return &x; }

	pointer			allocate( size_type size, CMemoryPoolAllocator<void>::const_pointer hint = 0 ) const
	{
		if( size == 1 )
		{
			return mPool.Allocate();
		}

		DAEDALUS_ERROR( "Can't handle multiple allocations - don't use this allocator e.g. with std::vector" );
		return NULL;
	}

	void deallocate( pointer p, size_type n ) const
	{
		DAEDALUS_ASSERT( n == 1, "Deallocating multiple items?" );
		for( u32 i = 0; i < n; ++i )
		{
			mPool.Deallocate( p );
		}
	}

	void construct( pointer p, const T & val )
	{
		new( p ) T( val );
	}
	void construct( pointer p )
	{
		new( p ) T();
	}

	void destroy( pointer p )
	{
		p->~T();
	}

private:
	static CMemoryPool< T >		mPool;
};

template< typename T > CMemoryPool< T > CMemoryPoolAllocator< T >::mPool;

template< typename T, typename U >
inline bool operator==( const CMemoryPoolAllocator< T > &, const CMemoryPoolAllocator< U > & )
{
	return true;
}

template< typename T, typename U >
inline bool operator!=( const CMemoryPoolAllocator< T > &, const CMemoryPoolAllocator< U > & )
{
	return false;
}

#endif // MEMORYPOOL_H_
