#ifndef INPUT_INPUTMANAGER_H_
#define INPUT_INPUTMANAGER_H_

#include "OSHLE/ultra_os.h"
#include "Utility/Singleton.h"

#include "Math/Vector2.h"

class CInputManager : public CSingleton< CInputManager >
{
	public:
		virtual ~CInputManager() {}

#ifdef DAEDALUS_PSP
		virtual u32				GetNumConfigurations() const {};
		virtual const char *	GetConfigurationName( u32 configuration_idx ) const {};
		virtual const char *	GetConfigurationDescription( u32 configuration_idx ) const {};
		virtual void			SetConfiguration( u32 configuration_idx ) {};

		virtual u32				GetConfigurationFromName( const char * name ) const {};
#endif
		virtual bool Initialise() {};
		virtual void Finalise() {};

		virtual void GetState( OSContPad pPad[4] ) {};

		static bool Init() { return CInputManager::Get()->Initialise();}
		static void Fini() { CInputManager::Get()->Finalise();}
};

#ifdef DAEDALUS_PSP
v2	ApplyDeadzone( const v2 & in, f32 min_deadzone, f32 max_deadzone );
#endif

#endif // INPUT_INPUTMANAGER_H_
