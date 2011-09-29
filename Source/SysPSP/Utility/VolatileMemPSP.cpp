
/*

Copyright (C) 2011 JJS

http://gitorious.org/~jjs/ags/ags-for-psp

*/

#include "stdafx.h"

#include "Utility/ModulePSP.h"

#include <pspctrl.h>
#include <pspsdk.h>
#include <pspkernel.h>
#include <psppower.h>
#include <pspsuspend.h>
#include <pspsysevent.h>

#include <malloc.h>

//
//	Refactor me
//

bool bVolatileMem = false;

struct PspSysEventHandler malloc_p5_sysevent_handler_struct;
//*************************************************************************************
//
//*************************************************************************************

extern "C" 
{ 
	int kernel_sceKernelRegisterSysEventHandler(PspSysEventHandler *handler);
	int kernel_sceKernelUnregisterSysEventHandler(PspSysEventHandler *handler);
}

//
// From: http://forums.ps2dev.org/viewtopic.php?p=70854#70854
//
//*************************************************************************************
//
//*************************************************************************************
int malloc_p5_sysevent_handler(int ev_id, char* ev_name, void* param, int* result)
{
  if (ev_id == 0x100) // PSP_SYSEVENT_SUSPEND_QUERY
    return -1;
  
  return 0;
} 

//*************************************************************************************
//
//*************************************************************************************
void VolatileMemInit()
{
	// Unlock memory partition 5
	void* pointer = NULL;
	int size = 0;
	int result = sceKernelVolatileMemLock(0, &pointer, &size);

	if (result == 0)
	{

		int sysevent = CModule::Load("sysevent.prx");
		if (sysevent >= 0)
		{
			// Register sysevent handler to prevent suspend mode because p5 memory cannot be resumed
			memset(&malloc_p5_sysevent_handler_struct, 0, sizeof(struct PspSysEventHandler));
			malloc_p5_sysevent_handler_struct.size = sizeof(struct PspSysEventHandler);
			malloc_p5_sysevent_handler_struct.name = (char*) "p5_suspend_handler";
			malloc_p5_sysevent_handler_struct.handler = &malloc_p5_sysevent_handler;
			malloc_p5_sysevent_handler_struct.type_mask = 0x0000FF00;
			kernel_sceKernelRegisterSysEventHandler(&malloc_p5_sysevent_handler_struct);

			CModule::Unload( sysevent );
		}

		printf("Successfully Unlocked Volatile Mem: %d KB\n",size / 1024);
		bVolatileMem = true;
	}
	else
	{
		printf( "Failed to unlock volatile mem: %08x\n", result );
		bVolatileMem = false;
	}

}
//u32 malloc_p5_memory_used = 0;
//*************************************************************************************
//
//*************************************************************************************
void* malloc_volatile(size_t size)
{
	//If volatile mem couldn't be unlocked, use normal memory
	//
	if (!bVolatileMem)	 return malloc(size);

	//void* result = (void*)malloc(size);

	//struct mallinfo info = _mallinfo_r(NULL);
//	printf("used memory %d of %d - %d\n", info.usmblks + info.uordblks, info.arena, malloc_p5_memory_used);

	// Only try to allocate to volatile mem if we run out of mem.
	//
	/*if (result)
		return result;*/

	SceUID uid = sceKernelAllocPartitionMemory(5, "", PSP_SMEM_Low, size + 8, NULL);
	if (uid >= 0)
	{

//		printf("getting memory from p5 %d KBS\n", size / 1024);  
//		malloc_p5_memory_used += size;

		u32* pointer = (u32*)sceKernelGetBlockHeadAddr(uid);
		*pointer = uid;
		*(pointer + 4) = size;
		return (void*)(pointer + 8);
	}
	else
	{

//		printf("*****failed to allocate %d byte from p5\n", size);
		return NULL;
	}
}