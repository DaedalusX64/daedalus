#ifndef MATH_MATRIX4X4_H_
#define MATH_MATRIX4X4_H_

#include "Base/Alignment.h"

#include "Vector3.h"
class v4;

ALIGNED_TYPE(class, Matrix4x4, 16)
{
	public:

		Matrix4x4()
		{
		}

		Matrix4x4( float _11, float _12, float _13, float _14,
				   float _21, float _22, float _23, float _24,
				   float _31, float _32, float _33, float _34,
				   float _41, float _42, float _43, float _44 )
		:	m11( _11 ), m12( _12 ), m13( _13 ), m14( _14 ),
			m21( _21 ), m22( _22 ), m23( _23 ), m24( _24 ),
			m31( _31 ), m32( _32 ), m33( _33 ), m34( _34 ),
			m41( _41 ), m42( _42 ), m43( _43 ), m44( _44 )
		{
		}

		Matrix4x4 operator*( const Matrix4x4 & rhs ) const;

		Matrix4x4 & SetIdentity();
		Matrix4x4 & SetScaling( float scale );
		Matrix4x4 & SetRotateX( float angle );
		Matrix4x4 & SetRotateY( float angle );
		Matrix4x4 & SetRotateZ( float angle );
		Matrix4x4 & SetTranslate( const v3 & vec );

		//Matrix4x4 Transpose() const;
		//Matrix4x4 Inverse() const;

		v3 TransformCoord( const v3 & vec ) const;
		v3 TransformNormal( const v3 & vec ) const;

		v3 Transform( const v3 & vec ) const;
		v4 Transform( const v4 & vec ) const;

	//	void	print() const;

	public:
		union
		{
			struct
			{
				float	m11, m12, m13, m14;
				float	m21, m22, m23, m24;
				float	m31, m32, m33, m34;
				float	m41, m42, m43, m44;
			};

			float	m[ 4 ][ 4 ];
			float	mRaw[ 16 ];
		};
};

DAEDALUS_STATIC_ASSERT( sizeof( Matrix4x4 ) == 16*4 );

extern const Matrix4x4	gMatrixIdentity;

void MatrixMultiplyUnaligned(Matrix4x4 * m_out, const Matrix4x4 *mat_a, const Matrix4x4 *mat_b);
void MatrixMultiplyAligned(Matrix4x4 * m_out, const Matrix4x4 *mat_a, const Matrix4x4 *mat_b);

#endif // MATH_MATRIX4X4_H_
