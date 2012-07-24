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

#ifndef __REGISTERS_H__
#define __REGISTERS_H__

#if defined(DAEDALUS_DEBUG_CONSOLE) || !defined(DAEDALUS_SILENT)
/*
    CPU: Instructions encoded by opcode field.
    31---------26---------------------------------------------------0
    |  opcode   |                                                   |
    ------6----------------------------------------------------------
    |--000--|--001--|--010--|--011--|--100--|--101--|--110--|--111--| lo
000 | *1    | *2    | J     | JAL   | BEQ   | BNE   | BLEZ  | BGTZ  |
001 | ADDI  | ADDIU | SLTI  | SLTIU | ANDI  | ORI   | XORI  | LUI   |
010 | *3    | *4    |  ---  |  ---  | BEQL  | BNEL  | BLEZL | BGTZL |
011 | DADDI |DADDIU | LDL   | LDR   |  xxx  |  xxx  |  xxx  |  xxx  | top 4 bits == 7
100 | LB    | LH    | LWL   | LW    | LBU   | LHU   | LWR   | LWU   |
101 | SB    | SH    | SWL   | SW    | SDL   | SDR   | SWR   | CACHE |
110 | LL    | LWC1  |  ---  |  ---  | LLD   | LDC1  | LDC2  | LD    |
111 | SC    | SWC1  |  xxx  |  ---  | SCD   | SDC1  | SDC2  | SD    |
 hi |-------|-------|-------|-------|-------|-------|-------|-------|
     *1 = SPECIAL, see SPECIAL list    *2 = REGIMM, see REGIMM list
     *3 = COP0                         *4 = COP1
*/

#define OP_IS_A_HACK(x) (((x) >> 28) == 0x7)



extern const char *Cop1WOpCodeNames[64];
extern const char *Cop1LOpCodeNames[64];

// Register names

/* Shared by RSP */
extern const char *RegNames[32];
extern const char *Cop0RegNames[32];
extern const char *ShortCop0RegNames[32];

#endif // DAEDALUS_SILENT

#endif
