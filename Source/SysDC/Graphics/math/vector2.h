#ifndef VECTOR2_H
#define VECTOR2_H

typedef struct tVector2
{
	public:
		// Konstruktor/Destruktor
		tVector2( float x = 0.0f, float y = 0.0f );
		~tVector2();
		
		// Operatoren
		bool operator == ( tVector2 );
		bool operator += ( const tVector2& );
		float& operator []( int );
			
		// Methoden
		bool isWithinTriangle( tVector2 vVert0, tVector2 vVert1, tVector2 vVert2 );
		
		// Attribute
		float x, y;
}tVector2;

#endif
