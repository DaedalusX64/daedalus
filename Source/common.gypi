{
  'target_defaults': {
    'include_dirs': [
      'Config/Release/',
    ],
    'cflags': [
      '-std=c++11', '-Wno-c++11-compat',
    ],
  },
  'conditions': [
    ['OS=="win"', {
      'target_defaults': {
        'include_dirs': [
          'SysW32/Include',
        ],
      },
    }],
    ['OS=="mac"', {
      'target_defaults': {
        'include_dirs': [
          'SysOSX/Include',
        ],
        'xcode_settings': {
          'OTHER_CFLAGS': [
            '-Werror',
            '-Wall',
            '-Wformat=0',
            #'-O4',
            #'-g'
          ],
          'USE_HEADERMAP': 'NO',
        },
      },
    }],
    ['OS=="linux"', {
      'target_defaults': {
        'include_dirs': [
          'SysLinux/Include',
        ],
      },
    }],
  ],
}
