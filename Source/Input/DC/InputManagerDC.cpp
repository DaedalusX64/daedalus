#include <kos.h>

#include <stack>
#include <string>
#include <vector>
#include <memory> 

#include "Core/CPU.h"
#include "Debug/DBGConsole.h"

#include "Input/InputManager.h"
#include "Utility/IniFile.h"
#include "Utility/Stream.h"


//*****************************************************************************
//
//*****************************************************************************

class IInputManager : public CInputManager
{
	public:
		IInputManager();
		virtual ~IInputManager();

		virtual bool						Initialise();
		virtual void						Finalise()					{}

		virtual void						GetState( OSContPad pPad[4] );

		virtual u32							GetNumConfigurations() const;
		virtual const char *				GetConfigurationName( u32 configuration_idx ) const;
		virtual const char *				GetConfigurationDescription( u32 configuration_idx ) const;
		virtual void						SetConfiguration( u32 configuration_idx );
};

//*****************************************************************************
//
//*****************************************************************************
IInputManager::IInputManager()
{
}

//*****************************************************************************
//
//*****************************************************************************
IInputManager::~IInputManager()
{
}

//*****************************************************************************
//
//*****************************************************************************
bool	IInputManager::Initialise()
{
	/*KOS
    CControllerConfig * p_default_config( BuildDefaultConfig() );

	mControllerConfigs.push_back( p_default_config );

	// Parse all the ini files here
	LoadControllerConfigs( "ControllerConfigs/" );

	mpControllerConfig = mControllerConfigs.front();*/

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void IInputManager::GetState( OSContPad pPad[4] )
{
    // Speichert den aktuellen Kontroller
    int iCont = 0;

    // Eingaben f�r jeden Kontroller zur�cksetzen
    for( int cont = 0; cont < 4; cont++ )
	{
		pPad[cont].button = 0;
		pPad[cont].stick_x = 0;
		pPad[cont].stick_y = 0;
	}

    // F�r jeden angeschlossenen Kontroller ausf�hren
	MAPLE_FOREACH_BEGIN( MAPLE_FUNC_CONTROLLER, cont_state_t, st )
        // Abbrechen, falls am ersten Kontroller Start, Trigger links und Trigger rechts gedr�ckt sind
		if( iCont == 0 && st->buttons & CONT_START && st->ltrig > 250 && st->rtrig > 250 )
            gCPUState.AddJob( CPU_STOP_RUNNING );

        // Achsen an den Emu weiterreichen (Wertebereich -128 bis 128 in Wertebereich -80 bis 80 umwandeln
        pPad[iCont].stick_x = (int)((float)st->joyx * 0.625f);
        pPad[iCont].stick_y = (int)((float)-st->joyy * 0.625f);

        // Buttons an den Emu weiterreichen
        if( st->buttons & CONT_START )
            pPad[iCont].button |= START_BUTTON;
        if( st->buttons & CONT_A )
            pPad[iCont].button |= A_BUTTON;
        if( st->buttons & CONT_B )
            pPad[iCont].button |= B_BUTTON;

        // Trigger an den Emu weiterreichen
        if( st->buttons & CONT_X )
            pPad[iCont].button |= Z_TRIG;
        if( st->rtrig > 16 )
            pPad[iCont].button |= R_TRIG;
        if( st->ltrig > 16 )
            pPad[iCont].button |= L_TRIG;

        // Steuerkreuz an den Emu weiterreichen
        if( st->buttons & CONT_DPAD_UP )
            pPad[iCont].button |= U_JPAD;
        if( st->buttons & CONT_DPAD_DOWN )
            pPad[iCont].button |= D_JPAD;
        if( st->buttons & CONT_DPAD_LEFT )
            pPad[iCont].button |= L_JPAD;
        if( st->buttons & CONT_DPAD_RIGHT )
            pPad[iCont].button |= R_JPAD;


		// Mit n�chstem Kontroller fortfahren
		iCont++;
	MAPLE_FOREACH_END()

}

//*****************************************************************************
//
//*****************************************************************************
template<> bool	CSingleton< CInputManager >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = std::make_shared<IInputManager>();
	return mpInstance->Initialise();
}

//*****************************************************************************
//
//*****************************************************************************
u32	 IInputManager::GetNumConfigurations() const
{
	return 1;
}

//*****************************************************************************
//
//*****************************************************************************
const char *	IInputManager::GetConfigurationName( u32 configuration_idx ) const
{
	return "?";
}

//*****************************************************************************
//
//*****************************************************************************
const char *	IInputManager::GetConfigurationDescription( u32 configuration_idx ) const
{
	return "?";
}

//*****************************************************************************
//
//*****************************************************************************
void			IInputManager::SetConfiguration( u32 configuration_idx )
{
}


