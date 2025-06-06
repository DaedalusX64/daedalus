// Copyright (C) 2001 StrmnNrmn
// Copyright (C) 2011 Corn

#include "as_reg_compat.h"

#define LIGHTSZ				6	//1<<6 bytes
#define PARAMS_FLAGS_NLIGHT_TXSCAL	0x00
#define PARAMS_LIGHTS		0x10
#define COORDMOD1		   (16 + 12 * (1<<LIGHTSZ) + 32)
#define COORDMOD2		   (16 + 12 * (1<<LIGHTSZ) + 48)
#define FOGPARAM		   (16 + 12 * (1<<LIGHTSZ) + 64)
#define LIGHTDIR			0
#define LIGHTCOL			16
#define LIGHTPOS			32
#define LIGHTSCL			48

#define	TNL_LIGHT			(1<<0)
#define	TNL_TEXGEN			(1<<1)
#define	TNL_TEXGENLIN		(1<<2)
#define	TNL_FOG				(1<<3)
#define	TNL_SHADE			(1<<4)
#define	TNL_ZBUFFER			(1<<5)
#define	TNL_TRICULL			(1<<6)
#define	TNL_CULLBACK		(1<<7)
#define	TNL_POINTLIGHT		(1<<8)
	
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
# R701: FogMult & FogOffs
# R721: Texture X & Y scale
# R702: Vertex position
# R703: project transform [x,y,z,w]
# t4 = cur_light
# t6 = first_light
# t7 = last_light
# v0 = TnLFlags

_TnLVFPU:
	lv.q		R000, 0($a0)				// Load mat world
	lv.q		R001, 16($a0)
	lv.q		R002, 32($a0)
	lv.q		R003, 48($a0)

	lv.q		R100, 0($a1)				// Load mat project
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)
	lv.q		R103, 48($a1)
	
	lv.q		R701, PARAMS_FLAGS_NLIGHT_TXSCAL($t1)	// Load params [Flags, Num_lights, tscale_x, tscale_y]
	vmov.p		R721, R721[y,x]				// Swizzle texture X&Y scale 
	mfv			$t7, S711					// Num_lights
	
# Calculate the last light index
	addiu		$t6, $t1, PARAMS_LIGHTS		// pointer to first_light = p_lights
	sll			$t7, $t7, LIGHTSZ			// num_lights*64
	addu		$t7, $t6, $t7				// last_light = p_lights + num_lights*64
	lv.q		R700, LIGHTCOL($t7)			// Load ambient color
	
	sll			$t0, $t0, 4					// count = count * 16
	addu		$t0, $a2, $t0				// end_ptr = start_ptr + count * 16
	beq			$a2, $t0, finished_
	mfv			$v0, S701					// TnL flags
	
	lv.s		S701, FOGPARAM($t1)			// Load fog param [FogMult]
	lv.s		S711, FOGPARAM+4($t1)		// Load fog param [FogOffs]

next_vertex_:
# Load and transform this vertex position
 	lv.s		S200, 0($a2)				// load word [y,x,?,z]
 	lv.s		S210, 4($a2)				// ulv.q is buggy on PHAT
	vs2i.p		R200, R200					// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 16				// int -> float
	vmov.q		R702, R200[y,x,w,1]			// unfiddle and save for point lighting
	vtfm4.q		R201, M000, R702			// World transform
	vtfm4.q		R703, M100, R702			// World*Projection transform
	sv.q		R201, 0x00($a3)				// Store world transform
	sv.q		R703, 0x10($a3)				// Store projection transform
	
# Compute the clip flags
	vcmp.q		LT, R703, R703[-w,-w,-w,0]	// x < -w, y < -w, z < -w
	lv.s		S200, 12($a2)				// S200 <- load model normal or color word [w,z,y,x]/[a,b,g,r]
	mfvc		$t4, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG (will become X_POS/Y_POS/Z_POS later)
	vcmp.q		GT, R703, R703[w,w,w,0]		// x > w, y > w, z > w, w > 0
	andi		$t4, $t4, 0x7				// Mask out the condition codes we don't care about
	sll			$t4, $t4, 3					// Shift up to create X_POS/Y_POS/Z_POS	
	mfvc		$t5, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t3, $t5, 0x8				// Keep the condition w > 0 (used for fog)
	andi		$t5, $t5, 0x7				// Mask out the condition codes we don't care about
	or			$t5, $t4, $t5

#Light or Color?
	andi		$t4, $v0, TNL_LIGHT			// if( TNL_LIGHT )
	beqz		$t4, do_color_
	sw			$t5, 0x38($a3)				// Store ClipFlags
	
#Convert the alpha in R200 to float and pass it along to light color
	vuc2ifs.s		R203, S200				// R200 = [a,z,y,x]
	vi2f.s		S431, S203, 31				// int -> float, R431 = [a * 1/256]

#Check if there are any lights to process
	beq			$t6, $t7, done_lighting_	// cur_light == last_light?
	vmov.q		R201, R700					// Colour = ambient

	or			$t4, $t6, $0				// cur_light = p_lights

# Convert the model normal in R200 to floats and transform
	.word		0xd0398080 | (8<<8) | (40)	// vc2i.s		R200, S200					// R200 = [a,z,y,x]
	vi2f.q		R203, R200[w,z,y,x], 0		// int -> float, Unfiddle & store vertice normal temporary for env map later (obliterates world transform)
	vtfm3.t		R200, M000, R203			// Transform with world matrix (only need 3x3)//Corn
	vdot.t		S202, R200, R200			// S202 = x*x + y*y + z*z
	vrsq.s		S202, S202					// S202 = 1/sqrt(x*x + y*y + z*z)
	vscl.t		R200, R200, S202			// S200 = v.normalise().

next_light_:
	lv.q		R301, LIGHTDIR($t4)			// Load Light normal
	vdot.t		S303[0:1], R200, R301		// x = clamp(dot(normal,(x,y,z)),0,1)
	lv.q		R302, LIGHTCOL($t4)			// Load Light colour
	addiu		$t4, $t4, (1<<LIGHTSZ)		// Skip to the next light
	vscl.t		R303, R302, S303			// r,g,b = r*x, g*x, b*x
	bne			$t4, $t7, next_light_
	vadd.t		R201, R201, R303			// col += r,g,b

done_lighting_:
	vmov.t		R401[0:1,0:1,0:1], R201		// Clamp 0..1 and merge with vertex alpha in S431

	andi		$t4, $v0, TNL_TEXGEN		// if( TNL_TEXGEN )
	beqz		$t4, do_texture_
	nop

# We use worldproject matrix to calc normals it gives a nicer effect (model view result is in R200) //Corn
	vtfm3.t		R200, M100, R203			// Transform with projworld matrix, looks nicer (only need 3x3)
	vdot.t		S201, R200, R200			// S201 = x*x + y*y + z*z
	vrsq.s		S201, S201					// S201 = 1/sqrt(x*x + y*y + z*z)

	andi		$t4, $v0, TNL_TEXGENLIN		// if( TNL_TEXGENLIN )
	beqz		$t4, do_texgen_
	vscl.p		R200, R200, S201			// R202 = v.normalise() (x & y).

# EnvMapped G_TEXTURE_GEN  t.x = 0.5 * (1.0 + n.x) t.y = 0.25 * (1.0 + n.y)
	vadd.p		R200, R200[1,1], R200		// 1+x, 1+y
	vmul.p		R200, R200[1/2,1/2], R200	// X * 0.5, Y * 0.25
	sv.s		S200, 0x30($a3)				// Store Texture.x
	b			vtx_done_
	sv.s		S210, 0x34($a3)				// Store Texture.y
		
do_texgen_:
# EnvMapped G_TEXTURE_GEN_LINEAR Cheap way to do acos(x)/PI -> 0.5f - 0.25f * absf(x) - 0.25f * absf(x) * absf(x) * absf(x) //Corn
	vabs.p		R200, R200					// absf(x), absf(y)
	vmul.p		R220, R200[1/4,1/4], R200	// X * 0.25, Y * 0.25
	vsub.p		R203, R200[1/2,1/2], R220	// result = 0.5 - X * 0.25
	vmul.p		R220, R200, R220			// X * X * 0.25, Y * Y * 0.25
	vmul.p		R220, R200, R220			// X * X * X * 0.25, Y * Y * Y * 0.25
	vsub.p		R203, R203, R220			// result -= X * X * X * 0.25
	sv.s		S203, 0x30($a3)				// Store Texture.x
	b			vtx_done_
	sv.s		S213, 0x34($a3)				// Store Texture.y
		
do_color_:
# Normalise the RGBA colour
	vuc2ifs.s		R200, S200				// R200 = [a,b,g,r]
	vi2f.q		R401, R200[w,z,y,x], 31		// int -> float, R200 = [r * 1/256, g * 1/256, b * 1/256, a * 1/256]

do_texture_:
# Textured t.x = (float)v.tu * mTextureScale.x 	t.y = (float)v.tv * mTextureScale.y
	lv.s		S202, 8($a2)				// load texture word [tv,tu] (N.B. due to swizzling these are 'backwards' from what you might expect)
	vs2i.s		R202, S202
	vi2f.p		R202, R202, 16				// int -> float
	vmul.p		R202, R202, R721			// multiply by mTextureScale
	sv.s		S212, 0x30($a3)				// Store Texture.x
	sv.s		S202, 0x34($a3)				// Store Texture.y

vtx_done_:
	andi		$t4, $v0, TNL_FOG
	beqz		$t4, fog_done_				// if( TNL_FOG )
	nop
	beqz		$t3, fog_done_				// if( proj.w > 0.0f )
	vzero.s		S431						// fog_alpha = 0.0f

#Calculate fog factor and put as alpha
	vrcp.s		S200, S733					// 1/w
	vmul.s		S201, S723, S701			// fogmul * z			
	vmul.s		S201, S201, S200			// fogmul * z * 1/w
	vadd.s		S431[0:1], S201, S711		// fog_alpha = Clamp[0:1] fogmul * z * 1/w + fogoffs	

fog_done_:
	sv.q		R401, 0x20($a3)				// Store colour

# Continue with the next vertex
	addiu		$a2, $a2, 16				// Next input vertex
	bne			$a2, $t0, next_vertex_
	addiu		$a3, $a3, 64				// Next output vertex

finished_:	
	jr			$ra
	nop

#Used by Zelda MM
############################
.global _TnLVFPU_Plight
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
# R301: Scratch
# R302: Scratch
# R303: Scratch
# R431: current vertex Alpha value
# R700: Ambient
# R701: FogMult & FogOffs
# R721: Texture X & Y scale
# R702: Vertex position
# R703: project transform [x,y,z,w]
# t4 = cur_light
# t6 = first_light
# t7 = last_light
# v0 = TnLFlags

_TnLVFPU_Plight:
	lv.q		R000, 0($a0)				// Load mat world
	lv.q		R001, 16($a0)
	lv.q		R002, 32($a0)
	lv.q		R003, 48($a0)

	lv.q		R100, 0($a1)				// Load mat project
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)
	lv.q		R103, 48($a1)
	
	lv.q		R701, PARAMS_FLAGS_NLIGHT_TXSCAL($t1)	// Load params [Flags, Num_lights, tscale_x, tscale_y]
	vmov.p		R721, R721[y,x]				// Swizzle texture X&Y scale 
	mfv			$t7, S711					// Num_lights
	mfv			$v0, S701					// TnL Flags

	lv.s		S701, FOGPARAM($t1)			// Load fog param [FogMult]
	lv.s		S711, FOGPARAM+4($t1)		// Load fog param [FogOffs]
		
# Calculate the last light index
	addiu		$t6, $t1, PARAMS_LIGHTS		// pointer to first_light = p_lights
	sll			$t7, $t7, LIGHTSZ			// num_lights*64
	addu		$t7, $t6, $t7				// last_light = p_lights + num_lights*64
	
	sll			$t0, $t0, 4					// count = count * 16
	addu		$t0, $a2, $t0				// end_ptr = start_ptr + count * 16
	beq			$a2, $t0, finished_Plight
	lv.q		R700, LIGHTCOL($t7)			// Load ambient color
	
next_vertex_Plight:
# Load and transform this vertex position
 	lv.s		S200, 0($a2)				// load word [y,x,?,z]
 	lv.s		S210, 4($a2)				// ulv.q is buggy on PHAT
	vs2i.p		R200, R200					// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 16				// int -> float
	vmov.q		R702, R200[y,x,w,1]			// unfiddle and save for point lighting
	vtfm4.q		R201, M000, R702			// World transform
	vtfm4.q		R703, M100, R702			// World*Projection transform
	sv.q		R201, 0x00($a3)				// Store world transform
	sv.q		R703, 0x10($a3)				// Store projection transform
	
# Compute the clip flags
	vcmp.q		LT, R703, R703[-w,-w,-w,0]	// x < -w, y < -w, z < -w
	lv.s		S200, 12($a2)				// S200 <- load model normal or color word [w,z,y,x]/[a,b,g,r]
	mfvc		$t4, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG (will become X_POS/Y_POS/Z_POS later)
	vcmp.q		GT, R703, R703[w,w,w,0]		// x > w, y > w, z > w, w > 0
	andi		$t4, $t4, 0x7				// Mask out the condition codes we don't care about
	sll			$t4, $t4, 3					// Shift up to create X_POS/Y_POS/Z_POS	
	mfvc		$t5, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t3, $t5, 0x8				// Keep the condition w > 0 (used for fog)
	andi		$t5, $t5, 0x7				// Mask out the condition codes we don't care about
	or			$t5, $t4, $t5
	sw			$t5, 0x38($a3)				// Store ClipFlags

#Convert the alpha in R200 to float and pass it along to light color
	vuc2ifs.s		R203, S200				// R200 = [a,z,y,x]
	vi2f.s		S431, S203, 31				// int -> float, R431 = [a * 1/256]

#Check if there are any lights to process
	beq			$t6, $t7, done_Plight		// cur_light == last_light?
	vmov.q		R201, R700					// Colour = ambient

	or			$t4, $t6, $0				// cur_light = p_lights

next_light_Plight:
	lv.q		R301, LIGHTPOS($t4)			// Load Light position
	vsub.t		R301, R301, R702			// light pos - vertex pos
	vdot.t		S321, R301, R301			// S321 (qlen) = x*x + y*y + z*z
	lv.q		R302, LIGHTSCL($t4)			// Load Light scaleing
	vsqrt.s		S311, S321					// S311 (llen) = SQRT(x*x + y*y + z*z)
	lv.q		R303, LIGHTCOL($t4)			// Load Light colour
	vdot.t		S302, R302, R301[1,y,z]		// S302 (L) = (1.0f*ca + llen*la + qlen*qa)
	mfv			$t5, S302					// S302 (L) -> t5
	beqz		$t5, skip_Plight			// Skip this light if L == 0.0f (0x00000000)
	addiu		$t4, $t4, (1<<LIGHTSZ)		// Advance pointer to the next light
	vrcp.s		S302, S302					// S302 (i) = 1.0f / L
	vscl.t		R303, R303, S302			// r,g,b = r*i, g*i, b*i
	vadd.t		R201, R201, R303			// col += r,g,b
skip_Plight:
	bne			$t4, $t7, next_light_Plight
	nop
	
done_Plight:
	vmov.t		R401[0:1,0:1,0:1], R201		// Clamp 0..1 and merge with vertex alpha in S431

# Textured t.x = (float)v.tu * mTextureScale.x 	t.y = (float)v.tv * mTextureScale.y
	lv.s		S202, 8($a2)				// load texture word [tv,tu] (N.B. due to swizzling these are 'backwards' from what you might expect)
	vs2i.s		R202, S202
	vi2f.p		R202, R202, 16				// int -> float
	vmul.p		R202, R202, R721			// multiply by mTextureScale
	sv.s		S212, 0x30($a3)				// Store Texture.x

	andi		$t4, $v0, TNL_FOG
	beqz		$t4, fog_done_plight		// if( TNL_FOG )
	sv.s		S202, 0x34($a3)				// Store Texture.y
	beqz		$t3, fog_done_plight		// if( proj.w > 0.0f )
	vzero.s		S431						// fog_alpha = 0.0f

#Calculate fog factor and put as alpha
	vrcp.s		S200, S733					// 1/w
	vmul.s		S201, S723, S701			// fogmul * z			
	vmul.s		S201, S201, S200			// fogmul * z * 1/w
	vadd.s		S431[0:1], S201, S711		// fog_alpha = Clamp[0:1] fogmul * z * 1/w + fogoffs	

fog_done_plight:
	sv.q		R401, 0x20($a3)				// Store colour

# Continue with the next vertex
	addiu		$a2, $a2, 16				// Next input vertex
	bne			$a2, $t0, next_vertex_Plight
	addiu		$a3, $a3, 64				// Next output vertex

finished_Plight:	
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
# R201: Light position 
# R202: projected
# R203: store raw material normal
# R300: color result
# R301: Light direction
# R302: Light color
# R303: Scratch
# R400: Accumulated colour
# R700: Ambient light color
# R721: Texture X & Y scale
# R702: Coord [8, 9, 10, 11]
# R703: Coord [12, 13, 14, 15]
# v0 = TnLFlags
# t4 = cur_light
# t5 = last_light(point light)
# t6 = first_light
# t7 = last_light

_TnLVFPUCBFD:
	lv.q		R000, 0($a0)				// Load mat world
	lv.q		R001, 16($a0)
	lv.q		R002, 32($a0)
	lv.q		R003, 48($a0)

	lv.q		R100, 0($a1)				// Load mat project
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)
	lv.q		R103, 48($a1)
	
	lv.q		R701, PARAMS_FLAGS_NLIGHT_TXSCAL($t1)	// Load params [Flags, Num_lights, tscale_x, tscale_y]
	vmov.p		R721, R721[y,x]				// Swizzle texture X&Y scale 
	mfv			$t7, S711					// Num_lights
	
# Load Coord Mod vectors
	lv.q		R702, COORDMOD1($t1)		// Load Coord [8, 9, 10, 11]
	lv.q		R703, COORDMOD2($t1)		// Load Coord [12, 13, 14, 15]

# Calculate the last light index
	addiu		$t6, $t1, PARAMS_LIGHTS		// pointer to first_light = p_lights
	sll			$t7, $t7, LIGHTSZ			// num_lights*64
	addu		$t7, $t6, $t7				// last_light = p_lights + num_lights*64
	lv.q		R700, LIGHTCOL($t7)			// Load ambient color

# Calculate the last vertex index
	sll			$t0, $t0, 4					// count = count * 16
	addu		$t0, $a2, $t0				// end_ptr = start_ptr + count * 16
	beq			$a2, $t0, finished_CBFD
	mfv			$v0, S701					// TnL flags
	
next_vertex_CBFD:
# Load and transform this vertex position
 	lv.s		S200, 0($a2)				// load word [y,x,?,z]
 	lv.s		S210, 4($a2)				// ulv.q is buggy on PHAT
	vs2i.p		R200, R200					// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 16				// int -> float
	vmov.q		R203, R200[y,x,w,1]			// unswizzle order
	vtfm4.q		R201, M000, R203			// World transform

#Load & Normalise the vertex RGBA colour
	lv.s		S200, 12($a2)				// load normal word [w,z,y,x]
	vuc2ifs.s		R200, S200				// R200 = [a,b,g,r]
	vi2f.q		R400, R200[w,z,y,x], 31		// int -> float, R403 = [r * 1/256, g * 1/256, b * 1/256, a * 1/256]

	vtfm4.q		R202, M100, R201			// Projection transform
	sv.q		R201, 0x00($a3)				// Store world transform
	sv.q		R202, 0x10($a3)				// Store projection transform
	
# Compute the clip flags
	vcmp.q		LT, R202, R202[-w,-w,-w,0]	// x < -w, y < -w, z < -w
	vnop
	mfvc		$t4, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG (will become X_POS/Y_POS/Z_POS later)
	vcmp.q		GT, R202, R202[w,w,w,0]		// x > w, y > w, z > w
	andi		$t4, $t4, 0x7				// Mask out the condition codes we don't care about
	sll			$t4, $t4, 3					// Shift up to create X_POS/Y_POS/Z_POS	
	mfvc		$t5, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t5, $t5, 0x7				// Mask out the condition codes we don't care about
	or			$t5, $t4, $t5

#LIGHT
	andi		$t4, $v0, TNL_LIGHT			// if( TNL_LIGHT )
	beqz		$t4, do_texture_CBFD
	sw			$t5, 0x38($a3)				// Store ClipFlags

#skip to normal light and avoid the model normal transform if only TNL_LIGHT is set
	andi		$t4, $v0, (TNL_POINTLIGHT | TNL_TEXGEN)			// if( TNL_POINTLIGHT | TNL_TEXGEN )
	beqz		$t4, do_normallight_CBFD
	nop
	
#Use world matrix to transform model normal
 	xori		$t5, $t3, 0x3				// = v0 ^ 3
	addu		$t5, $t2, $t5				// += base address
	lb			$t4, 0($t5)					// get normal x
	mtv			$t4, S203					// Store vertice normal X 
	addiu		$t5, $t3, 0x1				// = v0 + 1
 	xori		$t5, $t5, 0x3				// ^= 3
	addu		$t5, $t2, $t5				// += base address
	lb			$t4, 0($t5)					// get normal y
	mtv			$t4, S213					// Store vertice normal Y 
 	lb			$v1, 4($a2)					// Get vert_norm z
	mtv			$v1, S223					// Store vertice normal Z 
	vi2f.t		R203, R203, 0				// int -> float
	vtfm3.t		R200, M000, R203			// Transform with world matrix, (only need 3x3)
	vdot.t		S201, R200, R200			// S201 = x*x + y*y + z*z
	vrsq.s		S201, S201					// S201 = 1/sqrt(x*x + y*y + z*z)

	andi		$t4, $v0, TNL_POINTLIGHT	// if( TNL_POINTLIGHT )
	beqz		$t4, do_normallight_CBFD
	vscl.t		R200, R200, S201			// R200 = normalise transformed model vector (x, y, z).

#POINT LIGHT
	beq			$t6, $t7, done_plight_CBFD	// cur_light == last_light?
	vmov.q		R300, R700					// Colour = ambient

	addiu		$t5, $t7, -(1<<LIGHTSZ)		// Do one light less with point light
	beq			$t6, $t5, do_dot_plight_CBFD	// cur_light == last_light?
	or			$t4, $t6, $0				// cur_light = p_lights

	vadd.q		R202, R202, R702			// Proj + Coord1
	vmul.q		R202, R202, R703			// * Coord2

next_plight_CBFD:
	lv.q		R301, LIGHTDIR($t4)			// Load Light direction	& SkipIfZero
	mfv			$t8, S331					// SkipIfZero
	beqz		$t8, skip_plight_CBFD
	nop
	
	lv.q		R201, LIGHTPOS($t4)			// Load Light position
	vsub.q		R201, R202, R201			// ProjCoord - light position
	vdot.q		S201, R201, R201			// S201 = x*x + y*y + z*z
	lv.q		R302, LIGHTCOL($t4)			// Load Light colour & scale
	vrcp.s		S201, S201					// S201 = 1/(x*x + y*y + z*z)
	vmul.s		S332[0:1], S332, S201		// R332 = p_i clamped 0:1.
	vdot.t		S303[0:1], R200, R301		// intensity = clamp(dot(normal,(x,y,z)),0,1)
	vmul.s		S303, S303, S332			// intensity *= p_i
	vscl.t		R302, R302, S303			// r,g,b = r*i, g*i, b*i
	vadd.t		R300, R300, R302			// col += r,g,b

skip_plight_CBFD:
	addiu		$t4, $t4, (1<<LIGHTSZ)		// Skip to the next light
	bne			$t4, $t5, next_plight_CBFD
	nop
	
do_dot_plight_CBFD:
	lv.q		R301, LIGHTDIR($t4)			// Load Light normal
	vdot.t		S303[0:1], R200, R301		// intensity = clamp(dot(normal,(x,y,z)),0,1)
	lv.q		R302, LIGHTCOL($t4)			// Load Light colour & scale
	vscl.t		R302, R302, S303			// r,g,b = r*i, g*i, b*i
	vadd.t		R300[0:1,0:1,0:1], R300, R302	// col += r,g,b (and clamp result)

done_plight_CBFD:
	b			skip_to_envmap_CBFD
	vmul.t		R400, R400, R300			// Col *= col and merge with vertex alpha
	
#NORMAL LIGHT
do_normallight_CBFD:
	beq			$t6, $t7, done_nlight_CBFD	// cur_light == last_light?
	vmov.q		R300, R700					// Colour = ambient

	or			$t4, $t6, $0				// cur_light = p_lights
	vadd.q		R202, R202, R702			// Proj + Coord1
	vmul.q		R202, R202, R703			// * Coord2

next_nlight_CBFD:
	lv.q		R201, LIGHTPOS($t4)			// Load Light position
	vsub.q		R201, R202, R201			// ProjCoord - light position
	vdot.q		S201, R201, R201			// S201 = x*x + y*y + z*z
	lv.q		R302, LIGHTCOL($t4)			// Load Light colour & scale
	vrcp.s		S201, S201					// S201 = 1/(x*x + y*y + z*z)
	vmul.s		S332[0:1], S332, S201		// R332 = p_i clamped 0:1.
	addiu		$t4, $t4, (1<<LIGHTSZ)		// Skip to the next light
	vscl.t		R302, R302, S332			// r,g,b = r*i, g*i, b*i
	bne			$t4, $t7, next_nlight_CBFD
	vadd.t		R300, R300, R302			// col += r,g,b

	vmov.t		R300[0:1,0:1,0:1], R300
done_nlight_CBFD:
	vmul.t		R400, R400, R300			// Col *= col and merge with vertex alpha

#Check environment mapping
skip_to_envmap_CBFD:
	andi		$t4, $v0, TNL_TEXGEN		// if( TNL_TEXGEN )
	beqz		$t4, do_texture_CBFD
	nop
	
	andi		$t4, $v0, TNL_TEXGENLIN		// if( TNL_TEXGENLIN )
	beqz		$t4, do_texgen_CBFD
	nop
	
# EnvMapped G_TEXTURE_GEN_LINEAR Cheap way to do acos(x)/PI -> 0.5f - 0.25f * x - 0.25f * x * x * x //Corn
	vmul.p		R220, R200[1/4,1/4], R200	// X * 0.25, Y * 0.25
	vsub.p		R201, R200[1/2,1/2], R220	// result = 0.5 - X * 0.25
	vmul.p		R220, R200, R220			// X * X * 0.25, Y * Y * 0.25
	vmul.p		R220, R200, R220			// X * X * X * 0.25, Y * Y * Y * 0.25
	vsub.p		R201, R201, R220			// result -= X * X * X * 0.25
	sv.s		S201, 0x30($a3)				// Store Texture.x
	b			vtx_done_CBFD
	sv.s		S211, 0x34($a3)				// Store Texture.y
		
do_texgen_CBFD:
# EnvMapped G_TEXTURE_GEN  t.x = 0.5 * (1.0 + n.x) t.y = 0.5 * (1.0 + n.y)
	vadd.p		R200, R200[1,1], R200		// 1+x, 1+y
	vmul.p		R200, R200[1/2,1/2], R200	// X * 0.5, Y * 0.5
	sv.s		S200, 0x30($a3)				// Store Texture.x
	b			vtx_done_CBFD
	sv.s		S210, 0x34($a3)				// Store Texture.y
		
do_texture_CBFD:
# Textured t.x = (float)v.tu * mTextureScale.x 	t.y = (float)v.tv * mTextureScale.y
	lv.s		S202, 8($a2)				// load texture word [tv,tu] (N.B. due to swizzling these are 'backwards' from what you might expect)
	vs2i.s		R202, S202
	vi2f.p		R202, R202, 16				// int -> float
	vmul.p		R202, R202, R721			// multiply by mTextureScale
	sv.s		S212, 0x30($a3)				// Store Texture.x
	sv.s		S202, 0x34($a3)				// Store Texture.y

vtx_done_CBFD:
	sv.q		R400, 0x20($a3)				// Store colour

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
	lv.q		R000, 0($a0)				// Load mat world
	lv.q		R001, 16($a0)
	lv.q		R002, 32($a0)
	lv.q		R003, 48($a0)

	lv.q		R100, 0($a1)				// Load mat project
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)
	lv.q		R103, 48($a1)
	
	lv.q		R701, PARAMS_FLAGS_NLIGHT_TXSCAL($t1)	// Load params [Flags, Num_lights, tscale_x, tscale_y]
	vmov.p		R721, R721[y,x]				// Swizzle texture X&Y scale 
	mfv			$t7, S711					// Num_lights
	
# Calculate the last light index
	addiu		$t6, $t1, PARAMS_LIGHTS		// pointer to first_light = p_lights
	sll			$t7, $t7, LIGHTSZ			// num_lights*64
	addu		$t7, $t6, $t7				// last_light = p_lights + num_lights*64
	lv.q		R700, LIGHTCOL($t7)			// Load ambient color
	
	sll			$v1, $t0, 2					// count * 4
	sll			$t0, $t0, 3					// count * 8
	addu		$t0, $v1, $t0				// count = count * 12
	addu		$t0, $a2, $t0				// end_ptr = start_ptr + count * 12
	beq			$a2, $t0, finished_PD
	mfv			$v0, S701					// TnL flags

next_vertex_PD:
# Load and transform this vertex position
 	lv.s		S203, 0($a2)				// load word [y,x,?,z]
 	lv.s		S213, 4($a2)				// ulv.q is buggy on PHAT
	vs2i.p		R200, R203					// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 16				// int -> float
	vmov.q		R200, R200[y,x,w,1]
	vtfm4.q		R201, M000, R200			// World transform
 	mfv			$v1, S213					// Get Cindx
 	andi		$v1, 0xFF					// use only low Byte
	addu		$v1, $v1, $t2				// pointer = base vector + Cindx
	vtfm4.q		R202, M100, R201			// Projection transform
	sv.q		R201, 0x00($a3)				// Store world transform
	sv.q		R202, 0x10($a3)				// Store projection transform
	
# Compute the clip flags
	vcmp.q		LT, R202, R202[-w,-w,-w,0]	// x < -w, y < -w, z < -w
	vnop
	mfvc		$t4, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG (will become X_POS/Y_POS/Z_POS later)
	vcmp.q		GT, R202, R202[w,w,w,0]		// x > w, y > w, z > w
	andi		$t4, $t4, 0x7				// Mask out the condition codes we don't care about
	sll			$t4, $t4, 3					// Shift up to create X_POS/Y_POS/Z_POS	
	mfvc		$t5, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t5, $t5, 0x7				// Mask out the condition codes we don't care about
	or			$t5, $t4, $t5

#Load vertex Normal or Color
	lv.s		S200, 0($v1)				// load normal word [w,z,y,x]
	andi		$t4, $v0, TNL_LIGHT			// if( TNL_LIGHT )
	beqz		$t4, do_color_PD
	sw			$t5, 0x38($a3)				// Store ClipFlags
	
#Do lighting Convert the alpha in R200 to float and pass it along to light color
	vuc2ifs.s		R203, S200				// R200 = [a,z,y,x]
	vi2f.s		S431, S203, 31				// int -> float, R431 = [a * 1/256]
	
# Convert the normal in R200 to float and transform
	.word		0xd0398080 | (8<<8) | (40)	// vc2i.s		R200, S200					// R200 = [?,z,y,x]
	vi2f.q		R201, R200[w,z,y,x], 0		// int -> float, Unfiddle (obliterates world transform)
	vtfm3.t		R200, M000, R201			// Transform with world matrix (only need 3x3)//Corn
	vdot.t		S201, R200, R200			// S201 = x*x + y*y + z*z
	vrsq.s		S201, S201					// S201 = 1/sqrt(x*x + y*y + z*z)
	vscl.t		R200, R200, S201			// S200 = v.normalise().

	vmov.q		R201, R700					// Colour = ambient
	beq			$t6, $t7, done_lighting_PD	// cur_light == last_light?
	or			$t4, $t6, $0				// cur_light = p_lights

next_light_PD:
	lv.q		R301, LIGHTDIR($t4)			// Load Light normal
	vdot.t		S303[0:1], R200, R301		// x = clamp(dot(normal,(x,y,z)),0,1)
	lv.q		R302, LIGHTCOL($t4)			// Load Light colour
	addiu		$t4, $t4, (1<<LIGHTSZ)		// Skip to the next light
	vscl.t		R303, R302, S303			// r,g,b = r*x, g*x, b*x
	bne			$t4, $t7, next_light_PD
	vadd.t		R201, R201, R303			// col += r,g,b,a

done_lighting_PD:
	vmov.t		R401[0:1,0:1,0:1], R201		// Clamp 0..1 and merge with vertex alpha in S431

	andi		$t4, $v0, TNL_TEXGEN		// if( TNL_TEXGEN )
	beqz		$t4, do_texture_PD
	sv.q		R401, 0x20($a3)				// Store colour

	andi		$t4, $v0, TNL_TEXGENLIN		// if( TNL_TEXGENLIN )
	beqz		$t4, do_texgen_PD
	nop
	
# EnvMapped G_TEXTURE_GEN_LINEAR  Cheap way to do acos(x)/PI -> 0.5f - 0.25f * x - 0.25f * x * x * x //Corn
	vmul.p		R222, R202[1/4,1/4], R200	// X * 0.25, Y * 0.25
	vsub.p		R203, R202[1/2,1/2], R222	// result = 0.5 - X * 0.25
	vmul.p		R222, R200, R222			// X * X * 0.25, Y * Y * 0.25
	vmul.p		R222, R200, R222			// X * X * X * 0.25, Y * Y * Y * 0.25
	vsub.p		R203, R203, R222			// result -= X * X * X * 0.25
	sv.s		S203, 0x30($a3)				// Store Texture.x
	b			vtx_done_PD
	sv.s		S213, 0x34($a3)				// Store Texture.y
		
do_texgen_PD:
# EnvMapped G_TEXTURE_GEN  t.x = 0.5 * (1.0 + n.x) t.y = 0.5 * (1.0 + n.y)
	vadd.p		R202, R202[1,1], R200		// 1+x, 1+y
	vmul.p		R202, R202[1/2,1/2], R202	// X * 0.5, Y * 0.25
	sv.s		S202, 0x30($a3)				// Store Texture.x
	b			vtx_done_PD
	sv.s		S212, 0x34($a3)				// Store Texture.y
		
do_color_PD:
# Normalise the RGBA colour
	vuc2ifs.s		R200, S200				// R200 = [a,b,g,r]
	vi2f.q		R200, R200[w,z,y,x], 31		// int -> float, R200 = [r * 1/256, g * 1/256, b * 1/256, a * 1/256]
	sv.q		R200, 0x20($a3)				// Store colour

do_texture_PD:
# Textured t.x = (float)v.tu * mTextureScale.x t.y = (float)v.tv * mTextureScale.y
	lv.s		S202, 8($a2)				// load texture word [tv,tu] (N.B. due to swizzling these are 'backwards' from what you might expect)
	vs2i.s		R202, S202
	vi2f.p		R202, R202, 16				// int -> float
	vmul.p		R202, R202, R721			// multiply by mTextureScale
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
	lv.q		R100, 0($a1)				// Load mat worldproject
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)
	lv.q		R103, 48($a1)
	
	sll			$v0, $a0, 1					// count * 2
	sll			$a0, $a0, 3					// count * 8
	addu		$a0, $v0, $a0				// count = count * 10
	addu		$a0, $a2, $a0				// end_ptr = start_ptr + count * 10
	beq			$a2, $a0, finished_DKR
	vone.s		S233						// w = 1.0f
	
next_vertex_DKR:
# Load and transform this vertex position
 	xori		$v0, $a2, 0x2				// = vtx_ptr ^ 2
	lh			$t4, 0($v0)					// get vertex x coord	
	mtv			$t4, S203					// store on VFPU
	addiu		$a2, $a2, 0x2				// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2				// = vtx_ptr ^ 2
	lh			$t4, 0($v0)					// get vertex y coord
	mtv			$t4, S213					// store on VFPU
	addiu		$a2, $a2, 0x2				// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2				// = vtx_ptr ^ 2
	lh			$t4, 0($v0)					// get vertex z coord
	mtv			$t4, S223					// store on VFPU
	addiu		$a2, $a2, 0x2				// = vtx_ptr + 2

	vi2f.t		R203, R203, 0				// int -> float (x,y,z,1)
	vtfm4.q		R202, M100, R203			// Projection transform
	sv.q		R203, 0x00($a3)				// Store world transform
	sv.q		R202, 0x10($a3)				// Store world+projection transform
	
# Compute the clip flags
	vcmp.q		LT, R202, R202[-w,-w,-w,0]	// x < -w, y < -w, z < -w
	vnop
	mfvc		$t4, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG (will become X_POS/Y_POS/Z_POS later)
	vcmp.q		GT, R202, R202[w,w,w,0]		// x > w, y > w, z > w
	andi		$t4, $t4, 0x7				// Mask out the condition codes we don't care about
	sll			$t4, $t4, 3					// Shift up to create X_POS/Y_POS/Z_POS	
	mfvc		$t5, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t5, $t5, 0x7				// Mask out the condition codes we don't care about
	or			$t4, $t4, $t5
	sw			$t4, 0x38($a3)				// Store ClipFlags

# Normalise the RGBA colour
 	xori		$v0, $a2, 0x2				// = vtx_ptr ^ 2
	lhu			$t4, 0($v0)					// get vertex color	(hi)
	addiu		$a2, $a2, 0x2				// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2				// = vtx_ptr ^ 2
	lhu			$t5, 0($v0)					// get vertex color (lo)
	sll			$t4, $t4, 16				// pack
 	or		    $t5, $t5, $t4				// to 32bit
	addiu		$a2, $a2, 0x2				// = vtx_ptr + 2
	mtv			$t5, S200					// store on VFPU

	vuc2ifs.s		R200, S200				// R200 = [a,b,g,r]
	vi2f.q		R200, R200[w,z,y,x], 31		// int -> float, R200 = [r * 1/256, g * 1/256, b * 1/256, a * 1/256]
	sv.q		R200, 0x20($a3)				// Store colour

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
	lv.q		R100, 0($a1)				// Load mat worldproject (matrix[0] and only 3x3 is needed)
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)

	lv.s		S000, 128($a1)				// Load element 0 in matrix[2]
	lv.s		S020, 168($a1)				// Load element 10 in matrix[2]
	vmov.s		S010, S000					// Copy element 0 (instead of 5)
	vfim.s		S011, 0.375					// Y scale factor
	vmul.t		R000, R000, R001[1/2,y,1/2]	// Prepare X and Z scaling values  
	vscl.t		C100, C100, S000			// Scale X matrix colum 
	vscl.t		C110, C110, S010			// Scale Y matrix colum 
	vscl.t		C120, C120, S020			// Scale Z matrix colum 
	 
	lv.q		R003, -64($a3)				// Get base vector to add to the billbord geometry (in position 0)

	sll			$v0, $a0, 1					// count * 2
	sll			$a0, $a0, 3					// count * 8
	addu		$a0, $v0, $a0				// count = count * 10
	addu		$a0, $a2, $a0				// end_ptr = start_ptr + count * 10
	beq			$a2, $a0, finished_DKRB
	vone.s		S233						// w = 1.0f
	
next_vertex_DKRB:
# Load and transform this vertex position
 	xori		$v0, $a2, 0x2				// = vtx_ptr ^ 2
	lh			$t4, 0($v0)					// get vertex x coord	
	mtv			$t4, S203					// store on VFPU
	addiu		$a2, $a2, 0x2				// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2				// = vtx_ptr ^ 2
	lh			$t4, 0($v0)					// get vertex y coord
	mtv			$t4, S213					// store on VFPU
	addiu		$a2, $a2, 0x2				// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2				// = vtx_ptr ^ 2
	lh			$t4, 0($v0)					// get vertex z coord
	mtv			$t4, S223					// store on VFPU
	addiu		$a2, $a2, 0x2				// = vtx_ptr + 2
	vi2f.t		R203, R203, 0				// int -> float (x,y,z,1)
	vtfm3.t		R202, M100, R203			// 3x3 transform

# Clip flags
	sw			$zero, 0x38($a3)			// Clear ClipFlags

	vadd.t		R203, R202, R003			// Add basevector
	sv.q		R203, 0x00($a3)				// Store world transform (x,y,z,1)
	

# Normalise the RGBA colour
 	xori		$v0, $a2, 0x2				// = vtx_ptr ^ 2
	lhu			$t4, 0($v0)					// get vertex color	(hi)
	addiu		$a2, $a2, 0x2				// = vtx_ptr + 2
 	xori		$v0, $a2, 0x2				// = vtx_ptr ^ 2
	lhu			$t5, 0($v0)					// get vertex color (lo)
	sll			$t4, $t4, 16				// pack
 	or		    $t5, $t5, $t4				// to 32bit
	addiu		$a2, $a2, 0x2				// = vtx_ptr + 2
	mtv			$t5, S200					// store on VFPU

	vuc2ifs.s		R200, S200				// R200 = [a,b,g,r]
	vi2f.q		R200, R200[w,z,y,x], 31		// int -> float, R200 = [r * 1/256, g * 1/256, b * 1/256, a * 1/256]
	sv.q		R200, 0x20($a3)				// Store colour

# Continue with the next vertex
	bne			$a2, $a0, next_vertex_DKRB
	addiu		$a3, $a3, 64				// Next output vertex

finished_DKRB:	
	jr			$ra
	nop
	
	
.set pop
