#include "as_reg_compat.h"

//The top two defines needs to be adjusted depending on how gCPUState struct is formated in CPU.h!! //Corn
//
#define _C0_Count	(0x100 + 9 * 4)	//CPU_Control_base + 9*8(64bit regs) or 9*4(32bit regs)
#define _AuxBase	0x280	//Base pointer to Aux regs
#define _CurrentPC	(_AuxBase + 0x00)
#define _TargetPC	(_AuxBase + 0x04)
#define _Delay		(_AuxBase + 0x08)
#define _StuffToDo	(_AuxBase + 0x0c)
#define _MultLo		(_AuxBase + 0x10)
#define _MultHi		(_AuxBase + 0x18)
#define _Events		(_AuxBase + 0x20)

	.set noat

	.extern HandleException_extern
	.extern CPU_UpdateCounter
	.extern IndirectExitMap_Lookup
	.extern g_ReadAddressLookupTableForDynarec
	.extern Write32BitsForDynaRec
	.extern Write16BitsForDynaRec
	.extern Write8BitsForDynaRec
	.extern CPU_HANDLE_COUNT_INTERRUPT
	

	.global _EnterDynaRec
	.global _ReturnFromDynaRec
	.global _DirectExitCheckNoDelay
	.global _DirectExitCheckDelay
	.global _IndirectExitCheck
	.global _ReturnFromDynaRecIfStuffToDo
	.global _DaedalusICacheInvalidate

	.global _ReadBitsDirect_u8
	.global _ReadBitsDirect_s8
	.global _ReadBitsDirect_u16
	.global _ReadBitsDirect_s16
	.global _ReadBitsDirect_u32

	.global _ReadBitsDirectBD_u8
	.global _ReadBitsDirectBD_s8
	.global _ReadBitsDirectBD_u16
	.global _ReadBitsDirectBD_s16
	.global _ReadBitsDirectBD_u32
	
	.global _WriteBitsDirect_u32
	.global _WriteBitsDirect_u16
	.global _WriteBitsDirect_u8
	.global _WriteBitsDirectBD_u32
	.global _WriteBitsDirectBD_u16
	.global _WriteBitsDirectBD_u8
	
	.global	_FloatToDouble
	.global	_DoubleToFloat

    .data
exit_dynarec_text:
	.asciiz		"Exiting dynarec (PC is %08x StuffToDo is 0x%x)\n"
	.text
temp_storage_for_ra:
	nop
    
    .set push 
    .set noreorder 
    
#######################################################################################
#    Invalidate a1 bytes of icache from a0
#	a0 - the base address of the memory to invalidate in the icache
#	a1 - the number of bytes to invalidate
_DaedalusICacheInvalidate:

	addiu		$t0, $0, 0xffc0			# t0 = ~63
	
	# Store base+size in a1
	addu		$a1, $a1, $a0

	# Truncate 'start' pointer down to nearest 64 bytes boundary
	and			$a0, $a0, $t0

	# Truncate 'end' pointer down to nearest 64 bytes boundary
	and			$a1, $a1, $t0

	# Do while current < end
invalidate_next:
	cache		8, 0($a0)						# 8 is for icache invalidate
	bne			$a0, $a1, invalidate_next		# Keep looping until 
	addiu		$a0, $a0, 64

	jr			$ra
	nop
    
#######################################################################################
#    
#	a0 - fragment function to enter
#	a1 - gCPUState base pointer
#	a2 - Memory base offset (i.e. g_pu8RamBase - 0x80000000 )
#	a3 - Memory upper bound (e.g. 0x80400000)
#
#
_EnterDynaRec:
	addiu	$sp, $sp, -40	# Push return address on the stack
	sw		$fp, 4($sp)		# Store s8 - we use this as our base pointer
	sw		$ra, 0($sp)
	
	sw		$s0, 8($sp)
	sw		$s1, 12($sp)
	sw		$s2, 16($sp)
	sw		$s3, 20($sp)
	sw		$s4, 24($sp)
	sw		$s5, 28($sp)
	sw		$s6, 32($sp)
	sw		$s7, 36($sp)
	
	or		$s7, $a2, $0	# Read address table
	or		$s6, $a3, $0	# Upper bound
	
	jr		$a0				# Jump to our target function
	or		$fp, $a1, $0
	
#######################################################################################
#	Check gCPUState.StuffToDo. If non-zero, performs any required handling then exits
#	the dynarec system. If the flag is zero this just returns immediately.
#	NB: As a significant optinisation the dynarec system patches the first two ops
#	of this function to return immediately in the case the gCPUState.StuffToDo is not set.
#
_ReturnFromDynaRecIfStuffToDo:

# Sanity checking logic
#	lw		$t0, _StuffToDo($fp)		# StuffToDo
#	bne		$t0, $0, exception_exit
#	nop
#	jr		$ra					# Just return back to caller
#	nop
#exception_exit:

	jal		HandleException_extern
	nop

# Fall through to this
#	j		_ReturnFromDynaRec
#	nop

#######################################################################################
#
_ReturnFromDynaRec:

	lw		$s0, 8($sp)
	lw		$s1, 12($sp)
	lw		$s2, 16($sp)
	lw		$s3, 20($sp)
	lw		$s4, 24($sp)
	lw		$s5, 28($sp)
	lw		$s6, 32($sp)
	lw		$s7, 36($sp)

	lw		$ra, 0($sp)		# Restore our return address
	lw		$fp, 4($sp)		# And s8
	jr		$ra
	addiu	$sp, $sp, +40



#######################################################################################
#	Check if we need to exit the dynarec system and jump out as necessary.
#	If we are ok to continue, this returns control to the calling code.
#	Calling this function updates the COUNT register with the specified number of cycles.
#	a0	- instructions executed
#	a1	- exit pc
_DirectExitCheckNoDelay:
	
	# The code below corresponds to CPU_UpdateCounter
	lw		$t0, _C0_Count($fp)		# COUNT register
	lw		$t1, _Events($fp)		# Events[0].mCount

	addu	$t0, $t0, $a0		# COUNT + ops_executed
	sw		$t0, _C0_Count($fp)		# COUNT = COUNT + ops_executed

	sw		$a1, _CurrentPC($fp) 	# CurrentPC
	sw		 $0, _Delay($fp)		# Delay = NO_DELAY
	
	subu	$t1, $t1, $a0		# Events[0].mCount - ops_executed
	blez	$t1, _DirectExitCheckCheckCount
	sw		$t1, _Events($fp)		# Events[0].mCount = Events[0].mCount - ops_executed

	jr		$ra					# Return back to caller
	nop
	
#######################################################################################
#	Check if we need to exit the dynarec system and jump out as necessary.
#	If we are ok to continue, this returns control to the calling code.
#	Calling this function updates the COUNT register with the specified number of cycles.
#	a0	- instructions executed
#	a1	- exit pc
#	a2	- target pc
_DirectExitCheckDelay:

	# The code below corresponds to CPU_UpdateCounter
	lw		$t0, _C0_Count($fp)		# COUNT register
	lw		$t1, _Events($fp)		# Events[0].mCount

	addu	$t0, $t0, $a0		# COUNT + ops_executed
	sw		$t0, _C0_Count($fp)		# COUNT = COUNT + ops_executed
	
	sw		$a1, _CurrentPC($fp) 	# CurrentPC
	sw		$a2, _TargetPC($fp) 	# TargetPC
	li		$t0, 1				# EXEC_DELAY
	sw		$t0, _Delay($fp)		# Delay

	subu	$t1, $t1, $a0		# Events[0].mCount - ops_executed
	blez	$t1, _DirectExitCheckCheckCount
	sw		$t1, _Events($fp)		# Events[0].mCount = Events[0].mCount - ops_executed

	jr		$ra
	nop

#######################################################################################
#	Utility routine for _DirectExitCheckXX. 
#
_DirectExitCheckCheckCount:
	or		$s0, $ra, $0		# Keep track of return addresss
	jal		CPU_HANDLE_COUNT_INTERRUPT
	nop

	lw		$t0, _StuffToDo($fp)		# StuffToDo
	bne		$t0, $0, _ReturnFromDynaRec
	nop
	jr		$s0					# Return back to caller
	nop


#######################################################################################
#	Update counter. If StuffToDo flags is clear on return, 
#	a0 - instructions executed
#	a1 - CIndirectExitMap pointer
#	a2 - exit pc (exit delay is always NO_DELAY)
_IndirectExitCheck:
	or		$s0, $a1, $0		# Keep track of map pointer
	or		$s1, $a2, $0		# and the exit pc

	# Can avoid these until _ReturnFromDynaRec?	
	sw		$a2, _CurrentPC($fp) 	# CurrentPC
	jal		CPU_UpdateCounter	# a0 holds instructions executed
	sw		$0,  _Delay($fp)		# Delay (NO_DELAY)

	lw		$t0, _StuffToDo($fp)		# StuffToDo
	bne		$t0, $0, _ReturnFromDynaRec
	nop
	
	or		$a0, $s0, $0		# p_map
	jal		IndirectExitMap_Lookup
	or		$a1, $s1, $0		# exit_pc

	# $v0 holds pointer to indirect target. If it's 0, it means it's not compiled yet
	beq		$v0, $0,  _ReturnFromDynaRec
	nop
	
	jr		$v0
	nop


#######################################################################################
#	u32 ret = u32( *(T *)FuncTableReadAddress( address ) );
#	_ReturnFromDynaRecIfStuffToDo( 0 );
#	return ret;
#
# a0 address (pre-swizzled)
# a1 current_pc
#
.macro READ_BITS	function, load_instruction
\function:

	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)

	srl		$v1, $a0, 0x12
	sll		$v1, $v1, 0x3	# * 8 to index the two pointer struct
	la		$v0, g_ReadAddressLookupTableForDynarec
	lw		$v0, 0($v0)			# The above is a pointer to our table
	addu	$v1, $v1, $v0
	lw		$v0, 4($v1)	#offset 4 to get the second pointer
	
	jalr	$v0
	sw		$a1, _CurrentPC($fp) 	# CurrentPC

	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0

	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	
	jr		$ra
	\load_instruction	$v0, 0($v0)		# e.g. lbu, lhu, lw etc
	
.endm

.macro READ_BITS_BD	function, load_instruction
\function:

	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)

	sw		$a1, _CurrentPC($fp) 	# CurrentPC
	li		$t0, 1				# EXEC_DELAY

	srl		$v1, $a0, 0x12
	sll		$v1, $v1, 0x3	# * 8 to index the two pointer struct
	la		$v0, g_ReadAddressLookupTableForDynarec
	lw		$v0, 0($v0)			# The above is a pointer to our table
	addu	$v1, $v1, $v0
	lw		$v0, 4($v1)	#offset 4 to get the second pointer
	
	jalr	$v0
	sw		$t0, _Delay($fp)		# Delay

	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0

	sw		$0, _Delay($fp)		# Delay <- NO_DELAY
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	
	jr		$ra
	\load_instruction	$v0, 0($v0)		# e.g. lbu, lhu, lw etc
	
.endm

	READ_BITS _ReadBitsDirect_u8, lbu
	READ_BITS _ReadBitsDirect_s8, lb
	READ_BITS _ReadBitsDirect_u16, lhu
	READ_BITS _ReadBitsDirect_s16, lh
	READ_BITS _ReadBitsDirect_u32, lw

	READ_BITS_BD _ReadBitsDirectBD_u8, lbu
	READ_BITS_BD _ReadBitsDirectBD_s8, lb
	READ_BITS_BD _ReadBitsDirectBD_u16, lhu
	READ_BITS_BD _ReadBitsDirectBD_s16, lh
	READ_BITS_BD _ReadBitsDirectBD_u32, lw
		
#######################################################################################
#	These functions handle writing a value out to memory.
#	They set up the PC (and optionally the branch delay flag for the BD versions)
#	After the memory has been written, _ReturnFromDynaRecIfStuffToDo is called,
#	which returns control back to the interpreter in the case that an exception
#	was triggered.
#
# a0 address (pre-swizzled)
# a1 value
# a2 current_pc
#
_WriteBitsDirect_u32:
	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)
	
	jal		Write32BitsForDynaRec
	sw		$a2, _CurrentPC($fp) 	# CurrentPC
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	nop

_WriteBitsDirectBD_u32:
	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)
	
	sw		$a2, _CurrentPC($fp) 	# CurrentPC
	li		$t0, 1				# EXEC_DELAY

	jal		Write32BitsForDynaRec
	sw		$t0, _Delay($fp)		# Delay
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	sw		$0, _Delay($fp)		# Delay <- NO_DELAY
	
_WriteBitsDirect_u16:
	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)
	
	jal		Write16BitsForDynaRec
	sw		$a2, _CurrentPC($fp) 	# CurrentPC
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	nop
	
	
_WriteBitsDirectBD_u16:
	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)
	
	sw		$a2, _CurrentPC($fp) 	# CurrentPC
	li		$t0, 1				# EXEC_DELAY
	
	jal		Write16BitsForDynaRec
	sw		$t0, _Delay($fp)		# Delay
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	sw		$0, _Delay($fp)		# Delay <- NO_DELAY
	
_WriteBitsDirect_u8:
	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)
	
	jal		Write8BitsForDynaRec
	sw		$a2, _CurrentPC($fp) 	# CurrentPC
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	nop		
	
_WriteBitsDirectBD_u8:
	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)
	
	sw		$a2, _CurrentPC($fp) 	# CurrentPC
	li		$t0, 1				# EXEC_DELAY
	
	jal		Write8BitsForDynaRec
	sw		$t0, _Delay($fp)		# Delay
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	sw		$0, _Delay($fp)		# Delay <- NO_DELAY
	
#######################################################################################
/**
 * convert float to double
 * double FloatToDouble(float a);
 *
 * input: a0	//fa0
 * output: v0,v1
 * clobber: a0,t1
 */
_FloatToDouble:
	//mfc1	$a0, $fa0				/* a0 = fa0 */
	ext	$t1, $a0, 23, 8			/* t1 = (a0 >> 23) & 0xFF */
	beq	$t1, $zero, ftod_denormal		/* if (t1==0) goto ftod_denormal */
	addiu	$v0, $t1, (-0x7F+0x3FF)		/* v0 = t1 - 0x7F + 0x3FF */
	xori	$t1, $t1, 0xFF			/* t1 = t1 ^ 0xFF */
	li	$v1, 0x7FF			/* v1 = 0x7FF */
	movz	$v0, $v1, $t1			/* v0 = (t1==0) ? v1 : v0 */
	ext	$v1, $a0,  3, 20			/* v1 = (a0 >> 3 ) & 0x00FFFFF */
	ins	$v1, $v0, 20, 11			/* v1 = (v1 & 0x800FFFFF) | ((v0<<20) & 0x7FF00000) */
	sll	$v0, $a0, 29			/* v0 = (a0 << 29) */
	srl	$a0, $a0, 31			/* a0 = (a0 >> 31) & 1 */
	jr	$ra				/* return */
	ins	$v1, $a0, 31, 1			/* v1 = (v1 & 0x7FFFFFFF) | ((a0<<31) & 0x80000000) */

ftod_denormal:
	sll	$v0, $a0, 9			/* v0 = a0 << 9 */
	beql	$v0, $zero, ftod_zero		/* if (v0==0) goto ftod_zero */
	move	$v1, $zero			/* v1 = 0 */
	li	$v1, 0x380			/* v1 = 0x380 */
	clz	$t1, $v0				/* t1 = clz(v0) */
	subu	$v0, $v1, $t1			/* v0 = v1 - v0 = 0x380 - clz(t1) */
	sllv	$t1, $a0, $t1			/* t1 = a0 << t1 */
	ext	$v1, $t1,  2, 20			/* v1 = (t1 >> 2 ) & 0x00FFFFF */
	ins	$v1, $v0, 20, 11			/* v1 = (v1 & 0x800FFFFF) | ((v0<<20) & 0x7FF00000) */
	sll	$v0, $t1, 30			/* v0 = (t1 << 30) */
ftod_zero:
	srl	$a0, $a0, 31			/* a0 = (a0 >> 31) & 1 */
	jr	$ra				/* return */
	ins	$v1, $a0, 31, 1			/* v1 = (v1 & 0x7FFFFFFF) | ((a0<<31) & 0x80000000) */

#######################################################################################
/**
 * convert double to float
 * float DoubleToFloat(double a);
 * input: a0,a1
 * output: v0
 * clobber: t0,t1,v0,v1
 */
_DoubleToFloat:
	ext	$t0, $a1, 20, 11		/* t0 = (a1>>20) & 0x000007FF */
	beq	$t0, $zero, dtof_zero	/* if (t0==0) goto dtof_zero */
	xori	$t1, $t0, 0x7FF		/* t1 = t0 ^ 0x7FF */
	beq	$t1, $zero, dtof_naninf	/* if (t1==0) goto dtof_naninf */
	addiu	$t1, $t0, (+0x7F-0x3FF)	/* t1 = t0 + 0x7F - 0x3FF */
	blez	$t1, dtof_denormal	/* if (t1<=0) goto dtof_denormal */
	addiu	$v1, $t1, -0xFE		/* v1 = t1 - 0xFE */
	bgtz	$v1, dtof_inf		/* if (v1 > 0) goto dtof_inf */
	move	$v0, $zero		/* v0 = 0 */

	srl	$v0, $a0, 29			/* v0 = (a0>>29) & 0x00000007 */
	ins	$v0, $a1, 3, 20			/* v0 = (v0 & 0xFF800007) | ((a1 & 0FFFFF)<<3) */
	beq	$v1, $zero, dtof_inf_normal	/* if (v1==0) goto dtof_inf_normal */
dtof_normal:
	srl	$v1, $a1, 31		/* v1 = (a1>>31) & 1 */
	ins	$v0, $v1, 31, 1		/* v0 = (v0 & 0x7FFFFFFF) | (v1 << 31) */
	jr	$ra			/* return */
	ins	$v0, $t1, 23, 8		/* v0 = (v0 & 0x8007FFFF) | (t1 << 23) */
	//mtc1	$v0, $fv0			/* fv0 = v0 */
dtof_denormal:
	sll	$t0, $a1, 12		/* t0 = a1 << 12 */
	srl	$v0, $t0, 10		/* v0 = t0 >> 10 */
	srl	$t0, $a0, 30		/* t0 = t0 >> 30 */
	or	$v0, $v0, t0		/* v0 = v0 | t0 */
	li	$t0, 0x00400000		/* t0 = 0x00400000 */
	or	$v0, $v0, $t0		/* v0 = v0 | t0 */
	subu	$t0, $zero, $t1		/* t0 = zero - t1 */
	sltiu	$t1, $t0, 22		/* t1 = (t0 < 22) */
	beq	$t1, $zero, dtof_min	/* if (t1==0) goto dtof_min */
	srlv	$v0, $v0, $t0		/* v0 = v0 >> t0 */
	srl	$v1, $a1, 31		/* v1 = (a1>>31) & 1 */
	jr	$ra			/* return */
	ins	$v0, $v1, 31, 1		/* v0 = (v0 & 0x7FFFFFFF) | (v1 << 31) */
	//mtc1	$v0, $fv0			/* fv0 = v0 */
dtof_zero:
	sll	$t0, $a1, 12		/* t0 = a1 << 12 */
	or	$t0, $t0, $a0		/* t0 = t0 | a0 */
dtof_min:
	li	$v0, 0x00000001		/* v0 = 0x00000001 */
	movz	$v0, $zero, $t0		/* v0 = (t0==0) ? zero : v0 */
	srl	$t0, $a1, 31		/* t0 = (a1 >> 31) & 1 */
	jr	$ra			/* return */
	ins	$v0, $t0, 31, 1		/* v0 = (v0 & 0x7FFFFFFF) | ((t0<<31) & 0x80000000) */
	//mtc1	$v0, $fv0			/* fv0 = v0 */
dtof_inf_normal:
	nor	$t0, $zero, $a1		/* t0 = ~a1 */
	sll	$t0, $t0, 12		/* t0 = t0 << 12 */
	bne	$t0, $zero, dtof_normal	/* if (t0!=0) goto dtof_normal */
	srl	$t0, $a0, 28		/* t0 = a0 >> 28 */
	sltiu	$t0, $t0, 0xF		/* t0 = (t0 < 0xF) */
	bne	$t0, $zero, dtof_normal	/* if (t0!=0) goto dtof_normal */
	nop				/* waste delay slot */
	j	dtof_inf		/* goto dtof_inf */
	move	$v0, $zero		/* v0 = 0 */
dtof_naninf:
	sll	$t0, $a1, 12		/* t0 = a1 << 12 */
	or	$t1, $t0, $a0		/* t1 = t0 | a0 */
	srl	$v0, $t0, 9		/* v0 = t0 >> 9 */
	srl	$t0, $a0, 29		/* t0 = t0 >> 29 */
	or	$v0, $v0, $t0		/* v0 = v0 | t0 */
	sltiu	$t0, $v0, 1		/* t0 = (v0 < 1) */
	or	$v0, $v0, $t0		/* v0 = v0 | t0 */
	movz	$v0, $zero, $t1		/* v0 = (t1==0) ? zero : v0 */
dtof_inf:
	li	$t0, 0x7F800000		/* t0 = 0x7F800000 */
	or	$v0, $v0, $t0		/* v0 = v0 | t0 */
	srl	$t0, $a1, 31		/* t0 = (a1 >> 31) & 1 */
	jr	$ra			/* return */
	ins	$v0, $t0, 31, 1		/* v0 = (v0 & 0x7FFFFFFF) | ((t0<<31) & 0x80000000) */
	//mtc1	$v0, $fv0			/* fv0 = v0 */

#######################################################################################
	.set pop
