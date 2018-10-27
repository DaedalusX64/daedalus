#include <stdio.h>
#include <libpad.h>

#ifdef _XINPUT
#include <libmtap.h>
#endif

#include <input.h>

/* #ifdef __cplusplus
extern "C" {
#endif
*/

enum EN64Button
{
	N64Button_A = 0,
	N64Button_B,
	N64Button_Z,
	N64Button_L,
	N64Button_R,
	N64Button_Up,
	N64Button_Down,
	N64Button_Left,
	N64Button_Right,
	N64Button_CUp,
	N64Button_CDown,
	N64Button_CLeft,
	N64Button_CRight,
	N64Button_Start,
	NUM_N64_BUTTONS
};

const u32 gN64ButtonMasks[NUM_N64_BUTTONS] =
{
	A_BUTTON,		//N64Button_A = 0,
	B_BUTTON,		//N64Button_B,
	Z_TRIG,			//N64Button_Z,
	L_TRIG,			//N64Button_L,
	R_TRIG,			//N64Button_R,
	U_JPAD,			//N64Button_Up,
	D_JPAD,			//N64Button_Down,
	L_JPAD,			//N64Button_Left,
	R_JPAD,			//N64Button_Right,
	U_CBUTTONS,		//N64Button_CUp,
	D_CBUTTONS,		//N64Button_CDown,
	L_CBUTTONS,		//N64Button_CLeft,
	R_CBUTTONS,		//N64Button_CRight,
	START_BUTTON	//N64Button_Start,
};

const char * const gN64ButtonNames[NUM_N64_BUTTONS] =
{
	"N64.A",			//N64Button_A = 0,
	"N64.B",			//N64Button_B,
	"N64.Z",			//N64Button_Z,
	"N64.LTrigger",		//N64Button_L,
	"N64.RTrigger",		//N64Button_R,
	"N64.Up",			//N64Button_Up,
	"N64.Down",			//N64Button_Down,
	"N64.Left",			//N64Button_Left,
	"N64.Right",		//N64Button_Right,
	"N64.CUp",			//N64Button_CUp,
	"N64.CDown",		//N64Button_CDown,
	"N64.CLeft",		//N64Button_CLeft,
	"N64.CRight",		//N64Button_CRight,
	"N64.Start"			//N64Button_Start,
};

const char * GetN64ButtonName( EN64Button button )
{
	return gN64ButtonNames[ button ];
}

/* Todo: Fix this OOP mess

class CControllerConfig
{
	public:
		CControllerConfig();
		~CControllerConfig();

	public:
		void				SetName( const char * name )		{ mName = name; }
		void				SetDescription( const char * desc )	{ mDescription = desc; }
		void				SetJoySwap( const char * swap )		{ mJoySwap = swap[0]; }	//Save only first char which should be A|B|C for the joystick swapping

		const char *		GetName() const						{ return mName.c_str(); }
		const char *		GetDescription() const				{ return mDescription.c_str(); }
		char				GetJoySwap() const					{ return mJoySwap; }

		void				SetButtonMapping( EN64Button button, CButtonMapping * p_mapping );
		u32					GetN64ButtonsState( u32 psp_button_mask ) const;

	private:
		std::string			mName;
		std::string			mDescription;
		char				mJoySwap;

		CButtonMapping *	mButtonMappings[ NUM_N64_BUTTONS ];
};
*/

CControllerConfig::CControllerConfig()
{
	for( u32 i = 0; i < NUM_N64_BUTTONS; ++i )
	{
		mButtonMappings[ i ] = NULL;
	}
}

CControllerConfig::~CControllerConfig()
{
	for( u32 i = 0; i < NUM_N64_BUTTONS; ++i )
	{
		delete mButtonMappings[ i ];
	}
}

void	CControllerConfig::SetButtonMapping( EN64Button button, CButtonMapping * p_mapping )
{
	delete mButtonMappings[ button ];
	mButtonMappings[ button ] = p_mapping;
}

u32		CControllerConfig::GetN64ButtonsState( u32 psp_button_mask ) const
{
	u32	state = 0;

	for( u32 i = 0; i < NUM_N64_BUTTONS; ++i )
	{
		if( mButtonMappings[ i ] != NULL )
		{
			if( mButtonMappings[ i ]->Evaluate( psp_button_mask ) )
			{
				state |= gN64ButtonMasks[ i ];
			}
		}
	}

	return state;
  }
}

/* Wow great, more C++, what is this shit anyway?

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

		virtual u32							GetConfigurationFromName( const char * name ) const;

	private:
		void								SwapJoyStick(OSContPad *pPad, SceCtrlData *pad);
		void								LoadControllerConfigs( const char * p_dir );
		CControllerConfig *					BuildDefaultConfig();
		CControllerConfig *					BuildControllerConfig( const char * filename );

	private:
		CControllerConfig *					mpControllerConfig;
		std::vector<CControllerConfig*>		mControllerConfigs;
};

IInputManager::IInputManager()
:	mpControllerConfig( NULL )
{
}

IInputManager::IInputManager()
:	mpControllerConfig( NULL )
{
}

IInputManager::~IInputManager()
{
	for( u32 i = 0; i < mControllerConfigs.size(); ++i )
	{
		delete mControllerConfigs[ i ];
		mControllerConfigs[ i ] = NULL;
	}
}
*/

// Some code thanks to PS2 SDK

pad_t *pad_open(unsigned int port, unsigned int slot, unsigned int mode, unsigned int lock)
{

	pad_t *pad = (pad_t*)malloc(sizeof(pad_t));

	pad->buffer = (char*)memalign(64,256);
	pad->buttons = (struct padButtonStatus*)malloc(sizeof(struct padButtonStatus));
	pad->actuator = NULL;

	// there are only two ports
	if (port > 1)
	{
		free(pad->buttons);
		free(pad);
		return NULL;
	}

#ifdef _XINPUT
	if (mtapGetConnection(pad->port))
	{
		// there are only four slots
		if (slot > 3)
		{
			free(pad->buttons);
			free(pad);
			return NULL;
		}
	}
	else
	{
#endif
	// there's only one slot
	if (slot)
	{
		free(pad->buttons);
		free(pad);
		return NULL;
	}
#ifdef _XINPUT
	}
#endif

	pad->port = port;
#ifdef _XINPUT
	pad->slot = slot;
#else
	pad->slot = 0;
#endif

	// Placeholders
	pad->type = PAD_TYPE_DIGITAL;

	pad->state = 0;
	pad->last_state = -1;
	pad->exec_cmd = 0;
	pad->num_modes = -1;
	pad->sensitivity = 0;

	// These get verified by pad_set_mode()
	pad->mode = mode;
	pad->lock = lock;

	padPortOpen(pad->port,pad->slot,pad->buffer);

	pad_wait(pad);

	pad_set_mode(pad,pad->mode,pad->lock);

	return pad;

}

void pad_close(pad_t *pad)
{

	if (pad == NULL)
	{
		return;
	}

	if (pad->actuator != NULL)
	{
		free(pad->actuator);
	}

	padPortClose(pad->port,pad->slot);

	free(pad->buttons);
	free(pad->buffer);
	free(pad);

}

int pad_get_state(pad_t* pad)
{

	pad->state = padGetState(pad->port, pad->slot);

	return pad->state;

}
#ifdef DEBUG
void pad_print_state(pad_t* pad)
{

	char stateString[16];

	padStateInt2String(pad->state,stateString);
	printf("Pad(%d,%d) state is: %s\n",pad->port,pad->slot,stateString);

}
#endif
void pad_wait(pad_t* pad)
{

	pad_get_state(pad);

	if (pad->state != pad->last_state)
	{
#ifdef DEBUG
		pad_print_state(pad);
#endif
		pad->last_state = pad->state;

	}

	if (pad->state == PAD_STATE_STABLE) return;
	if (pad->state == PAD_STATE_ERROR) return;
	if (pad->state == PAD_STATE_DISCONN) return;
	if (pad->state == PAD_STATE_FINDCTP1) return;

	return pad_wait(pad);

}

int pad_get_num_modes(pad_t *pad)
{

	pad->num_modes = padInfoMode(pad->port,pad->slot,PAD_MODETABLE,-1);

	return pad->num_modes;

}
#ifdef DEBUG
void pad_print_supported_modes(pad_t *pad)
{

	int i = 0;
	int modes = pad_get_num_modes(pad);

	printf("The device has %d modes\n", modes);

	if (modes > 0)
	{

		printf("( ");

		for (i = 0; i < modes; i++)
		{
			printf("%d ", padInfoMode(0, 0, PAD_MODETABLE, i));
		}

		printf(")\n");

	}

}
#endif
int pad_has_type(pad_t *pad, int type)
{

	int i = 0;

	if (pad->num_modes < 0)
	{
		pad_get_num_modes(pad);
	}

	if ((pad->num_modes == 0) && (type == PAD_TYPE_DIGITAL))
	{
		return 1;
	}

	for (i = 0; i < pad->num_modes; i++)
	{
		if (padInfoMode(pad->port,pad->slot,PAD_MODETABLE,i) == type)
		{
			return 1;
		}

	}

	return 0;

}

int pad_get_type(pad_t *pad)
{

	pad->type = padInfoMode(pad->port,pad->slot,PAD_MODECURID,0);

	return pad->type;

}

void pad_set_mode(pad_t *pad, int mode, int lock)
{

	int status = 0;

	if (lock == PAD_MMODE_LOCK)
	{
		pad->lock = lock;
	}
	else
	{
		pad->lock = PAD_MMODE_UNLOCK;
	}

	if ((mode == PAD_MMODE_DUALSHOCK) || (mode == PAD_TYPE_DUALSHOCK))
	{
		if (pad_has_type(pad,PAD_TYPE_DUALSHOCK))
		{
			status = padSetMainMode(pad->port,pad->slot,pad->mode,pad->lock);
		}

		if (status)
		{
			pad->mode = PAD_MMODE_DUALSHOCK;
		}
	}
	else
	{
		if (pad_has_type(pad,PAD_TYPE_DIGITAL))
		{
			status = padSetMainMode(pad->port,pad->slot,pad->mode,pad->lock);
		}

		if (status)
		{
			pad->mode = PAD_MMODE_DIGITAL;
		}
	}

	if (status)
	{
		pad_wait(pad);
	}

}

void pad_set_sensitivity(pad_t *pad, int enable)
{

	if(padInfoPressMode(pad->port,pad->slot))
	{
		pad_wait(pad);
		pad->sensitivity = 1;
	}
	else
	{
		pad_wait(pad);
		pad->sensitivity = 0;
		return;
	}

	if (enable)
	{
		padEnterPressMode(pad->port,pad->slot);
	}
	else
	{
		padExitPressMode(pad->port,pad->slot);
	}


}

void pad_init_actuators(pad_t *pad)
{

	if (pad->actuator == NULL)
	{
#ifdef _XINPUT
		if (padInfoAct(pad->port, pad->slot, -1, 0))
#else
		if (padInfoMode(pad->port,pad->slot,PAD_MODECUREXID,0))
#endif
		{
			pad_wait(pad);
			pad->actuator = (actuator_t*)malloc(sizeof(actuator_t));
		}
		else
		{
			pad_wait(pad);
			return;
		}
	}

	pad->actuator->small = 0;
	pad->actuator->large = 0x00;

	pad->actuator->status[0] = 0;
	pad->actuator->status[1] = 1;
	pad->actuator->status[2] = 0xFF;
	pad->actuator->status[3] = 0xFF;
	pad->actuator->status[4] = 0xFF;
	pad->actuator->status[5] = 0xFF;


	pad_wait(pad);
	padSetActAlign(pad->port,pad->slot,pad->actuator->status);
	pad_wait(pad);

}

void pad_set_actuators(pad_t *pad, int small, unsigned char large)
{

	if (!small)
	{
		pad->actuator->small = small;
	}
	else
	{
		pad->actuator->small = 0x01;
	}

	pad->actuator->large = large;

	pad->actuator->status[0] = pad->actuator->small;
	pad->actuator->status[1] = pad->actuator->large;

	padSetActDirect(pad->port,pad->slot,pad->actuator->status);

}
