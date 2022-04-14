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

#include "stdafx.h"
#include "Input/InputManager.h"

#include "Core/CPU.h"
#include <3ds.h>
#include <stdio.h>

class IInputManager : public CInputManager
{
public:
	IInputManager();
	virtual ~IInputManager();

	virtual bool				Initialise();
	virtual void				Finalise();

	virtual void				GetState( OSContPad pPad[4] );

	virtual u32					GetNumConfigurations() const;
	virtual const char *		GetConfigurationName( u32 configuration_idx ) const;
	virtual const char *		GetConfigurationDescription( u32 configuration_idx ) const;
	virtual void				SetConfiguration( u32 configuration_idx );
	virtual u32					GetConfigurationFromName( const char * name ) const;

	void						GetGamePadStatus();
};

IInputManager::IInputManager()
{
}

IInputManager::~IInputManager()
{
}

static void CheckPadStatusVblHandler( void * arg )
{
	IInputManager * manager = static_cast< IInputManager * >( arg );

	// Only check the pad status every 60 vbls, otherwise it's too expensive.
	static u32 count = 0;
	if ((count % 60) == 0)
	{
		manager->GetGamePadStatus();
	}
	++count;
}

bool IInputManager::Initialise()
{
	CPU_RegisterVblCallback( &CheckPadStatusVblHandler, this );
	return true;
}

void IInputManager::Finalise()
{
	CPU_UnregisterVblCallback( &CheckPadStatusVblHandler, this );
}


void IInputManager::GetGamePadStatus()
{
}

void IInputManager::GetState( OSContPad pPad[4] )
{
	circlePosition circlepad;

	// Clear the initial state
	for(u32 cont = 0; cont < 4; cont++)
	{
		pPad[cont].button = 0;
		pPad[cont].stick_x = 0;
		pPad[cont].stick_y = 0;
	}

	hidScanInput();

	hidCircleRead(&circlepad);
	
	pPad[0].stick_x = circlepad.dx / 2;
	pPad[0].stick_y = circlepad.dy / 2;

	if (hidKeysHeld() & KEY_A)		pPad[0].button |= A_BUTTON;
	if (hidKeysHeld() & KEY_B)		pPad[0].button |= B_BUTTON;

	if (hidKeysHeld() & KEY_X)		pPad[0].button |= Z_TRIG;
	if (hidKeysHeld() & KEY_ZR)		pPad[0].button |= Z_TRIG;
	if (hidKeysHeld() & KEY_ZL)		pPad[0].button |= Z_TRIG;

	if (hidKeysHeld() & KEY_L)		pPad[0].button |= L_TRIG;
	if (hidKeysHeld() & KEY_R)		pPad[0].button |= R_TRIG;

	if (hidKeysHeld() & KEY_START)		pPad[0].button |= START_BUTTON;

	if (hidKeysHeld() & KEY_DUP)		pPad[0].button |= U_JPAD;
	if (hidKeysHeld() & KEY_DDOWN)		pPad[0].button |= D_JPAD;
	if (hidKeysHeld() & KEY_DLEFT)		pPad[0].button |= L_JPAD;
	if (hidKeysHeld() & KEY_DRIGHT)		pPad[0].button |= R_JPAD;

	if (hidKeysHeld() & KEY_CSTICK_UP)		pPad[0].button |= U_CBUTTONS;
	if (hidKeysHeld() & KEY_CSTICK_DOWN)		pPad[0].button |= D_CBUTTONS;
	if (hidKeysHeld() & KEY_CSTICK_LEFT)		pPad[0].button |= L_CBUTTONS;
	if (hidKeysHeld() & KEY_CSTICK_RIGHT)		pPad[0].button |= R_CBUTTONS;

}

template<> bool	CSingleton< CInputManager >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	IInputManager * manager = new IInputManager();

	if(manager->Initialise())
	{
		mpInstance = manager;
		return true;
	}

	delete manager;
	return false;
}

u32	 IInputManager::GetNumConfigurations() const
{
	return 0;
}

const char * IInputManager::GetConfigurationName( u32 configuration_idx ) const
{
	DAEDALUS_ERROR( "Invalid controller config" );
	return "?";
}

const char * IInputManager::GetConfigurationDescription( u32 configuration_idx ) const
{
	DAEDALUS_ERROR( "Invalid controller config" );
	return "?";
}

void IInputManager::SetConfiguration( u32 configuration_idx )
{
	DAEDALUS_ERROR( "Invalid controller config" );
}

u32		IInputManager::GetConfigurationFromName( const char * name ) const
{
	// Return the default controller config
	return 0;
}
