/*
Copyright (C) 2020 MasterFeizz

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
#include <stdio.h>

#include "DynaRec/AssemblyUtils.h"

namespace AssemblyUtils
{

//*****************************************************************************
//	Patch a long jump to target the specified location.
//	Return true if the patching succeeded (i.e. within range), false otherwise
//*****************************************************************************
    bool	PatchJumpLong( CJumpLocation jump [[maybe_unused]], CCodeLabel target [[maybe_unused]] )
    {
        return false;
    }
        
//*****************************************************************************
//	As above no (need to flush on intel)
//*****************************************************************************
    bool	PatchJumpLongAndFlush( CJumpLocation jump [[maybe_unused]], CCodeLabel target [[maybe_unused]] )
    {
        return false;
    }
}