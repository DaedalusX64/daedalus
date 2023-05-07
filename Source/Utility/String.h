/*
Copyright (C) 2001 StrmnNrmn

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

#ifndef UTILITY_STRING_H_
#define UTILITY_STRING_H_

#include <string.h>

#include "Base/Types.h"

#ifdef WIN32
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

struct ConstStringRef
{
	ConstStringRef() : Begin(NULL), End(NULL) {}
	/*explicit */ConstStringRef(const char * str) : Begin(str), End(str+strlen(str)) {}
	explicit ConstStringRef(const char * b, const char * e) : Begin(b), End(e) {}

	size_t size() const { return End - Begin; }

	bool operator==(const char * rhs) const	{ return operator==(ConstStringRef(rhs)); }
	bool operator==(const ConstStringRef & rhs) const
	{
		return size() == rhs.size() && memcmp(Begin, rhs.Begin, size()) == 0;
	}

	const char *	Begin;
	const char *	End;
};


#endif // UTILITY_STRING_H_
