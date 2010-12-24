#include "as_reg_compat.h"

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

	# Round 'start' pointer down to nearest 64 bytes
	and			$a0, $a0, $t0

	# Round 'end' pointer up to next 64 bytes boundary
	addiu		$a1, $a1, 63
	and			$a1, $a1, $t0

	# Do while current < end
invalidate_next:
	cache		8, 0($a0)						# 8 is for icache invalidate
	addiu		$a0, $a0, 64
	bne			$a0, $a1, invalidate_next		# Keep looping until 
	nop

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
#	lw		$t0, 0x40c($fp)		# StuffToDo
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
	lw		$t0, 0x148($fp)		# COUNT register
	lw		$t1, 0x420($fp)		# Events[0].mCount

	addu	$t0, $t0, $a0		# COUNT + ops_executed
	sw		$t0, 0x148($fp)		# COUNT = COUNT + ops_executed

	sw		$a1, 0x400($fp) 	# CurrentPC
	sw		 $0, 0x408($fp)		# Delay = NO_DELAY
	
	subu	$t1, $t1, $a0		# Events[0].mCount - ops_executed
	blez	$t1, _DirectExitCheckCheckCount
	sw		$t1, 0x420($fp)		# Events[0].mCount = Events[0].mCount - ops_executed

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
	lw		$t0, 0x148($fp)		# COUNT register
	lw		$t1, 0x420($fp)		# Events[0].mCount

	addu	$t0, $t0, $a0		# COUNT + ops_executed
	sw		$t0, 0x148($fp)		# COUNT = COUNT + ops_executed
	
	sw		$a1, 0x400($fp) 	# CurrentPC
	sw		$a2, 0x404($fp) 	# TargetPC
	li		$t0, 1				# EXEC_DELAY
	sw		$t0, 0x408($fp)		# Delay

	subu	$t1, $t1, $a0		# Events[0].mCount - ops_executed
	blez	$t1, _DirectExitCheckCheckCount
	sw		$t1, 0x420($fp)		# Events[0].mCount = Events[0].mCount - ops_executed

	jr		$ra
	nop

#######################################################################################
#	Utility routine for _DirectExitCheckXX. 
#
_DirectExitCheckCheckCount:
	or		$s0, $ra, $0		# Keep track of return addresss
	jal		CPU_HANDLE_COUNT_INTERRUPT
	nop

	lw		$t0, 0x40c($fp)		# StuffToDo
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
	sw		$a2, 0x400($fp) 	# CurrentPC
	sw		$0,  0x408($fp)		# Delay (NO_DELAY)
	
	jal		CPU_UpdateCounter	# a0 holds instructions executed
	nop

	lw		$t0, 0x40c($fp)		# StuffToDo
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

	sw		$a1, 0x400($fp) 	# CurrentPC

	srl		$v1, $a0, 0x12
	sll		$v1, $v1, 0x2
	la		$v0, g_ReadAddressLookupTableForDynarec
	lw		$v0, 0($v0)			# The above is a pointer to our table
	addu	$v1, $v1, $v0
	lw		$v0, 0($v1)
	
	
	jalr	$v0
	nop
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

	sw		$a1, 0x400($fp) 	# CurrentPC
	li		$t0, 1				# EXEC_DELAY
	sw		$t0, 0x408($fp)		# Delay

	srl		$v1, $a0, 0x12
	sll		$v1, $v1, 0x2
	la		$v0, g_ReadAddressLookupTableForDynarec
	lw		$v0, 0($v0)			# The above is a pointer to our table
	addu	$v1, $v1, $v0
	lw		$v0, 0($v1)
	
	
	jalr	$v0
	nop
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0

	sw		$0, 0x408($fp)		# Delay <- NO_DELAY
	
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
	
	sw		$a2, 0x400($fp) 	# CurrentPC
	
	jal		Write32BitsForDynaRec
	nop
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	nop

_WriteBitsDirectBD_u32:
	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)
	
	sw		$a2, 0x400($fp) 	# CurrentPC
	li		$t0, 1				# EXEC_DELAY
	sw		$t0, 0x408($fp)		# Delay

	jal		Write32BitsForDynaRec
	nop
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	sw		$0, 0x408($fp)		# Delay <- NO_DELAY
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	nop
	
_WriteBitsDirect_u16:
	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)
	
	sw		$a2, 0x400($fp) 	# CurrentPC
	
	jal		Write16BitsForDynaRec
	nop
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	nop
	
	
_WriteBitsDirectBD_u16:
	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)
	
	sw		$a2, 0x400($fp) 	# CurrentPC
	li		$t0, 1				# EXEC_DELAY
	sw		$t0, 0x408($fp)		# Delay
	
	jal		Write16BitsForDynaRec
	nop
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	sw		$0, 0x408($fp)		# Delay <- NO_DELAY
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	nop
	
_WriteBitsDirect_u8:
	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)
	
	sw		$a2, 0x400($fp) 	# CurrentPC
	
	jal		Write8BitsForDynaRec
	nop
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	nop		
	
_WriteBitsDirectBD_u8:
	la		$t0, temp_storage_for_ra
	sw		$ra, 0($t0)
	
	sw		$a2, 0x400($fp) 	# CurrentPC
	li		$t0, 1				# EXEC_DELAY
	sw		$t0, 0x408($fp)		# Delay
	
	jal		Write8BitsForDynaRec
	nop
	
	jal		_ReturnFromDynaRecIfStuffToDo
	move	$a0,$0
	
	sw		$0, 0x408($fp)		# Delay <- NO_DELAY
	
	la		$t0, temp_storage_for_ra
	lw		$ra, 0($t0)
	jr		$ra
	nop			
	
#######################################################################################
	.set pop
