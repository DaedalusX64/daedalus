 {
    'includes': [
      'common.gypi',
    ],
    'targets': [
      {
        'target_name': 'daedalus_lib',
        'type': 'static_library',
        'dependencies': [
          'SysGL/SysGL.gyp:SysGL',
          'third_party/glew/glew.gyp:glew', # FIXME: should transitively pull in include dir
          'third_party/glfw/glfw.gyp:glfw', # FIXME: should transitively pull in include dir
          'third_party/libpng/libpng.gyp:libpng',
          'third_party/webby/webby.gyp:webby',
          'third_party/zlib/zlib.gyp:minizip',
          'third_party/zlib/zlib.gyp:zlib',
        ],
        'include_dirs': [
          '.',
        ],
        'defines': [
          'DAEDALUS_ACCURATE_TMEM',
        ],
        'direct_dependent_settings': {
          'include_dirs': [
            '.'
          ],
        },
        'sources': [
          'Config/ConfigOptions.cpp',
          'Core/Cheats.cpp',
          'Core/CPU.cpp',
          'Core/DMA.cpp',
          'Core/Dynamo.cpp',
          'Core/FlashMem.cpp',
          'Core/Interpret.cpp',
          'Core/Interrupts.cpp',
          'Core/JpegTask.cpp',
          'Core/Memory.cpp',
          'Core/PIF.cpp',
          'Core/R4300.cpp',
          'Core/Registers.cpp',
          'Core/ROM.cpp',
          'Core/ROMBuffer.cpp',
          'Core/ROMImage.cpp',
          'Core/RomSettings.cpp',
          'Core/RSP_HLE.cpp',
          'Core/Save.cpp',
          'Core/SaveState.cpp',
          'Core/TLB.cpp',
          'Debug/DebugConsoleImpl.cpp',
          'Debug/DebugLog.cpp',
          'Debug/Dump.cpp',
          'DynaRec/BranchType.cpp',
          'DynaRec/Fragment.cpp',
          'DynaRec/FragmentCache.cpp',
          'DynaRec/IndirectExitMap.cpp',
          'DynaRec/StaticAnalysis.cpp',
          'DynaRec/TraceRecorder.cpp',
          'Graphics/ColourValue.cpp',
          'Graphics/PngUtil.cpp',
          'Graphics/TextureTransform.cpp',
          'HLEAudio/ABI1.cpp',
          'HLEAudio/ABI2.cpp',
          'HLEAudio/ABI3.cpp',
          'HLEAudio/ABI3mp3.cpp',
          'HLEAudio/AudioBuffer.cpp',
          'HLEAudio/AudioHLEProcessor.cpp',
          'HLEAudio/HLEMain.cpp',
          'HLEGraphics/BaseRenderer.cpp',
          'HLEGraphics/CachedTexture.cpp',
          'HLEGraphics/ConvertImage.cpp',
          'HLEGraphics/ConvertTile.cpp',
          'HLEGraphics/DLDebug.cpp',
          'HLEGraphics/DLParser.cpp',
          'HLEGraphics/Microcode.cpp',
          'HLEGraphics/RDP.cpp',
          'HLEGraphics/RDPStateManager.cpp',
          'HLEGraphics/TextureCache.cpp',
          'HLEGraphics/TextureCacheWebDebug.cpp',
          'HLEGraphics/TextureInfo.cpp',
          'HLEGraphics/uCodes/Ucode.cpp',
          'Interface/RomDB.cpp',
          'Math/Matrix4x4.cpp',
          'OSHLE/OS.cpp',
          'OSHLE/patch.cpp',
          'Plugins/GraphicsPlugin.cpp',
          'System/Paths.cpp',
          'System/System.cpp',
          'Test/BatchTest.cpp',
          'Utility/CRC.cpp',
          'Utility/DataSink.cpp',
          'Utility/FastMemcpy.cpp',
          'Utility/FramerateLimiter.cpp',
          'Utility/Hash.cpp',
          'Utility/IniFile.cpp',
          'Utility/MemoryHeap.cpp',
          'Utility/Preferences.cpp',
          'Utility/PrintOpCode.cpp',
          'Utility/Profiler.cpp',
          'Utility/ROMFile.cpp',
          'Utility/ROMFileCache.cpp',
          'Utility/ROMFileCompressed.cpp',
          'Utility/ROMFileMemory.cpp',
          'Utility/ROMFileUncompressed.cpp',
          'Utility/Stream.cpp',
          'Utility/StringUtil.cpp',
          'Utility/Synchroniser.cpp',
          'Utility/Timer.cpp',
          'Utility/ZLibWrapper.cpp',

          #FIXME
          'SysW32/DynaRec/x86/AssemblyUtilsX86.cpp',
        ],
        'conditions': [
          ['OS=="win"', {
            'sources': [
              'SysW32/HLEAudio/AudioPluginW32.cpp',
              'SysW32/Debug/DaedalusAssertW32.cpp',
              'SysW32/Debug/DebugConsoleW32.cpp',
              'SysW32/Utility/IOW32.cpp',
              'SysW32/Utility/ThreadW32.cpp',
              'SysW32/Utility/TimingW32.cpp',
            ],
          }],
          ['OS=="mac"', {
            'link_settings': {
              'libraries': [
                '$(SDKROOT)/System/Library/Frameworks/AudioToolbox.framework',
              ],
            },
            'sources': [
              'SysOSX/HLEAudio/AudioPluginOSX.cpp',
              'SysOSX/Debug/DaedalusAssertOSX.cpp',
              'SysOSX/Debug/DebugConsoleOSX.cpp',
              'SysOSX/Debug/WebDebug.cpp',
              'SysOSX/Debug/WebDebugTemplate.cpp',
              'SysOSX/DynaRec/CodeBufferManagerOSX.cpp',
              'SysOSX/HLEGraphics/DisplayListDebugger.cpp',
              'SysPosix/Utility/CondPosix.cpp',
              'SysPosix/Utility/IOPosix.cpp',
              'SysPosix/Utility/ThreadPosix.cpp',
              'SysPosix/Utility/TimingPosix.cpp',
            ],
          }],
          ['OS=="linux"', {
            'sources': [
              # FIXME - we should move these to a common SysPosix dir...
              'SysOSX/Debug/DaedalusAssertOSX.cpp',
              'SysOSX/Debug/DebugConsoleOSX.cpp',
              'SysOSX/Debug/WebDebug.cpp',
              'SysOSX/Debug/WebDebugTemplate.cpp',
              'SysOSX/DynaRec/CodeBufferManagerOSX.cpp',
              'SysOSX/HLEGraphics/DisplayListDebugger.cpp',
              'SysPosix/Utility/CondPosix.cpp',
              'SysPosix/Utility/IOPosix.cpp',
              'SysPosix/Utility/ThreadPosix.cpp',
              'SysPosix/Utility/TimingPosix.cpp',

              'SysLinux/HLEAudio/AudioPluginLinux.cpp',
            ],
          }],
        ],
        'copies': [
          {
            'destination': '<(PRODUCT_DIR)/',
            'files': [
              '../Data/PSP/roms.ini',
            ],
          },
          {
            'destination': '<(PRODUCT_DIR)/Web/css',
            'files': [
              'SysOSX/Debug/Web/css/bootstrap-responsive.css',
              'SysOSX/Debug/Web/css/bootstrap-responsive.min.css',
              'SysOSX/Debug/Web/css/bootstrap.css',
              'SysOSX/Debug/Web/css/bootstrap.min.css',
              'SysOSX/Debug/Web/css/dldebugger.css',
            ],
          },
          {
            'destination': '<(PRODUCT_DIR)/Web/html/',
            'files': [
              'SysOSX/Debug/Web/html/dldebugger.html',
            ],
          },
          {
            'destination': '<(PRODUCT_DIR)/Web/img',
            'files': [
              'SysOSX/Debug/Web/img/glyphicons-halflings-white.png',
              'SysOSX/Debug/Web/img/glyphicons-halflings.png',
            ],
          },
          {
            'destination': '<(PRODUCT_DIR)/Web/js',
            'files': [
              'SysOSX/Debug/Web/js/bootstrap.js',
              'SysOSX/Debug/Web/js/bootstrap.min.js',
              'SysOSX/Debug/Web/js/dldebugger.js',
              'SysOSX/Debug/Web/js/jquery-1.9.1.min.js',
            ],
          },
        ],
      },
      {
        'target_name': 'daedalus',
        'type': 'executable',
        'dependencies': [
          'daedalus_lib',
        ],
        'conditions': [
          ['OS=="win"', {
            'sources': ['SysW32/main.cpp'],
          }],
          ['OS=="mac"', {
            'sources': ['SysOSX/main.cpp'],
          }],
          ['OS=="linux"', {
              # FIXME - we should move these to a common SysPosix dir...
            'sources': ['SysOSX/main.cpp'],
          }],
        ],
      },
      {
        'target_name': 'daedalus_test',
        'type': 'executable',
        'dependencies': [
          'daedalus_lib',
          'third_party/gtest/gtest.gyp:gtest_main',
        ],
        'include_dirs': [
          '.',
        ],
        'sources': [
          'Utility/FastMemcpy_test.cpp',
        ],
      }
    ],
  }
