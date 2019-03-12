#include <pspkernel.h>
#include <psppower.h>
#include <pspdebug.h>

PSP_MODULE_INFO("exception", 0x1007, 1, 1);  // better not unload

PspDebugErrorHandler curr_handler;
PspDebugRegBlock *exception_regs;

void _pspDebugExceptionHandler(void);
int sceKernelRegisterDefaultExceptionHandler(void *func);

int module_start(SceSize args, void *argp)
{
	if(args != 8) return -1;
	curr_handler = (PspDebugErrorHandler)((int *)argp)[0];
	exception_regs = (PspDebugRegBlock *)((int *)argp)[1];
	if(!curr_handler || !exception_regs) return -1;

	return sceKernelRegisterDefaultExceptionHandler((void *)_pspDebugExceptionHandler);

}
