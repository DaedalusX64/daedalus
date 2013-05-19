/*
Copyright (C) 2001,2005 StrmnNrmn

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

#ifndef SYSW32_DYNAREC_X86_DYNARECTARGETX86_H_
#define SYSW32_DYNAREC_X86_DYNARECTARGETX86_H_

// Intel register codes. Odd ordering is for intel bytecode
enum EIntelReg {
	INVALID_CODE = 0xFFFFFFFF,
	EAX_CODE = 0,
	ECX_CODE = 1,
	EDX_CODE = 2,
	EBX_CODE = 3,
	ESP_CODE = 4,
	EBP_CODE = 5,
	ESI_CODE = 6,
	EDI_CODE = 7,

	NUM_X86_REGISTERS = 8,
};


#endif // SYSW32_DYNAREC_X86_DYNARECTARGETX86_H_
