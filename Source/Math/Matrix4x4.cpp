#include "stdafx.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Math/Math.h"	// VFPU Math
#include "Math/MathUtil.h" // Swap

#include <malloc.h>
#include <pspvfpu.h>

// http://forums.ps2dev.org/viewtopic.php?t=5557
// http://bradburn.net/mr.mr/vfpu.html

// Many of these mtx funcs should be inline since they are simple enough and called frequently - Salvy

#ifdef DAEDALUS_PSP_USE_VFPU
//*****************************************************************************
//
//*****************************************************************************
/*
inline void vsincosf(float angle, v4* result)
{
	__asm__ volatile (
		"mtv %1, S000\n"
		"vrot.q C010, S000, [s, c, 0, 0]\n"
		"usv.q C010, 0 + %0\n"
	: "+m"(*result) : "r"(angle));
}
*/
//*****************************************************************************
//
//*****************************************************************************
void matrixMultiplyUnaligned(Matrix4x4 * m_out, const Matrix4x4 *mat_a, const Matrix4x4 *mat_b)
{
	__asm__ volatile (

		"ulv.q   R000, 0  + %1\n"
		"ulv.q   R001, 16 + %1\n"
		"ulv.q   R002, 32 + %1\n"
		"ulv.q   R003, 48 + %1\n"

		"ulv.q   R100, 0  + %2\n"
		"ulv.q   R101, 16 + %2\n"
		"ulv.q   R102, 32 + %2\n"
		"ulv.q   R103, 48 + %2\n"

		"vmmul.q   M200, M000, M100\n"

		"usv.q   R200, 0  + %0\n"
		"usv.q   R201, 16 + %0\n"
		"usv.q   R202, 32 + %0\n"
		"usv.q   R203, 48 + %0\n"

		: "=m" (*m_out) : "m" (*mat_a) ,"m" (*mat_b) : "memory" );
}

//*****************************************************************************
//
//*****************************************************************************
void matrixMultiplyAligned(Matrix4x4 * m_out, const Matrix4x4 *mat_a, const Matrix4x4 *mat_b)
{
	__asm__ volatile (

		"lv.q   R000, 0  + %1\n"
		"lv.q   R001, 16 + %1\n"
		"lv.q   R002, 32 + %1\n"
		"lv.q   R003, 48 + %1\n"

		"lv.q   R100, 0  + %2\n"
		"lv.q   R101, 16 + %2\n"
		"lv.q   R102, 32 + %2\n"
		"lv.q   R103, 48 + %2\n"

		"vmmul.q   M200, M000, M100\n"

		"sv.q   R200, 0  + %0\n"
		"sv.q   R201, 16 + %0\n"
		"sv.q   R202, 32 + %0\n"
		"sv.q   R203, 48 + %0\n"

		: "=m" (*m_out) : "m" (*mat_a) ,"m" (*mat_b) : "memory" );
}

//*****************************************************************************
//
//*****************************************************************************
/*
void myCopyMatrix(Matrix4x4 *m_out, const Matrix4x4 *m_in)
{
	__asm__ volatile (
		"lv.q   R000, 0x0(%1)\n"
		"lv.q   R001, 0x10(%1)\n"
		"lv.q   R002, 0x20(%1)\n"
		"lv.q   R003, 0x30(%1)\n"

		"sv.q   R000, 0x0(%0)\n"
		"sv.q   R001, 0x10(%0)\n"
		"sv.q   R002, 0x20(%0)\n"
		"sv.q   R003, 0x30(%0)\n"
	: : "r" (m_out) , "r" (m_in) );
}
*/
//*****************************************************************************
//
//*****************************************************************************
/*
void myApplyMatrix(v4 *v_out, const Matrix4x4 *mat, const v4 *v_in)
{
	__asm__ volatile (
		"lv.q   R000, 0x0(%1)\n"
		"lv.q   R001, 0x10(%1)\n"
		"lv.q   R002, 0x20(%1)\n"
		"lv.q   R003, 0x30(%1)\n"

		"lv.q   R100, 0x0(%2)\n"

		"vtfm4.q R200, E000, R100\n"
		"sv.q   R200, 0x0(%0)\n"
	: : "r" (v_out) , "r" (mat) ,"r" (v_in) );
}*/
#endif // DAEDALUS_PSP_USE_VFPU
//*****************************************************************************
//
//*****************************************************************************
Matrix4x4 & Matrix4x4::SetIdentity()
{
	*this = gMatrixIdentity;
	return *this;
}

//*****************************************************************************
//
//*****************************************************************************
Matrix4x4 & Matrix4x4::SetScaling( float scale )
{
	for ( u32 r = 0; r < 4; ++r )
	{
		for ( u32 c = 0; c < 4; ++c )
		{
			m[ r ][ c ] = ( r == c ) ? scale : 0;
		}
	}
	return *this;
}

//*****************************************************************************
//
//*****************************************************************************
Matrix4x4 & Matrix4x4::SetRotateX( float angle )
{
//	float	s( vfpu_sinf( angle ) );
//	float	c( vfpu_cosf( angle ) );
	float	s;
	float	c;
	vfpu_sincos(angle, &s, &c);

	m11 = 1;	m12 = 0;	m13 = 0;	m14 = 0;
	m21 = 0;	m22 = c;	m23 = -s;	m24 = 0;
	m31 = 0;	m32 = s;	m33 = c;	m34 = 0;
	m41 = 0;	m42 = 0;	m43 = 0;	m44 = 1;
	return *this;
}

//*****************************************************************************
//
//*****************************************************************************
Matrix4x4 & Matrix4x4::SetRotateY( float angle )
{
//	float	s( vfpu_sinf( angle ) );
//	float	c( vfpu_cosf( angle ) );
	float	s;
	float	c;
	vfpu_sincos(angle, &s, &c);

	m11 = c;	m12 = 0;	m13 = s;	m14 = 0;
	m21 = 0;	m22 = 1;	m23 = 0;	m24 = 0;
	m31 = -s;	m32 = 0;	m33 = c;	m34 = 0;
	m41 = 0;	m42 = 0;	m43 = 0;	m44 = 1;
	return *this;
}

//*****************************************************************************
//
//*****************************************************************************
Matrix4x4 & Matrix4x4::SetRotateZ( float angle )
{
//	float	s( vfpu_sinf( angle ) );
//	float	c( vfpu_cosf( angle ) );
	float	s;
	float	c;
	vfpu_sincos(angle, &s, &c);

	m11 = c;	m12 = -s;	m13 = 0;	m14 = 0;
	m21 = s;	m22 = c;	m23 = 0;	m24 = 0;
	m31 = 0;	m32 = 0;	m33 = 1;	m34 = 0;
	m41 = 0;	m42 = 0;	m43 = 0;	m44 = 1;
	return *this;
}

//*****************************************************************************
//
//*****************************************************************************
v3 Matrix4x4::TransformCoord( const v3 & vec ) const
{
	return v3( vec.x * m11 + vec.y * m21 + vec.z * m31 + m41,
			   vec.x * m12 + vec.y * m22 + vec.z * m32 + m42,
			   vec.x * m13 + vec.y * m23 + vec.z * m33 + m43 );
}

//*****************************************************************************
//
//*****************************************************************************
v3 Matrix4x4::TransformNormal( const v3 & vec ) const
{
	return v3( vec.x * m11 + vec.y * m21 + vec.z * m31,
			   vec.x * m12 + vec.y * m22 + vec.z * m32,
			   vec.x * m13 + vec.y * m23 + vec.z * m33 );
}

//*****************************************************************************
//
//*****************************************************************************
v4 Matrix4x4::Transform( const v4 & vec ) const
{
	return v4( vec.x * m11 + vec.y * m21 + vec.z * m31 + vec.w * m41,
			   vec.x * m12 + vec.y * m22 + vec.z * m32 + vec.w * m42,
			   vec.x * m13 + vec.y * m23 + vec.z * m33 + vec.w * m43,
			   vec.x * m14 + vec.y * m24 + vec.z * m34 + vec.w * m44 );
}

//*****************************************************************************
//
//*****************************************************************************
v3 Matrix4x4::Transform( const v3 & vec ) const
{
	v4	trans( vec.x * m11 + vec.y * m21 + vec.z * m31 + m41,
			   vec.x * m12 + vec.y * m22 + vec.z * m32 + m42,
			   vec.x * m13 + vec.y * m23 + vec.z * m33 + m43,
			   vec.x * m14 + vec.y * m24 + vec.z * m34 + m44 );

	if(pspFpuAbs(trans.w) > 0.0f)
	{
		return v3( trans.x / trans.w, trans.y / trans.w, trans.z / trans.w );
	}

	return v3(trans.x, trans.y, trans.z);
}

//*****************************************************************************
//
//*****************************************************************************
Matrix4x4		Matrix4x4::Transpose() const
{
	return Matrix4x4( m11, m21, m31, m41,
					  m12, m22, m32, m42,
					  m13, m23, m33, m43,
					  m14, m24, m34, m44 );
}

//*****************************************************************************
//
//*****************************************************************************
Matrix4x4	Matrix4x4::Inverse() const
{
	/*Matrix4x4 temp;

	invert_matrix_general( (const float*)m, (float*)temp.m );

	return temp;
*/
	float	augmented[ 4 ][ 8 ];

	//
	// Init augmented array
	//
	for ( u32 r = 0; r < 4; ++r )
	{
		for ( u32 c = 0; c < 8; ++c )
		{
			if ( c < 4 )
			{
				augmented[ r ][ c ] = m[ r ][ c ];
			}
			else
			{
				augmented[ r ][ c ] = float( ( c == r + 4 ) ? 1 : 0 );
			}
		}
	}

	for ( u32 j = 0; j < 4; ++j )
	{
		bool	found( false );
		for ( u32 i = j; i < 4; ++i )
		{
			if ( augmented[ i ][ j ] != 0 )
			{
				if ( i != j )
				{
					// Exchange rows i and j
					for ( u32 k = 0; k < 8; ++k )
					{
						Swap( augmented[ i ][ k ], augmented[ j ][ k ] );
					}
				}
				found = true;
				break;
			}
		}

		if ( found == false )
		{
			return gMatrixIdentity;
		}

		//
		// Multiply the row by 1/Mjj
		//
		float	rcp( 1.0f / augmented[ j ][ j ] );
		for ( u32 k = 0; k < 8; ++k )
		{
			augmented[ j ][ k ] *= rcp;
		}

		for ( u32 r = 0; r < 4; ++r )
		{
			float	q( -augmented[ r ][ j ] );
			if ( r != j )
			{
				for ( u32 k = 0; k < 8; k++ )
				{
					augmented[ r ][ k ] += q * augmented[ j ][ k ];
				}
			}
		}
	}


	return Matrix4x4( augmented[ 0 ][ 4 ], augmented[ 0 ][ 5 ], augmented[ 0 ][ 6 ], augmented[ 0 ][ 7 ],
					  augmented[ 1 ][ 4 ], augmented[ 1 ][ 5 ], augmented[ 1 ][ 6 ], augmented[ 1 ][ 7 ],
					  augmented[ 2 ][ 4 ], augmented[ 2 ][ 5 ], augmented[ 2 ][ 6 ], augmented[ 2 ][ 7 ],
					  augmented[ 3 ][ 4 ], augmented[ 3 ][ 5 ], augmented[ 3 ][ 6 ], augmented[ 3 ][ 7 ] );
}
//*****************************************************************************
//
//*****************************************************************************
/*
void myMulMatrixCPU(Matrix4x4 * m_out, const Matrix4x4 *mat_a, const Matrix4x4 *mat_b)
{
	for ( u32 i = 0; i < 4; ++i )
	{
		for ( u32 j = 0; j < 4; ++j )
		{
			m_out->m[ i ][ j ] = mat_a->m[ i ][ 0 ] * mat_b->m[ 0 ][ j ] +
							mat_a->m[ i ][ 1 ] * mat_b->m[ 1 ][ j ] +
							mat_a->m[ i ][ 2 ] * mat_b->m[ 2 ][ j ] +
							mat_a->m[ i ][ 3 ] * mat_b->m[ 3 ][ j ];
		}
	}

}
*/

//#include "Utility/Timing.h"
//*****************************************************************************
//
//*****************************************************************************
Matrix4x4 Matrix4x4::operator*( const Matrix4x4 & rhs ) const
{
	Matrix4x4 r;

//VFPU
#ifdef DAEDALUS_PSP_USE_VFPU
	matrixMultiplyUnaligned( &r, this, &rhs );
//CPU
#else
	for ( u32 i = 0; i < 4; ++i )
	{
		for ( u32 j = 0; j < 4; ++j )
		{
			r.m[ i ][ j ] = m[ i ][ 0 ] * rhs.m[ 0 ][ j ] +
							m[ i ][ 1 ] * rhs.m[ 1 ][ j ] +
							m[ i ][ 2 ] * rhs.m[ 2 ][ j ] +
							m[ i ][ 3 ] * rhs.m[ 3 ][ j ];
		}
	}
#endif
	return r;
}

//*****************************************************************************
//
//*****************************************************************************
const Matrix4x4	gMatrixIdentity( 1.0f, 0.0f, 0.0f, 0.0f,
								 0.0f, 1.0f, 0.0f, 0.0f,
								 0.0f, 0.0f, 1.0f, 0.0f,
								 0.0f, 0.0f, 0.0f, 1.0f );
//void test( const Matrix4x4 & rhs )
//{
//	Matrix4x4 r;
//	Matrix4x4	r2;
//
//	Matrix4x4 * p_r = (Matrix4x4*)memalign(VFPU_ALIGNMENT, sizeof(Matrix4x4));
//
//	u32	NUM_LOOPS = 1000000;
//
//	u64 start_time, end_time, end_time2;
//
//	NTiming::GetPreciseTime( &start_time );
//
//	for( u32 i= 0; i < NUM_LOOPS;++i)
//	{
//		myMulMatrixCPU( &r, this, &rhs );
//	}
//
//	NTiming::GetPreciseTime( &end_time );
//
//	for( u32 i= 0; i < NUM_LOOPS;++i)
//	{
//		matrixMultiplyUnaligned( p_r, this, &rhs );
//	}
//
//	NTiming::GetPreciseTime( &end_time2 );
//
//
//	float	time_1( end_time - start_time );
//	float	time_2( end_time2 - end_time );
//
//	printf( "CPU  took %fus\n", time_1 );
//	printf( "CFPU took %fus\n", time_2 );
//
//
//	printf( "Multiplying: \n" );
//	print();
//	printf( "by:\n" );
//	rhs.print();
//	printf( "Gives: \n" );
//	r.print();
//
//
//	printf( "\nBy VFPU\n" );
//	printf( "Gives: \n" );
//	p_r->print();
//
//	return r;
//}

/*
void	Matrix4x4::print() const
{
	printf(
	" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
	" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
	" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
	" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
	m[0][0], m[0][1], m[0][2], m[0][3],
	m[1][0], m[1][1], m[1][2], m[1][3],
	m[2][0], m[2][1], m[2][2], m[2][3],
	m[3][0], m[3][1], m[3][2], m[3][3]);
}
*/


