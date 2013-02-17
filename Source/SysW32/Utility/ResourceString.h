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


#ifndef RESOURCESTRING_H__
#define RESOURCESTRING_H__


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CResourceString
{
	public:
		CResourceString( u32 nResourceID )
		{
#ifdef DAEDALUS_W32
			BOOL result = LoadString( _Module.GetResourceInstance(), nResourceID, mBuffer.GetUnsafePtr(), mBuffer.MaxLength() );
			DAEDALUS_ASSERT( result, "Error loading string" );
#else
			mBuffer = "<No Resource Strings>";
#endif
		}

		virtual ~CResourceString()
		{
		}

		operator const char * () const { return mBuffer; }
		operator char * ()				{ return mBuffer.GetUnsafePtr(); }

		const char * c_str() const		{ return mBuffer; }

	protected:
		// For the time being we hard code the size. If would be better
		// to determine how large the registry resource is..
		CFixedString< 1024 >		mBuffer;
};

#endif // #ifndef RESOURCESTRING_H__
