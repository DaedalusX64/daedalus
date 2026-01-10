#include "matrix.h"

#include <kos.h>
#include <math.h>

/*
 * Standardkonstruktor
 */
tMatrix::tMatrix()
{
	// Identitätsmatrix festlegen
	m[0][0] = 1.0f;
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = 0.0f;

	m[1][0] = 0.0f;
	m[1][1] = 1.0f;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;
	
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = 1.0f;
	m[2][3] = 0.0f;
	
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;	
}

/*
 * Destruktor
 */
tMatrix::~tMatrix()
{
	
}

/*
 * Überladener * Operator, welcher zwei Matrizzen multipliziert
 * m: Matrix, mit welcher die linksseitige Matrix multipliziert wird
 * returns: Multiplizierte Matrix
 */
tMatrix tMatrix::operator * ( tMatrix rhs )
{
	// Ergebnismatrix erstellen
	tMatrix erg;

	erg.m[0][0] = ( rhs.m[0][0] * m[0][0] ) + ( rhs.m[1][0] * m[0][1] ) + ( rhs.m[2][0] * m[0][2] ) + ( rhs.m[3][0] * m[0][3] );
	erg.m[1][0] = ( rhs.m[0][0] * m[1][0] ) + ( rhs.m[1][0] * m[1][1] ) + ( rhs.m[2][0] * m[1][2] ) + ( rhs.m[3][0] * m[1][3] );
	erg.m[2][0] = ( rhs.m[0][0] * m[2][0] ) + ( rhs.m[1][0] * m[2][1] ) + ( rhs.m[2][0] * m[2][2] ) + ( rhs.m[3][0] * m[2][3] );
	erg.m[3][0] = ( rhs.m[0][0] * m[3][0] ) + ( rhs.m[1][0] * m[3][1] ) + ( rhs.m[2][0] * m[3][2] ) + ( rhs.m[3][0] * m[3][3] );
	
	erg.m[0][1] = ( rhs.m[0][1] * m[0][0] ) + ( rhs.m[1][1] * m[0][1] ) + ( rhs.m[2][1] * m[0][2] ) + ( rhs.m[3][1] * m[0][3] );
	erg.m[1][1] = ( rhs.m[0][1] * m[1][0] ) + ( rhs.m[1][1] * m[1][1] ) + ( rhs.m[2][1] * m[1][2] ) + ( rhs.m[3][1] * m[1][3] );
	erg.m[2][1] = ( rhs.m[0][1] * m[2][0] ) + ( rhs.m[1][1] * m[2][1] ) + ( rhs.m[2][1] * m[2][2] ) + ( rhs.m[3][1] * m[2][3] );
	erg.m[3][1] = ( rhs.m[0][1] * m[3][0] ) + ( rhs.m[1][1] * m[3][1] ) + ( rhs.m[2][1] * m[3][2] ) + ( rhs.m[3][1] * m[3][3] );
	
	erg.m[0][2] = ( rhs.m[0][2] * m[0][0] ) + ( rhs.m[1][2] * m[0][1] ) + ( rhs.m[2][2] * m[0][2] ) + ( rhs.m[3][2] * m[0][3] );
	erg.m[1][2] = ( rhs.m[0][2] * m[1][0] ) + ( rhs.m[1][2] * m[1][1] ) + ( rhs.m[2][2] * m[1][2] ) + ( rhs.m[3][2] * m[1][3] );
	erg.m[2][2] = ( rhs.m[0][2] * m[2][0] ) + ( rhs.m[1][2] * m[2][1] ) + ( rhs.m[2][2] * m[2][2] ) + ( rhs.m[3][2] * m[2][3] );
	erg.m[3][2] = ( rhs.m[0][2] * m[3][0] ) + ( rhs.m[1][2] * m[3][1] ) + ( rhs.m[2][2] * m[3][2] ) + ( rhs.m[3][2] * m[3][3] );
	
	erg.m[0][3] = ( rhs.m[0][3] * m[0][0] ) + ( rhs.m[1][3] * m[0][1] ) + ( rhs.m[2][3] * m[0][2] ) + ( rhs.m[3][3] * m[0][3] );
	erg.m[1][3] = ( rhs.m[0][3] * m[1][0] ) + ( rhs.m[1][3] * m[1][1] ) + ( rhs.m[2][3] * m[1][2] ) + ( rhs.m[3][3] * m[1][3] );
	erg.m[2][3] = ( rhs.m[0][3] * m[2][0] ) + ( rhs.m[1][3] * m[2][1] ) + ( rhs.m[2][3] * m[2][2] ) + ( rhs.m[3][3] * m[2][3] );
	erg.m[3][3] = ( rhs.m[0][3] * m[3][0] ) + ( rhs.m[1][3] * m[3][1] ) + ( rhs.m[2][3] * m[3][2] ) + ( rhs.m[3][3] * m[3][3] );
	
	// Ergebnis zurückgeben
	return erg;
}

/*
 * Überladener / Operator, welcher die Matrix um den angegebenen Faktor skaliert
 * rhs: Faktor zum Skalieren der Matrix
 * returns: Skalierte Matrix
 */
tMatrix tMatrix::operator / ( float rhs )
{
	// Speichert die Ergebnismatrix
	tMatrix erg;
	
	// Teilung durch 0 abfangen
	if( rhs==0.0f )
	{
		// Alle Elemente auf 0 setzen
		for( int i = 0; i < 4; i++ )
			for( int j = 0; j < 4; j++ )
				erg.m[i][j] = 0.0f;
				
		return erg;
	}
	
	// Nur einmal teilen, dann multiplizieren (ist schneller)	
	float temp= 1.0f / rhs;

	for( int i = 0; i < 4; i++ )
		for( int j = 0; j < 4; j++ )
			erg.m[i][j] = m[i][j] * temp;
			
	// Ergebnis zurückgeben
	return erg;
}

/*
 * Gibt die Inverse-Matrix zurück
 * returns: Inverse-Matrix
 * remarks: Enthält Fehler
 */
tMatrix tMatrix::invert()
{
	tMatrix erg;

	float tmp[12];												//temporary pair storage
	float fDet;													//determinant

	// Paare für die ersten 8 Elemente berechnen (Cofaktoren)
	tmp[0]  = m[2][2] * m[3][3];
	tmp[1]  = m[2][3] * m[3][2];
	tmp[2]  = m[2][1] * m[3][3];
	tmp[3]  = m[2][3] * m[3][1];
	tmp[4]  = m[2][1] * m[3][2];
	tmp[5]  = m[2][2] * m[3][1];
	tmp[6]  = m[2][0] * m[3][3];
	tmp[7]  = m[2][3] * m[3][0];
	tmp[8]  = m[2][0] * m[3][2];
	tmp[9]  = m[2][2] * m[3][0];
	tmp[10] = m[2][0] * m[3][1];
	tmp[11] = m[2][1] * m[3][0];

	// Die ersten 8 Elemente berechnen
	erg.m[0][0] = tmp[0] * m[1][1] + tmp[3] * m[1][2] + tmp[4]  * m[1][3] - tmp[1] * m[1][1] - tmp[2] * m[1][2] - tmp[5]  * m[1][3];
	erg.m[0][1] = tmp[1] * m[1][0] + tmp[6] * m[1][2] + tmp[9]  * m[1][3] - tmp[0] * m[1][0] - tmp[7] * m[1][2] - tmp[8]  * m[1][3];
	erg.m[0][2] = tmp[2] * m[1][0] + tmp[7] * m[1][1] + tmp[10] * m[1][3] - tmp[3] * m[1][0] - tmp[6] * m[1][1] - tmp[11] * m[1][3];
	erg.m[0][3] = tmp[5] * m[1][0] + tmp[8] * m[1][1] + tmp[11] * m[1][2] - tmp[4] * m[1][0] - tmp[9] * m[1][1] - tmp[10] * m[1][2];
	erg.m[1][0] = tmp[1] * m[0][1] + tmp[2] * m[0][2] + tmp[5]  * m[0][3] - tmp[0] * m[0][1] - tmp[3] * m[0][2] - tmp[4]  * m[0][3];
	erg.m[1][1] = tmp[0] * m[0][0] + tmp[7] * m[0][2] + tmp[8]  * m[0][3] - tmp[1] * m[0][0] - tmp[6] * m[0][2] - tmp[9]  * m[0][3];
	erg.m[1][2] = tmp[3] * m[0][0] + tmp[6] * m[0][1] + tmp[11] * m[0][3] - tmp[2] * m[0][0] - tmp[7] * m[0][1] - tmp[10] * m[0][3];
	erg.m[1][3] = tmp[4] * m[0][0] + tmp[9] * m[0][1] + tmp[10] * m[0][2] - tmp[5] * m[0][0] - tmp[8] * m[0][1] - tmp[11] * m[0][2];

	// Paare für die zweiten 8 Elemente berechnen (Cofaktoren)
	tmp[0]  = m[0][2] * m[1][3];
	tmp[1]  = m[0][3] * m[1][2];
	tmp[2]  = m[0][1] * m[1][3];
	tmp[3]  = m[0][3] * m[1][1];
	tmp[4]  = m[0][1] * m[1][2];
	tmp[5]  = m[0][2] * m[1][1];
	tmp[6]  = m[0][0] * m[1][3];
	tmp[7]  = m[0][3] * m[1][0];
	tmp[8]  = m[0][0] * m[1][2];
	tmp[9]  = m[0][2] * m[1][0];
	tmp[10] = m[0][0] * m[1][1];
	tmp[11] = m[0][1] * m[1][0];

	// Die zweiten 8 Elemente berechnen
	erg.m[2][0] = tmp[0] * m[3][1]  + tmp[3]  * m[3][2] + tmp[4]  * m[3][3] - tmp[1]  * m[3][1] - tmp[2]  * m[3][2] - tmp[5]  * m[3][3];
	erg.m[2][1] = tmp[1] * m[3][0]  + tmp[6]  * m[3][2] + tmp[9]  * m[3][3] - tmp[0]  * m[3][0] - tmp[7]  * m[3][2] - tmp[8]  * m[3][3];
	erg.m[2][2] = tmp[2] * m[3][0]  + tmp[7]  * m[3][1] + tmp[10] * m[3][3] - tmp[3]  * m[3][0] - tmp[6]  * m[3][1] - tmp[11] * m[3][3];
	erg.m[2][3] = tmp[5] * m[3][0]  + tmp[8]  * m[3][1] + tmp[11] * m[3][2] - tmp[4]  * m[3][0] - tmp[9]  * m[3][1] - tmp[10] * m[3][2];
	erg.m[3][0] = tmp[2] * m[2][2]  + tmp[5]  * m[2][3] + tmp[1]  * m[2][1] - tmp[4]  * m[2][3] - tmp[0]  * m[2][1] - tmp[3]  * m[2][2];
	erg.m[3][1] = tmp[8] * m[2][3]  + tmp[0]  * m[2][0] + tmp[7]  * m[2][2] - tmp[6]  * m[2][2] - tmp[9]  * m[2][3] - tmp[1]  * m[2][0];
	erg.m[3][2] = tmp[6] * m[2][1]  + tmp[11] * m[2][3] + tmp[3]  * m[2][0] - tmp[10] * m[2][3] - tmp[2]  * m[2][0] - tmp[7]  * m[2][1];
	erg.m[3][3] = tmp[10] * m[2][2] + tmp[4]  * m[2][0] + tmp[9]  * m[2][1] - tmp[8]  * m[2][1] - tmp[11] * m[2][2] - tmp[5]  * m[2][0];

	// Determinante berechnen
	fDet = m[0][0] * erg.m[0][0] + m[0][1] * erg.m[0][1] + m[0][2] * erg.m[0][2] + m[0][3] * erg.m[0][3];

	// Auf nicht-invertierbare Matrix prüfen
	if( fDet == 0.0f )
		return tMatrix();
	
	// Matrix skalieren
	erg = erg / fDet;

	erg = erg.transpose();

	return erg;
}

/*
 * Berechnet die Transponierte der Matrix
 * returns: Transponierte der Matrix
 */
tMatrix tMatrix::transpose()
{
	// Speichert die Ergebnismatrix
	tMatrix erg;
	
	erg.m[0][0] = erg.m[0][0];
	erg.m[0][1] = erg.m[1][0];
	erg.m[0][2] = erg.m[2][0];
	erg.m[0][3] = erg.m[3][1];
	erg.m[1][0] = erg.m[0][1];
	erg.m[1][1] = erg.m[1][1];
	erg.m[1][2] = erg.m[2][1];
	erg.m[1][3] = erg.m[3][1];
	erg.m[2][0] = erg.m[0][2];
	erg.m[2][1] = erg.m[1][2];
	erg.m[2][2] = erg.m[2][2];
	erg.m[2][3] = erg.m[3][2];
	erg.m[3][0] = erg.m[0][3];
	erg.m[3][1] = erg.m[1][3];
	erg.m[3][2] = erg.m[2][3];
	erg.m[3][3] = erg.m[3][3];

	// Ergebnis zurückgeben
	return erg;	
}

/*
 * Legt die Rotation der Matrix fest
 * vAngles: Vektor mit den Drehwinkeln in Bogenmaß
 */
void tMatrix::SetRotationRadians( tVector3 vAngles )
{	
	// Matrix mit Daten füllen
	float cr, sr, cp, sp, cy, sy, srsp, crsp;

	cr = cos( vAngles.x );
  	sr = sin( vAngles.x );
  	cp = cos( vAngles.y );
  	sp = sin( vAngles.y );
  	cy = cos( vAngles.z );
  	sy = sin( vAngles.z );

  	m[0][0] =   cp * cy;
  	m[0][1] =   cp * sy;
  	m[0][2] = -1.0 * sp;

  	srsp = sr * sp;
  	crsp = cr * sp;

  	m[1][0] = (srsp * cy) - (cr * sy);
  	m[1][1] = (srsp * sy) + (cr * cy);
  	m[1][2] = (sr * cp);

  	m[2][0] = (crsp * cy) + (sr * sy);
  	m[2][1] = (crsp * sy) - (sr * cy);
  	m[2][2] = (cr * cp);
}

/*
 * Legt die Verschiebung der Matrix fest
 * v: Vektor, welcher die Verschiebung angibt
 */
void tMatrix::SetTranslation( tVector3 v )
{
	// Matrix mit Daten füllen
	m[3][0] = v.x;
	m[3][1] = v.y;
	m[3][2] = v.z;
}
