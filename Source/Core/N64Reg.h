/*
Copyright (C) 2006 StrmnNrmn

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

#ifndef N64REG_H__
#define N64REG_H__

enum EN64Reg
{
	N64Reg_R0 = 0,
	N64Reg_AT, N64Reg_V0, N64Reg_V1, N64Reg_A0, N64Reg_A1, N64Reg_A2, N64Reg_A3,
	N64Reg_T0, N64Reg_T1, N64Reg_T2, N64Reg_T3, N64Reg_T4, N64Reg_T5, N64Reg_T6, N64Reg_T7,
	N64Reg_S0, N64Reg_S1, N64Reg_S2, N64Reg_S3, N64Reg_S4, N64Reg_S5, N64Reg_S6, N64Reg_S7,
	N64Reg_T8, N64Reg_T9, N64Reg_K0, N64Reg_K1, N64Reg_GP, N64Reg_SP, N64Reg_S8, N64Reg_RA,

	NUM_N64_REGS,
};

enum EN64FloatReg
{
	N64FloatReg_F00 = 0,N64FloatReg_F01, N64FloatReg_F02, N64FloatReg_F03, N64FloatReg_F04, N64FloatReg_F05, N64FloatReg_F06, N64FloatReg_F07,
	N64FloatReg_F08,	N64FloatReg_F09, N64FloatReg_F10, N64FloatReg_F11, N64FloatReg_F12, N64FloatReg_F13, N64FloatReg_F14, N64FloatReg_F15,
	N64FloatReg_F16,	N64FloatReg_F17, N64FloatReg_F18, N64FloatReg_F19, N64FloatReg_F20, N64FloatReg_F21, N64FloatReg_F22, N64FloatReg_F23,
	N64FloatReg_F24,	N64FloatReg_F25, N64FloatReg_F26, N64FloatReg_F27, N64FloatReg_F28, N64FloatReg_F29, N64FloatReg_F30, N64FloatReg_F31,

	NUM_N64_FP_REGS,
};

#endif // N64REG_H__
