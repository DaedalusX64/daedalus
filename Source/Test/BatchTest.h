/*
Copyright (C) 2009 StrmnNrmn

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

#ifdef DAEDALUS_BATCH_TEST_ENABLED

#include "Utility/Timer.h"

#include <vector>

class CBatchTestEventHandler
{
public:
	CBatchTestEventHandler();

	enum ETerminationReason
	{
		TR_UNKNOWN						= -1,
		TR_REACHED_DL_COUNT				= 0,
		TR_TIME_LIMIT_REACHED			= 0x80000000,
		TR_TOO_MANY_VBLS_WITH_NO_DL,
	};

	void				Reset();

	void				Terminate( ETerminationReason reason );

	void				OnDisplayListComplete();
	void				OnVerticalBlank();
	void				OnDebugMessage( const char * msg );

	EAssertResult		OnAssert( const char * expression, const char * file, unsigned int line, const char * formatted_msg );

	ETerminationReason	GetTerminationReason() const { return mTerminationReason; }

	void				PrintSummary( FILE * fh );

	static const char * GetTerminationReasonString( ETerminationReason reason );

private:
	CTimer				mTimer;
	u32					mNumDisplayListsCompleted;
	u32					mNumVerticalBlanksSinceDisplayList;
	ETerminationReason	mTerminationReason;

	std::vector<u32>	mAsserts;
};

CBatchTestEventHandler * BatchTest_GetHandler();

void BatchTestMain( int argc, char* argv[] );

#endif
