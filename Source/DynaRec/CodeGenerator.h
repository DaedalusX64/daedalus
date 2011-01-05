/*
Copyright (C) 2001,2005 StrmnNrmn

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
#endif

#ifndef CODEGENERATOR_H_
#define CODEGENERATOR_H_

struct	OpCode;
struct	SBranchDetails;
class	CIndirectExitMap;

#include "Core/R4300Instruction.h"
#include "AssemblyUtils.h"
#include "RegisterSpan.h"
#include "DynaRec/TraceRecorder.h"

// 
//	Allow an architecure-independent way of passing around register sets
//
struct RegisterSnapshotHandle
{
	explicit RegisterSnapshotHandle( u32 handle ) : Handle( handle ) {}

	u32		Handle;
};

class CCodeGenerator
{
	public:
		typedef void (*ExceptionHandlerFn)();

									CCodeGenerator() {}
		virtual						~CCodeGenerator() {}

		virtual void				Initialise( u32 entry_address, u32 exit_address, u32 * hit_counter, const void * p_base, const SRegisterUsageInfo & register_usage ) = 0;
		virtual void				Finalise( ExceptionHandlerFn p_exception_handler_fn, const std::vector< CJumpLocation > & exception_handler_jumps ) = 0;

		virtual void				UpdateRegisterCaching( u32 instruction_idx ) = 0;

		virtual RegisterSnapshotHandle	GetRegisterSnapshot() = 0;

		virtual CCodeLabel			GetEntryPoint() const = 0;
		virtual CCodeLabel			GetCurrentLocation() const = 0;
		virtual u32					GetCompiledCodeSize() const = 0;

		virtual	CJumpLocation		GenerateExitCode( u32 exit_address, u32 jump_address, u32 num_instructions, CCodeLabel next_fragment ) = 0;
		virtual void				GenerateEretExitCode( u32 num_instructions, CIndirectExitMap * p_map ) = 0;
		virtual void				GenerateIndirectExitCode( u32 num_instructions, CIndirectExitMap * p_map ) = 0;

		virtual void				GenerateBranchHandler( CJumpLocation branch_handler_jump, RegisterSnapshotHandle snapshot ) = 0;

		virtual CJumpLocation		GenerateOpCode(const STraceEntry& ti, bool branch_delay_slot, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump, StaticAnalysis::MemAcess memory) = 0;
		virtual CJumpLocation		ExecuteNativeFunction( CCodeLabel speed_hack, bool check_return = false ) = 0;
};

extern "C" 
{ 
	void R4300_CALL_TYPE _EnterDynaRec( const void * p_function, const void * p_base_pointer, const void * p_rebased_mem, u32 mem_limit ); 
}

#endif // CODEGENERATOR_H_
