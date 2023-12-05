/*
Copyright (C) 2012 StrmnNrmn

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


#include "Base/Types.h"
#include "DynaRec/CodeBufferManager.h"

#include <memory>
#include <stdlib.h>

class CCodeBufferManagerOSX : public CCodeBufferManager
{
public:
	CCodeBufferManagerOSX()
	{
	}

	virtual bool			Initialise();
	virtual void			Reset();
	virtual void			Finalise();

	virtual std::shared_ptr<CCodeGenerator> StartNewBlock();
	virtual u32				FinaliseCurrentBlock();
};

std::shared_ptr<CCodeBufferManager> CCodeBufferManager::Create()
{
	return std::make_unique<CCodeBufferManagerOSX>();
}

bool CCodeBufferManagerOSX::Initialise()
{
	DAEDALUS_ASSERT(false, "Unimplemented");
	return true;
}

void CCodeBufferManagerOSX::Reset()
{
	DAEDALUS_ASSERT(false, "Unimplemented");
}

void CCodeBufferManagerOSX::Finalise()
{
	DAEDALUS_ASSERT(false, "Unimplemented");
}

std::shared_ptr<CCodeGenerator> CCodeBufferManagerOSX::StartNewBlock()
{
	DAEDALUS_ASSERT(false, "Unimplemented");
	return nullptr;
}

u32 CCodeBufferManagerOSX::FinaliseCurrentBlock()
{
	DAEDALUS_ASSERT(false, "Unimplemented");
	return 0;
}


void Dynarec_ClearedCPUStuffToDo()
{}

void Dynarec_SetCPUStuffToDo()
{
}


extern "C" {
void _EnterDynaRec( const void * p_function, const void * p_base_pointer, const void * p_rebased_mem, u32 mem_limit )
{
	DAEDALUS_ASSERT(false, "Unimplemented");
}
}
