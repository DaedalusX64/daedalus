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

#ifndef __ULTRA_SPTASK_H__
#define __ULTRA_SPTASK_H__

typedef struct {
	u32	type;
	u32	flags;

	u64	*ucode_boot;
	u32	ucode_boot_size;

	u64	*ucode;
	u32	ucode_size;

	u64	*ucode_data;
	u32	ucode_data_size;

	u64	*dram_stack;
	u32	dram_stack_size;

	u64	*output_buff;
	u64	*output_buff_size;

	u64	*data_ptr;
	u32	data_size;

	u64	*yield_data_ptr;
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

#endif // __ULTRA_SPTASK_H__
