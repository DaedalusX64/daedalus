/*
Copyright (C) 2001 StrmnNrmn

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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef RSPHLE_H__
#define RSPHLE_H__

#include "Memory.h"

// Returns true if the rsp is running either LLE or HLE
inline bool RSP_IsRunning(){ return (Memory_SP_GetRegister( SP_STATUS_REG ) & SP_STATUS_HALT) == 0; }

#ifdef DAEDALUS_ENABLE_ASSERTS
extern volatile bool gRSPHLEActive;
inline bool RSP_IsRunningLLE(){ return RSP_IsRunning() && !gRSPHLEActive; }	// Returns true if the rsp is running with LLE
inline bool RSP_IsRunningHLE(){	return RSP_IsRunning() && gRSPHLEActive; }	// Returns true if the rsp is running with HLE
#endif

enum EProcessResult
{
	PR_NOT_STARTED,	// Couldn't start
	PR_STARTED,		// Was started asynchronously, active
	PR_COMPLETED,	// Was processed synchronously, completed
};

void RSP_HLE_ProcessTask();

#endif //RSPHLE_H__
