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

#include <stdio.h>
#include <stdlib.h>

#include "Memory.h"
#include "ROM.h"
#include "Config/ConfigOptions.h"

#include "OSHLE/ultra_R4300.h"
#include "System/Paths.h"
#include "Utility/IO.h"
#include "Utility/StringUtil.h"
#include "Utility/VolatileMem.h"

//
// Cheatcode routines and format based from 1964
//


#define CHEAT_CODE_MAGIC_VALUE 0xDEAD

CODEGROUP *codegrouplist;
u32		codegroupcount		{};
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
	CHEATCODENODE *code = (CHEATCODENODE*)codegrouplist[index].codelist;
	u32 num {codegrouplist[index].codecount}; // number of codes
	bool enable {codegrouplist[index].enable};
	bool skip {false};

	while(num--)
	{
		// Used by activator code->
		if(skip == true)
		{
			skip = false;
			code++;
			continue;
		}
		u32 address {(code->addr & 0xFFFFFF)};
		u16 value	{code->val};
		u32 type	{(code->addr >> 24) & 0xFF};
		u8* p_mem	{g_pu8RamBase + address}; // addr is already pre-swapped

		switch(type)
		{
		case 0x80:
		case 0xA0:
			// Check if orig value is initialized and valid to store current value
			if((code->orig == CHEAT_CODE_MAGIC_VALUE) && code->orig )
				code->orig = *(u8 *)(p_mem);

			// Cheat code->is no longer active, restore to saved value
			// Set CHEAT_CODE_MAGIC_VALUE as well to make sure we can save the most recent value later on
			if(!enable)
			{
				value = code->orig;
				code->orig = CHEAT_CODE_MAGIC_VALUE;
			}

			*(u8 *)(p_mem) = (u8)value;
			break;
		case 0x81:
		case 0xA1:
			// Check if orig value is initialized and valid to store current value
			if((code->orig == CHEAT_CODE_MAGIC_VALUE) && code->orig )
				code->orig = *(u16 *)(p_mem);

			// Cheat code->is no longer active, restore to saved value
			// Set CHEAT_CODE_MAGIC_VALUE as well to make sure we can save the most recent value later on
			if(!enable)
			{
				value = code->orig;
				code->orig = CHEAT_CODE_MAGIC_VALUE;
			}

			*(u16 *)(p_mem) = value;
			break;
		case 0xD0:
			skip = ( *(u8 *)(p_mem) != value );
			break;
		case 0xD1:
			skip = ( *(u16 *)(p_mem) != value );
			break;
		case 0xD2:
			skip = ( *(u8 *)(p_mem) == value );
			break;
		case 0xD3:
			skip = ( *(u16 *)(p_mem) == value );
			break;
		case 0x88:
			if( mode == GS_BUTTON )*(u8 *)(p_mem) = (u8)value;
			break;
		case 0x89:
			if( mode == GS_BUTTON )	*(u16 *)(p_mem) = value;
			break;
		case 0x04:
			if( ((code->addr >> 20) & 0xF) == 0x5 )
				Memory_AI_SetRegister(code->addr & 0x0FFFFFFF, value);
			break;
		case 0x50:
			{
				skip		= true;
				s32	count	{(s32)(address & 0x0000FF00) >> 8};	// repeat count
				u32	offset	{(address & 0x000000FF)};
				u16	valinc	{value};

				if( (num - 1) > 0)
				{
					code++;
					type		= code->addr >> 24;
					address		= (code->addr & 0x00FFFFFF);
					value		= code->val;
					p_mem		= g_pu8RamBase + address;

					switch(type)
					{
					case 0x80:
						do
						{
							*(u8 *)((u32)p_mem ^ U8_TWIDDLE) = (u8)value;
							p_mem += offset;
							value += (u8)valinc;
							count--;
						} while(count > 0);
						break;
					case 0x81:
						do
						{
							*(u16 *)((u32)p_mem ^ U16_TWIDDLE) = value;
							p_mem += offset;
							value += valinc;
							count--;
						} while(count > 0);
						break;
					default:
						break;
					}
				}
				break;
			}
		}
		code++;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void CheatCodes_Activate( CHEAT_MODE mode )
{
	for(u32 i {}; i < codegroupcount; i++)
	{
		// Apply only activated cheats
		if(codegrouplist[i].active)
		{
			// Keep track of active cheatcodes, when they are disable,
			// this flag will signal that we need to restore the hacked value to normal
			codegrouplist[i].enable = true;
			CheatCodes_Apply( i, mode);
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
void CheatCodes_Disable( u32 index )
{
	if(codegrouplist[index].enable)
	{
		codegrouplist[index].active = false;
		codegrouplist[index].enable = false;

		// restore their original value too
		CheatCodes_Apply(index, IN_GAME);
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
		free_volatile(codegrouplist);
		codegrouplist = NULL;
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
//
//*****************************************************************************
bool CheatCodes_Read(const char *rom_name, const char *file, u8 countryID)
{
	char			current_rom_name[128] {};
	static char		last_rom_name[128] {};

	char			line[2048] {}, romname[256] {}/*, errormessage[400]*/;	//changed line length to 2048 previous was 256
	bool			bfound;
	u32				c1 {}, c2 {};
	FILE			*stream {};

	// Add country ID to this ROM name, to avoid mixing cheat code of different region in the same entry
	// Only the country id is important (first char)
	sprintf( current_rom_name, "%s (%c)", rom_name, ROM_GetCountryNameFromID(countryID)[0]);

	// Do not parse again, if we already parsed for this ROM
	// Should compare codegrouplist instead, but it'll use up quiet bit of memory
	if(strcmp(current_rom_name, last_rom_name) == 0)
	{
		//printf("Cheat file is already parsed for this ROM\n");
		return true;
	}

	strcpy(last_rom_name, current_rom_name);

	// Always clear when parsing a new ROM
	CheatCodes_Clear();

	IO::Filename	path;
	IO::Path::Combine(path, gDaedalusExePath, file);

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

	// Locate the entry for current rom by searching for g_ROM.rh.Name
	//
	sprintf(romname, "[%s]", current_rom_name);

	bfound = false;

	while(fgets(line, 256, stream))
	{
		// Remove any extra character that is added at the end of the string
		Tidy(line);

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
		u32 numberofgroups {};

		// First step, read the number of (cheat) groups for the current rom
		//
		if(fgets(line, 256, stream))
		{
			Tidy(line);
			if(strncmp(line, "NumberOfGroups=", 15) == 0)
			{
				numberofgroups = atoi(line + 15);

				// Remove any excess of cheats to avoid wasting memory
				if( numberofgroups > MAX_CHEATCODE_PER_LOAD )
				{
					numberofgroups = MAX_CHEATCODE_PER_LOAD;
				}
			}
			else
			{
				// If for some reason NumberOfGroups is incorrect or invalid, just set the max of cheatcodes we allow
				numberofgroups = MAX_CHEATCODE_PER_LOAD;
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
			//printf("Cannot allocate memory to load cheat code");
			return false;
		}

		codegroupcount = 0;
		while(codegroupcount < numberofgroups && fgets(line, 32767, stream) && strlen(line) > 8)	// 32767 makes sure the entire line is read
		{
			// Codes for the group are in the string line[]
			for(c1 = 0; line[c1] != '=' && line[c1] != '\0'; c1++) codegrouplist[codegroupcount].name[c1] = line[c1];

			// FIXME: xocde reckons this is dodgy - if c1 is 0 (first char is '='), this will index [-1]!
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

			u32 addr {}, value {};
			codegrouplist[codegroupcount].active = (line[c1 + 1] - '0') ? true : false;
			codegrouplist[codegroupcount].enable = false;
			codegrouplist[codegroupcount].codecount = 0;

			c1 += 2;

			for(c2 = 0; c2 < (strlen(line) - c1 - 1) / 14; c2++, codegrouplist[codegroupcount].codecount++)
			{
				if (c2 < MAX_CHEATCODE_PER_ENTRY)
				{
					sscanf( line + c1 + 1 + c2 * 14,"%08x-%04x", &addr, &value );
					if( c2 > 0 && ((codegrouplist[codegroupcount].codelist[c2-1].addr >> 24) & 0xFF) == 0x50 )
					{
						//ToDO: Uncompress Serial Repeater cheat code; ex addr = (temp + offset) ^ U8_TWIDDLE
						//Then we can convert it to a normal cheat code by just filling each slot with the repeater counter, but it would alot of memory..
						//The way compressed cheat codes work is in pairs, the serial repeater 0x50xxx and the actual code 0x80/81xxx; ex 50001801-0001,80118444-0000
						//For now if previous cheatcode was compressed, don't preswap
					}
					else
					{
						// Pre-swap address
						switch((addr >> 24) & 0xFF)
						{
						case 0x80:
						case 0xA0:
						case 0xD0:
						case 0xD2:
						case 0x88:
							addr ^= U8_TWIDDLE;
							break;
						case 0x81:
						case 0xA1:
						case 0xD1:
						case 0xD3:
						case 0x89:
							addr ^= U16_TWIDDLE;
							break;
						case 0x04:
							break;
						case 0x50:
							break;
						default:
							//Unhandled cheat code? I think we handle most if not all cheat code->types?
							break;
						}
					}

					codegrouplist[codegroupcount].codelist[c2].orig = CHEAT_CODE_MAGIC_VALUE;
					codegrouplist[codegroupcount].codelist[c2].addr = addr;
					codegrouplist[codegroupcount].codelist[c2].val = (u16)value;
				}
				else
				{
					codegrouplist[codegroupcount].codecount=MAX_CHEATCODE_PER_ENTRY;
					/*sprintf (errormessage,
						     "Too many codes for cheat: %s (Max = %d)! Cheat will be truncated and won't work!",
							 codegrouplist[codegroupcount].name,
							 MAX_CHEATCODE_PER_ENTRY);
					printf (errormessage);*/
					break;
				}
			}
			codegroupcount++;
		}
	}
	else
	{
		// Cannot find entry for the current rom
		//printf("Cannot find entry %d groups of cheat code->n", codegroupcount);
	}

	fclose(stream);
	return true;
}
