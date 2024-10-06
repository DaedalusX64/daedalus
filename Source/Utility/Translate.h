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

#ifndef UTILITY_TRANSLATE_H_
#define UTILITY_TRANSLATE_H_

#include "Base/Types.h"
#include <filesystem>
#include <string>

bool		 Translate_Init();
//
// Translate multiple strings
//
const char * Translate_Strings(const std::string& original, u32 & len);
//
// Translate a single string
//
const char * Translate_String(const char *original);
bool		 Translate_Read(u32 idx, const std::filesystem::path& dir);
void		 Translate_Unload();
void		 Translate_Load( const std::filesystem::path& p_dir );
const char * Translate_Name(u32 idx);
u32			 Translate_Number();
u32			 Translate_IndexFromName( const char * name );
const char * Translate_NameFromIndex( u32 idx );

#endif // UTILITY_TRANSLATE_H_
