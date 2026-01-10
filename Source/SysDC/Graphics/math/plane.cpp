#include "plane.h"

/*
 * Initialisierungskonstruktor
 * vOrigin: Punkt auf der Ebene
 * vNormal: Normale der Ebene
 */
tPlane::tPlane( tVector3 vOrigin, tVector3 vNormal )
{
	// Die Ebene erstellen
	a = vNormal.x;
	b = vNormal.y;
	c = vNormal.z;
	d = -vNormal.dot( &vOrigin );
}

/*
 * Destruktor
 */
tPlane::~tPlane()
{
}

/*
 * Führt eine Kollisionserkennung zwischen der Ebene und einer Linie durch
 * vStart: Vektor, welcher den Startpunkt der Linie darstellt
 * vDir: Vektor, welcher die Richtung der Linie darstellt
 * t: Speichert den Zeitpunkt der Kollision
 * returns: true, falls eine Kollision aufgetreten ist, ansonsten false
 */
bool tPlane::intersectLine( tVector3 vStart, tVector3 vDir, float *t )
{
	// Speichert die zu ignorierende Ungenauigkeit ab
	const float fEpsilon = 0.0001f;

	// Speichert die für die Rechnung benötigten Hilfsvariablen
	float dot, dot2;

	// Zeiger auf die Ebene speichern
	tVector3 *vPlane = (tVector3 *)this;
   
	// Überprüfen, ob die Richtung der Bewegung parallel zu Ebene verläuft
	dot = vDir.dot( vPlane );
	
	// Dabei die Rechenungenauigkeit beachten
	if( dot < fEpsilon && dot > -fEpsilon )
		return false;

	// Abstand des Punktes von der Ebene des Dreiecks ermitteln
	dot2 = vStart.dot( vPlane ) + this->d;
	*t = -dot2 / dot;	
	
	// Wenn wir hier ankommen, dann ist keine Kollision aufgetreten
	return true;
}
