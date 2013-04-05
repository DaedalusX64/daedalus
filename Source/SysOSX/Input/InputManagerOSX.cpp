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

#include <GL/glfw.h>



class IInputManager : public CInputManager
{
public:
	IInputManager();
	virtual ~IInputManager();

	virtual bool						Initialise();
	virtual void						Finalise()					{}

	virtual bool						GetState( OSContPad pPad[4] );

	virtual u32							GetNumConfigurations() const;
	virtual const char *				GetConfigurationName( u32 configuration_idx ) const;
	virtual const char *				GetConfigurationDescription( u32 configuration_idx ) const;
	virtual void						SetConfiguration( u32 configuration_idx );

	virtual u32							GetConfigurationFromName( const char * name ) const;
private:
	bool InitGamePad();
	void GetJoyPad(OSContPad *pPad);

	u32 mNumAxes;
	u32 mNumButtoms;
	bool mIsGamePad;
	
	f32 *mJoyStick;
	u8 *mJoyButton;

};

IInputManager::IInputManager() :
	mJoyStick( NULL ),
	mJoyButton( NULL ),
	mNumAxes( 0 ),
	mNumButtoms( 0 ),
	mIsGamePad( false )
{
	if(!InitGamePad())
	{
        DAEDALUS_ASSERT(!mIsGamePad, "Couldn't init gamepad");
    }
}

IInputManager::~IInputManager()
{
	if(mJoyStick)
		delete [] mJoyStick;

	if(mJoyButton)
		delete [] mJoyButton;
}

bool IInputManager::Initialise()
{
	return true;
}

bool IInputManager::InitGamePad()
{
	mIsGamePad = glfwGetJoystickParam(GLFW_JOYSTICK_1, GLFW_PRESENT);
	if(mIsGamePad)
	{
		mNumAxes = glfwGetJoystickParam(GLFW_JOYSTICK_1,GLFW_AXES);
		mNumButtoms = glfwGetJoystickParam(GLFW_JOYSTICK_1,GLFW_BUTTONS);
		if(mNumAxes && mNumButtoms )
		{
			mJoyStick = new f32[2];	//Only two axis are needed
			mJoyButton = new u8[mNumButtoms];
			return true;
		}
		else
			mIsGamePad = false;
	}
	return false;
}

void IInputManager::GetJoyPad(OSContPad *pPad)
{
	static const s32 N64_ANALOGUE_STICK_RANGE =  80;

    if(!glfwGetJoystickPos(GLFW_JOYSTICK_1, mJoyStick, 2 ))
	{
		// gamepad was disconnected?
        DAEDALUS_ERROR("Couldn't read axes");
		mIsGamePad = false;
        return;
    }
    
	if(!glfwGetJoystickButtons(GLFW_JOYSTICK_1, mJoyButton, mNumButtoms))
	{
		// gamepad was disconnected?
		DAEDALUS_ERROR("Couldn't read buttons");
		mIsGamePad = false;
		return;
	}

	//ToDo: Different gamepads will need different configuration, this is for PS3/PS2 controller
	if (mJoyButton[11])		pPad->button |= START_BUTTON;
	if (mJoyButton[9])		pPad->button |= START_BUTTON;
	if (mJoyButton[2])		pPad->button |= A_BUTTON;
	if (mJoyButton[3])		pPad->button |= B_BUTTON;
	if (mJoyButton[6])		pPad->button |= Z_TRIG;
	if (mJoyButton[4])		pPad->button |= L_TRIG;
	if (mJoyButton[5])		pPad->button |= R_TRIG;

	// Used to see key presses, useful to add a different button configuration
	//for(u32 i = 0; i < mNumButtoms; i++)
	//	printf("%d",mJoyButton[i]);

	//printf("\n");

	pPad->stick_x =  s8(mJoyStick[0] * N64_ANALOGUE_STICK_RANGE);
	pPad->stick_y =  s8(mJoyStick[1] * N64_ANALOGUE_STICK_RANGE);

	//ToDo: Map DPAD and c buttons
	//DPAD and hat POV are implemented until glfw 3.0 shall we update? :) 
}

bool IInputManager::GetState( OSContPad pPad[4] )
{
	// Clear the initial state
	for(u32 cont = 0; cont < 4; cont++)
	{
		pPad[cont].button = 0;
		pPad[cont].stick_x = 0;
		pPad[cont].stick_y = 0;
	}

	//Check if a gamepad is connected, If not fallback to keyboard
	if( mIsGamePad == true)
	{
		GetJoyPad(&pPad[0]);
	}
	else
	{
		if (glfwGetKey( 'A' ))		pPad[0].button |= START_BUTTON;
		if (glfwGetKey( 'S' ))		pPad[0].button |= A_BUTTON;
		if (glfwGetKey( 'X' ))		pPad[0].button |= B_BUTTON;
		if (glfwGetKey( 'Z' ))		pPad[0].button |= Z_TRIG;
		if (glfwGetKey( 'Y' ))		pPad[0].button |= Z_TRIG;		// For German keyboards :)
		if (glfwGetKey( 'C' ))		pPad[0].button |= L_TRIG;
		if (glfwGetKey( 'V' ))		pPad[0].button |= R_TRIG;

		if (glfwGetKey( 'T' ))		pPad[0].button |= U_JPAD;
		if (glfwGetKey( 'G' ))		pPad[0].button |= D_JPAD;
		if (glfwGetKey( 'F' ))		pPad[0].button |= L_JPAD;
		if (glfwGetKey( 'H' ))		pPad[0].button |= R_JPAD;

		if (glfwGetKey( 'I' ))		pPad[0].button |= U_CBUTTONS;
		if (glfwGetKey( 'K' ))		pPad[0].button |= D_CBUTTONS;
		if (glfwGetKey( 'J' ))		pPad[0].button |= L_CBUTTONS;
		if (glfwGetKey( 'L' ))		pPad[0].button |= R_CBUTTONS;

		if (glfwGetKey( GLFW_KEY_LEFT ))	pPad[0].stick_x = -80;
		if (glfwGetKey( GLFW_KEY_RIGHT ))	pPad[0].stick_x = +80;
		if (glfwGetKey( GLFW_KEY_UP ))		pPad[0].stick_y = +80;
		if (glfwGetKey( GLFW_KEY_DOWN ))	pPad[0].stick_y = -80;
	}

	return true;
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
