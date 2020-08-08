/*
Copyright (C) 2006,2007 StrmnNrmn
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

#ifndef HLEGRAPHICS_TMEM_H_
#define HLEGRAPHICS_TMEM_H_

#ifdef DAEDALUS_ACCURATE_TMEM

void CopyLineQwords(void * dst, const void * src, u32 qwords);
void CopyLineQwordsSwap(void * dst, const void * src, u32 qwords);
void CopyLineQwordsSwap32(void * dst, const void * src, u32 qwords);
void CopyLine(void * dst, const void * src, u32 bytes);
void CopyLine16(u16 * dst16, const u16 * src16, u32 words);
void CopyLineSwap(void * dst, const void * src, u32 bytes);
void CopyLineSwap32(void * dst, const void * src, u32 bytes);

#endif // DAEDALUS_ACCURATE_TMEM

#endif // HLEGRAPHICS_TMEM_H_