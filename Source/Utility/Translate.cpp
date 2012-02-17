/*
Copyright (C) 2012 Salvy6735
Copyright (C) 2012 StrmnNrmn

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

#include "Translate.h"
#include "IO.h"

#include "SysPSP/Utility/PathsPSP.h"
#include "SysPSP/Utility/VolatileMemPSP.h"

#include <vector>
#include <string>

#define TRANSLATE_DUMP_VALUE 0xDAEDDAED
//*****************************************************************************
//
//*****************************************************************************
struct pTranslate
{
	u32		hash;			// hash that corresponds to string
	char	*translated;	// Translated string
};

pTranslate				 text[180];
std::vector<std::string> gLanguage;
//*****************************************************************************
//
//*****************************************************************************
u32 HashString(const char* s)
{
    u32 hash = 0;
    while (*s)
    {
        hash = hash * 101  +  *s++;
    }
    return hash;
}

//*****************************************************************************
//
//*****************************************************************************
const char * Translate_String(const char *original)
{
	u32 hash = HashString(original);
	if( hash == 0 )
		return original;

	for( u32 i=0; i < ARRAYSIZE(text); i++ )
	{
		if( text[i].hash == hash )
		{
			if( text[i].translated )
				return text[i].translated;
			else 
				return original;
		}
	}
	return original;
}

//*****************************************************************************
//
//*****************************************************************************
void Translate_Unload()
{
	// Clear translations
	for( u32 i = 0; i < ARRAYSIZE(text); ++i )
	{
		free_volatile(text[i].translated);
		text[i].translated = NULL;
	}
}
//*****************************************************************************
//
//*****************************************************************************
bool	Translate_Init()
{
	// Init translations if available
	Translate_Load( DAEDALUS_PSP_PATH("Languages/") );	

	return /*gLanguage.empty() == 0*/ true;
}

//*****************************************************************************
//
//*****************************************************************************
void	Translate_Load( const char * p_dir )
{
	// Set default language
	gLanguage.push_back("English");

	IO::FindHandleT		find_handle;
	IO::FindDataT		find_data;

	if(IO::FindFileOpen( p_dir, &find_handle, find_data ))
	{
		do
		{	
			char * filename( find_data.Name );
			char * last_period( strrchr( filename, '.' ) );
			if(last_period != NULL)
			{
				if( _strcmpi(last_period, ".lng") == 0 )
				{
					IO::Path::RemoveExtension( filename );
					gLanguage.push_back( filename );
					
				}
			}
		}
		while(IO::FindFileNext( find_handle, find_data ));

		IO::FindFileClose( find_handle );
	}
}

//*****************************************************************************
//
//*****************************************************************************
const char * Translate_Name(u32 idx)		
{	
	if( idx < gLanguage.size())
	{
		return gLanguage[ idx ].c_str();	
	}

	return "?";
}

//*****************************************************************************
//
//*****************************************************************************
u32 Translate_Number()			
{	
	return gLanguage.size()-1;			
}

//*****************************************************************************
//
//*****************************************************************************
u32	Translate_IndexFromName( const char * name )
{
	for( u32 i = 0; i < gLanguage.size(); ++i )
	{
		if( _strcmpi(  gLanguage[ i ].c_str(), name ) == 0 )
		{
			return i;
		}
	}

	// Default language (English)
	return 0;
}

//*****************************************************************************
//
//*****************************************************************************
const char * Translate_NameFromIndex( u32 idx )
{
	if( idx < gLanguage.size())
	{
		return gLanguage[ idx ].c_str();
	}

	return "?";
}

//*****************************************************************************
// Restores escape characters which were removed when parsing
// Which are needed by line-breaking and back-slash
//*****************************************************************************
const char * Restore(char *s, u32 len)
{
	for (u32 i = 0; i < len; i++) 
	{
		if (s[i] == '\\') 
		{
			if( s[i+1] == 'n' )
			{
				s[i+1] = '\b';	s[i] = '\n';
				i++;
			}
			else if( s[i+1] == '\\' )
			{	
				s[i+1] = '\b';	s[i] = '\\';
				i++;
			}
		}
	}
	return s;
}

//*****************************************************************************
//
//*****************************************************************************
void Translate_Dump(const char *string, bool dump)
{
	if(dump)
	{
		FILE * fh = fopen( "hash.txt", "a" );
		if(fh)
		{
			fprintf( fh,  "%08x,%s\n", HashString(string), string );
			fclose(fh);
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
bool Translate_Read(u32 idx, const char * dir)
{
	static u32 temp = 0;

	// Do not parse again if the same language
	if( temp == idx )	return true;	temp = idx;

	const char * ext( ".lng" );
	char line[1024];
	char path[MAX_PATH];
	char *string;
	FILE *stream;

	u32 count = 0;
	u32 hash  = 0;
	u32	len   = 0;

	// Build path where we'll load the translation file(s)
	strcpy(path, dir);
	strcat(path, gLanguage[ idx ].c_str());
	strcat(path, ext);

	// Always unload previous language file
	Translate_Unload();

	stream = fopen(path,"r");
	if( stream == NULL )
	{
		return false;
	}

	while( fgets(line, 1023, stream) )
	{
		// Strip spaces from end of lines
		IO::Path::Tidy(line);

		// Handle comments
		if (line[0] == '/')
			continue;

		string = strchr(line,',');
		if( string != NULL )
		{
			string++;
			len = strlen( string );
			sscanf( line,"%08x", &hash );
			if( count < ARRAYSIZE(text) )
			{
				// Write translated id/hash to array
				text[count].hash = hash;
				Translate_Dump( string, hash == TRANSLATE_DUMP_VALUE );

				text[count].translated = (char*)malloc_volatile(len+1); // Leave space for terminator
				strcpy( text[count].translated, Restore( string, len ) );
				count++;
			}
		}
	}
	fclose(stream);
	return true;
}