#include <pspsdk.h>
#include <pspkernel.h>
#include <string.h>


#define VERS 1
#define REVS 0


PSP_MODULE_INFO("MediaEngine", 0x1006, VERS, REVS);
PSP_MAIN_THREAD_ATTR(0);


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


extern void me_stub(void);
extern void me_stub_end(void);


/*
 * cache functions
 *
 */

#define store_tag(index, hi, lo) __asm__ (".set push\n" \
                 ".set noreorder\n" \
                 "mtc0 %0, $28\n"   \
                 "mtc0 %1, $29\n"   \
                 ".set pop\n"       \
             ::"r"(lo),"r"(hi));    \
             __builtin_allegrex_cache(0x11, index);

void dcache_inv_all()
{
   int i;
   for(i = 0; i < 16384; i += 64) {
      store_tag(i, 0, 0);
      __builtin_allegrex_cache(0x13, i);
      __builtin_allegrex_cache(0x11, i);
   }
}

void dcache_inv_range(void *addr, int size)
{
   int i, j = (int)addr;
   for(i = j; i < size+j; i += 64)
      __builtin_allegrex_cache(0x19, i);
}

void dcache_wbinv_all()
{
   int i;
   for(i = 0; i < 8192; i += 64)
   {
      __builtin_allegrex_cache(0x14, i);
      __builtin_allegrex_cache(0x14, i);
   }
}

void dcache_wbinv_range(void *addr, int size)
{
   int i, j = (int)addr;
   for(i = j; i < size+j; i += 64)
      __builtin_allegrex_cache(0x1b, i);
}


static void me_loop(volatile struct me_struct *mei)
{
	unsigned int k1;

	k1 = pspSdkSetK1(0);

	while (mei->init) // ME runs this loop until killed
	{
		while (mei->start == 0); // wait for function
		mei->start = 0;
		if (mei->precache_len)
		{
			if (mei->precache_len < 0)
				dcache_inv_all();
			else
				dcache_inv_range(mei->precache_addr, mei->precache_len);
		}
		mei->result = mei->func(mei->param); // run function
		if (mei->postcache_len)
		{
			if (mei->postcache_len < 0)
				dcache_wbinv_all();
			else
				dcache_wbinv_range(mei->postcache_addr, mei->postcache_len);
		}
		mei->done = 1;
	}

	pspSdkSetK1(k1);

	while (1); // loop forever until ME reset
}


int InitME(volatile struct me_struct *mei)
{
	unsigned int k1;

	k1 = pspSdkSetK1(0);

	if (mei == 0)
	{
   		pspSdkSetK1(k1);
   		return -1;
	}

	// initialize the MediaEngine Instance
	mei->start = 0;
	mei->done = 1;
	mei->func = 0;
	mei->param = 0;
	mei->result = 0;
	mei->precache_len = 0;
	mei->precache_addr = 0;
	mei->postcache_len = 0;
	mei->postcache_addr = 0;
	mei->signals = 0;
	mei->init = 1;

	// start the MediaEngine
	memcpy((void *)0xbfc00040, me_stub, (int)(me_stub_end - me_stub));
	_sw((unsigned int)me_loop,  0xbfc00600);	// k0
	_sw((unsigned int)mei, 0xbfc00604);			// a0
	sceKernelDcacheWritebackAll();
	sceSysregMeResetEnable();
	sceSysregMeBusClockEnable();
	sceSysregMeResetDisable();
	pspSdkSetK1(k1);

	return 0;
}


void KillME(volatile struct me_struct *mei)
{
	unsigned int k1;

	k1 = pspSdkSetK1(0);

	if (mei == 0)
	{
		pspSdkSetK1(k1);
		return;
	}

	mei->init = 0;

	sceSysregMeResetEnable();
	pspSdkSetK1(k1);
}


int module_start(SceSize args, void *argp)
{
	return 0;
}

int module_stop()
{
	return 0;
}
