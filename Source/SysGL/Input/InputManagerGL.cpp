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

#include <SDL2/SDL.h>


#include "Base/Types.h"
#include "Input/InputManager.h"

#include "Core/CPU.h"
#include "SysGL/GL.h"

#include "UI/UIContext.h" // for Input structures

#include <algorithm>

//Windows Xinput support 
#ifdef DAEDALUS_W32
#include <iostream>
#include <Windows.h>
#include <Xinput.h>

class CXBOXController
{
private:
	XINPUT_STATE _controllerState;
	int _controllerNum;
public:
	CXBOXController(int playerNumber);
	XINPUT_STATE GetState();
	bool IsConnected();
	void Vibrate(int leftVal = 0, int rightVal = 0);
};

CXBOXController::CXBOXController(int playerNumber)
{
	// Set the Controller Number
	_controllerNum = playerNumber - 1;
}

XINPUT_STATE CXBOXController::GetState()
{
	// Zeroise the state
	ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));

	// Get the state
	XInputGetState(_controllerNum, &_controllerState);

	return _controllerState;
}

bool CXBOXController::IsConnected()
{
	// Zeroise the state
	ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));

	// Get the state
	DWORD Result = XInputGetState(_controllerNum, &_controllerState);

	if (Result == ERROR_SUCCESS)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CXBOXController::Vibrate(int leftVal, int rightVal)
{
	// Create a Vibraton State
	XINPUT_VIBRATION Vibration;

	// Zeroise the Vibration
	ZeroMemory(&Vibration, sizeof(XINPUT_VIBRATION));

	// Set the Vibration Values
	Vibration.wLeftMotorSpeed = leftVal;
	Vibration.wRightMotorSpeed = rightVal;

	// Vibrate the controller
	XInputSetState(_controllerNum, &Vibration);
}

#endif


//TODO: Implement gamepad support with SDL
//#define GAMEPAD_SUPPORT

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
#ifdef GAMEPAD_SUPPORT
	void						GetGamePadStatus();

private:
	void GetJoyPad(OSContPad *pPad);
	bool mGamePadAvailable;
#endif
#ifdef DAEDALUS_W32

	CXBOXController* Player1;
	CXBOXController* Player2;
	CXBOXController* Player3;
	CXBOXController* Player4;

#endif

};

IInputManager::~IInputManager()
{
}

IInputManager::IInputManager()
#ifdef GAMEPAD_SUPPORT
:	mGamePadAvailable(false)
#endif
{
}

#ifdef GAMEPAD_SUPPORT
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
#endif

bool IInputManager::Initialise()
{
#ifdef DAEDALUS_W32
	Player1 = new CXBOXController(1);
	if (Player1->IsConnected()){
		std::cout << "Xinput device detected! ";
	}
	else{
		std::cout << "Xinput device not detected!";
	}
#endif

#ifdef GAMEPAD_SUPPORT	
	CPU_RegisterVblCallback( &CheckPadStatusVblHandler, this );
#endif
	return true;

}

void IInputManager::Finalise()
{
#ifdef GAMEPAD_SUPPORT
	CPU_UnregisterVblCallback( &CheckPadStatusVblHandler, this );
#endif
}

#ifdef GAMEPAD_SUPPORT
void IInputManager::GetGamePadStatus()
{
	mGamePadAvailable = glfwJoystickPresent(GLFW_JOYSTICK_1) ? true : false;
}

void InputManager::GetJoyPad(OSContPad *pPad)
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
#endif
void IInputManager::GetState( OSContPad pPad[4] )
{
	// Clear the initial state
	for(u32 cont = 0; cont < 4; cont++)
	{
		pPad[cont].button = 0;
		pPad[cont].stick_x = 0;
		pPad[cont].stick_y = 0;
	}

	if(SDL_Window* window = gWindow)
	{
		const u8* keys = SDL_GetKeyboardState( NULL );

		if (keys [ SDL_SCANCODE_UP ] ) {pPad[0].stick_y = +80;}
		if (keys [ SDL_SCANCODE_DOWN ] ) {pPad[0].stick_y = -80;}
		if (keys [ SDL_SCANCODE_LEFT ] ) {pPad[0].stick_x = -80;}
		if (keys [ SDL_SCANCODE_RIGHT ] ) {pPad[0].stick_x = +80;}

		if (keys [ SDL_SCANCODE_X ] ) {pPad[0].button |= A_BUTTON;}
		if (keys [ SDL_SCANCODE_C ] ) {pPad[0].button |= B_BUTTON;}
		if (keys [ SDL_SCANCODE_Z ] ) {pPad[0].button |= Z_TRIG;}
		if (keys [ SDL_SCANCODE_A ] ) {pPad[0].button |= L_TRIG;}
		if (keys [ SDL_SCANCODE_S ] ) {pPad[0].button |= R_TRIG;}


		if (keys [ SDL_SCANCODE_RETURN ] ) {pPad[0].button |= START_BUTTON;}

		if (keys [ SDL_SCANCODE_KP_8 ] ){  pPad[0].button |= U_JPAD;}
		if (keys [ SDL_SCANCODE_KP_2 ] ){  pPad[0].button |= D_JPAD;}
		if (keys [ SDL_SCANCODE_KP_4 ] ){  pPad[0].button |= L_JPAD;}
		if (keys [ SDL_SCANCODE_KP_6 ] ){  pPad[0].button |= R_JPAD;}

		if (keys [ SDL_SCANCODE_HOME ] ){  pPad[0].button |= U_CBUTTONS;}
		if (keys [ SDL_SCANCODE_END ] ){  pPad[0].button |= D_CBUTTONS;}
		if (keys [ SDL_SCANCODE_DELETE ] ){  pPad[0].button |= L_CBUTTONS;}
		if (keys [ SDL_SCANCODE_PAGEDOWN ] ){  pPad[0].button |= R_CBUTTONS;}
	}

#ifdef DAEDALUS_W32
	if (Player1->IsConnected())
	{
#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
		if (Player1->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_A){ pPad[0].button |= A_BUTTON;}
		if (Player1->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_B){ pPad[0].button |= B_BUTTON;}
		if (Player1->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_START) { pPad[0].button |= START_BUTTON; }
		if (Player1->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) { pPad[0].button |= L_TRIG; }
		if (Player1->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) { pPad[0].button |= R_TRIG; }
		if (Player1->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { pPad[0].button |= L_JPAD; }
		if (Player1->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { pPad[0].button |= R_JPAD; }
		if (Player1->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { pPad[0].button |= U_JPAD; }
		if (Player1->GetState().Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { pPad[0].button |= D_JPAD; }

		if (Player1->GetState().Gamepad.bLeftTrigger > 30) { pPad[0].button |= Z_TRIG; }
		if (Player1->GetState().Gamepad.bRightTrigger > 30 ) { pPad[0].button |= Z_TRIG; }

		pPad[0].stick_x = s8(Player1->GetState().Gamepad.sThumbLX / 500);
		pPad[0].stick_y = s8(Player1->GetState().Gamepad.sThumbLY / 500);

		//Xinput Righstick to C buttons
		if (s8(Player1->GetState().Gamepad.sThumbRX / 500 ) < -40) pPad[0].button |= L_CBUTTONS;
		if (s8(Player1->GetState().Gamepad.sThumbRX / 500 ) > 40) pPad[0].button |= R_CBUTTONS;
		if (s8(Player1->GetState().Gamepad.sThumbRY / 500 ) < -40) pPad[0].button |= D_CBUTTONS;
		if (s8(Player1->GetState().Gamepad.sThumbRY / 500 ) > 40) pPad[0].button |= U_CBUTTONS;
	
	}
#endif

#ifdef GAMEPAD_SUPPORT
	// Override the keyboard with the gamepad if it's available.
	if(mGamePadAvailable)
	{
		GetJoyPad(&pPad[0]);
	}
#endif
}

template<> bool CSingleton< CInputManager >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = std::make_shared<IInputManager>();
	return mpInstance->Initialise();
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

static bool toggle_fullscreen = false;
static s16 button = 0;
void sceCtrlPeekBufferPositive(SceCtrlData *data, int count){

	SDL_Event event;
	SDL_PumpEvents();

	while (SDL_PeepEvents( &event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 0)
	{
		if (event.type == SDL_QUIT)
		{
			CPU_Halt("Window Closed");	// SDL window was closed
            // Optionally, you can also call SDL_Quit() to terminate SDL subsystems
            SDL_Quit();
            // Exit the application
            exit(0);
		}
		else if(event.type == SDL_KEYDOWN)
		{
			if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
			{
				CPU_Halt("Window Closed");	// User pressed escape to exit
			}

			if (event.key.keysym.scancode == SDL_SCANCODE_F11)
			{
				if (toggle_fullscreen == false) {
					SDL_SetWindowFullscreen(gWindow, SDL_TRUE);
					toggle_fullscreen = true;
				}
				else
				{
					SDL_SetWindowFullscreen(gWindow, SDL_FALSE);
					toggle_fullscreen = false;
				}
			}

			// if (event.key.keysym.scancode == SDL_SCANCODE_UP) {data->Ly = +80;}
			// if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {data->Ly = -80;}
			// if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {data->Lx = -80;}
			// if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {data->Lx = +80;}

			if (event.key.keysym.scancode == SDL_SCANCODE_X) {button |= A_BUTTON;}
			if (event.key.keysym.scancode == SDL_SCANCODE_C) {button |= B_BUTTON;}
			if (event.key.keysym.scancode == SDL_SCANCODE_Z) {button |= Z_TRIG;}
			if (event.key.keysym.scancode == SDL_SCANCODE_A) {button |= L_TRIG;}
			if (event.key.keysym.scancode == SDL_SCANCODE_S) {button |= R_TRIG;}

			if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {button |= START_BUTTON;}

			if (event.key.keysym.scancode == SDL_SCANCODE_UP){  button |= U_JPAD;}
			if (event.key.keysym.scancode == SDL_SCANCODE_DOWN){  button |= D_JPAD;}
			if (event.key.keysym.scancode == SDL_SCANCODE_LEFT){  button |= L_JPAD;}
			if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT){  button |= R_JPAD;}

			if (event.key.keysym.scancode == SDL_SCANCODE_HOME){  button |= U_CBUTTONS;}
			if (event.key.keysym.scancode == SDL_SCANCODE_END){  button |= D_CBUTTONS;}
			if (event.key.keysym.scancode == SDL_SCANCODE_DELETE){  button |= L_CBUTTONS;}
			if (event.key.keysym.scancode == SDL_SCANCODE_PAGEDOWN){  button |= R_CBUTTONS;}
		}
		else if(event.type == SDL_KEYUP)
		{
			if (event.key.keysym.scancode == SDL_SCANCODE_X) {button &= ~A_BUTTON;}
			if (event.key.keysym.scancode == SDL_SCANCODE_C) {button &= ~B_BUTTON;}
			if (event.key.keysym.scancode == SDL_SCANCODE_Z) {button &= ~Z_TRIG;}
			if (event.key.keysym.scancode == SDL_SCANCODE_A) {button &= ~L_TRIG;}
			if (event.key.keysym.scancode == SDL_SCANCODE_S) {button &= ~R_TRIG;}

			if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {button &= ~START_BUTTON;}

			if (event.key.keysym.scancode == SDL_SCANCODE_UP){  button &= ~U_JPAD;}
			if (event.key.keysym.scancode == SDL_SCANCODE_DOWN){  button &= ~D_JPAD;}
			if (event.key.keysym.scancode == SDL_SCANCODE_LEFT){  button &= ~L_JPAD;}
			if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT){  button &= ~R_JPAD;}

			if (event.key.keysym.scancode == SDL_SCANCODE_HOME){  button &= ~U_CBUTTONS;}
			if (event.key.keysym.scancode == SDL_SCANCODE_END){  button &= ~D_CBUTTONS;}
			if (event.key.keysym.scancode == SDL_SCANCODE_DELETE){  button &= ~L_CBUTTONS;}
			if (event.key.keysym.scancode == SDL_SCANCODE_PAGEDOWN){  button &= ~R_CBUTTONS;}
		}
	}

	data->Buttons = button;
	data->Lx = 128;
	data->Ly = 128;
}


//*************************************************************************************
//
//*************************************************************************************
v2	ProjectToUnitSquare( const v2 & in )
{
	f32		length( in.Length() );
	float	abs_x( fabsf( in.x ) );
	float	abs_y( fabsf( in.y ) );
	float	scale;

	//
	//	Select the longest axis, and
	//
	if( length < 0.01f )
	{
		scale = 1.0f;
	}
	else if( abs_x > abs_y )
	{
		scale = length / abs_x;
	}
	else
	{
		scale = length / abs_y;
	}

	return in * scale;
}

//*************************************************************************************
//
//*************************************************************************************
v2	ApplyDeadzone( const v2 & in, f32 min_deadzone, f32 max_deadzone )
{
#ifdef DAEDALUS_ENABLE_ASSERTS

	DAEDALUS_ASSERT( min_deadzone >= 0.0f && min_deadzone <= 1.0f, "Invalid min deadzone" );
	DAEDALUS_ASSERT( max_deadzone >= 0.0f && max_deadzone <= 1.0f, "Invalid max deadzone" );
#endif
	float	length( in.Length() );

	if( length < min_deadzone )
		return v2( 0,0 );

	float	scale( ( length - min_deadzone ) / ( max_deadzone - min_deadzone )  );

	scale = std::clamp( scale, 0.0f, 1.0f );

	return ProjectToUnitSquare( in * (scale / length) );
}

void sceKernelExitGame() {
	// todo
}