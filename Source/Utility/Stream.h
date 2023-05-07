/*
Copyright (C) 2006 StrmnNrmn

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

#ifndef UTILITY_STREAM_H_
#define UTILITY_STREAM_H_

#include "Base/Types.h"

class COutputStream
{
public:
	virtual ~COutputStream() {}

	virtual	COutputStream & operator<<( const char * p_str ) = 0;
	virtual	COutputStream & operator<<( char val ) = 0;
	virtual	COutputStream & operator<<( s32 val ) = 0;
	virtual	COutputStream & operator<<( u32 val ) = 0;
};

class CNullOutputStream : public COutputStream
{
public:
	virtual ~CNullOutputStream() {}

	virtual	COutputStream & operator<<( const char * p_str ) { return *this; }
	virtual	COutputStream & operator<<( char val ) { return *this; }
	virtual	COutputStream & operator<<( s32 val ) { return *this; }
	virtual	COutputStream & operator<<( u32 val ) { return *this; }

	const char *		c_str() const { return ""; }
};

class COutputStringStream : public COutputStream
{
public:
	COutputStringStream();
	~COutputStringStream();

	void				Clear();

	COutputStream & operator<<( const char * p_str );
	COutputStream & operator<<( char val );
	COutputStream & operator<<( s32 val );
	COutputStream & operator<<( u32 val );

	const char *		c_str() const;
private:
	class COutputStringStreamImpl *		mpImpl;
};

inline COutputStream & operator<<( COutputStream & str, CNullOutputStream & rhs )
{
	return str;
}

COutputStream & operator<<( COutputStream & str, COutputStringStream & rhs );

#endif // UTILITY_STREAM_H_
