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


DAED_MAIN_SRCS =	Source/ConfigOptions.o \
			Source/SysPSP/main.o \
			Source/Test/BatchTest.o

DAED_DEBUG_SRCS =	Source/SysPSP/Debug/DBGConsolePSP.o \
			Source/Debug/DebugLog.o \
			Source/Debug/Dump.o

DAED_CORE_SRCS =	Source/System.o \
			Source/Core/CPU.o \
			Source/Core/DMA.o \
			Source/Core/Interrupts.o \
			Source/Core/Memory.o \
			Source/Core/PIF.o \
			Source/Core/R4300.o \
			Source/Core/Registers.o \
			Source/Core/ROM.o \
			Source/Core/RomSettings.o \
			Source/Core/ROMBuffer.o \
			Source/Core/ROMImage.o \
			Source/Core/RSP.o \
			Source/Core/RSP_HLE.o \
			Source/Core/Savestate.o \
			Source/Core/TLB.o \
			Source/Core/Dynamo.o \
			Source/Core/Interpret.o \
			Source/Core/Save.o \
			Source/Core/JpegTask.o \
			Source/Core/FlashMem.o

DAED_INTERFACE_SRCS =	Source/Interface/RomDB.o

DAED_INPUT_SRCS =	Source/SysPSP/Input/InputManagerPSP.o

DAED_DYNREC_SRCS =  	Source/SysPSP/DynaRec/AssemblyUtilsPSP.o \
			Source/SysPSP/DynaRec/AssemblyWriterPSP.o \
			Source/SysPSP/DynaRec/CodeBufferManagerPSP.o \
			Source/SysPSP/DynaRec/CodeGeneratorPSP.o \
			Source/SysPSP/DynaRec/DynarecTargetPSP.o \
			Source/SysPSP/DynaRec/N64RegisterCachePSP.o \
			Source/SysPSP/DynaRec/DynaRecStubs.o \
			Source/DynaRec/BranchType.o \
			Source/DynaRec/DynaRecProfile.o \
			Source/DynaRec/Fragment.o \
			Source/DynaRec/FragmentCache.o \
			Source/DynaRec/IndirectExitMap.o \
			Source/DynaRec/StaticAnalysis.o \
			Source/DynaRec/TraceRecorder.o

DAED_UTILITY_SRCS =	Source/Utility/CRC.o \
			Source/Utility/FramerateLimiter.o \
			Source/Utility/IniFile.o \
			Source/Utility/Hash.o \
			Source/Utility/MemoryHeap.o \
			Source/Utility/Preferences.o \
			Source/Utility/Profiler.o \
			Source/Utility/ROMFile.o \
			Source/Utility/ROMFileCache.o \
			Source/Utility/ROMFileCompressed.o \
			Source/Utility/ROMFileUncompressed.o \
			Source/Utility/Stream.o \
			Source/Utility/Timer.o \
			Source/Utility/unzip.o \
			Source/Utility/ZLibWrapper.o \
			Source/Utility/PrintOpCode.o \
			Source/Math/Matrix4x4.o

DAED_PSP_SRCS =		Source/SysPSP/Graphics/DrawText.o \
			Source/SysPSP/Graphics/GraphicsContext.o \
			Source/SysPSP/Graphics/NativeTexturePSP.o \
			Source/SysPSP/Graphics/VideoMemoryManager.o \
			Source/SysPSP/Graphics/PngUtilPSP.o \
			Source/SysPSP/Graphics/intraFont/intraFont.c \
			Source/SysPSP/Graphics/intraFont/libccc.c \
			Source/SysPSP/Debug/DaedalusAssertPSP.o \
			Source/Graphics/ColourValue.o \
			Source/SysPSP/UI/UIContext.o \
			Source/SysPSP/UI/UIElement.o \
			Source/SysPSP/UI/UICommand.o \
			Source/SysPSP/UI/UIComponent.o \
			Source/SysPSP/UI/UISetting.o \
			Source/SysPSP/UI/UIScreen.o \
			Source/SysPSP/UI/AboutComponent.o \
			Source/SysPSP/UI/ColourPulser.o \
			Source/SysPSP/UI/GlobalSettingsComponent.o \
			Source/SysPSP/UI/RomPreferencesScreen.o \
			Source/SysPSP/UI/AdvancedOptionsScreen.o \
			Source/SysPSP/UI/SavestateSelectorComponent.o \
			Source/SysPSP/UI/PauseOptionsComponent.o \
			Source/SysPSP/UI/SelectedRomComponent.o \
			Source/SysPSP/UI/AdjustDeadzoneScreen.o \
			Source/SysPSP/UI/MainMenuScreen.o \
			Source/SysPSP/UI/PauseScreen.o \
			Source/SysPSP/UI/SplashScreen.o \
			Source/SysPSP/Utility/AtomicPrimitives.o \
			Source/SysPSP/Utility/BatteryPSP.o \
			Source/SysPSP/Utility/JobManager.o \
			Source/SysPSP/Utility/DebugMemory.o \
			Source/SysPSP/Utility/DisableFPUExceptions.o \
			Source/SysPSP/Utility/IOPSP.o \
			Source/SysPSP/Utility/ThreadPSP.o \
			Source/SysPSP/Utility/TimingPSP.o \
			Source/SysPSP/Utility/FastMemcpy.o \
			Source/SysPSP/MediaEnginePRX/MediaEngine.o \
			Source/SysPSP/MediaEnginePRX/me.c \
			Source/SysPSP/DveMgr/pspDveManager.o \
			Source/SysPSP/KernelButtonsPrx/imposectrl.o \
			Source/SysPSP/Utility/Buttons.o \
			Source/SysPSP/Utility/exception.o

DAED_HLEGFX_SRCS =	Source/SysPSP/Plugins/GraphicsPluginPSP.o \
			Source/Plugins/GraphicsPlugin.o \
			Source/HLEGraphics/Blender.o \
			Source/HLEGraphics/BlendModes.o \
			Source/HLEGraphics/ColourAdjuster.o \
			Source/HLEGraphics/ConvertImage.o \
			Source/HLEGraphics/DisplayListDebugger.o \
			Source/HLEGraphics/DLParser.o \
			Source/HLEGraphics/Microcode.o \
			Source/HLEGraphics/RDP.o \
			Source/HLEGraphics/RDPStateManager.o \
			Source/HLEGraphics/PSPRenderer.o \
			Source/HLEGraphics/Texture.o \
			Source/HLEGraphics/TextureCache.o \
			Source/HLEGraphics/TextureDescriptor.o \
			Source/HLEGraphics/Ucode.o \
			Source/HLEGraphics/gsp/gspMacros.o \
			Source/HLEGraphics/gsp/gspSprite2D.o \
			Source/HLEGraphics/gsp/gspS2DEX.o \
			Source/HLEGraphics/gsp/gspCustom.o \
			Source/SysPSP/HLEGraphics/ConvertVertices.o \
			Source/SysPSP/HLEGraphics/TransformWithColour.o\
			Source/SysPSP/HLEGraphics/TransformWithLighting.o\
			Source/SysPSP/HLEGraphics/VectorClipping.o \
			Source/HLEGraphics/Combiner/CombinerExpression.o \
			Source/HLEGraphics/Combiner/CombinerTree.o \
			Source/HLEGraphics/Combiner/RenderSettings.o

DAED_AUDIO_SRCS =  	Source/HLEAudio/ABI1.o \
			Source/HLEAudio/ABI2.o \
			Source/HLEAudio/ABI3.o \
			Source/HLEAudio/ABI3mp3.o \
			Source/HLEAudio/AudioBuffer.o \
			Source/HLEAudio/AudioHLEProcessor.o \
			Source/HLEAudio/HLEMain.o \
			Source/SysPSP/HLEAudio/AudioCodePSP.o \
			Source/SysPSP/HLEAudio/AudioPluginPSP.o

DAED_OSHLE_SRCS = 	Source/OSHLE/OS.o \
			Source/OSHLE/patch.o

DAED_RELEASE_SRCS 	= 	Source/SysPSP/UI/RomSelectorComponent.o 

DAED_GPROF_SRCS 	= 	Source/SysPSP/Debug/prof.o \
			Source/SysPSP/Debug/mcount.o

ADDITIONAL_DEBUG_SRCS = Source/SysPSP/UI/RomSelectorComponent.o

ADDITIONAL_SYNC_SRCS  = Source/Utility/Synchroniser.o Source/Utility/ZLibWrapper.o

CORE_SRCS = $(DAED_MAIN_SRCS) $(DAED_DEBUG_SRCS) $(DAED_CORE_SRCS) $(DAED_INTERFACE_SRCS) \
	    $(DAED_INPUT_SRCS) $(DAED_DYNREC_SRCS) $(DAED_UTILITY_SRCS) $(DAED_PSP_SRCS) \
	    $(DAED_HLEGFX_SRCS) $(DAED_AUDIO_SRCS) $(DAED_OSHLE_SRCS)
		
ifdef PSPGPROF	
	CONFIG=Profile
	
	CFLAGS			= -pg -g -O2 -G0 -D_DEBUG -Wall -MD -ffast-math -fsingle-precision-constant

	SRCS			= $(CORE_SRCS) $(DAED_RELEASE_SRCS)	$(DAED_GPROF_SRCS) 
else 
ifdef DEBUG
	CONFIG=Dev #default config in Debug build is "Dev"

	CFLAGS			= -g -O1 -fno-omit-frame-pointer -G0 -D_DEBUG -MD \
				  -W -Wcast-qual -Wchar-subscripts -Wno-unused -Wpointer-arith\
				  -Wredundant-decls -Wshadow -Wwrite-strings
				#-Winline -Wcast-align 

	OBJS 			= $(CORE_SRCS) $(ADDITIONAL_DEBUG_SRCS) $(ADDITIONAL_SYNC_SRCS)
else 
	CFLAGS			= -O2 -G0 -DNDEBUG -Wall -MD -ffast-math -fsingle-precision-constant
					#-fno-builtin
					#-fgcse-after-reload
					#-funroll-loops 
	LDFLAGS 		= "-Wl,-O1"

	OBJS 			= $(CORE_SRCS) $(DAED_RELEASE_SRCS)
endif
endif

#PSPPORT = Source/ConfigOptions.o Source/Test/BatchTest.o $(CORE_SRCS) $(DAED_RELEASE_SRCS)
#OBJS = Source/SysPSP/main.o $(PSPPORT)


ifndef CONFIG
	CONFIG=Release
endif

CXXFLAGS = -fno-exceptions -fno-rtti -iquote./Source/SysPSP/Include -iquote./Source/Config/$(CONFIG) -iquote./Source -iquote./Source/SysPSP

#OBJS := $(SRCS:.o=.o)
#OBJS := $(OBJS:.c=.o)
#OBJS := $(OBJS:.S=.o)

#DEP_FILES := $(SRCS:.o=.d)
#DEP_FILES := $(DEP_FILES:.c=.d)
#DEP_FILES := $(DEP_FILES:.S=.d)

ASFLAGS =

INCDIR = $(PSPDEV)/SDK/include ./SDK/include
LIBDIR = $(PSPDEV)/SDK/lib ./SDK/lib


LIBS = -lsupc++ -lstdc++ -lpsppower -lpspgu -lpspaudiolib -lpspaudio -lpsprtc -lc -lpng -lz -lg -lm -lpspfpu -lpspvfpu -lpspkubridge

EXTRA_TARGETS = EBOOT.PBP dvemgr.prx exception.prx mediaengine.prx imposectrl.prx

PSP_EBOOT_TITLE = DaedalusX64 Alpha
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

ifeq ($(VERSION),)
	#Windows
	EXTRA_TARGETS := svn $(EXTRA_TARGETS)
	include $(PSPSDK)/lib/build.mak
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

#-include $(DEP_FILES)

