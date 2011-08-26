/*
Copyright (C) 2011 Salvy6735
Copyright (C) 2001-2009 StrmnNrmn
Copyright (C) 1999-2004 Joel Middendorf, <schibo@emulation64.com>

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

#include "Cheats.h"
#include "Memory.h"

#include "OSHLE/ultra_R4300.h"
#include "ConfigOptions.h"

//
// Cheatcode routines and format based from 1964
//

#define CHEAT_CODE_MAGIC_VALUE 0xDEAD

CODEGROUP *codegrouplist;
u32		codegroupcount		= 0;
s32		currentgroupindex	= -1;
char	current_rom_name[128];
extern void* malloc_volatile(size_t size);
extern bool bVolatileMem;
//enum { CHEAT_ALL_COUNTRY, CHEAT_USA, CHEAT_JAPAN, CHEAT_USA_AND_JAPAN, CHEAT_EUR, CHEAT_AUS, CHEAT_FR, CHEAT_GER };
//*****************************************************************************
//
//*****************************************************************************
// This is similar to ROM_GetCountryNameFromID but we don't get the whole name as Europe or USA to avoid slowing down parsing by adding inecesary strings 
// We can get just the country ID name from it but they are really confusing.. ex E for USA..
// Since this such a small function is alright even though it seems rebundant
//
const char * GetCountryName( u8 country )
{
	switch(country)
	{
	case 0x45:	return " (U)";		//USA
	case 0x50:	return " (E)";		//Europe
	case 0x4A:	return " (J)";		//Japan
	case 0x46:	return " (F)";		//French
	case 0x44:	return " (G)";		//Germany
	case 0x53:	return " (S)";		//Spanish
	case 0x55:	return " (A)";		//Australia
	case 0x49:	return " (I)";		//Italian
	case 0x37:	return " (B)";		//Beta (7)
	case 0x59:	return " (P)";		//PAL
	default:	return " (?)";		//0x41/0x58
	}
}

//*****************************************************************************
//
//*****************************************************************************

// Apply game shark code

//Supports N64 game shark code types: 
//
//Code Type Format Code Type Description 
//80-XXXXXX 00YY	8-Bit Constant Write 
//81-XXXXXX YYYY 16-Bit Constant Write 
//50-00AABB CCCC Serial Repeater 
//88-XXXXXX 00YY 8-Bit GS Button Write 
//89-XXXXXX YYYY 16-Bit GS Button Write 
//A0-XXXXXX 00YY 8-Bit Constant Write (Uncached) 
//A1-XXXXXX YYYY 16-Bit Constant Write (Uncached) 
//D0-XXXXXX 00YY 8-Bit If Equal To 
//D1-XXXXXX YYYY 16-Bit If Equal To 
//D2-XXXXXX 00YY 8-Bit If Not Equal To 
//D3-XXXXXX YYYY 16-Bit If Not Equal To 
//04-XXXXXX ---- Write directly to Audio Interface (Daedalus only)

//*****************************************************************************
//
//*****************************************************************************
static void CheatCodes_Apply(u32 index, u32 mode) 
{
	bool	executenext = true;

	for (u32 i = 0; i < codegrouplist[index].codecount; i ++) 
	{
		// Used by activator codes, skip until the specified button is pressed
		//
		if(executenext == false)
		{
			executenext = true;
			continue;
		}

		u32 address = PHYS_TO_K0(codegrouplist[index].codelist[i].addr & 0xFFFFFF);
		u16 value	= codegrouplist[index].codelist[i].val;
		u32 type	= (codegrouplist[index].codelist[i].addr >> 24) & 0xFF;

		if( mode == GS_BUTTON )
		{
			switch(type)
			{
			case 0x88:			
				Write8Bits(address,(u8)value);
				break;
			case 0x89:
				Write16Bits(address, value);
				break;
			}
			break;	
		}
		else
		{
			switch(type)
			{
			case 0x80:
			case 0xA0:
				// Check if orig value is unitialized and valid to store current value
				//
				if(codegrouplist[index].codelist[i].orig && (codegrouplist[index].codelist[i].orig == CHEAT_CODE_MAGIC_VALUE))
					codegrouplist[index].codelist[i].orig = Read8Bits(address);
			
				// Cheat code is no longer active, restore to original value
				//
				if(codegrouplist[index].enable==false)
					value = codegrouplist[index].codelist[i].orig;

				Write8Bits(address,(u8)value);
				break;
			case 0x81:
			case 0xA1:
				// Check if orig value is unitialized and valid to store current value
				//
				if(codegrouplist[index].codelist[i].orig && (codegrouplist[index].codelist[i].orig == CHEAT_CODE_MAGIC_VALUE))
					codegrouplist[index].codelist[i].orig = Read16Bits(address);

				// Cheat code is no longer active, restore to original value
				//
				if(codegrouplist[index].enable==false)
					value = codegrouplist[index].codelist[i].orig;
		
				Write16Bits(address, value);
				break;
			//case 0xD8:
			case 0xD0:
				if(Read8Bits(address) != value) executenext = false;
				break;
			//case 0xD9:
			case 0xD1:
				if(Read16Bits(address) != value) executenext = false;
				break;
			case 0xD2:
				if(Read8Bits(address) == value) executenext = false;
				break;
			case 0xD3:
				if(Read16Bits(address) == value) executenext = false;
				break;
			case 0x04:
				switch((codegrouplist[index].codelist[i].addr >> 20) & 0xF)
				{
					case 0x5:
							Memory_AI_SetRegister(codegrouplist[index].codelist[i].addr & 0x0FFFFFFF, value);
							break;
				}
			case 0x50:	
				{
					s32	repeatcount = (address & 0x0000FF00) >> 8;
					u32	addroffset	= (address & 0x000000FF);
					u16	valinc		= value;

					if(i + 1 < codegrouplist[index].codecount)
					{
						type	= codegrouplist[index].codelist[i + 1].addr >> 24;
						address		= PHYS_TO_K0(codegrouplist[index].codelist[i + 1].addr & 0x00FFFFFF);
						value		= codegrouplist[index].codelist[i + 1].val;
						u8 valbyte = (u8)value;

						switch(type)
						{
							case 0x80:
								do
								{
									Write8Bits(address,valbyte);
									address += addroffset;
									valbyte += (u8)valinc;
									repeatcount--;
								} while(repeatcount > 0);
								break;
							case 0x81:
								do
								{
									Write16Bits(address, value);
									address += addroffset;
									value += valinc;
									repeatcount--;
								} while(repeatcount > 0);
								break;
						}
					}
				}
				executenext = false;
				break;
			}
		} 
	}	
}

//*****************************************************************************
//
//*****************************************************************************
void CheatCodes_Activate( CHEAT_MODE mode )
{
	for(u32 i = 0; i < codegroupcount; i++)
	{
		// Apply only activated cheats
		//
		if(codegrouplist[i].active)
		{
			// Keep track of active cheatcodes, when they are disable, 
			// this flag will signal that we need to restore the hacked value to normal
			//
			codegrouplist[i].enable = true;

			CheatCodes_Apply( i, mode);
		}
		// If cheat code is no longer active, but was enabled, do one pass to restore original value
		//
		else if(codegrouplist[i].enable)
		{
			codegrouplist[i].enable = false;
			CheatCodes_Apply( i, mode);
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
static void CheatCodes_Clear()
{
	codegroupcount = 0;

	if(codegrouplist != NULL)
	{
		memset(codegrouplist, 0, sizeof(codegrouplist));
	}
}

//*****************************************************************************
//
//*****************************************************************************
// Works, but unused ATM
/*
void CheatCodes_Delete(u32 index)
{
	if(index < codegroupcount - 1)
	{
		for(u32 j = index; j < codegroupcount - 1; j++)
		{
			memcpy(&codegrouplist[j], &codegrouplist[j + 1], sizeof(CODEGROUP));
		}
	}
	codegroupcount--;
}
*/
//*****************************************************************************
// (STRMNNRMN - Strip spaces from end of names)
//*****************************************************************************
static char * tidy(char * s)
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
//*****************************************************************************
//
//*****************************************************************************
bool CheatCodes_Read(char *rom_name, char *file, u8 countryID)
{
	static char		last_rom_name[128];
	char			path[MAX_PATH];
	char			line[2048], romname[256]/*, errormessage[400]*/;	//changed line length to 2048 previous was 256
	bool			bfound;
	u32				c1, c2;
	FILE			*stream;

	strcpy(current_rom_name, rom_name);

	// Add country ID to this ROM name, to avoid mixing cheat codes of different region in the same entry
	strcat(current_rom_name, GetCountryName(countryID));

	// Do not parse again, if we already parsed for this ROM
	if(strcmp(current_rom_name, last_rom_name) == 0)
	{
		//printf("Cheat file is already parsed for this ROM\n");
		return true;
	}

	strcpy(last_rom_name, current_rom_name);

	// Always clear when parsing a new ROM
	CheatCodes_Clear();

	strcpy(path, gDaedalusExePath);
	strcat(path, file);

	stream = fopen(path, "rt");
	if(stream == NULL)
	{
		// File does not exist, try to create a new empty one
		stream = fopen(path, "wt");

		if(stream == NULL)
		{
			//printf("Cannot find Daedalus.cht file and cannot create it.");
			return false;
		}

		//printf("Cannot find Daedalus.cht file, creating an empty one\n");
		fclose(stream);
		return true;
	}

	// g_ROM.rh.Name adds extra spaces, remove 'em
	// No longer needed as we now fetch ROM name from Rom.ini
	//tidy( current_rom_name );

	// Locate the entry for current rom by searching for g_ROM.rh.Name
	//
	sprintf(romname, "[%s]", current_rom_name);

	bfound = false;

	while(fgets(line, 256, stream))
	{
		// Remove any extra character that is added at the end of the string
		//
		tidy(line);

		if(strcmp(line, romname) == 0)
		{
			// Found cheatcode entry
			bfound = true;
			break;
		}
		else
		{
			// No match? Keep looking for cheatcode..
			continue;
		}
	}

	if( bfound )
	{
		u32 numberofgroups;

		// First step, read the number of (cheat) groups for the current rom
		//
		if(fgets(line, 256, stream))
		{
			tidy(line);
			if(strncmp(line, "NumberOfGroups=", 15) == 0)
			{
				numberofgroups = atoi(line + 15);

				// Remove any excess of cheats to avoid wasting memory
				if( numberofgroups > MAX_CHEATCODE_PER_GROUP )
				{
					numberofgroups = MAX_CHEATCODE_PER_GROUP;
				}
			}
			else
			{
				// If for some reason NumberOfGroups is incorrect or invalid, just set the max of cheatcodes we allow
				numberofgroups = MAX_CHEATCODE_PER_GROUP;
			}
		}
		else
		{
			// Auch no number of groups? Cheat must be formated incorrectly
			return false;
		}

		// Allocate memory for groups
		//
//		printf("number of cheats loaded %d | %d kbs used of memory\n",numberofgroups,(numberofgroups *sizeof(CODEGROUP))/ 1024);
		codegrouplist = (CODEGROUP *) malloc_volatile(numberofgroups *sizeof(CODEGROUP));
		if(codegrouplist == NULL)
		{
			//printf("Cannot allocate memory to load cheat codes");
			return false;
		}
	
		codegroupcount = 0;
		while(codegroupcount < numberofgroups && fgets(line, 32767, stream) && strlen(line) > 8)	// 32767 makes sure the entire line is read
		{																							
			// Codes for the group are in the string line[]
			for(c1 = 0; line[c1] != '=' && line[c1] != '\0'; c1++) codegrouplist[codegroupcount].name[c1] = line[c1];

			if(codegrouplist[codegroupcount].name[c1 - 1] != ',')
			{
				codegrouplist[codegroupcount].name[c1] = '\0';
			}
			else
			{
				codegrouplist[codegroupcount].name[c1 - 1] = '\0';
			}

			if(line[c1 + 1] == '"')
			{
				// we have a note for this cheat code group
				u32 c3;

				for(c3 = 0; line[c3 + c1 + 2] != '"' && line[c3 + c1 + 2] != '\0'; c3++)
				{
					codegrouplist[codegroupcount].note[c3] = line[c3 + c1 + 2];
				}

				codegrouplist[codegroupcount].note[c3] = '\0';
				c1 = c1 + c3 + 3;
			}
			else
			{
				codegrouplist[codegroupcount].note[0] = '\0';
			}

			u32 addr, value;
			codegrouplist[codegroupcount].active = line[c1 + 1] - '0';
			codegrouplist[codegroupcount].enable = false;
			codegrouplist[codegroupcount].codecount = 0;

			c1 += 2;

			for(c2 = 0; c2 < (strlen(line) - c1 - 1) / 14; c2++, codegrouplist[codegroupcount].codecount++)
			{
				if (c2 < MAX_CHEATCODE_PER_GROUP)
				{
					sscanf( line + c1 + 1 + c2 * 14,"%08x-%04x", &addr, &value );

					codegrouplist[codegroupcount].codelist[c2].orig = CHEAT_CODE_MAGIC_VALUE;
					codegrouplist[codegroupcount].codelist[c2].addr = addr;
					codegrouplist[codegroupcount].codelist[c2].val = (u16)value;
				}
				else
				{
					codegrouplist[codegroupcount].codecount=MAX_CHEATCODE_PER_GROUP;
					/*sprintf (errormessage,
						     "Too many codes for cheat: %s (Max = %d)! Cheat will be truncated and won't work!",
							 codegrouplist[codegroupcount].name,
							 MAX_CHEATCODE_PER_GROUP);
					printf (errormessage);*/
					break;
				}
			}

			codegroupcount++;
		}

		//printf("Succesfully Loaded %d groups of cheat codes\n", codegroupcount);
	}
	else
	{
		// Cannot find entry for the current rom
		//printf("Cannot find entry %d groups of cheat code\n", codegroupcount);
	}

	fclose(stream);
	return true;
}
