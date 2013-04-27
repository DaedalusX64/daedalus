{
  'target_defaults': {
    'include_dirs': [
      'Config/Dev/',
    ],
    'cflags': [
      '-std=c++11', '-Wno-c++11-compat',
    ],
  },
  'conditions': [
    [
      'OS=="mac"',
      {
        'target_defaults': {
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
      },
    ],
  ],
}
