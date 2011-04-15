#include "as_reg_compat.h"

.text
.set		push
.set		noreorder
.set		noat

.macro TransformWithLightingM	function, fog_mode, texture_mode
.global \function
\function:

#define PARAMS_AMBIENT		0x00
#define PARAMS_FOG_TXSCAL	0x10

############################
#	a0 - world matrix				- must be aligned to 16 bytes
#	a1 - world*projection matrix	- must be aligned to 16 bytes
#	a2 - Fiddled vertices			- stride 16
#	a3 - Output vertices			- must be aligned to 16 bytes, stride 64
#	t0 - num vertices
#	t1 - params
#	t2 - lights
#	t3 - num_lights
	lv.q		R000, 0($a0)		// Load mat world
	lv.q		R001, 16($a0)
	lv.q		R002, 32($a0)
	lv.q		R003, 48($a0)

	lv.q		R100, 0($a1)		// Load mat project
	lv.q		R101, 16($a1)
	lv.q		R102, 32($a1)
	lv.q		R103, 48($a1)
	
	lv.q		R700, PARAMS_AMBIENT($t1)		// Load ambient
	lv.q		R701, PARAMS_FOG_TXSCAL($t1)	// Load params [fog_m, fog_o, tscale_x, tscale_y]
	
	# Load 1/256 (vuc2i/vi2f end up converting 0xff to 256.0)
	vfim.s		S400, 0.00390625
	
	# Calculate the last light index
	or			$t6, $t2, $0		// first_light = p_lights
	sll			$t7, $t3, 5			// num_lights*32
	addu		$t7, $t6, $t7		// last_light = p_lights + num_lights*32
	
	sll			$t0, $t0, 4			// count = count * 16
	addu		$t0, $a2, $t0		// end_ptr = start_ptr + count * 16

	beq			$a2, $t0, finished_\function
	nop

next_vertex_\function:
	# Load and transform this vertex position
 	lv.s		S200, 0($a2)				// load word [y,x,?,z]
 	lv.s		S210, 4($a2)				//		should align this to 16 bytes so we can do a single load?
	vs2i.p		R200, R200					// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 16				// int -> float
	vmov.q		R200, R200[y,x,w,1]

	vtfm4.q		R201, M000, R200			// World transform
	vtfm4.q		R202, M100, R200			// Projection transform
	
	sv.q		R201, 0($a3)				// Store world transform
	sv.q		R202, 16($a3)				// Store projection transform
	vrcp.s		S231, S232					// 1.0 / projected.w
	sv.s		S231, 60($a3)				// Store 1.0 / projected.w
	
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
	vcmp.q		LT, R202, R202[-w,-w,-w,0]	// x < -w, y < -w, z < -w
	vnop
	mfvc		$t4, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG (will become X_POS/Y_POS/Z_POS later)
	andi		$t4, $t4, 0x7				// Mask out the condition codes we don't care about
	
	vcmp.q		GT, R202, R202[w,w,w,0]		// x > w, y > w, z > w
	vnop
	mfvc		$t5, $131					// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t5, $t5, 0x7				// Mask out the condition codes we don't care about
	
	nor			$t1, $t4, $t4				// If X_POS etc is set, don't set X_NEG
	and			$t5, $t5, $t1				// mask out any values which are already set
	sll			$t4, $t4, 3					// Shift up to create X_POS/Y_POS/Z_POS	
	or			$t4, $t4, $t5

	sw			$t4, 56($a3)				// Store ClipFlags
	
	
	# Convert the alpha in R200 to float and pass it along to light color
	lv.s		S200, 12($a2)				// load normal word [w,z,y,x]
	.word		0xd0380000 | (8<<8) | (43)	// vuc2i.s	R203, S200					// R200 = [?,z,y,x]
	vi2f.s		S203, S203, 23				// int -> float
	vmul.s		S431, S400, S203			// R431 = [a * 1/256]
	
	# Convert the normal in R200 to float and transform
	.word		0xd0398080 | (8<<8) | (40)	// vc2i.s		R200, S200					// R200 = [?,z,y,x]
	vi2f.q		R200, R200, 0				// int -> float (obliterates world transform)
	vmov.q		R201, R200[w,z,y,0]			// Unfiddle
	vmov.q		R203, R201					// store vertice normal temporary for env map later
	vtfm3.t		R200, M000, R201			// Transform with world matrix (only need 3x3)//Corn
	
	vdot.t		S201, R200, R200			// S201 = x*x + y*y + z*z
	vrsq.s		S201, S201					// S201 = 1/sqrt(x*x + y*y + z*z)
	vscl.t		R200, R200, S201			// S200 = v.normalise().


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
	

.elseif \texture_mode == 2
# EnvMapped G_TEXTURE_GEN_LINEAR
#	t.x = (0.5f * ( 1.0f + ( n.x*world.m11 + n.y*world.m21 + n.z*world.m31 ) ));
#	t.y = (0.5f * ( 1.0f - ( n.x*world.m12 + n.y*world.m22 + n.z*world.m32 ) ));
//	vdot.t		S202, R200, C000					// n.x*m.11 + n.y*m.21 + n.z*m.31
//	vdot.t		S203, R200, C010					// n.x*m.12 + n.y*m.22 + n.z*m.32
//	vadd.p      C202, C202[1,1], C202[x,-y]			// 1+x, 1-y
//	vmul.p		C202, C202[1/2,1/2], C202			// * 0.5
//	sv.s		S202, 0x30($a3)						// Store Texture.x
//	sv.s		S203, 0x34($a3)						// Store Texture.y

# We use worldproject matrix to calc normals it gives a nicer effect (model view result is in R200) //Corn
	vtfm3.t		R202, M100, R203					// Transform with projworld matrix, looks nicer (only need 3x3)
	vdot.t		S201, R202, R202					// S201 = x*x + y*y + z*z
	vrsq.s		S201, S201							// S201 = 1/sqrt(x*x + y*y + z*z)
	vscl.q		R202, R202, S201					// R202 = v.normalise().

# t.x = Acos(n.x) / Pi
# t.y = Acos(n.y) / Pi
//	vasin.p		R202, R202							// Asin()
//	vocp.p		R202, R202							// 1.0-Asin() = Acos()
//	vmul.p		R202, R202[1/2,1/2], R202			// * 0.5
//	sv.s		S202, 0x30($a3)						// Store Texture.x
//	sv.s		S212, 0x34($a3)						// Store Texture.y

# Cheap way to do acos(x)/PI -> 0.5f - 0.25f * absf(x) - 0.25f * absf(x) * absf(x) * absf(x) //Corn
	vmov.p		R202, R202[|x|,|y|]					// absf(x), absf(y)
	vmov.p		R203, R200[1/2,1/2]					// result = 0.5
	vmul.p		R222, R202[1/4,1/4], R202			// X * 0.25, Y * 0.25
	vsub.p		R203, R203, R222					// result -= X * 0.25
	vmul.p		R222, R202, R222					// X * X * 0.25, Y * Y * 0.25
	vmul.p		R222, R202, R222					// X * X * X * 0.25, Y * Y * Y * 0.25
	vsub.p		R203, R203, R222					// result -= X * X * X * 0.25
	sv.s		S203, 0x30($a3)						// Store Texture.x
	sv.s		S213, 0x34($a3)						// Store Texture.y

.elseif \texture_mode == 3
# EnvMapped G_TEXTURE_GEN
# We use worldproject matrix to calc normals it gives a nicer effect (model view result is in R200) //Corn
	vtfm3.t		R202, M100, R203					// Transform with projworld matrix, looks nicer (only need 3x3)
	vdot.t		S201, R202, R202					// S201 = x*x + y*y + z*z
	vrsq.s		S201, S201							// S201 = 1/sqrt(x*x + y*y + z*z)
	vscl.q		R202, R202, S201					// R202 = v.normalise().

# t.x = 0.5 * (1.0 + n.x)
# t.y = 0.25 * (1.0 + n.y)
	vadd.p		R202, R202[1,1], R202[x,y]			// 1+x, 1+y
	vmul.p		R202, R202[1/2,1/2], R202			// X * 0.5, Y * 0.25
	sv.s		S202, 0x30($a3)						// Store Texture.x
	sv.s		S212, 0x34($a3)						// Store Texture.y
	
.endif

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
	# R400: 1/256
	# R431: current vertex Alpha value
	# R600: Computed fog colour
	# R700: Ambient
	# t1 = last_light
	# t4 = cur_light
	# t6 = first_light
	# t7 = last_light
	
	vmov.q		R201, R700					// Colour = ambient

	beq			$t6, $t7, done_lighting_\function		// cur_light == last_light?
	or			$t4, $t6, $0				// cur_light = p_lights
next_light_\function:
	lv.q		R301, 0($t4)				// Load normal
	vdot.t		S303[0:1], R200, R301		// x = clamp(dot(normal,(x,y,z,0)),0,1)

	lv.q		R302, 16($t4)				// Load colour
	addiu		$t4, $t4, 32				// Skip to the next light
	vscl.t		R303, R302, S303			// r,g,b,a = r*x, g*x, b*x, a*x

	bne			$t4, $t7, next_light_\function
	vadd.t		R201, R201, R303			// col += r,g,b,a

done_lighting_\function:
	vmov.t		R401[0:1,0:1,0:1], R201		// Clamp 0..1 and merge with vertex alpha in S431
	
.if \fog_mode == 1
	# Merge in the computed fog colour
	vmov.s		S431, S600
.endif

	
	addiu		$a2, $a2, 16				// Next input vertex
	sv.q		R401, 32($a3)				// Store colour

	# Continue with the next vertex
	bne			$a2, $t0, next_vertex_\function
	addiu		$a3, $a3, 64				// Next output vertex

finished_\function:	
	jr			$ra
	nop
.endm

TransformWithLightingM _TransformVerticesWithLighting_f0_t0, 0, 0
TransformWithLightingM _TransformVerticesWithLighting_f0_t1, 0, 1
TransformWithLightingM _TransformVerticesWithLighting_f0_t2, 0, 2
TransformWithLightingM _TransformVerticesWithLighting_f0_t3, 0, 3
TransformWithLightingM _TransformVerticesWithLighting_f1_t0, 1, 0
TransformWithLightingM _TransformVerticesWithLighting_f1_t1, 1, 1
TransformWithLightingM _TransformVerticesWithLighting_f1_t2, 1, 2


.global _CalcClipFlagsVFPU
_CalcClipFlagsVFPU:
	lv.q		R000, 0($a0)		// Load vertex
	
	vcmp.q		GT, R000[-x,-y,-z,0], R000[w,w,w,0]		//-x>w,-y>w,-z>w
	vnop
	mfvc		$v0, $131			// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$v0, $v0, 0x7
	
	vcmp.q		GT, R000, R000[w,w,w,0]					// x>w, y>w, z>w
	vnop
	mfvc		$t0, $131			// VFPU_CC. Corresponds to X_NEG/Y_NEG/Z_NEG
	andi		$t0, $t0, 0x7
	
	nor			$t1, $v0, $v0
	and			$t0, $t0, $t1		// mask out any values which are already set


	sll			$v0, $v0, 3			// Shift up to create X_POS/Y_POS/Z_POS	
	jr			$ra
	or			$v0, $v0, $t0


.set pop