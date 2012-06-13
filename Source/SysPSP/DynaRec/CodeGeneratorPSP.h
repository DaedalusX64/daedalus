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

#ifndef CODEGENERATORPSP_H_
#define CODEGENERATORPSP_H_

#include "DynaRec/CodeGenerator.h"
#include "AssemblyWriterPSP.h"
#include "N64RegisterCachePSP.h"
#include <stack>

class CAssemblyBuffer;


class CCodeGeneratorPSP : public CCodeGenerator, public CAssemblyWriterPSP
{
	public:
		CCodeGeneratorPSP( CAssemblyBuffer * p_buffer_a, CAssemblyBuffer * p_buffer_b );

		virtual void				Initialise( u32 entry_address, u32 exit_address, u32 * hit_counter, const void * p_base, const SRegisterUsageInfo & register_usage );
		virtual void				Finalise();

		virtual void				UpdateRegisterCaching( u32 instruction_idx );

		virtual RegisterSnapshotHandle	GetRegisterSnapshot();

		virtual CCodeLabel			GetEntryPoint() const;
		virtual CCodeLabel			GetCurrentLocation() const;
		virtual u32					GetCompiledCodeSize() const;

		virtual	CJumpLocation		GenerateExitCode( u32 exit_address, u32 jump_address, u32 num_instructions, CCodeLabel next_fragment );
		virtual void				GenerateEretExitCode( u32 num_instructions, CIndirectExitMap * p_map );
		virtual void				GenerateIndirectExitCode( u32 num_instructions, CIndirectExitMap * p_map );

		virtual void				GenerateBranchHandler( CJumpLocation branch_handler_jump, RegisterSnapshotHandle snapshot );

		virtual CJumpLocation		GenerateOpCode( const STraceEntry& ti, bool branch_delay_slot, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump);

		virtual CJumpLocation		ExecuteNativeFunction( CCodeLabel speed_hack, bool check_return = false );

private:
		// Not virtual base
				void				SetVar( u32 * p_var, u32 value );
				void				SetVar( u32 * p_var, EPspReg reg_src );
				void				SetFloatVar( f32 * p_var, EPspFloatReg reg_src );
				void				GetVar( EPspReg dst_reg, const u32 * p_var );
				void				GetFloatVar( EPspFloatReg dst_reg, const f32 * p_var );
				void				GetBaseRegisterAndOffset( const void * p_address, EPspReg * p_reg, s16 * p_offset );

				//void				UpdateAddressAndDelay( u32 address, bool set_branch_delay );

				bool				NeedLoadHi( int value, int logic );
				bool				NeedLoadHi( bool known, int value, int logic )	{ if(!known) return true;	return NeedLoadHi( value, logic );	}
				bool				NeedLoadHi( EN64Reg n64_reg, int logic );	


				void				GenerateCACHE( EN64Reg base, s16 offset, u32 cache_op );

				void				GenerateJAL( u32 address );
				void				GenerateJR( EN64Reg rs, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump );
				void				GenerateJALR( EN64Reg rs, EN64Reg rd, u32 address, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump );

				void				GenerateMFLO( EN64Reg rd );
				void				GenerateMFHI( EN64Reg rd );
				void				GenerateMTLO( EN64Reg rs );
				void				GenerateMTHI( EN64Reg rs );
				void				GenerateMULT( EN64Reg rs, EN64Reg rt );
				void				GenerateMULTU( EN64Reg rs, EN64Reg rt );
				void				GenerateDIV( EN64Reg rs, EN64Reg rt );
				void				GenerateDIVU( EN64Reg rs, EN64Reg rt );

				void				GenerateADDU( EN64Reg rd, EN64Reg rs, EN64Reg rt );
				void				GenerateSUBU( EN64Reg rd, EN64Reg rs, EN64Reg rt );
				void				GenerateAND( EN64Reg rd, EN64Reg rs, EN64Reg rt );
				void				GenerateOR( EN64Reg rd, EN64Reg rs, EN64Reg rt );
				void				GenerateXOR( EN64Reg rd, EN64Reg rs, EN64Reg rt );
				void				GenerateNOR( EN64Reg rd, EN64Reg rs, EN64Reg rt );
				void				GenerateSLT( EN64Reg rd, EN64Reg rs, EN64Reg rt );
				void				GenerateSLTU( EN64Reg rd, EN64Reg rs, EN64Reg rt );

				void				GenerateDADDU( EN64Reg rd, EN64Reg rs, EN64Reg rt );
				void				GenerateDADDIU( EN64Reg rt, EN64Reg rs, s16 immediate );
				void				GenerateDSRA32( EN64Reg rd, EN64Reg rt, u32 sa );
				void				GenerateDSLL32( EN64Reg rd, EN64Reg rt, u32 sa );

				void				GenerateADDIU( EN64Reg rt, EN64Reg rs, s16 immediate );
				void				GenerateANDI( EN64Reg rt, EN64Reg rs, u16 immediate );
				void				GenerateORI( EN64Reg rt, EN64Reg rs, u16 immediate );
				void				GenerateXORI( EN64Reg rt, EN64Reg rs, u16 immediate );
				void				GenerateLUI( EN64Reg rt, s16 immediate );
				void				GenerateSLTI( EN64Reg rt, EN64Reg rs, s16 immediate );
				void				GenerateSLTIU( EN64Reg rt, EN64Reg rs, s16 immediate );
				void				GenerateSLL( EN64Reg rd, EN64Reg rt, u32 sa );
				void				GenerateSRL( EN64Reg rd, EN64Reg rt, u32 sa );
				void				GenerateSRA( EN64Reg rd, EN64Reg rt, u32 sa );

				void				GenerateLB ( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset );
				void				GenerateLBU( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset );
				void				GenerateLH ( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset );
				void				GenerateLHU( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset );
				void				GenerateLW ( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s16 offset );
				void				GenerateLD ( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s16 offset );
				void				GenerateLWC1( u32 current_pc, bool set_branch_delay, u32 ft, EN64Reg base, s32 offset );
				void				GenerateLDC1( u32 current_pc, bool set_branch_delay, u32 ft, EN64Reg base, s32 offset );

				void				GenerateSB( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset );
				void				GenerateSH( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset );
				void				GenerateSW( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset );
				void				GenerateSD( u32 current_pc, bool set_branch_delay, EN64Reg rt, EN64Reg base, s32 offset );
				void				GenerateSWC1( u32 current_pc, bool set_branch_delay, u32 ft, EN64Reg base, s32 offset );
				void				GenerateSDC1( u32 current_pc, bool set_branch_delay, u32 ft, EN64Reg base, s32 offset );

				void				GenerateMFC1( EN64Reg rt, u32 fs );
				void				GenerateMTC1( u32 fs, EN64Reg rt );
				void				GenerateCFC1( EN64Reg rt, u32 fs );
				void				GenerateCTC1( u32 fs, EN64Reg rt );

				void				GenerateADD_D_Sim( u32 fd, u32 fs, u32 ft );
				void				GenerateSUB_D_Sim( u32 fd, u32 fs, u32 ft );
				void				GenerateMUL_D_Sim( u32 fd, u32 fs, u32 ft );
				void				GenerateDIV_D_Sim( u32 fd, u32 fs, u32 ft );
				void				GenerateSQRT_D_Sim( u32 fd, u32 fs );
				void				GenerateABS_D_Sim( u32 fd, u32 fs );
				void				GenerateMOV_D_Sim( u32 fd, u32 fs );
				void				GenerateNEG_D_Sim( u32 fd, u32 fs );

				void				GenerateTRUNC_W_D_Sim( u32 fd, u32 fs );
				void				GenerateCVT_W_D_Sim( u32 fd, u32 fs );
				void				GenerateCVT_S_D_Sim( u32 fd, u32 fs );

				void				GenerateCMP_D_Sim( u32 fs, ECop1OpFunction cmp_op, u32 ft );

				void				GenerateADD_S( u32 fd, u32 fs, u32 ft );
				void				GenerateSUB_S( u32 fd, u32 fs, u32 ft );
				void				GenerateMUL_S( u32 fd, u32 fs, u32 ft );
				void				GenerateDIV_S( u32 fd, u32 fs, u32 ft );
				void				GenerateSQRT_S( u32 fd, u32 fs );
				void				GenerateABS_S( u32 fd, u32 fs );
				void				GenerateMOV_S( u32 fd, u32 fs );
				void				GenerateNEG_S( u32 fd, u32 fs );
				void				GenerateCMP_S( u32 fs, ECop1OpFunction cmp_op, u32 ft );

				void				GenerateCVT_W_S( u32 fd, u32 fs );
				void				GenerateCVT_D_S_Sim( u32 fd, u32 fs );
				void				GenerateCVT_D_S( u32 fd, u32 fs );
				void				GenerateCVT_D_W_Sim( u32 fd, u32 fs );

				void				GenerateADD_D( u32 fd, u32 fs, u32 ft );
				void				GenerateSUB_D( u32 fd, u32 fs, u32 ft );
				void				GenerateMUL_D( u32 fd, u32 fs, u32 ft );
				void				GenerateDIV_D( u32 fd, u32 fs, u32 ft );
				void				GenerateSQRT_D( u32 fd, u32 fs );
				void				GenerateMOV_D( u32 fd, u32 fs );
				void				GenerateNEG_D( u32 fd, u32 fs );

				void				GenerateCVT_S_W( u32 fd, u32 fs );
				void				GenerateTRUNC_W_S( u32 fd, u32 fs );

				void				GenerateMFC0( EN64Reg rt, u32 fs );

				CJumpLocation		GenerateBranchAlways( CCodeLabel target );
				CJumpLocation		GenerateBranchIfSet( const u32 * p_var, CCodeLabel target );
				CJumpLocation		GenerateBranchIfNotSet( const u32 * p_var, CCodeLabel target );
				CJumpLocation		GenerateBranchIfEqual( const u32 * p_var, u32 value, CCodeLabel target );
				CJumpLocation		GenerateBranchIfNotEqual( const u32 * p_var, u32 value, CCodeLabel target );
				CJumpLocation		GenerateBranchIfNotEqual( EPspReg reg_a, u32 value, CCodeLabel target );

				void				GenerateBEQ( EN64Reg rs, EN64Reg rt, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump );
				void				GenerateBNE( EN64Reg rs, EN64Reg rt, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump );
				void				GenerateBLEZ( EN64Reg rs, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump );
				void				GenerateBGTZ( EN64Reg rs, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump );
				void				GenerateBLTZ( EN64Reg rs, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump );
				void				GenerateBGEZ( EN64Reg rs, const SBranchDetails * p_branch, CJumpLocation * p_branch_jump );

				void				GenerateBC1F( const SBranchDetails * p_branch, CJumpLocation * p_branch_jump );
				void				GenerateBC1T( const SBranchDetails * p_branch, CJumpLocation * p_branch_jump );

				void				GenerateSLLV( EN64Reg rd, EN64Reg rs, EN64Reg rt );
				void				GenerateSRLV( EN64Reg rd, EN64Reg rs, EN64Reg rt );
				void				GenerateSRAV( EN64Reg rd, EN64Reg rs, EN64Reg rt );

private:
				void				SetRegisterSpanList( const SRegisterUsageInfo & register_usage, bool loops_to_self );

				void				ExpireOldIntervals( u32 instruction_idx );
				void				SpillAtInterval( const SRegisterSpan & live_span );

				EPspReg				GetRegisterNoLoad( EN64Reg n64_reg, u32 lo_hi_idx, EPspReg scratch_reg );
				EPspReg				GetRegisterNoLoadLo( EN64Reg n64_reg, EPspReg scratch_reg )		{ return GetRegisterNoLoad( n64_reg, 0, scratch_reg ); }
				EPspReg				GetRegisterNoLoadHi( EN64Reg n64_reg, EPspReg scratch_reg )		{ return GetRegisterNoLoad( n64_reg, 1, scratch_reg ); }

				EPspReg				GetRegisterAndLoad( EN64Reg n64_reg, u32 lo_hi_idx, EPspReg scratch_reg );
				EPspReg				GetRegisterAndLoadLo( EN64Reg n64_reg, EPspReg scratch_reg )	{ return GetRegisterAndLoad( n64_reg, 0, scratch_reg ); }
				EPspReg				GetRegisterAndLoadHi( EN64Reg n64_reg, EPspReg scratch_reg )	{ return GetRegisterAndLoad( n64_reg, 1, scratch_reg ); }

				void				GetRegisterValue( EPspReg psp_reg, EN64Reg n64_reg, u32 lo_hi_idx );

				void				LoadRegister( EPspReg psp_reg, EN64Reg n64_reg, u32 lo_hi_idx );
				void				LoadRegisterLo( EPspReg psp_reg, EN64Reg n64_reg )				{ LoadRegister( psp_reg, n64_reg, 0 ); }
				void				LoadRegisterHi( EPspReg psp_reg, EN64Reg n64_reg )				{ LoadRegister( psp_reg, n64_reg, 1 ); }

				void				PrepareCachedRegister( EN64Reg n64_reg, u32 lo_hi_idx );
				void				PrepareCachedRegisterLo( EN64Reg n64_reg )						{ PrepareCachedRegister( n64_reg, 0 ); }
				void				PrepareCachedRegisterHi( EN64Reg n64_reg )						{ PrepareCachedRegister( n64_reg, 1 ); }

				void				StoreRegister( EN64Reg n64_reg, u32 lo_hi_idx, EPspReg psp_reg );
				void				StoreRegisterLo( EN64Reg n64_reg, EPspReg psp_reg )				{ StoreRegister( n64_reg, 0, psp_reg ); }
				void				StoreRegisterHi( EN64Reg n64_reg, EPspReg psp_reg )				{ StoreRegister( n64_reg, 1, psp_reg ); }

				void				SetRegister64( EN64Reg n64_reg, s32 lo_value, s32 hi_value );

				void				SetRegister32s( EN64Reg n64_reg, s32 value );

				void				SetRegister( EN64Reg n64_reg, u32 lo_hi_idx, u32 value );
				/*
				enum EUpdateRegOptions
				{
					URO_HI_SIGN_EXTEND,		// Sign extend from src
					URO_HI_CLEAR,			// Clear hi bits
				};
				*/
				void				UpdateRegister( EN64Reg n64_reg, EPspReg psp_reg, bool options, EPspReg scratch_reg );
	

				EPspFloatReg		GetFloatRegisterAndLoad( EN64FloatReg n64_reg );
				void				UpdateFloatRegister( EN64FloatReg n64_reg );

				EPspFloatReg		GetSimFloatRegisterAndLoad( EN64FloatReg n64_reg );
				void				UpdateSimDoubleRegister( EN64FloatReg n64_reg );

				const CN64RegisterCachePSP & GetRegisterCacheFromHandle( RegisterSnapshotHandle snapshot ) const;

				void				FlushRegister( CN64RegisterCachePSP & cache, EN64Reg n64_reg, u32 lo_hi_idx, bool invalidate );
				void				FlushAllRegisters( CN64RegisterCachePSP & cache, bool invalidate );
				void				FlushAllFloatingPointRegisters( CN64RegisterCachePSP & cache, bool invalidate );
				void				FlushAllTemporaryRegisters( CN64RegisterCachePSP & cache, bool invalidate );

				void				RestoreAllRegisters( CN64RegisterCachePSP & current_cache, CN64RegisterCachePSP & new_cache );


				void				GenerateGenericR4300( OpCode op_code, CPU_Instruction p_instruction );

				typedef u32 (*ReadMemoryFunction)( u32 address, u32 current_pc );
				typedef void (*WriteMemoryFunction)( u32 address, u32 value, u32 current_pc );


				bool				GenerateDirectLoad( EPspReg psp_dst, EN64Reg n64_base, s16 offset, OpCodeValue load_op, u32 swizzle );
				void				GenerateSlowLoad( u32 current_pc, EPspReg psp_dst, EPspReg reg_address, ReadMemoryFunction p_read_memory );
				void				GenerateLoad( u32 current_pc, EPspReg psp_dst, EN64Reg n64_base, s16 offset, OpCodeValue load_op, u32 swizzle, ReadMemoryFunction p_read_memory );

				bool				GenerateDirectStore( EPspReg psp_src, EN64Reg n64_base, s16 offset, OpCodeValue store_op, u32 swizzle );
				void				GenerateStore( u32 current_pc,
												   EPspReg psp_src,
												   EN64Reg base,
												   s32 offset,
												   OpCodeValue store_op,
												   u32 swizzle,
												   WriteMemoryFunction p_write_memory );
				void				GenerateSlowStore( u32 current_pc, EPspReg psp_src, EPspReg reg_address, WriteMemoryFunction p_write_memory );

				struct SAddressCheckFixup
				{
					CJumpLocation		BranchToJump;			// Branches to our jump loc
					CCodeLabel			HandlerAddress;			// Address of our handler function in the second buffer

					SAddressCheckFixup( CJumpLocation branch, CCodeLabel label ) : BranchToJump( branch ), HandlerAddress( label ) {}
				};

				void				GenerateAddressCheckFixups();
				void				GenerateAddressCheckFixup( const SAddressCheckFixup & fixup );

private:
				const u8 *			mpBasePointer;
				EPspReg				mBaseRegister;
				RegisterSpanList	mRegisterSpanList;

				u32					mEntryAddress;
				CCodeLabel			mLoopTop;
				bool				mUseFixedRegisterAllocation;

				std::vector< CN64RegisterCachePSP >	mRegisterSnapshots;
				CN64RegisterCachePSP	mRegisterCache;

				// For register allocation
				RegisterSpanList	mActiveIntervals;
				std::stack<EPspReg>	mAvailableRegisters;

				bool							mQuickLoad;
				
				EN64Reg							mPrevious_base;
				EN64Reg							mPrevious_rt;
				bool							mKeepPreviousLoadBase;
				bool							mKeepPreviousStoreBase;
				bool							mFloatCMPIsValid;

				std::vector< SAddressCheckFixup >	mAddressCheckFixups;
};

#endif // CODEGENERATORPSP_H_
