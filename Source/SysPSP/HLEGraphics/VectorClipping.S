#include "as_reg_compat.h"

	.set noat

	
	.global		_ClipToHyperPlane

.text
	.set		push
	.set		noreorder
	
###############################################################################
#
#	a0		dest vertices			- must be aligned to 16 bytes
#	a1		source vertices			- must be aligned to 16 bytes
#	a2		plane					- must be aligned to 16 bytes
#	a3		in_vertex_count			- must be > 0 or we need to re apply check
#
#	v0		out_vertex_count
#
#	Trashes M000, M100, M200, M300
#
_ClipToHyperPlane:

	#
	#	a0			- current_dest
	#	a1			- current_source
	#	a2			- source (copy of original)
	#	a3			- verts_remaining
	#	t0			- temp
	#	t1			- source_end
	#	R000		- plane
	#	S003,013	- aDotPlane, bDotPlane, ...
	#	M100		- clipped_out
	#	M200		- a
	#	M300		- b
	#
	
	//beq			$a3, $0, finished_all_vertices	// verts_remaining == 0 --> finished_all_vertices
	//nop

	lv.q		R000, 0($a2)				// load plane equation (we reuse a2 for another purpose)
	
	or			$a2, $a1, $0				// take a copy of the original source pointer
	sll			$t1, $a3, 6					// t1 = num_verts*64
	addu		$t1, $a1, $t1				// source_end = source + num_verts*64
	
	// load initial values for b
	lv.q		R300, 0($a1)				// TransformedPos(V4)
	lv.q		R301, 16($a1)				// ProjectedPos(V4)
	lv.q		R302, 32($a1)				// Color(V4)
	lv.q		R303, 48($a1)				// Texture(V2) + Clipflags + 1/w
	addiu		$a1, $a1, 64				// skip the first vertex

	vdot.q		S013, R301, R000			// bDotPlane = dot( b.ProjectedPos, plane )

	or			$v0, $0, $0					// out_count = 0
	
get_next_vertex:
	// Wrap around the source pointer if necessary
	bne			$a1, $t1, load_a
	nop
	or			$a1, $a2, $0				// current_source = source

load_a:
	lv.q		R201, 16($a1)
	lv.q		R200, 0($a1)				// load a
	vdot.q		S003, R201, R000			// aDotPlane = dot( a.ProjectedPos, plane )
	lv.q		R202, 32($a1)
	vcmp.s		GT, S003, S003[0]
	lv.q		R203, 48($a1)

	bvt			0, a_is_outside				// aDotPlane > 0 --> a_is_outside
	nop
	
a_is_inside:
	vcmp.s		LE, S013, S013[0]
	bvt			0, a_is_inside_copy			// bDotPlane <= 0 --> a_is_inside_copy
	nop

	
	// Calculate the parametric intersection point
	vsub.q		R001, R301, R201			// R001 = b.ProjectedPos - a.ProjectedPos
	vdot.q		S001, R001, R000			// S001 = dot( (b.ProjectedPos - a.ProjectedPos), plane )
	vrcp.s		S001, S001
	vmul.s		S001, S013, S001

	// Interpolate a/b, output point
	vsub.q		R100, R200, R300			// (a-b)
	vsub.q		R101, R201, R301			//
	vsub.q		R102, R202, R302			//
	vsub.q		R103, R203, R303			//

	vmscl.q		M100, M100, S001			// (a-b)*x
	//vscl.q		R100, R100, S001			// (a-b)*x
	//vscl.q		R101, R101, S001			//
	//vscl.q		R102, R102, S001			//
	//vscl.q		R103, R103, S001			//

	vadd.q		R100, R300, R100			// b + (a-b)*x
	vadd.q		R101, R301, R101			//
	sv.q		R100, 0($a0)				// *out = *tmp
	vadd.q		R102, R302, R102			//
	sv.q		R101, 16($a0)
	vadd.q		R103, R303, R103			//
	sv.q		R102, 32($a0)
	addiu		$v0, $v0, 1					// outcout++
	sv.q		R103, 48($a0)
	addiu		$a0, $a0, 64				// out++
	
a_is_inside_copy:
	
	sv.q		R200, 0($a0)				// *out = *a
	sv.q		R201, 16($a0)
	sv.q		R202, 32($a0)
	sv.q		R203, 48($a0)

	addiu		$a0, $a0, 64				// out++
	b			finished_vertex
	addiu		$v0, $v0, 1					// outcout++

a_is_outside:
	vcmp.s		GT, S013, S013[0]
	bvt			0, finished_vertex			// bDotPlane <= 0 --> a_is_outside_copy
	nop
	
	// Calculate the parametric intersection point
	vsub.q		R001, R301, R201			// R001 = b.ProjectedPos - a.ProjectedPos
	vdot.q		S001, R001, R000			// S001 = dot( (b.ProjectedPos - a.ProjectedPos), plane )
	vrcp.s		S001, S001
	vmul.s		S001, S013, S001

	// Interpolate a/b, output point
	vsub.q		R100, R200, R300			// (a-b)
	vsub.q		R101, R201, R301			//
	vsub.q		R102, R202, R302			//
	vsub.q		R103, R203, R303			//

	vmscl.q		M100, M100, S001			// (a-b)*x
	//vscl.q		R100, R100, S001			// (a-b)*x
	//vscl.q		R101, R101, S001			//
	//vscl.q		R102, R102, S001			//
	//vscl.q		R103, R103, S001			//

	vadd.q		R100, R300, R100			// b + (a-b)*x
	vadd.q		R101, R301, R101			//
	sv.q		R100, 0($a0)				// *out = *tmp
	vadd.q		R102, R302, R102			//
	sv.q		R101, 16($a0)
	vadd.q		R103, R303, R103			//
	sv.q		R102, 32($a0)
	addiu		$v0, $v0, 1					// outcout++
	sv.q		R103, 48($a0)
	addiu		$a0, $a0, 64				// out++
	
finished_vertex:
	vmmov.q		M300, M200					// b = lat_point
	vmov.s		S013, S003					// move old aDotPlane as new bDotPlane

	addiu		$a3, $a3, -1				// verts_remaining--
	bne			$a3, $0, get_next_vertex	// verts_remaining != 0 --> get_next_vertex
	addiu		$a1, $a1, 64				// source++
	
finished_all_vertices:
	jr			$ra
	nop


	.set pop
