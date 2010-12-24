#ifndef VECTOR4_H__
#define VECTOR4_H__

#include "Math/Math.h"	// VFPU Math

ALIGNED_TYPE(class, v4, 16)
{
public:
	v4() {}
	v4( float _x, float _y, float _z, float _w ) : x( _x ), y( _y ), z( _z ), w( _w ) {}

	float Normalise()
	{
		float	len_sq( LengthSq() );
		if(len_sq > 0.0001f)
		{
			float r( vfpu_invSqrt( len_sq ) );
			x *= r;
			y *= r;
			z *= r;
			w *= r;
		}
		return len_sq;
	}

	v4 operator+( const v4 & v ) const
	{
		return v4( x + v.x, y + v.y, z + v.z, w + v.w );
	}
	v4 operator-( const v4 & v ) const
	{
		return v4( x - v.x, y - v.y, z - v.z, w - v.w );
	}

	v4 operator*( float s ) const
	{
		return v4( x * s, y * s, z * s, w * s );
	}

	float Length() const
	{
		return pspFpuSqrt( (x*x)+(y*y)+(z*z)+(w*w) );
	}

	float LengthSq() const
	{
		return (x*x)+(y*y)+(z*z)+(w*w);
	}

#if 0	//0=fast, 1=original //Corn
	float Dot( const v4 & rhs ) const
	{
		return (x*rhs.x) + (y*rhs.y) + (z*rhs.z) + (w*rhs.w);
	}
#else
	float Dot( const v4 & rhs ) const
	{
		return vfpu_dot_4Dvec(x,y,z,w,rhs.x,rhs.y,rhs.z,rhs.w);
	}
#endif


	float x, y, z, w;
};


#endif // VECTOR4_H__
