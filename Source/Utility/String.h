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

#ifndef DAEDALUS_STRING_H__
#define DAEDALUS_STRING_H__

#if defined(DAEDALUS_PSP)
#define _strcmpi stricmp
#elif defined(DAEDALUS_PS3) || defined(DAEDALUS_OSX)
#define _strcmpi strcasecmp
#endif

class CConstString
{
	public:
		CConstString()
		:	mpString( "" )
		{
		}

		CConstString( const char * string )
		:	mpString( string )
		{
		}

		inline operator const char * () const	{ return mpString; }
		inline const char * c_str() const		{ return mpString; }

		CConstString & operator=( const char * string )
		{
			mpString = string;
			return *this;
		}

		bool operator==( const char * string )
		{
			return Equals( string );
		}

		//
		// Case sensitive compare
		//
		bool Equals( const char * string ) const
		{
			if ( mpString == string )
			{
				return true;
			}
			else if ( string == NULL )
			{
				return strlen( mpString ) == 0;
			}
			else if ( mpString == NULL )
			{
				return strlen( string ) == 0;
			}
			else
			{
				return strcmp( mpString, string ) == 0;
			}
		}

		//
		// Case insensitive compare
		//
		bool IEquals( const char * string ) const
		{
			if ( mpString == string )
			{
				return true;
			}
			else if ( string == NULL )
			{
				return strlen( mpString ) == 0;
			}
			else if ( mpString == NULL )
			{
				return strlen( string ) == 0;
			}
			else
			{
				return _strcmpi( mpString, string ) == 0;
			}
		}

		u32 Length() const
		{
			if ( mpString == NULL )
			{
				return 0;
			}
			else
			{
				return strlen( mpString );
			}
		}

		bool IsNull() const
		{
			return mpString == NULL;
		}

		bool IsEmpty() const
		{
			return Length() == 0;
		}


	private:
		const char *		mpString;
};

// Was CStaticString, conflicts with ATL :(
template< u32 MAX_LENGTH > class CFixedString
{
	public:
		CFixedString()
		{
			strcpy( mpString, "" );
		}

		CFixedString( CConstString string )
		{
			Copy( string );
		}

		CFixedString & operator=( CConstString string )
		{
			Copy( string );
			return *this;
		}

		inline operator const char * () const	{ return mpString; }
		inline const char * c_str() const		{ return mpString; }

		bool operator==( CConstString string )
		{
			return Equals( string );
		}

		CFixedString & operator+=( CConstString string )
		{
			Append( string );
			return *this;
		}

		//
		// Case sensitive compare
		//
		bool Equals( CConstString string ) const
		{
			if ( string == NULL )
			{
				return strlen( mpString ) == 0;
			}
			else
			{
				return strcmp( mpString, string ) == 0;
			}
		}

		//
		// Case insensitive compare
		//
		bool IEquals( const char * string ) const
		{
			if ( string == NULL )
			{
				return strlen( mpString ) == 0;
			}
			else
			{
				return _strcmpi( mpString, string ) == 0;
			}
		}


		inline u32 Length() const		{ return strlen( mpString ); }
		inline u32 MaxLength() const	{ return MAX_LENGTH; }

		inline bool empty() const		{ return Length() == 0; }

		//
		// Access for functions which need to write to our buffer. Should try to avoid these!
		//
		inline char * GetUnsafePtr()	{ return mpString; }


	private:

		void Copy( CConstString string )
		{
			DAEDALUS_ASSERT( string.Length() <= MAX_LENGTH, "String '%s' is too long for copy, truncation will occur", string.c_str() );
			strncpy( mpString, string, MAX_LENGTH );
			mpString[ MAX_LENGTH ] = '\0';
		}

		void Append( CConstString string )
		{
			DAEDALUS_ASSERT( Length() + string.Length() < MAX_LENGTH, "String '%s' is too long append, truncation will occur", string.c_str() );
			strncat( mpString, string, MAX_LENGTH );
			mpString[ MAX_LENGTH ] = '\0';
		}

	private:
		char			mpString[ MAX_LENGTH + 1 ];
};

class CString
{
	public:
		CString()
		:	mpString( NULL )
		,	mMaxLength( 0 )
		{
		}

		CString( CConstString string )
		{
			mMaxLength = string.Length();
			mpString = new char[ mMaxLength + 1 ];
			strcpy( mpString, string );
		}

		CString( const CString & string )
		{
			mMaxLength = string.MaxLength();
			mpString = new char[ mMaxLength + 1 ];
			strcpy( mpString, string );
		}


		CString & operator=( CConstString string )
		{
			Copy( string );
			return *this;
		}

		CString & operator+=( CConstString string )
		{
			Append( string );
			return *this;
		}


		CString & operator=( const CString & string )
		{
			// Check for a = a
			if ( this != &string )
			{
				Copy( string );
			}
			return *this;
		}

		CString & operator+=( const CString & string )
		{
			// Check for a += a
			DAEDALUS_ASSERT( this != &string, "Appending self - unhandled" );

			Append( string );
			return *this;
		}

		CString operator+( CConstString string ) const
		{
			CString	ret( *this );
			ret.Append( string );
			return ret;
		}


		CString operator+( const CString & string ) const
		{
			CString	ret( *this );
			ret.Append( string );
			return ret;
		}

		inline operator const char * () const	{ return mpString; }
		inline const char * c_str() const		{ return mpString; }

		bool operator==( CConstString string )
		{
			return Equals( string );
		}

		//
		// Case sensitive compare
		//
		bool Equals( CConstString string ) const
		{
			if ( mpString == string )
			{
				return true;
			}
			else if ( string == NULL )
			{
				return strlen( mpString ) == 0;
			}
			else if ( mpString == NULL )
			{
				return strlen( string ) == 0;
			}
			else
			{
				return strcmp( mpString, string ) == 0;
			}
		}

		//
		// Case insensitive compare
		//
		bool IEquals( CConstString string ) const
		{
			if ( mpString == string )
			{
				return true;
			}
			else if ( string == NULL )
			{
				return strlen( mpString ) == 0;
			}
			else if ( mpString == NULL )
			{
				return strlen( string ) == 0;
			}
			else
			{
				return _strcmpi( mpString, string ) == 0;
			}
		}

		u32 Length() const
		{
			if ( mpString == NULL )
			{
				return 0;
			}
			else
			{
				return strlen( mpString );
			}
		}

		bool IsNull() const
		{
			return mpString == NULL;
		}

		bool IsEmpty() const
		{
			return Length() == 0;
		}

		u32 MaxLength() const
		{
			return mMaxLength;
		}

		//
		// Access for functions which need to write to our buffer. Should try to avoid these!
		//
		char * GetUnsafePtr()
		{
			return mpString;
		}

	private:

		void Size( u32 length )
		{
			delete [] mpString;
			mMaxLength = length;
			mpString = new char[ length + 1 ];
		}

		void Resize( u32 length )
		{
			DAEDALUS_ASSERT( length > Length(), "Resize should always increase buffer length" );

			char * p_new = new char[ length + 1 ];

			if ( mpString == NULL )
			{
				strcpy( p_new, "" );
			}
			else
			{
				strcpy( p_new, mpString );
				delete [] mpString;
			}

			mMaxLength = length;
			mpString = p_new;
		}

		void Copy( const char * string )
		{
			if ( string == NULL )
			{
				if ( mMaxLength == 0 )
				{
					Size( 0 );
				}

				strcpy( mpString, "" );
			}
			else
			{
				u32 length( strlen( string ) );

				if ( length > mMaxLength )
				{
					Size( length );
				}

				strcpy( mpString, string );
			}
		}

		void Append( const char * string )
		{
			if ( string == NULL )
			{
				// Nothing to do
			}
			else
			{
				u32 length( Length() + strlen( string ) );

				if ( length > mMaxLength || mMaxLength == 0 )
				{
					Resize( length );
				}

				strcat( mpString, string );
			}

		}


	private:
		char *			mpString;
		u32				mMaxLength;
};

inline char * Tidy(char * s)
{
	if (s == NULL || *s == '\0')
		return s;

	char * p = s + strlen(s);

	p--;
	while (p >= s && (*p == ' ' || *p == '\r' || *p == '\n'))
	{
		*p = 0;
		p--;
	}
	return s;
}

#endif //#DAEDALUS_STRING_H__
