#include "vector2.h"

/*
 * Initialisierungskonstruktor
 * x: X-Komponente des Vektors
 * y: Y-Komponente des Vektors
 */
tVector2::tVector2( float x, float y )
{
	this->x = x;
	this->y = y;
}

/*
 * Destruktor
 */
tVector2::~tVector2()
{
}

/*
 * Überladener Vergleichsoperator, welcher zwei Vektoren auf Gleichheit überprüft
 * other: Vektor, mit dem der linksseitige Operant verglichen wird
 * returns: true, bei Gleichheit der Objekte, ansonsten false
 */
bool tVector2::operator == ( tVector2 other )
{
	if( other.x == x && other.y == y )
		return true;
	else
		return false;
}

bool tVector2::operator += ( const tVector2& v)
{
    this->x += v.x;
    this->y += v.y;
}

/*
 * Überladener [] Operator für den Zugriff auf die einzelnen Komponenten des Vektors
 * iIndex: Index des Elements
 * returns: Wert des Elements mit dem angegebenen Index
 */
float& tVector2::operator [] ( int iIndex )
{
	return ((float *)this)[iIndex];
}

bool tVector2::isWithinTriangle( tVector2 vVert0, tVector2 vVert1, tVector2 vVert2 )
{
	// Anfang des Barycentric-Algorythmus
	float u0, v0, u1, v1, u2, v2, alpha, beta;
	u0 = this->x  - vVert0.x;
	v0 = this->y  - vVert0.y;
	u1 = vVert1.x - vVert0.x;
	u2 = vVert2.x - vVert0.x;
	v1 = vVert1.y - vVert0.y;
	v2 = vVert2.y - vVert0.y;

	// Die barycentric-Koordinaten berechnen und vergleichen
	if( u1 == 0 ) 
	{
		// Tritt selten auf
		beta = u0 / u2;
		if( beta < 0.0f || beta > 1.0f )
			return false;
		alpha = ( v0 - beta * v2 ) / v1;
	}
	else
	{
		// Tritt häufiger auf
		beta = ( v0 * u1  - u0 * v1 ) / ( v2 * u1 - u2 * v1 );
		if( beta < 0.0f || beta > 1.0f )
			return false;
		alpha = ( u0 - beta * u2 ) / u1;
	}
	
	if( alpha < 0.0f || alpha + beta > 1.0f )
		return false;
	
	// Kollision aufgetreten
	return true;
}
