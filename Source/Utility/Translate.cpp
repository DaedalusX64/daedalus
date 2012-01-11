/*
Copyright (C) 2012 Salvy6735

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

#include <vector>
#include <string>
//*****************************************************************************
//
//*****************************************************************************
struct pTranslate
{
	u32		hash;			// hash that corresponds to string
	char	*translated;	// Translated string
};

pTranslate				 text[148];
std::vector<std::string> gLanguage;
//*****************************************************************************
//
//*****************************************************************************
u32 HashString(const char* s, u32 seed = 0)
{
    u32 hash = seed;
    while (*s)
    {
        hash = hash * 101  +  *s++;
    }
    return hash;
}

//*****************************************************************************
//
//*****************************************************************************
const char * Translate(const char *original)
{
	u32 hash = HashString( original );
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
void Translate_Clear()
{
	// Clear translations
	memset(text, 0, sizeof(text));	

	// Clear languages
	gLanguage.clear();
}

//*****************************************************************************
//
//*****************************************************************************
void	Translate_Load( const char * p_dir )
{
	// Reserve first entry, this will be replaced by our default language "English"
	// We could append our default language here, but we clear all language/translation contents to avoid wasting memory
	gLanguage.push_back( "" );

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
const char * GetLanguageName(u32 idx)		
{	
	return gLanguage[ idx ].c_str();	
}

//*****************************************************************************
//
//*****************************************************************************
u32 GetLanguageNum()			
{	
	return gLanguage.size()-1;			
}

//*****************************************************************************
//
//*****************************************************************************

bool Translate_Read(u32 idx, const char * dir)
{
	const char * ext( ".lng" );
	char line[1024];
	char path[MAX_PATH];
	char *string;
	FILE *stream;

	u32 count = 0;
	u32 hash  = 0;

	// Build path where we'll load the translation file(s)
	strcpy(path, dir);
	strcat(path, gLanguage[ idx ].c_str());
	strcat(path, ext);

	stream = fopen(path,"r");
	if( stream == NULL )
	{
		return false;
	}

	while( fgets(line, 1023, stream) )
	{
		IO::Path::Tidy(line);			// Strip spaces from end of lines

		// Handle comments
		if (line[0] == '/')
			continue;

		string = strchr(line,',');
		sscanf( line,"%08x", &hash );
		if( string != NULL )
		{
			string++;
			if( count <= ARRAYSIZE(text) )
			{
				// Write translated id/hash to array
				text[count].hash = hash;
				text[count].translated = (char*)malloc(strlen(string)+2);
				if(text[count].translated == NULL)
				{
					printf("Cannot allocate memory to load translated strings");
					return false;
				}
				strcpy(text[count].translated, string);
				/*FILE * fh = fopen( "hash.txt", "a" );
				if ( fh )
				{
					fprintf( fh,  "%08x, \"%s\"\n", text[count].hash, text[count].translated );
					fclose(fh);
				}
				*/
				count++;
			}
		}
	}
	fclose(stream);
	return true;
}