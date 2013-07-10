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
#include "SysGL/GL.h"

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

private:
	void GetJoyPad(OSContPad *pPad);
	bool mGamePadAvailable;
};

IInputManager::IInputManager()
:	mGamePadAvailable(false)
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
	mGamePadAvailable = glfwJoystickPresent(GLFW_JOYSTICK_1);
}

void IInputManager::GetJoyPad(OSContPad *pPad)
{
	static const s32 N64_ANALOGUE_STICK_RANGE =  80;

	int num_axes;
	const float * axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &num_axes);
    if(!axes || num_axes < 2)
	{
		// gamepad was disconnected?
        DAEDALUS_ERROR("Couldn't read axes");
        return;
    }

    int num_buttons;
	const u8 * buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &num_buttons);
	if(!buttons || num_buttons < 24)
	{
		// gamepad was disconnected?
		DAEDALUS_ERROR("Couldn't read buttons");
		return;
	}

	//ToDo: Different gamepads will need different configuration, this is for PS3/PS2 controller
	if (buttons[11])	pPad->button |= START_BUTTON;
	if (buttons[9])		pPad->button |= START_BUTTON;
	if (buttons[2])		pPad->button |= A_BUTTON;
	if (buttons[3])		pPad->button |= B_BUTTON;
	if (buttons[6])		pPad->button |= Z_TRIG;
	if (buttons[4])		pPad->button |= L_TRIG;
	if (buttons[5])		pPad->button |= R_TRIG;

	if (buttons[20])	pPad->button |= U_JPAD;
	if (buttons[22])	pPad->button |= D_JPAD;
	if (buttons[23])	pPad->button |= L_JPAD;
	if (buttons[21])	pPad->button |= R_JPAD;

	// Hold O button and use hat buttons for N64 c buttons (same as the PSP)
	// We could use the second analog stick to map them, but will require to translate asix >=2
	if(buttons[1])
	{
		if (buttons[20])	pPad->button |= U_CBUTTONS;
		if (buttons[22])	pPad->button |= D_CBUTTONS;
		if (buttons[23])	pPad->button |= L_CBUTTONS;
		if (buttons[21])	pPad->button |= R_CBUTTONS;
	}

	// Used to see key presses, useful to add a different button configuration
	//for(int i = 0; i < num_buttons; i++)
	//{
	//	if(buttons[i])
	//		printf("%d\n",i);
	//}

	pPad->stick_x =  s8(axes[0] * N64_ANALOGUE_STICK_RANGE);
	pPad->stick_y =  s8(axes[1] * N64_ANALOGUE_STICK_RANGE);
}

void IInputManager::GetState( OSContPad pPad[4] )
{
	// Clear the initial state
	for(u32 cont = 0; cont < 4; cont++)
	{
		pPad[cont].button = 0;
		pPad[cont].stick_x = 0;
		pPad[cont].stick_y = 0;
	}

	// Check if a gamepad is connected, If not fallback to keyboard
	if(mGamePadAvailable)
	{
		GetJoyPad(&pPad[0]);
	}
	else if(GLFWwindow* window = gWindow)
	{
		if (glfwGetKey( window, 'X' ))		pPad[0].button |= A_BUTTON;
		if (glfwGetKey( window, 'C' ))		pPad[0].button |= B_BUTTON;
		if (glfwGetKey( window, 'Z' ))		pPad[0].button |= Z_TRIG;
		if (glfwGetKey( window, 'Y' ))		pPad[0].button |= Z_TRIG;		// For German keyboards :)
		if (glfwGetKey( window, 'A' ))		pPad[0].button |= L_TRIG;
		if (glfwGetKey( window, 'S' ))		pPad[0].button |= R_TRIG;

		if (glfwGetKey( window, GLFW_KEY_ENTER ))		pPad[0].button |= START_BUTTON;

		if (glfwGetKey( window, GLFW_KEY_KP_8 ))		pPad[0].button |= U_JPAD;
		if (glfwGetKey( window, GLFW_KEY_KP_2 ))		pPad[0].button |= D_JPAD;
		if (glfwGetKey( window, GLFW_KEY_KP_4 ))		pPad[0].button |= L_JPAD;
		if (glfwGetKey( window, GLFW_KEY_KP_6 ))		pPad[0].button |= R_JPAD;

		if (glfwGetKey( window, GLFW_KEY_HOME ))		pPad[0].button |= U_CBUTTONS;
		if (glfwGetKey( window, GLFW_KEY_END ))			pPad[0].button |= D_CBUTTONS;
		if (glfwGetKey( window, GLFW_KEY_DELETE ))		pPad[0].button |= L_CBUTTONS;
		if (glfwGetKey( window, GLFW_KEY_PAGE_DOWN ))	pPad[0].button |= R_CBUTTONS;

		if (glfwGetKey( window, GLFW_KEY_LEFT ))		pPad[0].stick_x = -80;
		if (glfwGetKey( window, GLFW_KEY_RIGHT ))		pPad[0].stick_x = +80;
		if (glfwGetKey( window, GLFW_KEY_UP ))			pPad[0].stick_y = +80;
		if (glfwGetKey( window, GLFW_KEY_DOWN ))		pPad[0].stick_y = -80;
	}
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
