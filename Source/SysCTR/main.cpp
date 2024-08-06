#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <iostream> 
#include <fstream> 

#include <3ds.h>
#include <GL/picaGL.h>


#include "Interface/ConfigOptions.h"
#include "Interface/Cheats.h"
#include "Core/CPU.h"
#include "Core/CPU.h"
#include "Core/Memory.h"
#include "Core/PIF.h"
#include "RomFile/RomSettings.h"
#include "Core/Save.h"
#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "Graphics/GraphicsContext.h"
#include "HLEGraphics/TextureCache.h"
#include "Input/InputManager.h"
#include "Interface/RomDB.h"
#include "System/SystemInit.h"
#include "Utility/BatchTest.h"

#include "UI/UserInterface.h"
#include "UI/RomSelector.h"


#include "Interface/Preferences.h"
#include "Utility/Profiler.h"
#include "System/Thread.h"


#include "Utility/Timer.h"
#include "RomFile/RomFile.h"
#include "Utility/MemoryCTR.h"

bool isN3DS = false;
bool shouldQuit = false;
    std::streambuf* coutBuf = nullptr;
    std::streambuf* cerrBuf = nullptr;
EAudioPluginMode enable_audio = APM_ENABLED_ASYNC;

#ifdef DAEDALUS_LOG
// void log2file(const char *format, ...) {
// 	__gnuc_va_list arg;
// 	int done;
// 	va_start(arg, format);
// 	char msg[512];
// 	done = vsprintf(msg, format, arg);
// 	va_end(arg);
// 	snprintf(msg, sizeof(msg),  "%s\n", msg);
// 	std::ofstream log("sdmc:/DaedalusX64.log", std::ios::out);
// 	if (log.is_open()) {
// 		log.write(reinterpret_cast<char*>(msg), strlen(msg));
// 		log.close();
// 	}
// }
void redirectOutputToLogFile(std::ofstream& logFile, std::streambuf*& coutBuf, std::streambuf*& cerrBuf) {
    // Open the log file
    logFile.open("sdmc:/3ds/DaedalusX64/daedalus.log");

    // Check if the file opened successfully
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file." << std::endl;
        return;
    }

    // Redirect stdout and stderr to the log file
    coutBuf = std::cout.rdbuf();
    cerrBuf = std::cerr.rdbuf();
    std::cout.rdbuf(logFile.rdbuf());
    std::cerr.rdbuf(logFile.rdbuf());
}

#endif

static void CheckDSPFirmware()
{	
	std::filesystem::path firmwarePath = "sdmc:/3ds/dspfirm.cdc";
	std::ifstream firmware(firmwarePath, std::ios::binary |  std::ios::in);

	if(firmware.is_open())
	{
		firmware.close();
		return;
	}
	else
	{
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);

	std::cout << "DSP Firmware not found " << std::endl;
	std::cout << "Press START to exit" << std::endl;
	while(aptMainLoop())
	{
		hidScanInput();

		if(hidKeysDown() == KEY_START)
			exit(1);
	}
	}

}	

static void Initialize()
{
	    // Variables to store the original stream buffers




	CheckDSPFirmware();
	
	_InitializeSvcHack();

	romfsInit();
	
	APT_CheckNew3DS(&isN3DS);
	osSetSpeedupEnable(true);
	
	gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, true);

	if(isN3DS)
		gfxSetWide(true);
	
	pglInitEx(0x080000, 0x040000);

	UI::Initialize();

	System_Init();
}


void HandleEndOfFrame()
{
	shouldQuit = !aptMainLoop();
	
	if (shouldQuit)
	{
		CPU_Halt("Exiting");
	}
}

extern u32 __ctru_heap_size;

int main(int argc, char* argv[])
{
	    // File stream for logging
    std::ofstream logFile;

    // Redirect output to log file
    redirectOutputToLogFile(logFile, coutBuf, cerrBuf);
	char fullpath[512];

	Initialize();
		std::cout << "Hello this is being logged" << std::endl;
	while(shouldQuit == false)
	{
	// Set the default path

	std::string rom = UI::DrawRomSelector();
	std::filesystem::path RomPath = setBasePath("Roms");
	// std::filesystem::path rom = "Super Mario 64 (USA).z64";
	RomPath /= rom;
	System_Open(RomPath.string().c_str());
	CPU_Run();
	System_Close();
	}
	
	System_Finalize();
	pglExit();

	return 0;
}
