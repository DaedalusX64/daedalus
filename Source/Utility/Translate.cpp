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

#include "Utility/IO.h"
//*****************************************************************************
//
//*****************************************************************************
struct Translate
{
	u32		id;				// ID that corresponds to string
	char	*translated;	// Translated string
};

Translate text[20];
//*****************************************************************************
//
//*****************************************************************************
char * Translate(u32 id, char *original)
{
	for( u32 i=0; i < ARRAYSIZE(text); i++ )
	{
		if( text[i].id == id )
		{
			if( text[i].translated )
				return text[i].translated;
		}
	}
	return original;
}

//*****************************************************************************
//
//*****************************************************************************
void Translate_Clear()
{
	memset(text, 0, sizeof(text));
}

//*****************************************************************************
//
//*****************************************************************************
bool Translate_Read(char *file)
{
	char line[1024];
	char path[MAX_PATH];
	char *string;
	FILE *stream;

	u32 count = 0;
	u32 id	  = 0;

	strcpy(path, gDaedalusExePath);
	strcat(path, file);

	stream = fopen(path,"r");
	if( stream )
	{
		while( fgets(line, 1023, stream) )
		{
			//
			// Remove any extra character that is added at the end of the string
			//
			IO::Path::Tidy(line);
			//
			//

			id = atoi(line);
			string = strchr(line,',');
			if( string != NULL )
			{
				string++;
				if( count <= ARRAYSIZE(text) )
				{
					// Write translated id/strings to array
					text[count].id = id;
					text[count].translated = (char*)malloc(strlen(string));
					if(text[count].translated == NULL)
					{
						//printf("Cannot allocate memory to load translated strings");
						return false;
					}
		
					strcpy(text[count].translated, string);
					count++;
				}
			}
			else
			{
				// ToDo : Remove me
				//printf("Hack : Nullifing...\n");
			}
		}
		fclose(stream);
		return true;
	}
	else
	{
		return false;
	}
}