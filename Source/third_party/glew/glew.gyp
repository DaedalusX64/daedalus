{
  'targets': [
    {
      'target_name': 'glew',
      'type': 'static_library',
      'xcode_settings': {
        'OTHER_CFLAGS': [
          #'-Werror',
          '-O3',
          '-g'
        ],
      },
      'direct_dependent_settings': {
        'include_dirs': [
          'include/',
        ],
        'defines': [
          'GLEW_STATIC',
        ],
      },
      'include_dirs': [
        'include/',
      ],
      'defines': [
        'GLEW_STATIC',
      ],
      'sources': [
        'src/glew.c',
      ],
      'conditions': [
        ['OS=="mac"', {
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
            ],
          },
        }],
      ],
    },
  ],
}
