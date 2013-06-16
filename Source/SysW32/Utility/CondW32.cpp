#include "stdafx.h"
#include "Utility/Cond.h"
#include "Utility/Mutex.h"

#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

const double kTimeoutInfinity = 0.f;

Cond * CondCreate()
{
	DAEDALUS_ASSERT(false, "Unimplemented");
	return (Cond *)NULL;
}

void CondDestroy(Cond * cond)
{
	DAEDALUS_ASSERT(false, "Unimplemented");
}

void CondWait(Cond * cond, Mutex * mutex, double timeout)
{
	DAEDALUS_ASSERT(false, "Unimplemented");
}

void CondSignal(Cond * cond)
{
	DAEDALUS_ASSERT(false, "Unimplemented");
}
