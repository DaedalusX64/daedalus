#include "as_reg_compat.h"

.text
	.set		push
	.set		noreorder
	.set		noat

.macro TransformWithColourM	function, fog_mode, texture_mode
.global \function
\function:

############################
#	a0 - world matrix				- must be aligned to 16 bytes
#	a1 - world projection matrix	- must be aligned to 16 bytes
#	a2 - Fiddled vertices			- stride 16
#	a3 - Output vertices			- must be aligned to 16 bytes, stride 64
#	t0 - num vertices
#	t1 - params

	lv.q		R000, 0($a0)		// Load mat world
	lv.q		R001, 16($a0)
	lv.q		R002, 32($a0)
	lv.q		R003, 48($a0)

	lv.q		R100, 0($a1)		// Load mat project
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)
	lv.q		R103, 48($a1)
	
	lv.q		R701, 16($t1)		// Load params [fog_m, fog_o, tscale_x, tscale_y]
	
	// XXXX Todo - can this be improved?
	# Load 1/256 (vuc2i/vi2f end up converting 0xff to 256.0)
	vfim.s		S203, 0.00390625

	sll			$t0, $t0, 4			// count = count * 16
	addu		$t0, $a2, $t0		// end_ptr = start_ptr + count * 16

	beq			$a2, $t0, finished_colour_\function
	nop

next_vertex_colour_\function:
	# Load and transform this vertex position
 	lv.s		S200, 0($a2)					// load word [y,x,?,z]
 	lv.s		S210, 4($a2)					//		should align this to 16 bytes so we can do a single load?
	vs2i.p		R200, R200						// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 16					// int -> float
	vmov.q		R200, R200[y,x,w,1]				// Have to permute here, as sadly can't do this with first vtfm4.q
 
	vtfm4.q		R201, M000, R200				// World transform
	vtfm4.q		R202, M100, R200				// Projection transform
	
	sv.q		R201, 0($a3)					// Store world transform
	sv.q		R202, 16($a3)					// Store projection transform
	
.if \fog_mode == 1
	#	Calculate the Colour alpha value from the foq. We do this while the projected point
	#	is still loaded, and merge it in after the rgb components are calculated by the lighting calc below
	#	float eyespace_z = projected.z / projected.w;
	#	fog_coeff = (eyespace_z * m_fFogMult) + m_fFogOffset;
	#	mVtxProjected[i].Colour.w = Clamp< f32 >( fog_coeff, 0.0f, 1.0f );
	vmov.s		S200, S232					// get w component
	vmov.s		S201, S222					// v = z
	vrcp.s		S200, S200					// 1.0 / projected.w
	vmul.s		S201, S201, S701			// v = z * fog_mult
	vmul.s		S201, S201, S200			// v = z*fog_mult*(1/w)
	vadd.s		S600[0:1], S201, S710		// v = z*fog_mult*(1/w) + fog_offset
.endif

	# Compute the clip flags
	vcmp.q		LT, R202, R202[-w,-w,-w,0]		// x < -w, y < -w, z < -w
	vnop
	mfvc		$t4, $131						// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG (will become X_POS/Y_POS/Z_POS later)
	andi		$t4, $t4, 0x7					// Mask out the condition codes we don't care about
	sll			$t4, $t4, 3						// Shift up to create X_POS/Y_POS/Z_POS	
	
	vcmp.q		GT, R202, R202[w,w,w,0]			// x > w, y > w, z > w
	vnop
	mfvc		$t5, $131						// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t5, $t5, 0x7					// Mask out the condition codes we don't care about
	or			$t4, $t4, $t5

	sw			$t4, 56($a3)					// Store ClipFlags
	
	
.if \texture_mode == 0
	# Nothing to do

.elseif \texture_mode == 1
# Textured
#	t.x = (float)v.tu * mTextureScale.x
#	t.y = (float)v.tv * mTextureScale.y

	lv.s		S202, 8($a2)				// load texture word [tv,tu] (N.B. due to swizzling these are 'backwards' from what you might expect)
	vs2i.s		R202, S202
	vi2f.q		R202, R202, 16				// int -> float
	vmul.q		R202, R202, R701[w,z,0,0]	// multiply by mTextureScale
	sv.s		S212, 0x30($a3)				// Store Texture.x
	sv.s		S202, 0x34($a3)				// Store Texture.y
	
.endif
	
	# Load and normalise the RGBA colour
	lv.s		S200, 12($a2)					// load colour word [a,b,g,r]
	.word		0xd0380000 | (8<<8) | (40)		// vuc2i.s	R200, S200					// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 23					// int -> float
	vscl.q		R200, R200[w,z,y,x], S203		// R200 = [r * 1/256, g * 1/256, b * 1/256, a * 1/256]
	
.if \fog_mode == 1
	# Merge in the computed fog colour
	vmov.s		S230, S600
.endif

	sv.q		R200, 32($a3)					// Store colour

	# Continue with the next vertex
	addiu		$a2, $a2, 16		// Next input vertex
	bne			$a2, $t0, next_vertex_colour_\function
	addiu		$a3, $a3, 64		// Next output vertex
	
finished_colour_\function:
	jr			$ra
	nop

.endm

TransformWithColourM _TransformVerticesWithColour_f0_t0, 0, 0
TransformWithColourM _TransformVerticesWithColour_f0_t1, 0, 1
TransformWithColourM _TransformVerticesWithColour_f1_t0, 1, 0
TransformWithColourM _TransformVerticesWithColour_f1_t1, 1, 1

	.set pop

