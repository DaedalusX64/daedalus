
	.global _DisableFPUExceptions

    .set push 
    .set noreorder 
_DisableFPUExceptions: 
    cfc1    $2, $31 
    lui     $8, 0x80 
    and     $8, $2, $8     # Mask off all bits except for 23 of FCR 
    ctc1    $8, $31 
    jr      $31 
    nop 
    .set pop 
