#include "stdafx.h"

#include <pspgu.h>
#include <pspkernel.h>
#include <pspsdk.h>
#include <pspctrl.h>
#include <kubridge.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "Debug/Dump.h"
#include "ConfigOptions.h"
#include "Core/ROM.h"
#include "Core/RomSettings.h"
#include "Core/CPU.h"

#include "Utility/PrintOpCode.h"

#include "svnversion.h"

PspDebugRegBlock *exception_regs;

static const char *codeTxt[32] =
{
    "Interrupt", "TLB modification", "TLB load/inst fetch", "TLB store",
    "Address load/inst fetch", "Address store", "Bus error (instr)",
    "Bus error (data)", "Syscall", "Breakpoint", "Reserved instruction",
    "Coprocessor unusable", "Arithmetic overflow", "Unknown 14",
    "Unknown 15", "Unknown 16", "Unknown 17", "Unknown 18", "Unknown 19",
    "Unknown 20", "Unknown 21", "Unknown 22", "Unknown 23", "Unknown 24",
    "Unknown 25", "Unknown 26", "Unknown 27", "Unknown 28", "Unknown 29",
    "Unknown 31"
};

static const unsigned char regName[32][5] =
{
    "zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

const char * const		pspModel[6] =
{
	"PSP PHAT", "PSP SLIM", "PSP BRITE", "PSP BRITE", "PSP GO", "UNKNOWN PSP"
};
extern bool PSP_IS_SLIM;

static void DumpInformation(PspDebugRegBlock * regs)
{
	FILE *fp = fopen("exception.txt", "wt");
	if (fp == NULL)
		return;

	const u32 RDRAM_base = (u32)g_pu8RamBase;
	const u32 RDRAM_end = (u32)g_pu8RamBase + 8 * 1024 * 1024;

	fprintf(fp, "Exception details:\n");
	{
		fprintf(fp, "\tException - %s\n", codeTxt[(regs->cause >> 2) & 31]);
		fprintf(fp, "\tEPC       - %08X\n", (int)regs->epc);
		fprintf(fp, "\tCause     - %08X\n", (int)regs->cause);
		fprintf(fp, "\tStatus    - %08X\n", (int)regs->status);
		fprintf(fp, "\tBadVAddr  - %08X\n", (int)regs->badvaddr);
		fprintf(fp, "\tRDRAM %08X - %08X\n", (int)RDRAM_base, (int)RDRAM_end - 1);
	}
	// output CPU Regs
	fprintf(fp, "\nPSP CPU registers: ('*' -> pointing inside RDRAM else ':')\n");
	{
		for(int i=0; i<32; i+=4)
			fprintf(fp, "\t%s%s%08X %s%s%08X %s%s%08X %s%s%08X\n", regName[i],   ( ((u32)regs->r[i]   >= RDRAM_base) & ((u32)regs->r[i]   < RDRAM_end) ) ? "*" : ":", (int)regs->r[i],
																   regName[i+1], ( ((u32)regs->r[i+1] >= RDRAM_base) & ((u32)regs->r[i+1] < RDRAM_end) ) ? "*" : ":", (int)regs->r[i+1],
																   regName[i+2], ( ((u32)regs->r[i+2] >= RDRAM_base) & ((u32)regs->r[i+2] < RDRAM_end) ) ? "*" : ":", (int)regs->r[i+2],
																   regName[i+3], ( ((u32)regs->r[i+3] >= RDRAM_base) & ((u32)regs->r[i+3] < RDRAM_end) ) ? "*" : ":", (int)regs->r[i+3]);
	}

#ifndef DAEDALUS_SILENT
	fprintf(fp, "\nDisassembly:\n");
	u32 inst_before_ex = 24;
	u32 inst_after_ex = 24;
	const OpCode * p( (OpCode *)(regs->epc - (inst_before_ex * 4)) );

	while( p < (OpCode *)(regs->epc + (inst_after_ex * 4)) )
	{
		char opinfo[128];

		OpCode op( *p );

		SprintOpCodeInfo( opinfo, (u32)p, op );
		fprintf(fp, "\t%s%p: <0x%08x> %s\n",(u32)regs->epc == (u32)p ? "*":" ", p, op._u32, opinfo);

		++p;
	}	
#endif

	// output FPU Regs
	fprintf(fp, "\nPSP FPU registers:\n");
	{
		for(int i=0; i<32; i+=4)
			fprintf(fp, "\t%#+12.7f %#+12.7f %#+12.7f %#+12.7f\n", (f32)regs->fpr[i+0], (f32)regs->fpr[i+1], (f32)regs->fpr[i+2], (f32)regs->fpr[i+3]);
	}

	fprintf(fp, "\nRom Infomation:\n");
	{
		fprintf(fp, "\tClockrate:       0x%08x\n", g_ROM.rh.ClockRate);
		fprintf(fp, "\tBootAddr:        0x%08x\n", SwapEndian(g_ROM.rh.BootAddress));
		fprintf(fp, "\tRelease:         0x%08x\n", g_ROM.rh.Release);
		fprintf(fp, "\tCRC1:            0x%08x\n", g_ROM.rh.CRC1);
		fprintf(fp, "\tCRC2:            0x%08x\n", g_ROM.rh.CRC2);
		fprintf(fp, "\tImageName:       '%s'\n",   g_ROM.rh.Name);
		fprintf(fp, "\tManufacturer:    0x%02x\n", g_ROM.rh.Manufacturer);
		fprintf(fp, "\tCartID:          0x%04x\n", g_ROM.rh.CartID);
		fprintf(fp, "\tCountryID:       0x%02x - '%c'\n", g_ROM.rh.CountryID, (char)g_ROM.rh.CountryID);
	}

	fprintf(fp, "\nPSP Infomation:\n");
	{
		fprintf(fp, "\tFirmware:         0x%08x\n", sceKernelDevkitVersion());
		fprintf(fp, "\tModel:            %s\n", pspModel[ kuKernelGetModel() ]);
		fprintf(fp, "\t64MB Available:   %s\n", PSP_IS_SLIM ? "Yes" : "No");
		fprintf(fp, "\tEmulator Version: "SVNVERSION"\n");
	}

	fprintf(fp, "\nSettings:\n");
	{
		//fprintf(fp, "\tDynarecStackOptimisation:      %01d\n", gDynarecStackOptimisation);
		fprintf(fp, "\tDynarecAccessOptimisation:     %01d\n", gMemoryAccessOptimisation);
		fprintf(fp, "\tDynarecLoopOptimisation:       %01d\n", gDynarecLoopOptimisation);	
		fprintf(fp, "\tDynarecDoublesOptimisation:    %01d\n", gDynarecDoublesOptimisation);	
		fprintf(fp, "\tDoubleDisplayEnabled:          %01d\n", gDoubleDisplayEnabled);
		fprintf(fp, "\tDynarecEnabled:                %01d\n", gDynarecEnabled);
		fprintf(fp, "\tOSHooksEnabled:                %01d\n", gOSHooksEnabled);
	}

	fprintf(fp, "\nEmulation CPU State:\n");
	{
		for(int i=0; i<32; i+=4)
			fprintf(fp, "\t%s:%08X-%08X %s:%08X-%08X %s:%08X-%08X %s:%08X-%08X\n", 
			regName[i+0], gCPUState.CPU[i+0]._u32_1, gCPUState.CPU[i+0]._u32_0,
			regName[i+1], gCPUState.CPU[i+1]._u32_1, gCPUState.CPU[i+1]._u32_0, 
			regName[i+2], gCPUState.CPU[i+2]._u32_1, gCPUState.CPU[i+2]._u32_0,
			regName[i+3], gCPUState.CPU[i+3]._u32_1, gCPUState.CPU[i+3]._u32_0);

		fprintf(fp, "PC: %08x\n", gCPUState.CurrentPC);
	}

#ifndef DAEDALUS_SILENT
	fprintf(fp, "\nDisassembly:\n");
	inst_before_ex = 24;
	inst_after_ex = 24;

	u8 * p_base;
	Memory_GetInternalReadAddress(gCPUState.CurrentPC-32, (void**)&p_base);
	const OpCode * op_start( reinterpret_cast< const OpCode * >( p_base - inst_before_ex * 4) );
	const OpCode * op_end(   reinterpret_cast< const OpCode * >( p_base + inst_after_ex * 4 ) );

	while( op_start < op_end )
	{
		char opinfo[128];

		OpCode op( *op_start );

		SprintOpCodeInfo( opinfo, (u32)op_start, op );
		fprintf(fp, "\t%s%p: <0x%08x> %s\n",(u32)p_base == (u32)op_start ? "*":" ", op_start, op._u32, opinfo);

		++op_start;
	}	
#endif

	fclose(fp);
}	

void ExceptionHandler(PspDebugRegBlock * regs)
{
	const u32 RDRAM_base = (u32)g_pu8RamBase;
	const u32 RDRAM_end = (u32)g_pu8RamBase + 8 * 1024 * 1024;
    SceCtrlData pad;

	pspDebugScreenInit();
    pspDebugScreenSetBackColor(0x00FF0000);
    pspDebugScreenSetTextColor(0xFFFFFFFF);
    pspDebugScreenClear();
    //pspDebugScreenPrintf("\nYour PSP has just crashed!\n\n");
	pspDebugScreenPrintf("Exception->%s Rev"SVNVERSION" RAM %08X-%08X\n\n", codeTxt[(regs->cause >> 2) & 31], (int)RDRAM_base, (int)RDRAM_end - 1 );
	//pspDebugScreenPrintf("Game Name - %s\n",   g_ROM.settings.GameName.c_str());
	//pspDebugScreenPrintf("Firmware  - %08X\n", sceKernelDevkitVersion());
	//pspDebugScreenPrintf("Model	- %s\n", pspModel[ kuKernelGetModel() ]);
	//pspDebugScreenPrintf("64MB	 - %s\n", PSP_IS_SLIM ? "Yes" : "No");
	//pspDebugScreenPrintf("Revision  - "SVNVERSION"\n\n");
    //pspDebugScreenPrintf("Exception - %s\n",codeTxt[(regs->cause >> 2) & 31]);
    pspDebugScreenPrintf(" EPC[%08X] ", (int)regs->epc);
    pspDebugScreenPrintf("Cause[%08X] ", (int)regs->cause);
    pspDebugScreenPrintf("Status[%08X] ", (int)regs->status);
    pspDebugScreenPrintf("BadVAddr[%08X]\n\n", (int)regs->badvaddr);
    for(u32 i = 0; i<32; i+=4) 
	{
		pspDebugScreenPrintf(" %s%s%08X %s%s%08X %s%s%08X %s%s%08X\n", regName[i],   ( ((u32)regs->r[i]   >= RDRAM_base) & ((u32)regs->r[i]   < RDRAM_end) ) ? "*" : ":", (int)regs->r[i],
																   regName[i+1], ( ((u32)regs->r[i+1] >= RDRAM_base) & ((u32)regs->r[i+1] < RDRAM_end) ) ? "*" : ":", (int)regs->r[i+1],
																   regName[i+2], ( ((u32)regs->r[i+2] >= RDRAM_base) & ((u32)regs->r[i+2] < RDRAM_end) ) ? "*" : ":", (int)regs->r[i+2],
																   regName[i+3], ( ((u32)regs->r[i+3] >= RDRAM_base) & ((u32)regs->r[i+3] < RDRAM_end) ) ? "*" : ":", (int)regs->r[i+3]);
	}

#ifndef DAEDALUS_SILENT
	u32 inst_before_ex = 15;
	u32 inst_after_ex = 4;
	const OpCode * p( (OpCode *)(regs->epc - (inst_before_ex * 4)) );

	pspDebugScreenPrintf("\n");
	while( p < (OpCode *)(regs->epc + (inst_after_ex * 4)) )
	{
		char opinfo[128];

		OpCode op( *p );

		SprintOpCodeInfo( opinfo, (u32)p, op );
		pspDebugScreenPrintf("%s%p: <0x%08x> %s\n",(u32)regs->epc == (u32)p ? "*":" ", p, op._u32, opinfo);

		++p;
	}	
#endif
	
	sceKernelDelayThread(1000000);
    pspDebugScreenPrintf("\nPress (X) to dump info to exception.txt or (O) to quit");

    for (;;){
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_CROSS){
			DumpInformation(regs);
            break;
        }else if (pad.Buttons & PSP_CTRL_CIRCLE){
            break;
        }
    }    
    sceKernelExitGame();
}

void initExceptionHandler()
{
   SceKernelLMOption option;
   int args[2], fd, modid;

   memset(&option, 0, sizeof(option));
   option.size = sizeof(option);
   option.mpidtext = PSP_MEMORY_PARTITION_KERNEL;
   option.mpiddata = PSP_MEMORY_PARTITION_KERNEL;
   option.position = 0;
   option.access = 1;

   if((modid = sceKernelLoadModule("exception.prx", 0, &option)) >= 0)
   {
      args[0] = (int)ExceptionHandler;
      args[1] = (int)&exception_regs;
      sceKernelStartModule(modid, 8, args, &fd, NULL);
   }
}
