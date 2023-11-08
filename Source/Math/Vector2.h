#ifndef MATH_VECTOR2_H_
#define MATH_VECTOR2_H_

#include "Math/Math.h"	// VFPU Math


class v2
{
public:
	constexpr v2() {}
	constexpr v2( float _x, float _y ) : x( _x ), y( _y ) {}

	constexpr v2 operator+( const v2 & v ) const
	{
		return v2( x + v.x, y + v.y );
	}

	constexpr v2 operator-( const v2 & v ) const
	{
		return v2( x - v.x, y - v.y );
	}

	constexpr v2 operator+() const
	{
		return *this;
	}

	constexpr v2 operator-() const
	{
		return v2( -x, -y );
	}

	constexpr v2 operator*( float s ) const
	{
		return v2( x * s, y * s );
	}

	friend constexpr v2 operator*( float s, const v2 & v )
	{
		return v2( v.x * s, v.y * s );
	}

	v2 operator/( float s ) const
	{
		float r( 1.0f / s );
		return v2( x * r, y * r );
	}

	 v2 & operator+=( const v2 & rhs )
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	const v2 & operator*=( float s )
	{
		x *= s;
		y *= s;
		return *this;
	}

	float Length() const
	{
		return sqrtf( (x*x)+(y*y) );
	}

	float LengthSq() const
	{
		return (x*x)+(y*y);
	}

	float x, y;
};

#endif // MATH_VECTOR2_H_
