#
#	Specify :
#		DEBUG=y				# for a debug build
#		PSPGPROF=y			# for profiling with psp gprof
#
#		CONFIG=<configname>		# Build using Source/Configs/<configname> as config
#
#		Default config for DEBUG build is Dev
#		Default config for non DEBUG build is Release
#

TARGET = Daedalus

CORE_SRCS = \
	Source/ConfigOptions.cpp \
	Source/Core/Cheats.cpp \
	Source/Core/CPU.cpp \
	Source/Core/DMA.cpp \
	Source/Core/Dynamo.cpp \
	Source/Core/FlashMem.cpp \
	Source/Core/Interpret.cpp \
	Source/Core/Interrupts.cpp \
	Source/Core/JpegTask.cpp \
	Source/Core/Memory.cpp \
	Source/Core/PIF.cpp \
	Source/Core/R4300.cpp \
	Source/Core/Registers.cpp \
	Source/Core/ROM.cpp \
	Source/Core/ROMBuffer.cpp \
	Source/Core/ROMImage.cpp \
	Source/Core/RomSettings.cpp \
	Source/Core/RSP.cpp \
	Source/Core/RSP_HLE.cpp \
	Source/Core/Save.cpp \
	Source/Core/Savestate.cpp \
	Source/Core/TLB.cpp \
	Source/Debug/DebugLog.cpp \
	Source/Debug/Dump.cpp \
	Source/DynaRec/BranchType.cpp \
	Source/DynaRec/DynaRecProfile.cpp \
	Source/DynaRec/Fragment.cpp \
	Source/DynaRec/FragmentCache.cpp \
	Source/DynaRec/IndirectExitMap.cpp \
	Source/DynaRec/StaticAnalysis.cpp \
	Source/DynaRec/TraceRecorder.cpp \
	Source/Graphics/ColourValue.cpp \
	Source/Graphics/TextureTransform.cpp \
	Source/HLEAudio/ABI1.cpp \
	Source/HLEAudio/ABI2.cpp \
	Source/HLEAudio/ABI3.cpp \
	Source/HLEAudio/ABI3mp3.cpp \
	Source/HLEAudio/AudioBuffer.cpp \
	Source/HLEAudio/AudioHLEProcessor.cpp \
	Source/HLEAudio/HLEMain.cpp \
	Source/HLEGraphics/BaseRenderer.cpp \
	Source/HLEGraphics/CachedTexture.cpp \
	Source/HLEGraphics/ConvertImage.cpp \
	Source/HLEGraphics/DLDebug.cpp \
	Source/HLEGraphics/DLParser.cpp \
	Source/HLEGraphics/Microcode.cpp \
	Source/HLEGraphics/RDP.cpp \
	Source/HLEGraphics/RDPStateManager.cpp \
	Source/HLEGraphics/TextureCache.cpp \
	Source/HLEGraphics/TextureInfo.cpp \
	Source/HLEGraphics/uCodes/Ucode.cpp \
	Source/Interface/RomDB.cpp \
	Source/Math/Matrix4x4.cpp \
	Source/OSHLE/OS.cpp \
	Source/OSHLE/patch.cpp \
	Source/Plugins/GraphicsPlugin.cpp \
	Source/SysPSP/Debug/DaedalusAssertPSP.cpp \
	Source/SysPSP/Debug/DBGConsolePSP.cpp \
	Source/SysPSP/DveMgr/pspDveManager.S \
	Source/SysPSP/DynaRec/AssemblyUtilsPSP.cpp \
	Source/SysPSP/DynaRec/AssemblyWriterPSP.cpp \
	Source/SysPSP/DynaRec/CodeBufferManagerPSP.cpp \
	Source/SysPSP/DynaRec/CodeGeneratorPSP.cpp \
	Source/SysPSP/DynaRec/DynaRecStubs.S \
	Source/SysPSP/DynaRec/DynarecTargetPSP.cpp \
	Source/SysPSP/DynaRec/N64RegisterCachePSP.cpp \
	Source/SysPSP/Graphics/DrawText.cpp \
	Source/SysPSP/Graphics/GraphicsContext.cpp \
	Source/SysPSP/Graphics/intraFont/intraFont.c \
	Source/SysPSP/Graphics/intraFont/libccc.c \
	Source/SysPSP/Graphics/NativeTexturePSP.cpp \
	Source/SysPSP/Graphics/PngUtilPSP.cpp \
	Source/SysPSP/Graphics/VideoMemoryManager.cpp \
	Source/SysPSP/HLEAudio/AudioOutput.cpp \
	Source/SysPSP/HLEAudio/AudioPluginPSP.cpp \
	Source/SysPSP/HLEGraphics/Blender.cpp \
	Source/SysPSP/HLEGraphics/BlendModes.cpp \
	Source/SysPSP/HLEGraphics/ColourAdjuster.cpp \
	Source/SysPSP/HLEGraphics/Combiner/BlendConstant.cpp \
	Source/SysPSP/HLEGraphics/Combiner/CombinerExpression.cpp \
	Source/SysPSP/HLEGraphics/Combiner/CombinerTree.cpp \
	Source/SysPSP/HLEGraphics/Combiner/RenderSettings.cpp \
	Source/SysPSP/HLEGraphics/ConvertVertices.S \
	Source/SysPSP/HLEGraphics/DisplayListDebugger.cpp \
	Source/SysPSP/HLEGraphics/RendererPSP.cpp \
	Source/SysPSP/HLEGraphics/TnLVFPU.S\
	Source/SysPSP/HLEGraphics/VectorClipping.S \
	Source/SysPSP/Input/InputManagerPSP.cpp \
	Source/SysPSP/KernelButtonsPrx/imposectrl.S \
	Source/SysPSP/main.cpp \
	Source/SysPSP/MediaEnginePRX/me.c \
	Source/SysPSP/MediaEnginePRX/MediaEngine.S \
	Source/SysPSP/Plugins/GraphicsPluginPSP.cpp \
	Source/SysPSP/UI/AboutComponent.cpp \
	Source/SysPSP/UI/AdjustDeadzoneScreen.cpp \
	Source/SysPSP/UI/AdvancedOptionsScreen.cpp \
	Source/SysPSP/UI/CheatOptionsScreen.cpp \
	Source/SysPSP/UI/ColourPulser.cpp \
	Source/SysPSP/UI/Dialogs.cpp \
	Source/SysPSP/UI/GlobalSettingsComponent.cpp \
	Source/SysPSP/UI/MainMenuScreen.cpp \
	Source/SysPSP/UI/PauseOptionsComponent.cpp \
	Source/SysPSP/UI/PauseScreen.cpp \
	Source/SysPSP/UI/RomPreferencesScreen.cpp \
	Source/SysPSP/UI/RomSelectorComponent.cpp \
	Source/SysPSP/UI/SavestateSelectorComponent.cpp \
	Source/SysPSP/UI/SelectedRomComponent.cpp \
	Source/SysPSP/UI/SplashScreen.cpp \
	Source/SysPSP/UI/UICommand.cpp \
	Source/SysPSP/UI/UIComponent.cpp \
	Source/SysPSP/UI/UIContext.cpp \
	Source/SysPSP/UI/UIElement.cpp \
	Source/SysPSP/UI/UIScreen.cpp \
	Source/SysPSP/UI/UISetting.cpp \
	Source/SysPSP/Utility/AtomicPrimitives.S \
	Source/SysPSP/Utility/BatteryPSP.cpp \
	Source/SysPSP/Utility/Buttons.cpp \
	Source/SysPSP/Utility/DebugMemory.cpp \
	Source/SysPSP/Utility/DisableFPUExceptions.S \
	Source/SysPSP/Utility/exception.cpp \
	Source/SysPSP/Utility/FastMemcpy.cpp \
	Source/SysPSP/Utility/IOPSP.cpp \
	Source/SysPSP/Utility/JobManager.cpp \
	Source/SysPSP/Utility/ModulePSP.cpp \
	Source/SysPSP/Utility/ThreadPSP.cpp \
	Source/SysPSP/Utility/TimingPSP.cpp \
	Source/SysPSP/Utility/VolatileMemPSP.cpp \
	Source/System.cpp \
	Source/Test/BatchTest.cpp \
	Source/Utility/CRC.cpp \
	Source/Utility/FastMemcpy.cpp \
	Source/Utility/FramerateLimiter.cpp \
	Source/Utility/Hash.cpp \
	Source/Utility/IniFile.cpp \
	Source/Utility/MemoryHeap.cpp \
	Source/Utility/Preferences.cpp \
	Source/Utility/PrintOpCode.cpp \
	Source/Utility/Profiler.cpp \
	Source/Utility/ROMFile.cpp \
	Source/Utility/ROMFileCache.cpp \
	Source/Utility/ROMFileCompressed.cpp \
	Source/Utility/ROMFileMemory.cpp \
	Source/Utility/ROMFileUncompressed.cpp \
	Source/Utility/Stream.cpp \
	Source/Utility/Timer.cpp \
	Source/Utility/Translate.cpp \
	Source/Utility/ZLibWrapper.cpp

DAED_GPROF_SRCS = \
	Source/SysPSP/Debug/prof.c \
	Source/SysPSP/Debug/mcount.S

ADDITIONAL_SYNC_SRCS = \
	Source/Utility/Synchroniser.cpp \
	Source/Utility/ZLibWrapper.cpp	# Already specified in CORE_SRCS - remove


ifdef PSPGPROF
	CONFIG=Profile

	CFLAGS			= -pg -g -O2 -G0 -D_DEBUG -Wall -MD -ffast-math -fsingle-precision-constant

	SRCS			= $(CORE_SRCS) $(DAED_GPROF_SRCS)
else
ifdef DEBUG
	CONFIG=Dev #default config in Debug build is "Dev"

	CFLAGS			= -g -O1 -fno-omit-frame-pointer -G0 -D_DEBUG -MD \
				  -W -Wcast-qual -Wchar-subscripts -Wno-unused -Wpointer-arith\
				  -Wredundant-decls -Wshadow -Wwrite-strings
				#-Winline -Wcast-align

	SRCS			= $(CORE_SRCS) $(ADDITIONAL_SYNC_SRCS)
else
	CFLAGS			= -O2 -G0 -DNDEBUG -Wall -MD -ffast-math -fsingle-precision-constant -fpredictive-commoning -mno-check-zero-division -freorder-blocks-and-partition
					#-pipe
					#-Wextra
					#-fno-builtin
					#-fgcse-after-reload
					#-funroll-loops
	LDFLAGS 		= "-Wl,-O1"

	SRCS			= $(CORE_SRCS)
endif
endif

ifndef CONFIG
	CONFIG=Release
endif

CXXFLAGS = -fno-exceptions -fno-rtti -iquote./Source/SysPSP/Include -iquote./Source/Config/$(CONFIG) -iquote./Source -iquote./Source/SysPSP

OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.S=.o)

DEP_FILES := $(SRCS:.cpp=.d)
DEP_FILES := $(DEP_FILES:.c=.d)
DEP_FILES := $(DEP_FILES:.S=.d)

ASFLAGS =

INCDIR = $(PSPDEV)/SDK/include ./SDK/include
LIBDIR = $(PSPDEV)/SDK/lib ./SDK/lib


LIBS = -lstdc++ -lpsppower -lpspgu -lpspaudio -lpsprtc -lc -lpng -lz -lg -lm -lpspfpu -lpspkubridge

EXTRA_TARGETS = EBOOT.PBP dvemgr.prx exception.prx mediaengine.prx imposectrl.prx

PSP_EBOOT_TITLE = DaedalusX64 Beta 3 Update
PSP_EBOOT_ICON  = icon0.png
PSP_EBOOT_PIC1  = pic1.png
#PSP_EBOOT_ICON1 = ICON1.PMF
#PSP_EBOOT_UNKPNG = PIC0.PNG
#PSP_EBOOT_SND0 = SND0.AT3
#PSP_EBOOT_PSAR =

PSPSDK=$(shell psp-config --pspsdk-path)
#USE_PSPSDK_LIBC=1
PSP_FW_VERSION=500
BUILD_PRX = 1
PSP_LARGE_MEMORY = 1

EXTRA_CLEAN=$(DEP_FILES)


DATA_DIR = ../data
BUILDS_DIR = ./Builds
BUILDS_PSP_DIR = $(BUILDS_DIR)/PSP
BUILDS_GAME_DIR = $(BUILDS_PSP_DIR)/GAME
BUILDS_DX_DIR = $(BUILDS_GAME_DIR)/DaedalusX64


#RESULT := $(shell LC_ALL=C svn info | grep Revision | grep -e [0-9]* -o | tr -d '\n') #does not work with Windows...
VERSION = $(shell svnversion -n 2> Makefile.cache)

#If the .svn directory wasn't found, perhaps we're using git-svn?
ifeq ($(VERSION),Unversioned directory)
	VERSION = $(shell git svn info . | sed -ne 's/^Revision: //p')
endif

ifeq ($(VERSION),)
	#Windows
	EXTRA_TARGETS := svn $(EXTRA_TARGETS)
	#include $(PSPSDK)/lib/build.mak
	# Fix for MinPSPW's D support which conflicts with -MD (dependency build) flag..
	include SDK/lib/build.mak
svn:
	@echo svnversion not found, trying SubWCRev
	@-SubWCRev . ./Source/svnversion.txt ./Source/svnversion.h
else
	#Unix
	CFLAGS += -DSVNVERSION=\"$(VERSION)\"
	PSP_EBOOT_TITLE += $(VERSION)

	include $(PSPSDK)/lib/build.mak
endif


psplink: $(PSP_EBOOT) $(TARGET).elf
	prxtool -y $(TARGET).elf > $(BUILDS_DX_DIR)/$(TARGET).sym

#Need to create dirs one by one because windows can't use mkdir -p
install: $(PSP_EBOOT) $(TARGET).prx dvemgr.prx exception.prx mediaengine.prx kernelbuttons.prx $(TARGET).elf
	-mkdir "$(BUILDS_DIR)"
	-mkdir "$(BUILDS_PSP_DIR)"
	-mkdir "$(BUILDS_GAME_DIR)"
	-mkdir "$(BUILDS_DX_DIR)"
	svn export --force "$(DATA_DIR)" "$(BUILDS_DX_DIR)"
	cp $(PSP_EBOOT) "$(BUILDS_DX_DIR)"
	cp *.prx "$(BUILDS_DX_DIR)"

#this rule should only work with Unix environments such as GNU/Linux, Mac OS, Cygwin, etc...
#be carefull of probably already compiled files with old rev number
TMP_BASE_DIR=tmp_build
TMP_DX_DIR:=$(TMP_BASE_DIR)/PSP/GAME/DaedalusX64_$(VERSION)/
zip: $(PSP_EBOOT) dvemgr.prx mediaengine.prx exception.prx kernelbuttons.prx
	-mkdir tarballs 2> /dev/null
	-mkdir -p $(TMP_DX_DIR) 2> /dev/null
	cp $^ $(TMP_DX_DIR)
	-mkdir $(TMP_DX_DIR)/Resources 2> /dev/null
	cp $(DATA_DIR)/Resources/logo.png $(TMP_DX_DIR)/Resources
	-mkdir $(TMP_DX_DIR)/Roms 2> /dev/null
	svn export $(DATA_DIR)/SaveGames $(TMP_DX_DIR)/SaveGames
	svn export $(DATA_DIR)/ControllerConfigs $(TMP_DX_DIR)/ControllerConfigs
	-mkdir $(TMP_DX_DIR)/SaveStates 2> /dev/null
	cp $(DATA_DIR)/changes.txt \
		$(DATA_DIR)/copying.txt \
		$(DATA_DIR)/readme.txt \
		$(DATA_DIR)/roms.ini $(TMP_DX_DIR)
	REV=$$(LC_ALL=C svn info | grep Revision | grep -e [0-9]* -o | tr -d '\n') && \
	    cd tmp_build && zip -r ../tarballs/"DaedalusX64_$$REV.zip" PSP
	rm -r $(TMP_BASE_DIR) 2>/dev/null

Source/SysPSP/MediaEnginePRX/MediaEngine.S:
	$(MAKE) -C Source/SysPSP/MediaEnginePRX all

Source/SysPSP/DveMgr/pspDveManager.S:
	$(MAKE) -C Source/SysPSP/DveMgr all

dvemgr.prx:
	$(MAKE) -C Source/SysPSP/DveMgr all

mediaengine.prx:
	$(MAKE) -C Source/SysPSP/MediaEnginePRX all

exception.prx:
	$(MAKE) -C Source/SysPSP/ExceptionHandler/prx all

Source/SysPSP/KernelButtonsPrx/imposectrl.o:
	$(MAKE) -C Source/SysPSP/KernelButtonsPrx imposectrl.o

imposectrl.prx:
	$(MAKE) -C Source/SysPSP/KernelButtonsPrx all

allclean: clean
	$(MAKE) -C Source/SysPSP/ExceptionHandler/prx clean
	$(MAKE) -C Source/SysPSP/MediaEnginePRX clean
	$(MAKE) -C Source/SysPSP/DveMgr clean
	$(MAKE) -C Source/SysPSP/KernelButtonsPrx clean

-include $(DEP_FILES)

