#include <pspsdk.h>
#include <pspkernel.h>
#include <pspsysevent.h>
#include <string.h>


#define VERS 1
#define REVS 0


PSP_MODULE_INFO("MediaEngine", 0x1006, VERS, REVS);
PSP_MAIN_THREAD_ATTR(0);

//The one from the sdk is either wrong or legacy
typedef struct SceModule2
{ 
   struct SceModule2*   next; //0, 0x00 
   u16             attribute; //4, 0x04 
   u8                version[2]; //6, 0x06 
   char             modname[27]; //8, 0x08 
   char             terminal; //35, 0x23 
   u16             status; //36, 0x24 (Used in modulemgr for stage identification) 
   u16             padding26; //38, 0x26 
   u32             unk_28; //40, 0x28 
   SceUID             modid; //44, 0x2C 
   SceUID             usermod_thid; //48, 0x30 
   SceUID             memid; //52, 0x34 
   SceUID             mpidtext; //56, 0x38 
   SceUID             mpiddata; //60, 0x3C 
   void *            ent_top; //64, 0x40 
   u32             ent_size; //68, 0x44 
   void *            stub_top; //72, 0x48 
   u32             stub_size; //76, 0x4C 
   int             (* module_start)(SceSize, void *); //80, 0x50 
   int             (* module_stop)(SceSize, void *); //84, 0x54 
   int             (* module_bootstart)(SceSize, void *); //88, 0x58 
   int             (* module_reboot_before)(void *); //92, 0x5C 
   int             (* module_reboot_phase)(SceSize, void *); //96, 0x60 
   u32             entry_addr; //100, 0x64(seems to be used as a default address) 
   u32             gp_value; //104, 0x68 
   u32             text_addr; //108, 0x6C 
   u32             text_size; //112, 0x70 
   u32             data_size; //116, 0x74 
   u32             bss_size; //120, 0x78 
   u8                nsegment; //124, 0x7C 
   u8               padding7d[3]; //125, 0x7D 
   u32             segmentaddr[4]; //128, 0x80 
   u32             segmentsize[4]; //144, 0x90 
   int             module_start_thread_priority; //160, 0xA0 
   SceSize          module_start_thread_stacksize; //164, 0xA4 
   SceUInt          module_start_thread_attr; //168, 0xA8 
   int             module_stop_thread_priority; //172, 0xAC 
   SceSize          module_stop_thread_stacksize; //176, 0xB0 
   SceUInt          module_stop_thread_attr; //180, 0xB4 
   int             module_reboot_before_thread_priority; //184, 0xB8 
   SceSize          module_reboot_before_thread_stacksize; //188, 0xBC 
   SceUInt          module_reboot_before_thread_attr; //192, 0xC0 
} SceModule2;

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

extern int sceSysregVmeResetEnable371();	
extern int sceSysregAvcResetEnable371();	
extern int sceSysregMeResetEnable371();
extern int sceSysregMeResetDisable371();
extern int sceSysregMeBusClockEnable371();
extern int sceSysregMeBusClockDisable371();
extern int sceSysregAvcResetEnable();
extern int sceMeBootStart();
extern int sceMeBootStart371();
extern int sceMeBootStart380();
extern int sceMeBootStart395();
extern int sceMeBootStart500();
extern int sceMeBootStart620();
extern int sceMeBootStart635();
extern int sceMeBootStart660();
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
   for(i = 0; i < 8192; i += 64) {
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
      __builtin_allegrex_cache(0x14, i);
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


PspSysEventHandler *handler;
int InitME(volatile struct me_struct *mei, int devkitVersion)
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
	if (devkitVersion < 0x03070110)
	{
		sceSysregMeResetEnable();
		sceSysregMeBusClockEnable();
		sceSysregMeResetDisable();
	}
	else
	{
		sceSysregMeResetEnable371();
		sceSysregMeBusClockEnable371();
		sceSysregMeResetDisable371();
	}
	//Find SceMeRpc and disable it(causes indefinate wait when handling events, like suspend)
	PspSysEventHandler *handlers = sceKernelReferSysEventHandler();
	while (handlers != NULL){
		if (strcmp(handlers->name, "SceMeRpc") == 0){
			handler = handlers;
			sceKernelUnregisterSysEventHandler(handler);
			break;
		}
		handlers = handlers->next;
	}

	pspSdkSetK1(k1);

	return 0;
}


void KillME(volatile struct me_struct *mei, int devkitVersion)
{
	unsigned int k1;

	k1 = pspSdkSetK1(0);

	if (mei == 0)
	{
		pspSdkSetK1(k1);
		return;
	}

	mei->init = 0;
	if (devkitVersion == 0x03070110){
		sceSysregVmeResetEnable371();	
		sceSysregAvcResetEnable371();	
		sceSysregMeResetEnable371();
		sceSysregMeBusClockDisable371();
	}
	else{
		sceSysregVmeResetEnable();	
		sceSysregAvcResetEnable();	
		sceSysregMeResetEnable();	
		sceSysregMeBusClockDisable();
	}

	pspSdkSetK1(k1);
}
SceModule2* mod;
void ResetME(int devkitVersion){
	unsigned int k1 = pspSdkSetK1(0);
	int *meStarted;
	int ret;
	if (devkitVersion < 0x03070000){
		meStarted = (int*)(mod->text_addr+0x00002C5C);//352
		*meStarted = 0;
		ret = sceMeBootStart(2);
	}
	else if (devkitVersion < 0x03080000){
		meStarted = (int*)(mod->text_addr+0x00002BCC);//371
		*meStarted = 0;
		ret = sceMeBootStart371(2);
	}
	else if (devkitVersion < 0x03090500){
		meStarted = (int*)(mod->text_addr+0x00002C2C);//380
		*meStarted = 0;
		ret = sceMeBootStart380(2);
	}
	else if (devkitVersion < 0x05000000){
//		*(int*)0xUNKNOWN3 = 0;//390
		ret = sceMeBootStart395(2);
	}
	else if (devkitVersion < 0x06000000){
		meStarted = (int*)(mod->text_addr+0x00002B9C);//500,550
		*meStarted = 0;
		ret = sceMeBootStart500(2);
	}
	else if (devkitVersion < 0x06020000){
		meStarted = (int*)(mod->text_addr+0x00002C5C);//600
		*meStarted = 0;
		ret = sceMeBootStart620(2);
	}
	else if (devkitVersion < 0x06030500){
		meStarted = (int*)(mod->text_addr+0x00002C4C);//620
		*meStarted = 0;
		ret = sceMeBootStart620(2);
	}
	else if (devkitVersion < 0x06060000){
		meStarted = (int*)(mod->text_addr+0x00002C4C);//635,638,639,660
		*meStarted = 0;
		ret = sceMeBootStart635(2);
	}//0x06030910
	else{
		meStarted = (int*)(mod->text_addr+0x00002C4C);//660
		*meStarted = 0;
		ret = sceMeBootStart660(2);
	}
	
	if (handler){
		sceKernelRegisterSysEventHandler(handler);
	}
	pspSdkSetK1(k1);

}
int module_start(SceSize args, void *argp)
{
	mod = sceKernelFindModuleByName("sceMeCodecWrapper");
	return 0;
}

int module_stop()
{
	return 0;
}
