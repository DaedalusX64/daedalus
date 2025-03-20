#ifndef INPUT_INPUTMANAGER_H_
#define INPUT_INPUTMANAGER_H_

#include "Ultra/ultra_os.h"
#include "Base/Singleton.h"

#include <glm/glm.hpp>

class CInputManager : public CSingleton< CInputManager >
{
	public:
		virtual ~CInputManager() {}

		virtual u32				GetNumConfigurations() const = 0;
		virtual const char *	GetConfigurationName( u32 configuration_idx ) const = 0;
		virtual const char *	GetConfigurationDescription( u32 configuration_idx ) const = 0;
		virtual void			SetConfiguration( u32 configuration_idx ) = 0;

		virtual u32				GetConfigurationFromName( const char * name ) const = 0;

		virtual bool Initialise() = 0;
		virtual void Finalise() = 0;

		virtual void GetState( OSContPad pPad[4] ) = 0;

		static bool Init() { return CInputManager::Get()->Initialise();}
		static void Fini() { CInputManager::Get()->Finalise();}
};

glm::vec2	ApplyDeadzone( const glm::vec2 & in, f32 min_deadzone, f32 max_deadzone );

#endif // INPUT_INPUTMANAGER_H_
