
#include "Base/Types.h"
#include "HLEAudio/AudioPlugin.h"
#include "Interface/ConfigOptions.h"

EAudioPluginMode gAudioPluginEnabled = APM_DISABLED;

std::unique_ptr<CAudioPlugin> CreateAudioPlugin()
{
	return NULL;
}
