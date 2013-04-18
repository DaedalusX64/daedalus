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
        'include_dirs': [
          '../',
          '../Config/Dev',
        ],
        'dependencies': [
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
              '../../Source/SysW32/Include',
            ],
          }],
          ['OS=="mac"', {
            'include_dirs': [
              '../../Source/SysOSX/Include',
            ],
          }],
  	   ['OS=="linux"', {
            'include_dirs': [
              '../../Source/SysOSX/Include',
            ],
          }],
        ],
        'copies': [
          {
            'destination': '<(PRODUCT_DIR)/',
            'files': [
              '../../Source/SysGL/HLEGraphics/n64.psh',
            ],
          },
        ],
      },
    ],
  }
