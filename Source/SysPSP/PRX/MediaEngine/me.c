#include "me.h"


int CallME(volatile struct me_struct *mei, int func, int param, int prelen, void *preadr, int postlen, void *postadr)
{
	if (!mei->done)
		return -1;

	mei->done = 0;
	// Warning : Quiet the compiler casting func. The behaviour is still compiler-defined.
	mei->func = (int (*)(int))func;
	mei->param = param;
	mei->result = 0;
	mei->precache_len = prelen;
	mei->precache_addr = preadr;
	mei->postcache_len = postlen;
	mei->postcache_addr = postadr;
	mei->signals = 0;
	mei->start = 1;

	while (!mei->done);
	return mei->result;
}


int WaitME(volatile struct me_struct *mei)
{
	while (!mei->done);
	return mei->result;
}


int BeginME(volatile struct me_struct *mei, int func, int param, int prelen, void *preadr, int postlen, void *postadr)
{
	if (!mei->done)
		return -1;

	mei->done = 0;
	// Warning : Quiet the compiler casting func. The behaviour is still compiler-defined.
	mei->func = (int (*)(int))func;
	mei->param = param;
	mei->result = 0;
	mei->precache_len = prelen;
	mei->precache_addr = preadr;
	mei->postcache_len = postlen;
	mei->postcache_addr = postadr;
	mei->signals = 0;
	mei->start = 1;

	return 0;
}


int CheckME(volatile struct me_struct *mei)
{
	return mei->done;
}


unsigned int SignalME(volatile struct me_struct *mei, unsigned int sigmask, unsigned int sigset)
{
unsigned int signals;

signals = mei->signals;
	mei->signals = (mei->signals & ~sigmask) | (sigset & sigmask);

	return signals;
}
