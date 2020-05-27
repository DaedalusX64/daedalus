/*
Copyright (C) 2020 Rinnegatamante

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

#pragma once

#ifndef RDRAM_H_
#define RDRAM_H_

/* Helper functions */
u8 clamp_u8(s16 x);
s16 clamp_s12(s16 x);
s16 clamp_s16(s32 x);
u16 clamp_RGBA_component(s16 x);

/* RDRam operations */
void rdram_read_many_u8(u8 *dst, u32 address, u32 count);
void rdram_read_many_u16(u16 *dst, u32 address, u32 count);
void rdram_write_many_u16(const u16 *src, u32 address, u32 count);
u32 rdram_read_u32(u32 address);
void rdram_write_many_u32(const u32 *src, u32 address, u32 count);
void rdram_read_many_u32(u32 *dst, u32 address, u32 count);

#endif // CORE_MEMORY_H_
