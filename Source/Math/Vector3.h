#ifndef MATH_VECTOR3_H_
#define MATH_VECTOR3_H_

#include "Math/Math.h"	// VFPU Math
#include <algorithm>
class v3
{
public:
	constexpr v3() {}
	constexpr v3( float _x, float _y, float _z ) : x( _x ), y( _y ), z( _z ) {}


	constexpr v3 operator+( const v3 & v ) const
	{
		return v3( x + v.x, y + v.y, z + v.z );
	}

	constexpr v3 operator-( const v3 & v ) const
	{
		return v3( x - v.x, y - v.y, z - v.z );
	}

	constexpr v3 operator+() const
	{
		return *this;
	}

	constexpr v3 operator-() const
	{
		return v3( -x, -y, -z );
	}

	constexpr v3 operator*( float s ) const
	{
		return v3( x * s, y * s, z * s );
	}

	constexpr friend v3 operator*( float s, const v3 & v )
	{
		return v3( v.x * s, v.y * s, v.z * s );
	}

	constexpr v3 operator/( float s ) const
	{
		float r( 1.0f / s );
		return v3( x * r, y * r, z * r );
	}

	constexpr const v3 & operator+=( const v3 & rhs )
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	constexpr v3 & operator*=( float s )
	{
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}

#ifdef DAEDALUS_PSP//Corn
	void Normalise()
	{
		vfpu_norm_3Dvec(&x, &y, &z);
	}
#else
	void Normalise()
	{
		float	len_sq( LengthSq() );
		if(len_sq > 0.0f)
		{
			float r( InvSqrt( len_sq ) );
			x *= r;
			y *= r;
			z *= r;
		}
	}
#endif

	constexpr float Length() const
	{
		return sqrtf( (x*x)+(y*y)+(z*z) );
	}

	constexpr float LengthSq() const
	{
		return ( x * x) + ( y * y ) + ( z * z );
	}

	constexpr float MinComponent() const
	{
		return std::min(std::min(x, y), z);
	}

	constexpr float Dot( const v3 & rhs ) const
	{
		return (x*rhs.x) + (y*rhs.y) + (z*rhs.z);
	}

	float x, y, z;
};

#endif // MATH_VECTOR3_H_
