#ifndef VECTOR3_H__
#define VECTOR3_H__

#include "Math/Math.h"	// VFPU Math

class v3
{
public:
	v3() {}
	v3( float _x, float _y, float _z ) : x( _x ), y( _y ), z( _z ) {}


	v3 operator+( const v3 & v ) const
	{
		return v3( x + v.x, y + v.y, z + v.z );
	}

	v3 operator-( const v3 & v ) const
	{
		return v3( x - v.x, y - v.y, z - v.z );
	}

	v3 operator+() const
	{
		return *this;
	}

	v3 operator-() const
	{
		return v3( -x, -y, -z );
	}

	v3 operator*( float s ) const
	{
		return v3( x * s, y * s, z * s );
	}

	inline friend v3 operator*( float s, const v3 & v )
	{
		return v3( v.x * s, v.y * s, v.z * s );
	}

	v3 operator/( float s ) const
	{
		float r( 1.0f / s );
		return v3( x * r, y * r, z * r );
	}

	const v3 & operator+=( const v3 & rhs )
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	const v3 & operator*=( float s )
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

	float Length() const
	{
		return Sqrt( (x*x)+(y*y)+(z*z) );
	}

	float LengthSq() const
	{
		return (x*x)+(y*y)+(z*z);
	}

	float MinComponent() const
	{
		if(x < y && x < z)
		{
			return x;
		}
		else if(y < z)
		{
			return y;
		}
		else
		{
			return z;
		}
	}

#ifdef DAEDALUS_PSP	//PSP=fast, Other=original //Corn
	float Dot( const v3 & rhs ) const
	{
		return vfpu_dot_3Dvec(x,y,z,rhs.x,rhs.y,rhs.z);
	}
#else
	float Dot( const v3 & rhs ) const
	{
		return (x*rhs.x) + (y*rhs.y) + (z*rhs.z);
	}
#endif


	float x, y, z;
};

#endif // VECTOR3_H__
