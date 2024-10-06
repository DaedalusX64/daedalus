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
#include <iostream>
SDL_GameController *controller;

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
	void 						GetJoyPad(OSContPad *pPad);
	
	private:
	
	bool mGamePadAvailable;
	//SDL_GameController *controller;

};

IInputManager::~IInputManager()
{
}

IInputManager::IInputManager()

:	mGamePadAvailable(false)

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


	//Init Joystick / Gamepad 
	CPU_RegisterVblCallback( &CheckPadStatusVblHandler, this );


	return true;
}

void IInputManager::Finalise()
{

	CPU_UnregisterVblCallback( &CheckPadStatusVblHandler, this );

}


void IInputManager::GetGamePadStatus()
{
	//Check for joystick SDL2 and open it if avaiable 
		controller = SDL_GameControllerOpen(0);
		if(!controller){
			std::cout << "Controller Not Found" << std::endl;
		}
		else{
			std::cout << "Controller found" << std::endl; // We probably need to confirm what the controller is
		}
	}

void IInputManager::GetJoyPad(OSContPad *pPad)
{
	static const s32 N64_ANALOGUE_STICK_RANGE =  80;

	//ToDo: Different gamepads will need different configuration, this is for PS3/PS2 controller
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START))					pPad->button |= START_BUTTON;
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A))						pPad->button |= A_BUTTON;
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B))						pPad->button |= B_BUTTON;
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))				pPad->button |= Z_TRIG;
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))			pPad->button |= Z_TRIG;
	if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16383)		pPad->button |= L_TRIG;
	if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16383)		pPad->button |= R_TRIG;

	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))		pPad->button |= U_JPAD;
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))	pPad->button |= D_JPAD;
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))	pPad->button |= L_JPAD;
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))	pPad->button |= R_JPAD;

	// Hold O button and use hat buttons for N64 c buttons (same as the PSP)
	// We could use the second analog stick to map them, but will require to translate asix >=2
	if(SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X))
	{
		if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))		pPad->button |= U_CBUTTONS;
		if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))	pPad->button |= D_CBUTTONS;
		if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))	pPad->button |= L_CBUTTONS;
		if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))	pPad->button |= R_CBUTTONS;
	}

	// Used to see key presses, useful to add a different button configuration
	//for(int i = 0; i < num_buttons; i++)
	//{
	//	if(buttons[i])
	//		printf("%d\n",i);
	//}

    const s32 SDL_AXIS_MIN = -32768;
    const s32 SDL_AXIS_MAX = 32767;

    // Get the raw axis values
    s32 raw_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
    s32 raw_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);

    // Normalize the axis values to the range [-1, 1]
    float normalized_x = raw_x / 32767.0f;
    float normalized_y = raw_y / 32767.0f;

    // Scale the normalized values to the N64 range
    s8 scaled_x = static_cast<s8>(normalized_x * N64_ANALOGUE_STICK_RANGE);
    s8 scaled_y = static_cast<s8>(normalized_y * N64_ANALOGUE_STICK_RANGE);

    // Manually clamp the values to ensure they are within the valid range
    if (scaled_x > N64_ANALOGUE_STICK_RANGE) {
        scaled_x = N64_ANALOGUE_STICK_RANGE;
    } else if (scaled_x < -N64_ANALOGUE_STICK_RANGE) {
        scaled_x = -N64_ANALOGUE_STICK_RANGE;
    }

    if (scaled_y > N64_ANALOGUE_STICK_RANGE) {
        scaled_y = N64_ANALOGUE_STICK_RANGE;
    } else if (scaled_y < -N64_ANALOGUE_STICK_RANGE) {
        scaled_y = -N64_ANALOGUE_STICK_RANGE;
    }

    pPad->stick_x = scaled_x;
    pPad->stick_y = -1 * scaled_y;

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

	if(SDL_Window* window = gWindow)
	{
		const u8* keys = SDL_GetKeyboardState( NULL );

			// Override the keyboard with the gamepad if it's available.
			//if(mGamePadAvailable)
			//{
				GetJoyPad(&pPad[0]);
			//}


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
	controller = SDL_GameControllerOpen(0);

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
		else if( event.type == SDL_KEYUP)
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


			//UI Controler
			if(controller && event.type == SDL_CONTROLLERBUTTONDOWN){	
				if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START))					button |= START_BUTTON;
				if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A))						button |= A_BUTTON;
				if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B))						button |= B_BUTTON;
				if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))			button |= L_TRIG;		
				if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))			button |= R_TRIG;	
				if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))					button |= U_JPAD;
				if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))				button |= D_JPAD;
				if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))				button |= L_JPAD;
				if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))				button |= R_JPAD;
				//Using this breaks the menu / Controller support 
				//if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK))				    button |= Z_TRIG;

			}
			else if(controller && event.type == SDL_CONTROLLERBUTTONUP){
				if (!SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START))					button &= ~START_BUTTON;
				if (!SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A))						button &= ~A_BUTTON;
				if (!SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B))						button &= ~B_BUTTON;
				if (!SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))			button &= ~L_TRIG;		
				if (!SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))			button &= ~R_TRIG;	
				if (!SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))				button &= ~U_JPAD;
				if (!SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))				button &= ~D_JPAD;
				if (!SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))				button &= ~L_JPAD;
				if (!SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))				button &= ~R_JPAD;
				//Using this breaks the menu / Controller support 
				//if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK))				    button &= ~Z_TRIG;
			}


	}

	//SDL_GameControllerClose(controller);

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