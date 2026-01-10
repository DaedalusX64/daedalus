#ifndef VECTOR3_H
#define VECTOR3_H

#include "matrix.h"

// Vorwärtsdefinition für Zirkelbezug
typedef struct tMatrix;

typedef struct tVector3
{
	public: 
		// Konstruktor/Destruktor
		tVector3( float x = 0.0f, float y = 0.0f, float z = 0.0f );
		~tVector3();
		
		// Operatoren
		bool operator == ( tVector3 );
		tVector3 operator + ( const tVector3 &rhs ) const;
		void     operator += ( tVector3 );
		tVector3 operator - () const;
		tVector3 operator - ( const tVector3 &rhs ) const;
		tVector3 operator * ( const float rhs ) const;
		tVector3 operator / ( float );
		float&   operator []( int );
		
		// Methoden
		float		dot( tVector3 *other );
		tVector3	cross( tVector3 *other );
		float 		length();
		float 		lengthSQRT();
		tVector3	normalize();
		tVector3	closestPointOnLine( tVector3 *a, tVector3 *b );
		tVector3	closestPointOnLine( tVector3 *a, tVector3 *b, float d );
		tVector3	translateCoord( tMatrix m );
		tVector3	inverseTranslateCoord( tMatrix m );
		tVector3	rotateCoord( tMatrix m );
		tVector3	inverseRotateCoord( tMatrix m );
		tVector3	transformCoord( tMatrix m );
		tVector3	transformNormal( tMatrix m );
		tVector3	inverseTransformCoord( tMatrix m );
		
		// Attribute
		float x, y, z;
}tVector3;

#endif
