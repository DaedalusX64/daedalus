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

#include <stack>
#include <string>
#include <vector>
#include <3ds.h>

#include "Config/ConfigOptions.h"
#include "Debug/DBGConsole.h"
#include "Math/Math.h"
#include "Math/MathUtil.h"
#include "Utility/IniFile.h"
#include "Utility/IO.h"
#include "Utility/Macros.h"
#include "Utility/Preferences.h"
#include "Utility/Stream.h"
#include "Utility/Synchroniser.h"

extern bool isN3DS;

class CButtonMapping
{
	public:
		virtual ~CButtonMapping() {}
		virtual bool	Evaluate( u32 in_buttons ) const = 0;
};

class CButtonMappingMask : public CButtonMapping
{
	public:
		CButtonMappingMask( u32 mask ) : mMask( mask ) {}

		virtual bool	Evaluate( u32 in_buttons ) const		{ return ( in_buttons & mMask ) != 0; }

	private:
		u32					mMask;
};

class CButtonMappingNegate : public CButtonMapping
{
	public:
		CButtonMappingNegate( CButtonMapping * p_mapping )
			:	mpMapping( p_mapping )
		{
		}

		~CButtonMappingNegate()
		{
			delete mpMapping;
		}

		virtual bool	Evaluate( u32 in_buttons ) const
		{
			return !mpMapping->Evaluate( in_buttons );
		}

	private:
		CButtonMapping *	mpMapping;
};

class CButtonMappingAnd : public CButtonMapping
{
	public:
		CButtonMappingAnd( CButtonMapping * p_a, CButtonMapping * p_b )
			:	mpMappingA( p_a )
			,	mpMappingB( p_b )
		{
		}

		~CButtonMappingAnd()
		{
			delete mpMappingA;
			delete mpMappingB;
		}

		virtual bool	Evaluate( u32 in_buttons ) const
		{
			return mpMappingA->Evaluate( in_buttons ) &&
				   mpMappingB->Evaluate( in_buttons );
		}

	private:
		CButtonMapping *	mpMappingA;
		CButtonMapping *	mpMappingB;
};


class CButtonMappingOr : public CButtonMapping
{
	public:
		CButtonMappingOr( CButtonMapping * p_a, CButtonMapping * p_b )
			:	mpMappingA( p_a )
			,	mpMappingB( p_b )
		{
		}

		~CButtonMappingOr()
		{
			delete mpMappingA;
			delete mpMappingB;
		}

		virtual bool	Evaluate( u32 in_buttons ) const
		{
			return mpMappingA->Evaluate( in_buttons ) ||
				   mpMappingB->Evaluate( in_buttons );
		}

	private:
		CButtonMapping *	mpMappingA;
		CButtonMapping *	mpMappingB;
};

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

//*****************************************************************************
//	A controller configuration
//*****************************************************************************

class CControllerConfig
{
	public:
		CControllerConfig();
		~CControllerConfig();

	public:
		void				SetName( const char * name )		{ mName = name; }
		void				SetDescription( const char * desc )	{ mDescription = desc; }

		const char *		GetName() const						{ return mName.c_str(); }
		const char *		GetDescription() const				{ return mDescription.c_str(); }

		void				SetButtonMapping( EN64Button button, CButtonMapping * p_mapping );
		u32					GetN64ButtonsState( u32 psp_button_mask ) const;

	private:
		std::string			mName;
		std::string			mDescription;

		CButtonMapping *	mButtonMappings[ NUM_N64_BUTTONS ];
};

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

//*****************************************************************************
//
//*****************************************************************************

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

	private:
		void								LoadControllerConfigs( const char * p_dir );
		CControllerConfig *					BuildDefaultConfig(bool ZSwap = false);
		CControllerConfig *					BuildControllerConfig( const char * filename );

		CControllerConfig *					mpControllerConfig;
		std::vector<CControllerConfig*>		mControllerConfigs;
};

IInputManager::IInputManager() : mpControllerConfig( NULL )
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

bool IInputManager::Initialise()
{
	if( !mControllerConfigs.empty() ) return true;

	CControllerConfig *p_default_config( BuildDefaultConfig() );
	mControllerConfigs.push_back( p_default_config );

	if( !isN3DS)
	{
		CControllerConfig *p_default_z_config( BuildDefaultConfig(true) );
		mControllerConfigs.push_back( p_default_z_config );
	}

	LoadControllerConfigs( DAEDALUS_CTR_PATH( "ControllerConfigs/" ) );

	SetConfiguration(0);

	return true;
}

void IInputManager::Finalise()
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

	pPad[0].button = mpControllerConfig->GetN64ButtonsState( hidKeysHeld() );
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
	return mControllerConfigs.size();
}

const char * IInputManager::GetConfigurationName( u32 configuration_idx ) const
{
	if( configuration_idx < mControllerConfigs.size() )
	{
		return mControllerConfigs[ configuration_idx ]->GetName();
	}

	return "?";
}

const char * IInputManager::GetConfigurationDescription( u32 configuration_idx ) const
{
	if( configuration_idx < mControllerConfigs.size() )
	{
		return mControllerConfigs[ configuration_idx ]->GetDescription();
	}

	return "?";
}

void IInputManager::SetConfiguration( u32 configuration_idx )
{
	if( configuration_idx < mControllerConfigs.size() )
	{
		mpControllerConfig = mControllerConfigs[ configuration_idx ];
	}
}

u32		IInputManager::GetConfigurationFromName( const char * name ) const
{
	for( u32 i = 0; i < mControllerConfigs.size(); ++i )
	{
		if( _strcmpi( mControllerConfigs[ i ]->GetName(), name ) == 0 )
		{
			return i;
		}
	}

	// Return the default controller config
	return 0;
}

void	IInputManager::LoadControllerConfigs( const char * p_dir )
{
	IO::FindHandleT		find_handle;
	IO::FindDataT		find_data;
	if(IO::FindFileOpen( p_dir, &find_handle, find_data ))
	{
		do
		{
			const char * filename( find_data.Name );
			const char * last_period( strrchr( filename, '.' ) );
			if(last_period != NULL)
			{
				if( _strcmpi(last_period, ".ini") == 0 )
				{
					std::string		full_path;

					full_path = p_dir;
					full_path += filename;

					CControllerConfig *	p_config( BuildControllerConfig( full_path.c_str() ) );

					if( p_config != NULL )
					{
						mControllerConfigs.push_back( p_config );
					}
				}
			}
		}
		while(IO::FindFileNext( find_handle, find_data ));

		IO::FindFileClose( find_handle );
	}
}

//*****************************************************************************
//	Some utility classes for handling our expression evaluation
//*****************************************************************************

struct SButtonNameMapping
{
	const char * ButtonName;
	u32			 ButtonMask;
};

const SButtonNameMapping	gButtonNameMappings[] =
{
	{ "3DS.Start",     KEY_START },
	{ "3DS.Select",    KEY_SELECT },
	{ "3DS.A",         KEY_A },
	{ "3DS.B",         KEY_B },
	{ "3DS.X",         KEY_X },
	{ "3DS.Y",         KEY_Y },
	{ "3DS.LTrigger",  KEY_L },
	{ "3DS.RTrigger",  KEY_R },
	{ "3DS.ZLTrigger", KEY_ZL },
	{ "3DS.ZRTrigger", KEY_ZR },
	{ "3DS.Up",        KEY_DUP },
	{ "3DS.Down",      KEY_DDOWN },
	{ "3DS.Left",      KEY_DLEFT },
	{ "3DS.Right",     KEY_DRIGHT },
	{ "3DS.CUp",       KEY_CSTICK_UP },
	{ "3DS.CDown",     KEY_CSTICK_DOWN },
	{ "3DS.CLeft",     KEY_CSTICK_LEFT },
	{ "3DS.CRight",     KEY_CSTICK_RIGHT },
};

u32 GetOperatorPrecedence( char op )
{
	switch ( op )
	{
	case '!':		return 2;
	case '&':		return 1;
	case '|':		return 0;
	default:
	#ifdef DAEDALUS_DEBUG_CONSOLE
		DAEDALUS_ERROR( "Unhandled operator" );
		#endif
		return 0;
	}
}

bool IsOperatorChar( char c )
{
	return c == '&' || c == '|' || c == '!';
}

bool IsIdentifierChar( char c )
{
	return isalpha( c ) || isdigit( c ) || c == '.';
}

u32 GetMaskFromIdentifier( const char * identifier )
{
	for( u32 i = 0; i < ARRAYSIZE( gButtonNameMappings ); ++i )
	{
		if( strcmp( gButtonNameMappings[ i ].ButtonName, identifier ) == 0 )
		{
			return gButtonNameMappings[ i ].ButtonMask;
		}
	}

	// Error - unknown button name!
	// This is handled by the parser below
	return 0;
}

class CButtonMappingExpressionEvaluator
{
	public:
		CButtonMappingExpressionEvaluator();

		CButtonMapping *			Parse( const char * expression );
		u32							GetNumErrors() const				{ return mNumErrors; }
		const char *				GetErrorString() const				{ return mErrorStream.c_str(); }

	private:
		static	void				HandleOperator( char op, std::stack<CButtonMapping*> & operands );
				COutputStream &		ReportError();

	private:
		u32							mNumErrors;
		COutputStringStream			mErrorStream;

};

CButtonMappingExpressionEvaluator::CButtonMappingExpressionEvaluator()
:	mNumErrors( 0 )
{
}

COutputStream &	CButtonMappingExpressionEvaluator::ReportError()
{
	mNumErrors++;
	return mErrorStream;
}

void	CButtonMappingExpressionEvaluator::HandleOperator( char op, std::stack<CButtonMapping*> & operands )
{
	switch( op )
	{
	case '!':
		if( !operands.empty() )
		{
			CButtonMapping *	p_arg( operands.top() );
			operands.pop();
			operands.push(  new CButtonMappingNegate( p_arg ));
		}
		break;
	case '&':
		if( !operands.empty() )
		{
			CButtonMapping * p_arg_b( operands.top() );
			operands.pop();
			if( !operands.empty() )
			{
				CButtonMapping * p_arg_a( operands.top() );
				operands.pop();

				operands.push( new CButtonMappingAnd( p_arg_a, p_arg_b ) );
			}
		}
		break;
	case '|':
		if( !operands.empty() )
		{
			CButtonMapping * p_arg_b( operands.top() );
			operands.pop();
			if( !operands.empty() )
			{
				CButtonMapping * p_arg_a( operands.top() );
				operands.pop();

				operands.push( new CButtonMappingOr( p_arg_a, p_arg_b ) );
			}
		}
		break;
		#ifdef DAEDALUS_ENABLE_ASSERTS
	default:
		DAEDALUS_ERROR( "Unhandled operator" );
		break;
		#endif
	}
}

CButtonMapping *	CButtonMappingExpressionEvaluator::Parse( const char * expression )
{
	mNumErrors = 0;
	mErrorStream.Clear();

	std::stack<char>					operator_stack;
	std::stack<CButtonMapping *>		operand_stack;

	const char * p_cur( expression );
	const char * p_end( expression + strlen( expression ) );
	for( ; p_cur < p_end; ++p_cur )
	{
		char c( *p_cur );

		if( isspace( c ) )
		{
			// Ignore
		}
		else if( c == '(' )
		{
			// Start of a set of parenthesis - push onto the token stack
			operator_stack.push( c );
		}
		else if( c == ')' )
		{
			// We've reached a closing parenthesis - pop operands from the stack
			bool found_opening = false;
			while( !found_opening )
			{
				if( operator_stack.empty() )
				{
					break;
				}

				char	op( operator_stack.top() );
				operator_stack.pop();

				// This must be an operand. Pop the arguments from the mapping stack and construct outputs
				switch( op )
				{
				case '(':
					// We've matched the opening parenthesis - can discard it and continue parsing
					found_opening = true;
					break;
				case '!':
				case '&':
				case '|':
					HandleOperator( op, operand_stack );
					break;
				default:
					ReportError() << "Unknown operator '" << op << "'\n";
					break;
				}
			}

			if( !found_opening )
			{
				ReportError() << "Couldn't match closing parenthesis at char " << u32(p_cur - expression) << ", '..." << p_cur << "'\n";
			}
		}
		else if( IsOperatorChar( c ) )
		{
			// Currently we only support single-character operators

			if( !operator_stack.empty() )
			{
				char	prevOp( operator_stack.top() );
				u32		newPrecedence( GetOperatorPrecedence( c ) );
				u32		prevPrecedence( GetOperatorPrecedence( prevOp ) );

				// If the new precedence is less than that of the previous operator then
				// consume the previous operator before continuing
				if( newPrecedence <= prevPrecedence )
				{
					operator_stack.pop();
					HandleOperator( prevOp, operand_stack );
				}
			}

			operator_stack.push( c );
		}
		else if( IsIdentifierChar( c ) )
		{
			const char * p_identifier_start( p_cur );
			const char * p;

			// Grab all the identifier chars we can
			for( p = p_cur; IsIdentifierChar( *p ); ++p )
			{
			}

			const char * p_identifier_end( p );
			p_cur = p - 1;

			std::string		identifier( p_identifier_start, p_identifier_end );

			u32			mask( GetMaskFromIdentifier( identifier.c_str() ) );

			if( mask == 0 )
			{
				ReportError() << "Unknown control '" << identifier.c_str() << "'\n";
			}

			operand_stack.push( new CButtonMappingMask( mask ) );
		}
	}

	//
	//	At this point we have evaluated the expression string, but might still have
	//	unhandled operators lying on the expression stack. Spin through it
	//	consuming operators (and presumably operands)
	//
	while( !operator_stack.empty() )
	{
		char	op( operator_stack.top() );
		operator_stack.pop();

		HandleOperator( op, operand_stack );
	}

	CButtonMapping * p_mapping( NULL );
	if( operand_stack.empty() )
	{
		ReportError() << "Term did not evaluate to an expression\n";
	}
	else
	{
		p_mapping = operand_stack.top();
		operand_stack.pop();

		if( !operand_stack.empty() )
		{
			ReportError() << "More than one expression remaining\n";
		}
	}

	//if( mNumErrors > 0 )
	//{
	//	printf( "%s", mErrorStream.c_str() );
	//}

	return p_mapping;
}

//*****************************************************************************
//	Build a default controller configuration
//	We do this in case the user deletes all the config files (or they're all
//	corrupt) so that we can ensure there's at least one valid config.
//*****************************************************************************
CControllerConfig *	IInputManager::BuildDefaultConfig(bool zSwap)
{
	CControllerConfig *	p_config( new CControllerConfig );

	p_config->SetName( zSwap ? "Default (Z-L Swapped)" : "Default" );

	CButtonMappingExpressionEvaluator	eval;

	p_config->SetButtonMapping( N64Button_Start, eval.Parse( "3DS.Start" ) );

	p_config->SetButtonMapping( N64Button_A, eval.Parse( "3DS.A" ) );
	p_config->SetButtonMapping( N64Button_B, eval.Parse( "3DS.B" ) );
	p_config->SetButtonMapping( N64Button_L, eval.Parse( zSwap ? "3DS.X" : "3DS.LTrigger" ) );
	p_config->SetButtonMapping( N64Button_R, eval.Parse( "3DS.RTrigger" ) );

	if(isN3DS)
	{
		p_config->SetDescription( "The default Daedalus controller configuration." );

		p_config->SetButtonMapping( N64Button_Z, eval.Parse( "3DS.ZLTrigger | 3DS.ZRTrigger" ) );

		p_config->SetButtonMapping( N64Button_Up,    eval.Parse( "3DS.Up" ) );
		p_config->SetButtonMapping( N64Button_Down,  eval.Parse( "3DS.Down" ) );
		p_config->SetButtonMapping( N64Button_Left,  eval.Parse( "3DS.Left" ) );
		p_config->SetButtonMapping( N64Button_Right, eval.Parse( "3DS.Right" ) );

		p_config->SetButtonMapping( N64Button_CUp,    eval.Parse( "3DS.CUp" ) );
		p_config->SetButtonMapping( N64Button_CDown,  eval.Parse( "3DS.CDown" ) );
		p_config->SetButtonMapping( N64Button_CLeft,  eval.Parse( "3DS.CLeft" ) );
		p_config->SetButtonMapping( N64Button_CRight, eval.Parse( "3DS.CRight" ) );
	}
	else
	{
		p_config->SetDescription(	"The default Daedalus controller configuration.\n" 
								 	"N64 C-Buttons are mapped to 3DS D-Pad\n"
								 	"N64 D-Pad is mapped to 3DS D-Pad + Y");

		p_config->SetButtonMapping( N64Button_Z, eval.Parse( zSwap ? "3DS.LTrigger" : "3DS.X") );

		p_config->SetButtonMapping( N64Button_Up,    eval.Parse( "3DS.Y & 3DS.Up" ) );
		p_config->SetButtonMapping( N64Button_Down,  eval.Parse( "3DS.Y & 3DS.Down" ) );
		p_config->SetButtonMapping( N64Button_Left,  eval.Parse( "3DS.Y & 3DS.Left" ) );
		p_config->SetButtonMapping( N64Button_Right, eval.Parse( "3DS.Y & 3DS.Right" ) );

		p_config->SetButtonMapping( N64Button_CUp,    eval.Parse( "!3DS.Y & 3DS.Up" ) );
		p_config->SetButtonMapping( N64Button_CDown,  eval.Parse( "!3DS.Y & 3DS.Down" ) );
		p_config->SetButtonMapping( N64Button_CLeft,  eval.Parse( "!3DS.Y & 3DS.Left" ) );
		p_config->SetButtonMapping( N64Button_CRight, eval.Parse( "!3DS.Y & 3DS.Right" ) );
	}

	return p_config;
}


CControllerConfig *	IInputManager::BuildControllerConfig( const char * filename )
{
	CIniFile * p_ini_file( CIniFile::Create( filename ) );
	if( p_ini_file == NULL )
	{
		return NULL;
	}

	const CIniFileProperty * p_property;
	CControllerConfig *		p_config( new CControllerConfig );

	//
	//	Firstly parse the default section
	//
	const CIniFileSection * p_default_section( p_ini_file->GetDefaultSection() );

	if( p_default_section->FindProperty( "Name", &p_property ) )
	{
		p_config->SetName( p_property->GetValue() );
	}
	if( p_default_section->FindProperty( "Description", &p_property ) )
	{
		p_config->SetDescription( p_property->GetValue() );
	}

	//
	//	Now parse all the buttons
	//
	const CIniFileSection *		p_button_section( p_ini_file->GetSectionByName( "Buttons" ) );

	if( p_button_section != NULL )
	{
		CButtonMappingExpressionEvaluator	eval;

		for( u32 i = 0; i < NUM_N64_BUTTONS; ++i )
		{
			EN64Button		button = EN64Button( i );
			const char *	button_name( GetN64ButtonName( button ) );

			if( p_button_section->FindProperty( button_name, &p_property ) )
			{
				p_config->SetButtonMapping( button, eval.Parse( p_property->GetValue() ) );
			}
			else
			{
				//printf( "There was no property for %s\n", button_name );
			}
		}
	}
	else
	{
		//printf( "Couldn't find buttons section\n" );
	}

	delete p_ini_file;

	return p_config;
}
