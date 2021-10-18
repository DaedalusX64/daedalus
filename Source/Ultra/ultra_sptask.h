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

#ifndef OSHLE_ULTRA_SPTASK_H_
#define OSHLE_ULTRA_SPTASK_H_

#include "Base/Types.h"


// Pointers are 32 bits.
using osptr = u32;

typedef struct {
	u32	type;
	u32	flags;

	osptr	ucode_boot;
	u32	ucode_boot_size;

	osptr	ucode;
	u32	ucode_size;

	osptr	ucode_data;
	u32	ucode_data_size;

	osptr	dram_stack;
	u32	dram_stack_size;

	osptr	output_buff;
	osptr	output_buff_size;

	osptr	data_ptr;
	u32	data_size;

	osptr	yield_data_ptr;
	u32	yield_data_size;

} OSTask_t;

typedef union {
    OSTask_t		t;
    u64	force_structure_alignment;
} OSTask;

#define OS_TASK_YIELDED			0x0001
#define OS_TASK_DP_WAIT			0x0002
#define OS_TASK_USR0			0x0010
#define OS_TASK_USR1			0x0020
#define OS_TASK_USR2			0x0040
#define OS_TASK_USR3			0x0080

#endif // OSHLE_ULTRA_SPTASK_H_
