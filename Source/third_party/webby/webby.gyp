{
  'targets': [
    {
      'target_name': 'webby',
      'type': 'static_library',
      'xcode_settings': {
        'OTHER_CFLAGS': [
          '-Werror',
          '-O3',
          '-g'
        ],
      },
      'sources': [
        'webby.c',
        'webby.h',
      ],
      'include_dirs': [
        '.',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '.',
        ],
      },
    },
  ],
}
