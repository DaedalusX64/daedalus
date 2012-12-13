
#ifndef FASTMEMCPY_H_
#define FASTMEMCPY_H_

// Define to profile memcpys (PSP only!)
//#define PROFILE_MEMCPY

#ifdef PROFILE_MEMCPY
void memcpy_test( void * dst, const void * src, size_t size );
#endif

void memcpy_vfpu( void* dst, const void* src, size_t size );	
void memcpy_vfpu_swizzle( void* dst, const void* src, size_t size );
void memcpy_swizzle( void* dst, const void* src, size_t size );	// Little endian, platform independent

inline void fast_memcpy( void* dst, const void* src, size_t size )
{
#ifdef DAEDALUS_PSP
	 memcpy_vfpu( dst, src, size );
#else
	 memcpy( dst, src, size );
#endif
}

inline void fast_memcpy_swizzle( void* dst, const void* src, size_t size )
{
#ifdef DAEDALUS_PSP
	 memcpy_vfpu_swizzle( dst, src, size );
#else
#if (DAEDALUS_ENDIAN_MODE == DAEDALUS_ENDIAN_BIG)
	memcpy( dst, src, size );
#else
	memcpy_swizzle( dst, src, size );
#endif

#endif //DAEDALUS_PSP
}


#endif // FASTMEMCPY_H_
