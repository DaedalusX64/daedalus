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

#ifndef __DAEDALUSOS_H__
#define __DAEDALUSOS_H__

#include "patch.h"
#include "patch_symbols.h"

#include <vector>

#define DAED_OS_MESSAGE_QUEUES

#ifdef DUMPOSFUNCTIONS
#ifdef DAED_OS_MESSAGE_QUEUES
typedef std::vector < u32 > QueueVector;
extern QueueVector g_MessageQueues;
#endif
#endif

void OS_Reset();

u32 OS_HLE___osProbeTLB(u32 vaddr);

#ifdef DAED_OS_MESSAGE_QUEUES
void OS_HLE_osCreateMesgQueue(u32 queue, u32 msgBuffer, u32 msgCount);
#endif

#endif //__DAEDALUSOS_H__
