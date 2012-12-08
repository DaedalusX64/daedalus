/*
Copyright (C) 2009 Raphael

E-mail:   raphael@fx-world.org
homepage: http://wordpress.fx-world.org

*/

#ifndef FASTMEMCPY_H_
#define FASTMEMCPY_H_

void memcpy_vfpu_LE( void* dst, const void* src, size_t size );	// Little endian
void memcpy_vfpu_BE( void* dst, const void* src, size_t size );	// Big endian
void memcpy_cpu_LE( void* dst, const void* src, size_t size );	// CPU only

void memcpy_test( void * dst, const void * src, size_t size );

#endif // FASTMEMCPY_H_
