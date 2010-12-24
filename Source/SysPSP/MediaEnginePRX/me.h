#ifndef me_h
#define me_h

#include <pspsdk.h>
#include <pspkernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct me_struct
{
	int start;
	int done;
	int (*func)(int);		// function ptr - func takes an int argument and returns int
	int param;				// function argument
	int result;				// function return value
	int precache_len;		// amount of space to invalidate before running func, -1 = all
	void *precache_addr;	// address of space to invalidate before running func
	int postcache_len;		// amount of space to flush after running func, -1 = all
	void *postcache_addr;	// address of space to flush after running func
	unsigned int signals;
	int init;
};

int InitME(volatile struct me_struct *mei );
void KillME(volatile struct me_struct *mei );
int CallME(volatile struct me_struct *mei, int func, int param, int prelen, void *preadr, int postlen, void *postadr);
int WaitME(volatile struct me_struct *mei);
int BeginME(volatile struct me_struct *mei, int func, int param, int prelen, void *preadr, int postlen, void *postadr);
int CheckME(volatile struct me_struct *mei);
unsigned int SignalME(volatile struct me_struct *mei, unsigned int sigmask, unsigned int sigset);

#ifdef __cplusplus
}
#endif

#endif
