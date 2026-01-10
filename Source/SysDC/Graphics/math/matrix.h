#ifndef MATRIX_H
#define MATRIX_H
#include <kos.h>
#include "vector3.h"

// Vorwärtsdefinition für Zirkelbezug
typedef struct tVector3;
typedef struct tQuaternion;

typedef struct tMatrix
{
	public: 
		// Konstruktor/Destruktor
		tMatrix();
		~tMatrix();
		
		// Operatoren
		//bool operator == ( tVector3 );
		tMatrix operator * ( tMatrix );
		tMatrix operator / ( float rhs );
		
		// Methoden
		void SetRotationRadians( tVector3 vAngles );
		void SetTranslation( tVector3 v );
		tMatrix transpose();
		tMatrix invert();

		// Attribute
		float m[4][4];
}tMatrix;

// Prototypen
tMatrix tMatrixRotationRadians( tVector3 vAngles );
tMatrix tMatrixTranslation( tVector3 v );

#endif
