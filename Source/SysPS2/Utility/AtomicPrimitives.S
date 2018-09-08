#include "as_reg_compat.h"

	.set noat

	.global _AtomicIncrement
	.global _AtomicDecrement
	.global _AtomicBitSet

    .set push 
    .set noreorder 

###############################################################################
#
#	Atomically increment the int pointed to by a0.
#	Not safe for calling from the ME.
#
#	Arguments
#		a0		p		- pointer to int to adjust. Must be cached. 
#
#	Returns
#		Incremented value
#
_AtomicIncrement: 
	ll		$t1, 0($a0)
	add		$t2, $t1, 1
	or		$v0, $t2, $0		# Copy for return value
	sc		$t2, 0($a0)
	beq		$t2, 0, _AtomicIncrement
	nop
	jr		$ra
	nop

###############################################################################
#
#	Atomically decrement the int pointed to by a0.
#	Not safe for calling from the ME.
#
#	Arguments
#		a0		p		- pointer to int to adjust. Must be cached. 
#
#	Returns
#		Decremented value
#
_AtomicDecrement: 
	ll		$t1, 0($a0)
	add		$t2, $t1, -1
	or		$v0, $t2, $0		# Copy for return value
	sc		$t2, 0($a0)
	beq		$t2, 0, _AtomicDecrement
	nop
	jr		$ra
	nop

###############################################################################
#
#	Atomically clear and set bits in the int pointed to by a0.
#	Performs '*p = (*p & and_bits) | or_bits' atomically.
#	Not safe for calling from the ME.
#
#	Arguments
#		a0		p			- pointer to int to adjust. Must be cached. 
#		a1		and_bits	- bits to and with
#		a2		or_bits		- bits to or with
#
#	Returns
#		Resulting value
#
_AtomicBitSet: 
	ll		$t1, 0($a0)
	and		$t2, $t1, $a1
	or		$t3, $t2, $a2
	or		$v0, $t3, $0		# Copy for return value
	sc		$t3, 0($a0)
	beq		$t3, 0, _AtomicBitSet
	nop
	jr		$ra
	nop


    .set pop 

