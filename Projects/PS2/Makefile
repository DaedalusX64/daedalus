TARGET = Daedalus
SRCDIR = ../../Source

CORE_SRCS = \
	$(SRCDIR)/Config/ConfigOptions.cpp \
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
	$(SRCDIR)/Debug/DebugConsoleImpl.cpp \
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
	$(SRCDIR)/System/Paths.cpp \
	$(SRCDIR)/System/System.cpp \
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
	$(SRCDIR)/Utility/Synchroniser.cpp \
	$(SRCDIR)/Utility/Timer.cpp \
	$(SRCDIR)/Utility/Translate.cpp \
	$(SRCDIR)/Utility/ZLibWrapper.cpp

SRCS = \
	$(SRCDIR)/SysPS2/main.cpp \

OBJS =

EE_BIN = daedalus.elf

EE_OBJS = $(OBJS)

EE_CFLAGS = -Wno-unused-value
EE_CXXFLAGS = -fno-exceptions -fno-rtti -iquote$(SRCDIR)/SysPS2/Include -iquote$(SRCDIR)/Config/$(CONFIG) -iquote$(SRCDIR) -iquote$(SRCDIR)/SysPS2

EE_LDFLAGS = -L$(PS2SDK)/ports/lib

CORE_OBJS := $(CORE_SRCS)
CORE_OBJS := $(CORE_OBJS:.cpp=.o)
CORE_OBJS := $(CORE_OBJS:.c=.o)
CORE_OBJS := $(CORE_OBJS:.S=.o)

OBJS = $(SRCS)
OBJS := $(OBJS:.cpp=.o)
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.S=.o)
OBJS += daedalus.a

DEP_FILES = $(SRCS) $(CORE_SRCS)
DEP_FILES := $(DEP_FILES:.cpp=.d)
DEP_FILES := $(DEP_FILES:.c=.d)
DEP_FILES := $(DEP_FILES:.S=.d)

ASFLAGS =

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
