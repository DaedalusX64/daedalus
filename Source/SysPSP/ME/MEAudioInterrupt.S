// Preserve the complete interrupted renderer/DSP state. EPC already names
// the branch when interrupted in a delay slot; eret replays it unchanged.
.text
.set push
.set noreorder
.set noat
.globl MEAudio_ExceptionEntry
.ent MEAudio_ExceptionEntry
MEAudio_ExceptionEntry:
    addiu $sp, $sp, -304
    sw $k0, 116($sp)
    sw $k1, 120($sp)
    mfc0 $k0, $13
    andi $k1, $k0, 0x7c
    bnez $k1, 2f
    nop
    andi $k1, $k0, 0x8000
    beqz $k1, 3f
    nop
    sw $at, 16($sp)
    sw $v0, 20($sp)
    sw $v1, 24($sp)
    sw $a0, 28($sp)
    sw $a1, 32($sp)
    sw $a2, 36($sp)
    sw $a3, 40($sp)
    sw $t0, 44($sp)
    sw $t1, 48($sp)
    sw $t2, 52($sp)
    sw $t3, 56($sp)
    sw $t4, 60($sp)
    sw $t5, 64($sp)
    sw $t6, 68($sp)
    sw $t7, 72($sp)
    sw $s0, 76($sp)
    sw $s1, 80($sp)
    sw $s2, 84($sp)
    sw $s3, 88($sp)
    sw $s4, 92($sp)
    sw $s5, 96($sp)
    sw $s6, 100($sp)
    sw $s7, 104($sp)
    sw $t8, 108($sp)
    sw $t9, 112($sp)
    sw $gp, 124($sp)
    sw $fp, 128($sp)
    sw $ra, 132($sp)
    mflo $k0
    sw $k0, 136($sp)
    mfhi $k0
    sw $k0, 140($sp)
    cfc1 $k0, $31
    sw $k0, 144($sp)
    swc1 $f0, 152($sp)
    swc1 $f1, 156($sp)
    swc1 $f2, 160($sp)
    swc1 $f3, 164($sp)
    swc1 $f4, 168($sp)
    swc1 $f5, 172($sp)
    swc1 $f6, 176($sp)
    swc1 $f7, 180($sp)
    swc1 $f8, 184($sp)
    swc1 $f9, 188($sp)
    swc1 $f10, 192($sp)
    swc1 $f11, 196($sp)
    swc1 $f12, 200($sp)
    swc1 $f13, 204($sp)
    swc1 $f14, 208($sp)
    swc1 $f15, 212($sp)
    swc1 $f16, 216($sp)
    swc1 $f17, 220($sp)
    swc1 $f18, 224($sp)
    swc1 $f19, 228($sp)
    swc1 $f20, 232($sp)
    swc1 $f21, 236($sp)
    swc1 $f22, 240($sp)
    swc1 $f23, 244($sp)
    swc1 $f24, 248($sp)
    swc1 $f25, 252($sp)
    swc1 $f26, 256($sp)
    swc1 $f27, 260($sp)
    swc1 $f28, 264($sp)
    swc1 $f29, 268($sp)
    swc1 $f30, 272($sp)
    swc1 $f31, 276($sp)
    jal MEAudio_Interrupt
    nop
    mfc0 $k0, $9
    li $k1, 16650
    addu $k0, $k0, $k1
    mtc0 $k0, $11
    sync
    lwc1 $f0, 152($sp)
    lwc1 $f1, 156($sp)
    lwc1 $f2, 160($sp)
    lwc1 $f3, 164($sp)
    lwc1 $f4, 168($sp)
    lwc1 $f5, 172($sp)
    lwc1 $f6, 176($sp)
    lwc1 $f7, 180($sp)
    lwc1 $f8, 184($sp)
    lwc1 $f9, 188($sp)
    lwc1 $f10, 192($sp)
    lwc1 $f11, 196($sp)
    lwc1 $f12, 200($sp)
    lwc1 $f13, 204($sp)
    lwc1 $f14, 208($sp)
    lwc1 $f15, 212($sp)
    lwc1 $f16, 216($sp)
    lwc1 $f17, 220($sp)
    lwc1 $f18, 224($sp)
    lwc1 $f19, 228($sp)
    lwc1 $f20, 232($sp)
    lwc1 $f21, 236($sp)
    lwc1 $f22, 240($sp)
    lwc1 $f23, 244($sp)
    lwc1 $f24, 248($sp)
    lwc1 $f25, 252($sp)
    lwc1 $f26, 256($sp)
    lwc1 $f27, 260($sp)
    lwc1 $f28, 264($sp)
    lwc1 $f29, 268($sp)
    lwc1 $f30, 272($sp)
    lwc1 $f31, 276($sp)
    lw $k0, 144($sp)
    ctc1 $k0, $31
    lw $k0, 136($sp)
    mtlo $k0
    lw $k0, 140($sp)
    mthi $k0
    lw $at, 16($sp)
    lw $v0, 20($sp)
    lw $v1, 24($sp)
    lw $a0, 28($sp)
    lw $a1, 32($sp)
    lw $a2, 36($sp)
    lw $a3, 40($sp)
    lw $t0, 44($sp)
    lw $t1, 48($sp)
    lw $t2, 52($sp)
    lw $t3, 56($sp)
    lw $t4, 60($sp)
    lw $t5, 64($sp)
    lw $t6, 68($sp)
    lw $t7, 72($sp)
    lw $s0, 76($sp)
    lw $s1, 80($sp)
    lw $s2, 84($sp)
    lw $s3, 88($sp)
    lw $s4, 92($sp)
    lw $s5, 96($sp)
    lw $s6, 100($sp)
    lw $s7, 104($sp)
    lw $t8, 108($sp)
    lw $t9, 112($sp)
    lw $k0, 116($sp)
    lw $k1, 120($sp)
    lw $gp, 124($sp)
    lw $fp, 128($sp)
    lw $ra, 132($sp)
    addiu $sp, $sp, 304
    eret
    nop
    nop
    nop
    nop
    nop
    nop
    nop
2:
    j MEGraphics_HandleFault
    nop
3:
    lui $k0, %hi(MEAudio_PreviousExceptionVector)
    lw $k0, %lo(MEAudio_PreviousExceptionVector)($k0)
    lw $k1, 120($sp)
    addiu $sp, $sp, 304
    jr $k0
    lw $k0, -188($sp)
.end MEAudio_ExceptionEntry
.set pop
