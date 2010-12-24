/*
Copyright (C) 2006 StrmnNrmn

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

#ifndef __DAEDALUS_TRACERECORDER_H__
#define __DAEDALUS_TRACERECORDER_H__

#include "Trace.h"
#include "RegisterSpan.h"

#include <vector>

class CFragment;
class CCodeBufferManager;

class CTraceRecorder
{

public:
	CTraceRecorder();

	void				StartTrace( u32 address );

	enum EUpdateTraceStatus
	{
		UTS_CONTINUE_TRACE,
		UTS_CREATE_FRAGMENT,
	};

	EUpdateTraceStatus	UpdateTrace( u32 address, bool branch_delay_slot, bool branch_taken, OpCode op_code, CFragment * p_fragment );
	void				StopTrace( u32 exit_address );
	CFragment *			CreateFragment( CCodeBufferManager * p_manager );
	void				AbortTrace();

	bool				IsTraceActive() const						{ return mTracing; }

	u32					GetStartTraceAddress() const				{ DAEDALUS_ASSERT_Q( mTracing ); return mStartTraceAddress; }

private:
	bool							mTracing;
	u32								mStartTraceAddress;
	std::vector< STraceEntry >		mTraceBuffer;
	std::vector< SBranchDetails >	mBranchDetails;

	u32								mExpectedExitTraceAddress;

	u32								mActiveBranchIdx;				// Index into mBranchDetails
	bool							mStopTraceAfterDelaySlot;
	bool							mNeedIndirectExitMap;

	void	Analyse(SRegisterUsageInfo & register_usage );
};
extern CTraceRecorder				gTraceRecorder;

#endif // __DAEDALUS_TRACERECORDER_H__
