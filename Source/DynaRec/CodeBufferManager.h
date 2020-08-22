/*
Copyright (C) 2005 StrmnNrmn

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

#ifndef DYNAREC_CODEBUFFERMANAGER_H_
#define DYNAREC_CODEBUFFERMANAGER_H_

#include "Base/Types.h"

class CCodeGenerator;

class CCodeBufferManager
{
public:
	virtual							~CCodeBufferManager() {}
	virtual	bool					Initialise() = 0;
	virtual void					Reset() = 0;
	virtual	void					Finalise() = 0;

	virtual	CCodeGenerator *		StartNewBlock() = 0;
	virtual	u32						FinaliseCurrentBlock() = 0;

public:
	static	CCodeBufferManager *	Create();
};


#endif // DYNAREC_CODEBUFFERMANAGER_H_
