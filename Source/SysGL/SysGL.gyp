 {
    'includes': [
      '../common.gypi',
    ],
    'targets': [
      {
        'target_name': 'SysGL',
        'type': 'static_library',
        'include_dirs': [
          '../',
        ],
        'dependencies': [
          '../third_party/glew/glew.gyp:glew',
          '../third_party/glfw/glfw.gyp:glfw',
          '../third_party/libpng/libpng.gyp:libpng',
        ],
        'sources': [
          'Graphics/GraphicsContextGL.cpp',
          'Graphics/NativeTextureGL.cpp',
          'HLEGraphics/GraphicsPluginGL.cpp',
          'HLEGraphics/RendererGL.cpp',
          'Input/InputManagerGL.cpp',
          'Interface/UI.cpp',
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
