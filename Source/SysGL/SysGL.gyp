 {
    'targets': [
      {
        'target_name': 'SysGL',
        'type': 'static_library',
        'xcode_settings': {
          'OTHER_CFLAGS': [
            '-Werror',
            '-Wformat=0',
            #'-O4',
            #'-g'
          ],
        },
        'cflags': [
          '-std=c++11', '-Wno-c++11-compat',
        ],
        'include_dirs': [
          '../',
        ],
        'dependencies': [
          '../Config/Config.gyp:Config',
          '../third_party/glfw/glfw.gyp:glfw',
          '../third_party/libpng/libpng.gyp:libpng',
        ],
        'sources': [
          'Graphics/GraphicsContextGL.cpp',
          'Graphics/NativeTextureGL.cpp',
          'HLEGraphics/GraphicsPluginGL.cpp',
          'HLEGraphics/RendererGL.cpp',
          'Interface/UI.cpp',
        ],
        'conditions': [
          ['OS=="win"', {
            'include_dirs': [
              '../SysW32/Include',
            ],
          }],
          ['OS=="mac"', {
            'include_dirs': [
              '../SysOSX/Include',
            ],
          }],
          ['OS=="linux"', {
            'include_dirs': [
              '../SysLinux/Include',
            ],
          }],
        ],
        'copies': [
          {
            'destination': '<(PRODUCT_DIR)/',
            'files': [
              '../SysGL/HLEGraphics/n64.psh',
            ],
          },
        ],
      },
    ],
  }
