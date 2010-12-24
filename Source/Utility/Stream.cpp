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

#include "stdafx.h"
#include "Stream.h"

#include <string>

//*****************************************************************************
//
//*****************************************************************************
class COutputStringStreamImpl
{
public:
	std::string			mString;
};

//*****************************************************************************
//
//*****************************************************************************
COutputStringStream::COutputStringStream()
:	mpImpl( new COutputStringStreamImpl )
{
}

//*****************************************************************************
//
//*****************************************************************************
COutputStringStream::~COutputStringStream()
{
	delete mpImpl;
}

//*****************************************************************************
//
//*****************************************************************************
void	COutputStringStream::Clear()
{
	mpImpl->mString.clear();
}

//*****************************************************************************
//
//*****************************************************************************
COutputStream & COutputStringStream::operator<<( const char * p_str )
{
	mpImpl->mString += p_str;
	return *this;
}

//*****************************************************************************
//
//*****************************************************************************
COutputStream & COutputStringStream::operator<<( char val )
{
	mpImpl->mString += val;
	return *this;
}

//*****************************************************************************
//
//*****************************************************************************
COutputStream & COutputStringStream::operator<<( s32 val )
{
	char	buffer[ 32+1 ];
	sprintf( buffer, "%d", val );
	mpImpl->mString += buffer;
	return *this;
}

//*****************************************************************************
//
//*****************************************************************************
COutputStream & COutputStringStream::operator<<( u32 val )
{
	char	buffer[ 32+1 ];
	sprintf( buffer, "%d", val );
	mpImpl->mString += buffer;
	return *this;
}

//*****************************************************************************
//
//*****************************************************************************
const char *	COutputStringStream::c_str() const
{
	return mpImpl->mString.c_str();
}

//*****************************************************************************
//
//*****************************************************************************
COutputStream & operator<<( COutputStream & str, COutputStringStream & rhs )
{
	str << rhs.c_str();
	return str;
}
