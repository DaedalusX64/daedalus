/*

  Copyright (C) 2002 StrmnNrmn

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

#include "stdafx.h"
#include "RDP.h"

// This needs a huge clean up or be removed as is now, it only holds debug code..

//*****************************************************************************
// RDP state
//*****************************************************************************
RDP_OtherMode		gRDPOtherMode;

#ifdef DAEDALUS_FAST_TMEM
//Granularity down to 24bytes is good enuff also only need to address the upper half of TMEM for palettes//Corn
u32* gTlutLoadAddresses[ 4096 >> 6 ];
#else
u16 gTextureMemory[ 512 ];
#endif
