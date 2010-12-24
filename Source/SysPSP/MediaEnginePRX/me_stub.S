// this stub is from the melib package, (c)mrbrown

#include <regdef.h>

#define STATUS	$12
#define CAUSE	$13
#define CONFIG	$16
#define TAGLO	$28
#define TAGHI	$29
#define IXIST	0x01
#define DXIST	0x11

	.global me_stub
	.global me_stub_end
	.set noreorder
	.set noat

me_stub:
	li		k0, 0xbc100000
	li		t0, 7
	sw		t0, 80(k0)
	li		t0, 2
	sw		t0, 64(k0)
	mtc0	zero, TAGLO
	mtc0	zero, TAGHI
	li		k1, 8192
a:
	addi	k1, k1, -64
	bne		k1, zero, a
	cache	IXIST, 0(k1)
	li		k1, 8192
b:
	addi	k1, k1, -64
	bne		k1, zero, b
	cache	DXIST, 0(k1)
	mtc0	zero, CAUSE

	li		t0, 0xbc00002c
	li		t1, 0xbc000000
	li		t2, -1
c:
	sw		t2, 0(t1)
	bne		t1, t0, c
	addiu	t1, t1, 4

	li		k0, 0x20000000
	mtc0	k0, STATUS
	sync
	li		t0, 0xbfc00000
	lw		a0, 0x604(t0)
	lw		k0, 0x600(t0)
	li		sp, 0x80200000
	jr		k0
	nop
me_stub_end:
