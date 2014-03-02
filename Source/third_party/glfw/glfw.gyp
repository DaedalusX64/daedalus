{
  'targets': [
    {
      'target_name': 'glfw',
      'type': 'static_library',
      'xcode_settings': {
        'OTHER_CFLAGS': [
          #'-Werror',
          '-O3',
          '-g'
        ],
      },
      'include_dirs': [
        'src/',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'include/',
        ],
      },
      'defines': [
        '_GLFW_USE_OPENGL=1',
        '_GLFW_VERSION_FULL="3.0.1"',
        '_GLFW_USE_MENUBAR=1',
      ],
      'sources': [
        'src/clipboard.c',
        'src/context.c',
        'src/gamma.c',
        'src/init.c',
        'src/input.c',
        'src/joystick.c',
        'src/monitor.c',
        'src/time.c',
        'src/window.c',
      ],
      'conditions': [
        ['OS=="mac"', {
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
              '$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
              '$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
            ],
          },
          'defines': [
            '_GLFW_COCOA=1',
            '_GLFW_NSGL=1',
          ],
          'sources': [
            'src/nsgl_context.m',
            'src/cocoa_clipboard.m',
            'src/cocoa_gamma.c',
            'src/cocoa_init.m',
            'src/cocoa_joystick.m',
            'src/cocoa_monitor.m',
            'src/cocoa_time.c',
            'src/cocoa_window.m',
          ],
        }],
        ['OS=="linux"', {
          'link_settings': {
            'libraries': [
              '-lX11',
              '-lGL',
              '-lrt',
            ],
          },
          'defines': [
            '_GLFW_X11=1',
            '_GLFW_GLX=1',
          ],
          'sources': [
            'src/glx_context.c', #Note if using ARM linux change to egl_context
            'src/x11_clipboard.c',
            'src/x11_gamma.c',
            'src/x11_init.c',
            'src/x11_joystick.c',
            'src/x11_monitor.c',
            'src/x11_time.c',
            'src/x11_unicode.c',
            'src/x11_window.c',
          ],
        }],
      ],
    }
  ],
}
