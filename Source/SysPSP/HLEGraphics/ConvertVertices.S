#include "as_reg_compat.h"

	.global		_ConvertVertices
	.global		_ConvertVerticesIndexed
	
.text
	.set		push
	.set		noreorder
	.set		noat

############################
#	a0 - Output vertices		- DaedalusVtx / stride 24
#	a1 - Input vertices			- DaedalusVtx4 / must be aligned to 16 bytes, stride 64
#	a2 - num vertices
#
#	The code handles the following:
#
#	for( u32 i = 0; i < num_vertices; ++i )
#	{
#		p_vertices[ i ].Texture = clipped_vertices[ i ].Texture;
#		p_vertices[ i ].Colour = c32( clipped_vertices[ i ].Colour );
#		p_vertices[ i ].Position.x = clipped_vertices[ i ].TransformedPos.x;
#		p_vertices[ i ].Position.y = clipped_vertices[ i ].TransformedPos.y;
#		p_vertices[ i ].Position.z = clipped_vertices[ i ].TransformedPos.z;
#	}
_ConvertVertices:

	sll			$a2, $a2, 6			// count = count * 64
	addu		$a2, $a1, $a2		// end_ptr = start_ptr + count * 64

	beq			$a1, $a2, finished_convert
	nop

	viim.s		S001, 255

next_vertex_convert:
	lw			$t0, 0x00( $a1 )	// TransformedPos.x
	lw			$t1, 0x04( $a1 )	// TransformedPos.y
	lw			$t2, 0x08( $a1 )	// TransformedPos.z
	lv.q		R000, 0x20( $a1 )	// Load Colour
	lw			$t3, 0x30( $a1 )	// Texture.x
	lw			$t4, 0x34( $a1 )	// Texture.y
	
	sw			$t0, 0x0c( $a0 )	// Position.x
	vsat0.q		R000, R000
	sw			$t1, 0x10( $a0 )	// Position.y
	vscl.q		R000, R000, S001
	sw			$t2, 0x14( $a0 )	// Position.z
	vf2iz.q		R000, R000, 23
	sw			$t3, 0x00( $a0 )	// Texture.x
	vi2uc.q		S000, R000
	sw			$t4, 0x04( $a0 )	// Texture.y
	
	sv.s		S000, 0x08($a0)		// Colour

	# Continue with the next vertex
	addiu		$a1, $a1, 64		// Next input vertex
	bne			$a1, $a2, next_vertex_convert
	addiu		$a0, $a0, 24		// Next output vertex
	

finished_convert:
	jr			$ra
	nop
	
############################
#	a0 - Output vertices		- DaedalusVtx / stride 24
#	a1 - Input vertices			- DaedalusVtx4 / must be aligned to 16 bytes, stride 64
#	a2 - num vertices
#	a3 - indices
#
#	As above, but vertices are indexed:
#	
_ConvertVerticesIndexed:

	sll			$a2, $a2, 1			// count = count * 2
	addu		$a2, $a3, $a2		// end_ptr = start_ptr + count * 64

	beq			$a3, $a2, finished_convert_idx
	nop

	viim.s		S001, 255

next_vertex_convert_idx:
	lhu			$at, 0x00( $a3 )	// Load index
	sll			$at, $at, 6			// offset = index * 64
	addu		$at, $a1, $at		// address = src + index*64

	lw			$t0, 0x00( $at )	// TransformedPos.x
	lw			$t1, 0x04( $at )	// TransformedPos.y
	lw			$t2, 0x08( $at )	// TransformedPos.z
	lv.q		R000, 0x20( $at )	// Load Colour
	lw			$t3, 0x30( $at )	// Texture.x
	lw			$t4, 0x34( $at )	// Texture.y
	
	sw			$t0, 0x0c( $a0 )	// Position.x
	vsat0.q		R000, R000
	sw			$t1, 0x10( $a0 )	// Position.y
	vscl.q		R000, R000, S001
	sw			$t2, 0x14( $a0 )	// Position.z
	vf2iz.q		R000, R000, 23
	sw			$t3, 0x00( $a0 )	// Texture.x
	vi2uc.q		S000, R000
	sw			$t4, 0x04( $a0 )	// Texture.y

	sv.s		S000, 0x08($a0)		// Colour

	# Continue with the next vertex
	addiu		$a3, $a3, 2			// Next input index
	bne			$a3, $a2, next_vertex_convert_idx
	addiu		$a0, $a0, 24		// Next output vertex
	

finished_convert_idx:
	jr			$ra
	nop
	
	.set pop

