// Copyright (C) 2001 StrmnNrmn
// Copyright (C) 2011 Corn

#include "as_reg_compat.h"

#define PARAMS_AMBIENT		0x00
#define PARAMS_FLAGS_NLIGHT_TXSCAL	0x10
#define PARAMS_LIGHTS		0x20

#define	TNL_LIGHT			0x0001
#define	TNL_TEXTURE			0x0002
#define	TNL_TEXGEN			0x0004
#define	TNL_TEXGENLIN		0x0008
#define	TNL_FOG				0x0010
#define	TNL_SHADE			0x0020
#define	TNL_ZBUFFER			0x0040
#define	TNL_TRICULL			0x0080
#define	TNL_CULLBACK		0x0100
	
.text
.set		push
.set		noreorder
.set		noat

############################
.global _TnLVFPU
############################
#	a0 - world matrix				- must be aligned to 16 bytes
#	a1 - world*projection matrix	- must be aligned to 16 bytes
#	a2 - Fiddled vertices			- stride 16
#	a3 - Output vertices			- must be aligned to 16 bytes, stride 64
#	t0 - num vertices
#	t1 - params

# Lighting calculation
# M000: World Matrix
# M100: Projection Matrix
# R200: Material normal 
# R201: Accumulated colour
# R202: ?
# R203: ?
# R300: ?
# R301: Light normal
# R302: Light colour
# R303: Scratch
# R431: current vertex Alpha value
# R700: Ambient
# R721: Texture X & Y scale
# t4 = cur_light
# t6 = first_light
# t7 = last_light
# v0 = TnLFlags

_TnLVFPU:
	lv.q		R000, 0($a0)		// Load mat world
	lv.q		R001, 16($a0)
	lv.q		R002, 32($a0)
	lv.q		R003, 48($a0)

	lv.q		R100, 0($a1)		// Load mat project
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)
	lv.q		R103, 48($a1)
	
	lv.q		R700, PARAMS_AMBIENT($t1)		// Load ambient
	lv.q		R701, PARAMS_FLAGS_NLIGHT_TXSCAL($t1)	// Load params [Flags, Num_lights, tscale_x, tscale_y]
	mfv			$v0, S701						// TnL flags
	mfv			$t7, S711						// Num_lights
	addiu		$t6, $t1, PARAMS_LIGHTS			// pointer to first_light = p_lights
	
# Calculate the last light index
	sll			$t7, $t7, 5			// num_lights*32
	
	sll			$t0, $t0, 4			// count = count * 16
	addu		$t0, $a2, $t0		// end_ptr = start_ptr + count * 16
	beq			$a2, $t0, finished_
	addu		$t7, $t6, $t7		// last_light = p_lights + num_lights*32

next_vertex_:
# Load and transform this vertex position
 	lv.s		S200, 0($a2)				// load word [y,x,?,z]
 	lv.s		S210, 4($a2)				// should align this to 16 bytes so we can do a single load?
	vs2i.p		R200, R200					// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 16				// int -> float
	vmov.q		R200, R200[y,x,w,1]
	vtfm4.q		R201, M000, R200			// World transform
	vtfm4.q		R202, M100, R200			// Projection transform
	sv.q		R201, 0x00($a3)				// Store world transform
	sv.q		R202, 0x10($a3)				// Store projection transform
	
# Compute the clip flags
	vcmp.q		LT, R202, R202[-w,-w,-w,0]	// x < -w, y < -w, z < -w
	vnop
	mfvc		$t4, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG (will become X_POS/Y_POS/Z_POS later)
	andi		$t4, $t4, 0x7				// Mask out the condition codes we don't care about
	sll			$t4, $t4, 3					// Shift up to create X_POS/Y_POS/Z_POS	
	vcmp.q		GT, R202, R202[w,w,w,0]		// x > w, y > w, z > w
	vnop
	mfvc		$t5, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t5, $t5, 0x7				// Mask out the condition codes we don't care about
	or			$t4, $t4, $t5
	sw			$t4, 0x38($a3)				// Store ClipFlags

#Load vertex Normal or Color
	andi		$t4, $v0, TNL_LIGHT			// if( TNL_LIGHT )
	beqz		$t4, do_color_
	lv.s		S200, 12($a2)				// load normal word [w,z,y,x]
	
#Do lighting Convert the alpha in R200 to float and pass it along to light color
	.word		0xd0380000 | (8<<8) | (43)	// vuc2i.s	R203, S200					// R200 = [?,z,y,x]
	vi2f.s		S431, S203, 31				// int -> float, R431 = [a * 1/256]
	
# Convert the normal in R200 to float and transform
	.word		0xd0398080 | (8<<8) | (40)	// vc2i.s		R200, S200					// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 0				// int -> float (obliterates world transform)
	vmov.q		R201, R200[w,z,y,0]			// Unfiddle
	vmov.q		R203, R201					// store vertice normal temporary for env map later
	vtfm3.t		R200, M000, R201			// Transform with world matrix (only need 3x3)//Corn
	vdot.t		S201, R200, R200			// S201 = x*x + y*y + z*z
	vrsq.s		S201, S201					// S201 = 1/sqrt(x*x + y*y + z*z)
	vscl.t		R200, R200, S201			// S200 = v.normalise().

	vmov.q		R201, R700					// Colour = ambient
	beq			$t6, $t7, done_lighting_	// cur_light == last_light?
	or			$t4, $t6, $0				// cur_light = p_lights
next_light_:
	lv.q		R301, 0($t4)				// Load Light normal
	vdot.t		S303[0:1], R200, R301		// x = clamp(dot(normal,(x,y,z)),0,1)
	lv.q		R302, 16($t4)				// Load Light colour
	addiu		$t4, $t4, 32				// Skip to the next light
	vscl.t		R303, R302, S303			// r,g,b = r*x, g*x, b*x
	bne			$t4, $t7, next_light_
	vadd.t		R201, R201, R303			// col += r,g,b,a

done_lighting_:
	vmov.t		R401[0:1,0:1,0:1], R201		// Clamp 0..1 and merge with vertex alpha in S431

	andi		$t4, $v0, TNL_TEXGEN		// if( TNL_TEXGEN )
	beqz		$t4, do_texture_
	sv.q		R401, 0x20($a3)				// Store colour

# We use worldproject matrix to calc normals it gives a nicer effect (model view result is in R200) //Corn
	vtfm3.t		R202, M100, R203					// Transform with projworld matrix, looks nicer (only need 3x3)
	vdot.t		S201, R202, R202					// S201 = x*x + y*y + z*z
	vrsq.s		S201, S201							// S201 = 1/sqrt(x*x + y*y + z*z)

	andi		$t4, $v0, TNL_TEXGENLIN				// if( TNL_TEXGENLIN )
	beqz		$t4, do_texgen_
	vscl.p		R202, R202, S201					// R202 = v.normalise() (x & y).

# EnvMapped G_TEXTURE_GEN  t.x = 0.5 * (1.0 + n.x) t.y = 0.25 * (1.0 + n.y)
	vadd.p		R202, R202[1,1], R202				// 1+x, 1+y
	vmul.p		R202, R202[1/2,1/2], R202			// X * 0.5, Y * 0.25
	sv.s		S202, 0x30($a3)						// Store Texture.x
	b			vtx_done_
	sv.s		S212, 0x34($a3)						// Store Texture.y
		
do_texgen_:
# EnvMapped G_TEXTURE_GEN_LINEAR Cheap way to do acos(x)/PI -> 0.5f - 0.25f * absf(x) - 0.25f * absf(x) * absf(x) * absf(x) //Corn
	vmov.p		R202, R202[|x|,|y|]					// absf(x), absf(y)
	vmov.p		R203, R200[1/2,1/2]					// result = 0.5
	vmul.p		R222, R202[1/4,1/4], R202			// X * 0.25, Y * 0.25
	vsub.p		R203, R203, R222					// result -= X * 0.25
	vmul.p		R222, R202, R222					// X * X * 0.25, Y * Y * 0.25
	vmul.p		R222, R202, R222					// X * X * X * 0.25, Y * Y * Y * 0.25
	vsub.p		R203, R203, R222					// result -= X * X * X * 0.25
	sv.s		S203, 0x30($a3)						// Store Texture.x
	b			vtx_done_
	sv.s		S213, 0x34($a3)						// Store Texture.y
		
do_color_:
# Normalise the RGBA colour
	.word		0xd0380000 | (8<<8) | (40)		// vuc2i.s	R200, S200					// R200 = [a,b,g,r]
	vi2f.q		R200, R200[w,z,y,x], 31			// int -> float, R200 = [r * 1/256, g * 1/256, b * 1/256, a * 1/256]
	sv.q		R200, 0x20($a3)					// Store colour

do_texture_:
# Textured t.x = (float)v.tu * mTextureScale.x 	t.y = (float)v.tv * mTextureScale.y
	lv.s		S202, 8($a2)				// load texture word [tv,tu] (N.B. due to swizzling these are 'backwards' from what you might expect)
	vs2i.s		R202, S202
	vi2f.p		R202, R202, 16				// int -> float
	vmul.p		R202, R202, R721[y,x]		// multiply by mTextureScale
	sv.s		S212, 0x30($a3)				// Store Texture.x
	sv.s		S202, 0x34($a3)				// Store Texture.y

vtx_done_:
# Continue with the next vertex
	addiu		$a2, $a2, 16				// Next input vertex
	bne			$a2, $t0, next_vertex_
	addiu		$a3, $a3, 64				// Next output vertex

finished_:	
	jr			$ra
	nop

############################
.global _TnLVFPUCBFD
############################
#	a0 - world matrix				- must be aligned to 16 bytes
#	a1 - world*projection matrix	- must be aligned to 16 bytes
#	a2 - Fiddled vertices			- stride 16
#	a3 - Output vertices			- must be aligned to 16 bytes, stride 64
#	t0 - num vertices
#	t1 - params
#	t2 - model normal pointer
#	t3 - v0

# Lighting calculation
# M000: World Matrix
# M100: Projection Matrix
# R200: Material normal 
# R201: Accumulated colour
# R202: ?
# R203: ?
# R300: ?
# R303: Scratch
# R700: Ambient
# R721: Texture X & Y scale
# v0 = TnLFlags

_TnLVFPUCBFD:
	lv.q		R000, 0($a0)		// Load mat world
	lv.q		R001, 16($a0)
	lv.q		R002, 32($a0)
	lv.q		R003, 48($a0)

	lv.q		R100, 0($a1)		// Load mat project
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)
	lv.q		R103, 48($a1)
	
	lv.q		R701, PARAMS_FLAGS_NLIGHT_TXSCAL($t1)	// Load params [Flags, Num_lights, tscale_x, tscale_y]
	
# Calculate the last vertex index
	sll			$t0, $t0, 4			// count = count * 16
	addu		$t0, $a2, $t0		// end_ptr = start_ptr + count * 16
	beq			$a2, $t0, finished_CBFD
	mfv			$v0, S701						// TnL flags
	
next_vertex_CBFD:
# Load and transform this vertex position
 	lv.s		S200, 0($a2)				// load word [y,x,?,z]
 	lv.s		S210, 4($a2)				// should align this to 16 bytes so we can do a single load?
 	lb			$v1, 4($a2)					// Get vert_norm z
	vs2i.p		R200, R200					// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 16				// int -> float
	vmov.q		R200, R200[y,x,w,1]
	vtfm4.q		R201, M000, R200			// World transform
	vtfm4.q		R202, M100, R200			// Projection transform
	sv.q		R201, 0x00($a3)				// Store world transform
	sv.q		R202, 0x10($a3)				// Store projection transform
	
# Compute the clip flags
	vcmp.q		LT, R202, R202[-w,-w,-w,0]	// x < -w, y < -w, z < -w
	vnop
	mfvc		$t4, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG (will become X_POS/Y_POS/Z_POS later)
	andi		$t4, $t4, 0x7				// Mask out the condition codes we don't care about
	sll			$t4, $t4, 3					// Shift up to create X_POS/Y_POS/Z_POS	
	vcmp.q		GT, R202, R202[w,w,w,0]		// x > w, y > w, z > w
	vnop
	mfvc		$t5, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t5, $t5, 0x7				// Mask out the condition codes we don't care about
	or			$t5, $t4, $t5

#Load vertex Normal or Color
	andi		$t4, $v0, TNL_LIGHT			// if( TNL_LIGHT )
	beqz		$t4, do_texture_CBFD
	sw			$t5, 0x38($a3)				// Store ClipFlags

	andi		$t4, $v0, TNL_TEXGEN		// if( TNL_TEXGEN )
	beqz		$t4, do_texture_CBFD
	mtv			$v1, S223					// Store vertice normal Z 
	
# We use world matrix to calc normals
 	xori		$t5, $t3, 0x3				// = v0 ^ 3
	addu		$t5, $t2, $t5				// += base address
	lb			$t4, 0($t5)					// get normal x
	mtv			$t4, S203					// Store vertice normal X 
	addiu		$t5, $t3, 0x1				// = v0 + 1
 	xori		$t5, $t5, 0x3				// ^= 3
	addu		$t5, $t2, $t5				// += base address
	lb			$t4, 0($t5)					// get normal y
	mtv			$t4, S213					// Store vertice normal Y 
	vi2f.t		R203, R203, 0				// int -> float
	vtfm3.t		R202, M000, R203					// Transform with world matrix, (only need 3x3)
	vdot.t		S201, R202, R202					// S201 = x*x + y*y + z*z
	vrsq.s		S201, S201							// S201 = 1/sqrt(x*x + y*y + z*z)

	andi		$t4, $v0, TNL_TEXGENLIN				// if( TNL_TEXGENLIN )
	beqz		$t4, do_texgen_CBFD
	vscl.p		R202, R202, S201					// R202 = v.normalise() (x & y).

# EnvMapped G_TEXTURE_GEN_LINEAR Cheap way to do acos(x)/PI -> 0.5f - 0.25f * absf(x) - 0.25f * absf(x) * absf(x) * absf(x) //Corn
	vmov.p		R203, R200[1/2,1/2]					// result = 0.5
	vmul.p		R222, R202[1/4,1/4], R202			// X * 0.25, Y * 0.25
	vsub.p		R203, R203, R222					// result -= X * 0.25
	vmul.p		R222, R202, R222					// X * X * 0.25, Y * Y * 0.25
	vmul.p		R222, R202, R222					// X * X * X * 0.25, Y * Y * Y * 0.25
	vsub.p		R203, R203, R222					// result -= X * X * X * 0.25
	sv.s		S203, 0x30($a3)						// Store Texture.x
	b			vtx_done_CBFD
	sv.s		S213, 0x34($a3)						// Store Texture.y
		
do_texgen_CBFD:
# EnvMapped G_TEXTURE_GEN  t.x = 0.5 * (1.0 + n.x) t.y = 0.5 * (1.0 + n.y)
	vadd.p		R202, R202[1,1], R202				// 1+x, 1+y
	vmul.p		R202, R202[1/2,1/2], R202			// X * 0.5, Y * 0.25
	sv.s		S202, 0x30($a3)						// Store Texture.x
	b			vtx_done_CBFD
	sv.s		S212, 0x34($a3)						// Store Texture.y
		
do_texture_CBFD:
# Textured t.x = (float)v.tu * mTextureScale.x 	t.y = (float)v.tv * mTextureScale.y
	lv.s		S202, 8($a2)				// load texture word [tv,tu] (N.B. due to swizzling these are 'backwards' from what you might expect)
	vs2i.s		R202, S202
	vi2f.p		R202, R202, 16				// int -> float
	vmul.p		R202, R202, R721[y,x]		// multiply by mTextureScale
	sv.s		S212, 0x30($a3)				// Store Texture.x
	sv.s		S202, 0x34($a3)				// Store Texture.y

vtx_done_CBFD:
# Normalise the RGBA colour
	lv.s		S200, 12($a2)					// load normal word [w,z,y,x]
	.word		0xd0380000 | (8<<8) | (40)		// vuc2i.s	R200, S200					// R200 = [a,b,g,r]
	vi2f.q		R200, R200[w,z,y,x], 31			// int -> float, R200 = [r * 1/256, g * 1/256, b * 1/256, a * 1/256]
	sv.q		R200, 0x20($a3)					// Store colour

# Continue with the next vertex
	addiu		$t3, $t3, 2					// inc v0 counter
	addiu		$a2, $a2, 16				// Next input vertex
	bne			$a2, $t0, next_vertex_CBFD
	addiu		$a3, $a3, 64				// Next output vertex

finished_CBFD:	
	jr			$ra
	nop
	
############################
.global _TnLVFPUPD
############################
#	a0 - world matrix				- must be aligned to 16 bytes
#	a1 - world*projection matrix	- must be aligned to 16 bytes
#	a2 - Fiddled vertices			- stride 16
#	a3 - Output vertices			- must be aligned to 16 bytes, stride 64
#	t0 - num vertices
#	t1 - params
#	t2 - CI pointer

# Lighting calculation
# M000: World Matrix
# M100: Projection Matrix
# R200: Material normal 
# R201: Accumulated colour
# R202: ?
# R203: ?
# R300: ?
# R301: Light normal
# R302: Light colour
# R303: Scratch
# R431: current vertex Alpha value
# R700: Ambient
# R721: Texture X & Y scale
# t4 = cur_light
# t6 = first_light
# t7 = last_light
# v0 = TnLFlags
# v1 = color index pointer

_TnLVFPUPD:
	lv.q		R000, 0($a0)		// Load mat world
	lv.q		R001, 16($a0)
	lv.q		R002, 32($a0)
	lv.q		R003, 48($a0)

	lv.q		R100, 0($a1)		// Load mat project
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)
	lv.q		R103, 48($a1)
	
	lv.q		R700, PARAMS_AMBIENT($t1)		// Load ambient
	lv.q		R701, PARAMS_FLAGS_NLIGHT_TXSCAL($t1)	// Load params [Flags, Num_lights, tscale_x, tscale_y]
	mfv			$v0, S701						// TnL flags
	mfv			$t7, S711						// Num_lights
	addiu		$t6, $t1, PARAMS_LIGHTS			// pointer to first_light = p_lights
	
# Calculate the last light index
	sll			$t7, $t7, 5			// num_lights*32
	
	sll			$v1, $t0, 2			// count * 4
	sll			$t0, $t0, 3			// count * 8
	addu		$t0, $v1, $t0		// count = count * 12
	addu		$t0, $a2, $t0		// end_ptr = start_ptr + count * 12
	beq			$a2, $t0, finished_PD
	addu		$t7, $t6, $t7		// last_light = p_lights + num_lights*32

next_vertex_PD:
# Load and transform this vertex position
 	lv.s		S200, 0($a2)				// load word [y,x,?,z]
 	lv.s		S210, 4($a2)				// should align this to 16 bytes so we can do a single load?
	vs2i.p		R200, R200					// R200 = [?,z,y,x]
 	lbu			$v1, 4($a2)					// Get Cindx
	addu		$v1, $v1, $t2				// pointer = base vector + Cindx
	vi2f.q		R200, R200, 16				// int -> float
	vmov.q		R200, R200[y,x,w,1]
	vtfm4.q		R201, M000, R200			// World transform
	vtfm4.q		R202, M100, R200			// Projection transform
	sv.q		R201, 0x00($a3)				// Store world transform
	sv.q		R202, 0x10($a3)				// Store projection transform
	
# Compute the clip flags
	vcmp.q		LT, R202, R202[-w,-w,-w,0]	// x < -w, y < -w, z < -w
	vnop
	mfvc		$t4, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG (will become X_POS/Y_POS/Z_POS later)
	andi		$t4, $t4, 0x7				// Mask out the condition codes we don't care about
	sll			$t4, $t4, 3					// Shift up to create X_POS/Y_POS/Z_POS	
	vcmp.q		GT, R202, R202[w,w,w,0]		// x > w, y > w, z > w
	vnop
	mfvc		$t5, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t5, $t5, 0x7				// Mask out the condition codes we don't care about
	or			$t4, $t4, $t5
	sw			$t4, 0x38($a3)				// Store ClipFlags

#Load vertex Normal or Color
	andi		$t4, $v0, TNL_LIGHT			// if( TNL_LIGHT )
	beqz		$t4, do_color_PD
	lv.s		S200, 0($v1)				// load normal word [w,z,y,x]
	
#Do lighting Convert the alpha in R200 to float and pass it along to light color
	.word		0xd0380000 | (8<<8) | (43)	// vuc2i.s	R203, S200					// R200 = [?,z,y,x]
	vi2f.s		S431, S203, 31				// int -> float, R431 = [a * 1/256]
	
# Convert the normal in R200 to float and transform
	.word		0xd0398080 | (8<<8) | (40)	// vc2i.s		R200, S200					// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 0				// int -> float (obliterates world transform)
	vmov.q		R201, R200[w,z,y,0]			// Unfiddle
	vmov.q		R203, R201					// store vertice normal temporary for env map later
	vtfm3.t		R200, M000, R201			// Transform with world matrix (only need 3x3)//Corn
	vdot.t		S201, R200, R200			// S201 = x*x + y*y + z*z
	vrsq.s		S201, S201					// S201 = 1/sqrt(x*x + y*y + z*z)
	vscl.t		R200, R200, S201			// S200 = v.normalise().

	vmov.q		R201, R700					// Colour = ambient
	beq			$t6, $t7, done_lighting_PD	// cur_light == last_light?
	or			$t4, $t6, $0				// cur_light = p_lights
next_light_PD:
	lv.q		R301, 0($t4)				// Load Light normal
	vdot.t		S303[0:1], R200, R301		// x = clamp(dot(normal,(x,y,z)),0,1)
	lv.q		R302, 16($t4)				// Load Light colour
	addiu		$t4, $t4, 32				// Skip to the next light
	vscl.t		R303, R302, S303			// r,g,b = r*x, g*x, b*x
	bne			$t4, $t7, next_light_PD
	vadd.t		R201, R201, R303			// col += r,g,b,a

done_lighting_PD:
	vmov.t		R401[0:1,0:1,0:1], R201		// Clamp 0..1 and merge with vertex alpha in S431

	andi		$t4, $v0, TNL_TEXGEN		// if( TNL_TEXGEN )
	beqz		$t4, do_texture_PD
	sv.q		R401, 0x20($a3)				// Store colour

	andi		$t4, $v0, TNL_TEXGENLIN				// if( TNL_TEXGENLIN )
	beqz		$t4, do_texgen_PD
	nop
	
# EnvMapped G_TEXTURE_GEN_LINEAR  Cheap way to do acos(x)/PI -> 0.5f - 0.25f * absf(x) - 0.25f * absf(x) * absf(x) * absf(x) //Corn
	vmov.p		R203, R200[1/2,1/2]					// result = 0.5
	vmul.p		R222, R202[1/4,1/4], R200			// X * 0.25, Y * 0.25
	vsub.p		R203, R203, R222					// result -= X * 0.25
	vmul.p		R222, R200, R222					// X * X * 0.25, Y * Y * 0.25
	vmul.p		R222, R200, R222					// X * X * X * 0.25, Y * Y * Y * 0.25
	vsub.p		R203, R203, R222					// result -= X * X * X * 0.25
	sv.s		S203, 0x30($a3)						// Store Texture.x
	b			vtx_done_PD
	sv.s		S213, 0x34($a3)						// Store Texture.y
		
do_texgen_PD:
# EnvMapped G_TEXTURE_GEN  t.x = 0.5 * (1.0 + n.x) t.y = 0.5 * (1.0 + n.y)
	vadd.p		R202, R202[1,1], R200				// 1+x, 1+y
	vmul.p		R202, R202[1/2,1/2], R202			// X * 0.5, Y * 0.25
	sv.s		S202, 0x30($a3)						// Store Texture.x
	b			vtx_done_PD
	sv.s		S212, 0x34($a3)						// Store Texture.y
		
do_color_PD:
# Normalise the RGBA colour
	.word		0xd0380000 | (8<<8) | (40)		// vuc2i.s	R200, S200					// R200 = [a,b,g,r]
	vi2f.q		R200, R200[w,z,y,x], 31			// int -> float, R200 = [r * 1/256, g * 1/256, b * 1/256, a * 1/256]
	sv.q		R200, 0x20($a3)					// Store colour

do_texture_PD:
# Textured t.x = (float)v.tu * mTextureScale.x t.y = (float)v.tv * mTextureScale.y
	lv.s		S202, 8($a2)				// load texture word [tv,tu] (N.B. due to swizzling these are 'backwards' from what you might expect)
	vs2i.s		R202, S202
	vi2f.p		R202, R202, 16				// int -> float
	vmul.p		R202, R202, R721[y,x]		// multiply by mTextureScale
	sv.s		S212, 0x30($a3)				// Store Texture.x
	sv.s		S202, 0x34($a3)				// Store Texture.y

vtx_done_PD:
# Continue with the next vertex
	addiu		$a2, $a2, 12				// Next input vertex
	bne			$a2, $t0, next_vertex_PD
	addiu		$a3, $a3, 64				// Next output vertex

finished_PD:	
	jr			$ra
	nop
	

############################
.global _TnLVFPUDKR
############################
#	a0 - num vertices
#	a1 - world*projection matrix	- must be aligned to 16 bytes
#	a2 - Fiddled vertices			- stride 16
#	a3 - Output vertices			- must be aligned to 16 bytes, stride 64

_TnLVFPUDKR:
	lv.q		R100, 0($a1)		// Load mat worldproject
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)
	lv.q		R103, 48($a1)
	
	sll			$v0, $a0, 1			// count * 2
	sll			$a0, $a0, 3			// count * 8
	addu		$a0, $v0, $a0		// count = count * 10
	addu		$a0, $a2, $a0		// end_ptr = start_ptr + count * 10
	beq			$a2, $a0, finished_DKR
	vone.s		S233				// w = 1.0f
	
next_vertex_DKR:
# Load and transform this vertex position
 	xori		$v0, $a2, 0x2		// = vtx_ptr ^ 2
	lh			$t4, 0($v0)			// get vertex x coord	
	mtv			$t4, S203			// store on VFPU
	addiu		$a2, $a2, 0x2		// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2		// = vtx_ptr ^ 2
	lh			$t4, 0($v0)			// get vertex y coord
	mtv			$t4, S213			// store on VFPU
	addiu		$a2, $a2, 0x2		// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2		// = vtx_ptr ^ 2
	lh			$t4, 0($v0)			// get vertex z coord
	mtv			$t4, S223			// store on VFPU
	addiu		$a2, $a2, 0x2		// = vtx_ptr + 2

	vi2f.t		R203, R203, 0				// int -> float (x,y,z,1)
	vtfm4.q		R202, M100, R203			// Projection transform
	sv.q		R203, 0x00($a3)				// Store world transform
	sv.q		R202, 0x10($a3)				// Store projection transform
	
# Compute the clip flags
	vcmp.q		LT, R202, R202[-w,-w,-w,0]	// x < -w, y < -w, z < -w
	vnop
	mfvc		$t4, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG (will become X_POS/Y_POS/Z_POS later)
	andi		$t4, $t4, 0x7				// Mask out the condition codes we don't care about
	sll			$t4, $t4, 3					// Shift up to create X_POS/Y_POS/Z_POS	
	vcmp.q		GT, R202, R202[w,w,w,0]		// x > w, y > w, z > w
	vnop
	mfvc		$t5, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t5, $t5, 0x7				// Mask out the condition codes we don't care about
	or			$t4, $t4, $t5
	sw			$t4, 0x38($a3)				// Store ClipFlags

# Normalise the RGBA colour
 	xori		$v0, $a2, 0x2		// = vtx_ptr ^ 2
	lhu			$t4, 0($v0)			// get vertex color	(hi)
	addiu		$a2, $a2, 0x2		// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2		// = vtx_ptr ^ 2
	lhu			$t5, 0($v0)			// get vertex color (lo)
	sll			$t4, $t4, 16		// pack
 	or		    $t5, $t5, $t4		// to 32bit
	addiu		$a2, $a2, 0x2		// = vtx_ptr + 2
	mtv			$t5, S200			// store on VFPU

	.word		0xd0380000 | (8<<8) | (40)		// vuc2i.s	R200, S200					// R200 = [a,b,g,r]
	vi2f.q		R200, R200[w,z,y,x], 31			// int -> float, R200 = [r * 1/256, g * 1/256, b * 1/256, a * 1/256]
	sv.q		R200, 0x20($a3)					// Store colour

# Continue with the next vertex
	bne			$a2, $a0, next_vertex_DKR
	addiu		$a3, $a3, 64				// Next output vertex

finished_DKR:	
	jr			$ra
	nop

############################
.global _TnLVFPUDKRB
############################
#	a0 - num vertices
#	a1 - world*projection matrix	- must be aligned to 16 bytes
#	a2 - Fiddled vertices			- stride 16
#	a3 - Output vertices			- must be aligned to 16 bytes, stride 64

_TnLVFPUDKRB:
	lv.q		R100, 0($a1)		// Load mat worldproject (matrix[0] and only 3x3 is needed)
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)

	lv.s		S000, 128($a1)		// Load element 0 in matrix[2]
	lv.s		S020, 168($a1)		// Load element 10 in matrix[2]
	vmov.s		S010, S000			// Copy element 0 (instead of 5)
	vfim.s		S011, 0.375			// Y scale factor
	vmul.t		R000, R000, R001[1/2,y,1/2]	// Prepare X and Z scaling values  
	vscl.t		C100, C100, S000	// Scale X matrix colum 
	vscl.t		C110, C110, S010	// Scale Y matrix colum 
	vscl.t		C120, C120, S020	// Scale Z matrix colum 
	 
	lv.q		R003, -64($a3)		// Get base vector to add to the billbord geometry (in position 0)

	sll			$v0, $a0, 1			// count * 2
	sll			$a0, $a0, 3			// count * 8
	addu		$a0, $v0, $a0		// count = count * 10
	addu		$a0, $a2, $a0		// end_ptr = start_ptr + count * 10
	beq			$a2, $a0, finished_DKRB
	vone.s		S233				// w = 1.0f
	
next_vertex_DKRB:
# Load and transform this vertex position
 	xori		$v0, $a2, 0x2		// = vtx_ptr ^ 2
	lh			$t4, 0($v0)			// get vertex x coord	
	mtv			$t4, S203			// store on VFPU
	addiu		$a2, $a2, 0x2		// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2		// = vtx_ptr ^ 2
	lh			$t4, 0($v0)			// get vertex y coord
	mtv			$t4, S213			// store on VFPU
	addiu		$a2, $a2, 0x2		// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2		// = vtx_ptr ^ 2
	lh			$t4, 0($v0)			// get vertex z coord
	mtv			$t4, S223			// store on VFPU
	addiu		$a2, $a2, 0x2		// = vtx_ptr + 2

	vi2f.t		R203, R203, 0		// int -> float (x,y,z,1)
	vtfm3.t		R202, M100, R203	// 3x3 transform
	vadd.t		R203, R202, R003	// Add basevector
	sv.q		R203, 0x00($a3)		// Store world transform (x,y,z,1)
	
# Clip flags
	sw			$zero, 0x38($a3)	// Clear ClipFlags

# Normalise the RGBA colour
 	xori		$v0, $a2, 0x2		// = vtx_ptr ^ 2
	lhu			$t4, 0($v0)			// get vertex color	(hi)
	addiu		$a2, $a2, 0x2		// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2		// = vtx_ptr ^ 2
	lhu			$t5, 0($v0)			// get vertex color (lo)
	sll			$t4, $t4, 16		// pack
 	or		    $t5, $t5, $t4		// to 32bit
	addiu		$a2, $a2, 0x2		// = vtx_ptr + 2
	mtv			$t5, S200			// store on VFPU

	.word		0xd0380000 | (8<<8) | (40)		// vuc2i.s	R200, S200					// R200 = [a,b,g,r]
	vi2f.q		R200, R200[w,z,y,x], 31			// int -> float, R200 = [r * 1/256, g * 1/256, b * 1/256, a * 1/256]
	sv.q		R200, 0x20($a3)					// Store colour

# Continue with the next vertex
	bne			$a2, $a0, next_vertex_DKRB
	addiu		$a3, $a3, 64				// Next output vertex

finished_DKRB:	
	jr			$ra
	nop
	
	
.set pop