#
#	Specify :
#		DEBUG=y				# for a debug build
#		PSPGPROF=y			# for profiling with psp gprof
#
#		CONFIG=<configname>		# Build using $(SRCDIR)/Configs/<configname> as config
#
#		Default config for DEBUG build is Dev
#		Default config for non DEBUG build is Release
#

TARGET = Daedalus
SRCDIR = ../../Source

CORE_SRCS = \
	$(SRCDIR)/ConfigOptions.cpp \
	$(SRCDIR)/Core/Cheats.cpp \
	$(SRCDIR)/Core/CPU.cpp \
	$(SRCDIR)/Core/DMA.cpp \
	$(SRCDIR)/Core/Dynamo.cpp \
	$(SRCDIR)/Core/FlashMem.cpp \
	$(SRCDIR)/Core/Interpret.cpp \
	$(SRCDIR)/Core/Interrupts.cpp \
	$(SRCDIR)/Core/JpegTask.cpp \
	$(SRCDIR)/Core/Memory.cpp \
	$(SRCDIR)/Core/PIF.cpp \
	$(SRCDIR)/Core/R4300.cpp \
	$(SRCDIR)/Core/Registers.cpp \
	$(SRCDIR)/Core/ROM.cpp \
	$(SRCDIR)/Core/ROMBuffer.cpp \
	$(SRCDIR)/Core/ROMImage.cpp \
	$(SRCDIR)/Core/RomSettings.cpp \
	$(SRCDIR)/Core/RSP_HLE.cpp \
	$(SRCDIR)/Core/Save.cpp \
	$(SRCDIR)/Core/SaveState.cpp \
	$(SRCDIR)/Core/TLB.cpp \
	$(SRCDIR)/Debug/DebugLog.cpp \
	$(SRCDIR)/Debug/Dump.cpp \
	$(SRCDIR)/DynaRec/BranchType.cpp \
	$(SRCDIR)/DynaRec/DynaRecProfile.cpp \
	$(SRCDIR)/DynaRec/Fragment.cpp \
	$(SRCDIR)/DynaRec/FragmentCache.cpp \
	$(SRCDIR)/DynaRec/IndirectExitMap.cpp \
	$(SRCDIR)/DynaRec/StaticAnalysis.cpp \
	$(SRCDIR)/DynaRec/TraceRecorder.cpp \
	$(SRCDIR)/Graphics/ColourValue.cpp \
	$(SRCDIR)/Graphics/PngUtil.cpp \
	$(SRCDIR)/Graphics/TextureTransform.cpp \
	$(SRCDIR)/HLEAudio/ABI1.cpp \
	$(SRCDIR)/HLEAudio/ABI2.cpp \
	$(SRCDIR)/HLEAudio/ABI3.cpp \
	$(SRCDIR)/HLEAudio/ABI3mp3.cpp \
	$(SRCDIR)/HLEAudio/AudioBuffer.cpp \
	$(SRCDIR)/HLEAudio/AudioHLEProcessor.cpp \
	$(SRCDIR)/HLEAudio/HLEMain.cpp \
	$(SRCDIR)/HLEGraphics/BaseRenderer.cpp \
	$(SRCDIR)/HLEGraphics/CachedTexture.cpp \
	$(SRCDIR)/HLEGraphics/ConvertImage.cpp \
	$(SRCDIR)/HLEGraphics/ConvertTile.cpp \
	$(SRCDIR)/HLEGraphics/DLDebug.cpp \
	$(SRCDIR)/HLEGraphics/DLParser.cpp \
	$(SRCDIR)/HLEGraphics/Microcode.cpp \
	$(SRCDIR)/HLEGraphics/RDP.cpp \
	$(SRCDIR)/HLEGraphics/RDPStateManager.cpp \
	$(SRCDIR)/HLEGraphics/TextureCache.cpp \
	$(SRCDIR)/HLEGraphics/TextureInfo.cpp \
	$(SRCDIR)/HLEGraphics/uCodes/Ucode.cpp \
	$(SRCDIR)/Interface/RomDB.cpp \
	$(SRCDIR)/Math/Matrix4x4.cpp \
	$(SRCDIR)/OSHLE/OS.cpp \
	$(SRCDIR)/OSHLE/patch.cpp \
	$(SRCDIR)/Plugins/GraphicsPlugin.cpp \
	$(SRCDIR)/SysPSP/Debug/DaedalusAssertPSP.cpp \
	$(SRCDIR)/SysPSP/Debug/DBGConsolePSP.cpp \
	$(SRCDIR)/SysPSP/DveMgr/pspDveManager.S \
	$(SRCDIR)/SysPSP/DynaRec/AssemblyUtilsPSP.cpp \
	$(SRCDIR)/SysPSP/DynaRec/AssemblyWriterPSP.cpp \
	$(SRCDIR)/SysPSP/DynaRec/CodeBufferManagerPSP.cpp \
	$(SRCDIR)/SysPSP/DynaRec/CodeGeneratorPSP.cpp \
	$(SRCDIR)/SysPSP/DynaRec/DynaRecStubs.S \
	$(SRCDIR)/SysPSP/DynaRec/DynarecTargetPSP.cpp \
	$(SRCDIR)/SysPSP/DynaRec/N64RegisterCachePSP.cpp \
	$(SRCDIR)/SysPSP/Graphics/DrawText.cpp \
	$(SRCDIR)/SysPSP/Graphics/GraphicsContext.cpp \
	$(SRCDIR)/SysPSP/Graphics/intraFont/intraFont.c \
	$(SRCDIR)/SysPSP/Graphics/intraFont/libccc.c \
	$(SRCDIR)/SysPSP/Graphics/NativeTexturePSP.cpp \
	$(SRCDIR)/SysPSP/Graphics/VideoMemoryManager.cpp \
	$(SRCDIR)/SysPSP/HLEAudio/AudioOutput.cpp \
	$(SRCDIR)/SysPSP/HLEAudio/AudioPluginPSP.cpp \
	$(SRCDIR)/SysPSP/HLEGraphics/Blender.cpp \
	$(SRCDIR)/SysPSP/HLEGraphics/BlendModes.cpp \
	$(SRCDIR)/SysPSP/HLEGraphics/ColourAdjuster.cpp \
	$(SRCDIR)/SysPSP/HLEGraphics/Combiner/BlendConstant.cpp \
	$(SRCDIR)/SysPSP/HLEGraphics/Combiner/CombinerExpression.cpp \
	$(SRCDIR)/SysPSP/HLEGraphics/Combiner/CombinerTree.cpp \
	$(SRCDIR)/SysPSP/HLEGraphics/Combiner/RenderSettings.cpp \
	$(SRCDIR)/SysPSP/HLEGraphics/ConvertVertices.S \
	$(SRCDIR)/SysPSP/HLEGraphics/DisplayListDebugger.cpp \
	$(SRCDIR)/SysPSP/HLEGraphics/GraphicsPluginPSP.cpp \
	$(SRCDIR)/SysPSP/HLEGraphics/RendererPSP.cpp \
	$(SRCDIR)/SysPSP/HLEGraphics/TnLVFPU.S\
	$(SRCDIR)/SysPSP/HLEGraphics/VectorClipping.S \
	$(SRCDIR)/SysPSP/Input/InputManagerPSP.cpp \
	$(SRCDIR)/SysPSP/KernelButtonsPrx/imposectrl.S \
	$(SRCDIR)/SysPSP/main.cpp \
	$(SRCDIR)/SysPSP/MediaEnginePRX/me.c \
	$(SRCDIR)/SysPSP/MediaEnginePRX/MediaEngine.S \
	$(SRCDIR)/SysPSP/UI/AboutComponent.cpp \
	$(SRCDIR)/SysPSP/UI/AdjustDeadzoneScreen.cpp \
	$(SRCDIR)/SysPSP/UI/AdvancedOptionsScreen.cpp \
	$(SRCDIR)/SysPSP/UI/CheatOptionsScreen.cpp \
	$(SRCDIR)/SysPSP/UI/ColourPulser.cpp \
	$(SRCDIR)/SysPSP/UI/Dialogs.cpp \
	$(SRCDIR)/SysPSP/UI/GlobalSettingsComponent.cpp \
	$(SRCDIR)/SysPSP/UI/MainMenuScreen.cpp \
	$(SRCDIR)/SysPSP/UI/PauseOptionsComponent.cpp \
	$(SRCDIR)/SysPSP/UI/PauseScreen.cpp \
	$(SRCDIR)/SysPSP/UI/RomPreferencesScreen.cpp \
	$(SRCDIR)/SysPSP/UI/RomSelectorComponent.cpp \
	$(SRCDIR)/SysPSP/UI/SavestateSelectorComponent.cpp \
	$(SRCDIR)/SysPSP/UI/SelectedRomComponent.cpp \
	$(SRCDIR)/SysPSP/UI/SplashScreen.cpp \
	$(SRCDIR)/SysPSP/UI/UICommand.cpp \
	$(SRCDIR)/SysPSP/UI/UIComponent.cpp \
	$(SRCDIR)/SysPSP/UI/UIContext.cpp \
	$(SRCDIR)/SysPSP/UI/UIElement.cpp \
	$(SRCDIR)/SysPSP/UI/UIScreen.cpp \
	$(SRCDIR)/SysPSP/UI/UISetting.cpp \
	$(SRCDIR)/SysPSP/Utility/AtomicPrimitives.S \
	$(SRCDIR)/SysPSP/Utility/BatteryPSP.cpp \
	$(SRCDIR)/SysPSP/Utility/Buttons.cpp \
	$(SRCDIR)/SysPSP/Utility/DebugMemory.cpp \
	$(SRCDIR)/SysPSP/Utility/DisableFPUExceptions.S \
	$(SRCDIR)/SysPSP/Utility/exception.cpp \
	$(SRCDIR)/SysPSP/Utility/FastMemcpy.cpp \
	$(SRCDIR)/SysPSP/Utility/IOPSP.cpp \
	$(SRCDIR)/SysPSP/Utility/JobManager.cpp \
	$(SRCDIR)/SysPSP/Utility/ModulePSP.cpp \
	$(SRCDIR)/SysPSP/Utility/ThreadPSP.cpp \
	$(SRCDIR)/SysPSP/Utility/TimingPSP.cpp \
	$(SRCDIR)/SysPSP/Utility/VolatileMemPSP.cpp \
	$(SRCDIR)/System.cpp \
	$(SRCDIR)/Test/BatchTest.cpp \
	$(SRCDIR)/Utility/CRC.cpp \
	$(SRCDIR)/Utility/DataSink.cpp \
	$(SRCDIR)/Utility/FastMemcpy.cpp \
	$(SRCDIR)/Utility/FramerateLimiter.cpp \
	$(SRCDIR)/Utility/Hash.cpp \
	$(SRCDIR)/Utility/IniFile.cpp \
	$(SRCDIR)/Utility/MemoryHeap.cpp \
	$(SRCDIR)/Utility/Preferences.cpp \
	$(SRCDIR)/Utility/PrintOpCode.cpp \
	$(SRCDIR)/Utility/Profiler.cpp \
	$(SRCDIR)/Utility/ROMFile.cpp \
	$(SRCDIR)/Utility/ROMFileCache.cpp \
	$(SRCDIR)/Utility/ROMFileCompressed.cpp \
	$(SRCDIR)/Utility/ROMFileMemory.cpp \
	$(SRCDIR)/Utility/ROMFileUncompressed.cpp \
	$(SRCDIR)/Utility/Stream.cpp \
	$(SRCDIR)/Utility/StringUtil.cpp \
	$(SRCDIR)/Utility/Timer.cpp \
	$(SRCDIR)/Utility/Translate.cpp \
	$(SRCDIR)/Utility/ZLibWrapper.cpp

DAED_GPROF_SRCS = \
	$(SRCDIR)/SysPSP/Debug/prof.c \
	$(SRCDIR)/SysPSP/Debug/mcount.S

ADDITIONAL_SYNC_SRCS = \
	$(SRCDIR)/Utility/Synchroniser.cpp \
	$(SRCDIR)/Utility/ZLibWrapper.cpp	# Already specified in CORE_SRCS - remove


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

CXXFLAGS = -fno-exceptions -fno-rtti -iquote$(SRCDIR)/SysPSP/Include -iquote$(SRCDIR)/Config/$(CONFIG) -iquote$(SRCDIR) -iquote$(SRCDIR)/SysPSP

OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.S=.o)

DEP_FILES := $(SRCS:.cpp=.d)
DEP_FILES := $(DEP_FILES:.c=.d)
DEP_FILES := $(DEP_FILES:.S=.d)

ASFLAGS =

INCDIR = $(PSPDEV)/SDK/include ../../SDK/include
LIBDIR = $(PSPDEV)/SDK/lib ../../SDK/lib


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
	include ../../SDK/lib/build.mak
svn:
	@echo svnversion not found, trying SubWCRev
	@-SubWCRev . $(SRCDIR)/svnversion.txt $(SRCDIR)/svnversion.h
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

$(SRCDIR)/SysPSP/MediaEnginePRX/MediaEngine.S:
	$(MAKE) -C $(SRCDIR)/SysPSP/MediaEnginePRX all

$(SRCDIR)/SysPSP/DveMgr/pspDveManager.S:
	$(MAKE) -C $(SRCDIR)/SysPSP/DveMgr all

dvemgr.prx:
	$(MAKE) -C $(SRCDIR)/SysPSP/DveMgr all

mediaengine.prx:
	$(MAKE) -C $(SRCDIR)/SysPSP/MediaEnginePRX all

exception.prx:
	$(MAKE) -C $(SRCDIR)/SysPSP/ExceptionHandler/prx all

$(SRCDIR)/SysPSP/KernelButtonsPrx/imposectrl.o:
	$(MAKE) -C $(SRCDIR)/SysPSP/KernelButtonsPrx imposectrl.o

imposectrl.prx:
	$(MAKE) -C $(SRCDIR)/SysPSP/KernelButtonsPrx all

allclean: clean
	$(MAKE) -C $(SRCDIR)/SysPSP/ExceptionHandler/prx clean
	$(MAKE) -C $(SRCDIR)/SysPSP/MediaEnginePRX clean
	$(MAKE) -C $(SRCDIR)/SysPSP/DveMgr clean
	$(MAKE) -C $(SRCDIR)/SysPSP/KernelButtonsPrx clean

-include $(DEP_FILES)

