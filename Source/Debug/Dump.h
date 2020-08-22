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

#ifndef DEBUG_DUMP_H_
#define DEBUG_DUMP_H_

#include <stdio.h>

#include "Base/Types.h"

struct OpCode;

void Dump_GetDumpDirectory(char * rootdir, const char * subdir);
void Dump_GetSaveDirectory(char * rootdir, const char * rom_filename, const char * extension);

#ifndef DAEDALUS_SILENT

void Dump_MemoryRange(FILE * fh, u32 address_offset, const u32 * b, const u32 * e);
void Dump_DisassembleRSPRange(FILE * fh, u32 address_offset, const OpCode * b, const OpCode * e);

void Dump_DisassembleMIPSRange(FILE * fh, u32 address_offset, const OpCode * b, const OpCode * e);
void Dump_Disassemble(u32 start, u32 end, const char * p_file_name);
void Dump_RSPDisassemble(const char * p_file_name);
void Dump_Strings(const char * p_file_name);

#endif

#endif // DEBUG_DUMP_H_
