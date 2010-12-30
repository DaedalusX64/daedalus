/*
Copyright (C) 2010 Salvy6735

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl_kernel.h>
 
PSP_MODULE_INFO("impose ctrl", 0x1000, 1, 1);
PSP_MAIN_THREAD_ATTR(0);
//PSP_HEAP_SIZE_KB(32);
//PSP_NO_CREATE_MAIN_THREAD();
//*************************************************************************************
//
//*************************************************************************************
void SetImposeHomeButton()
{
	sceCtrlSetButtonMasks(0x10000, 0);
}

//*************************************************************************************
//
//*************************************************************************************
u32 module_start(SceSize args, void *argp)
{
	return 0;
}

//*************************************************************************************
//
//*************************************************************************************
u32 module_stop()
{
	return 0;
}
