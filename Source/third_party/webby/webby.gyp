{
  'targets': [
    {
      'target_name': 'webby',
      'type': 'static_library',
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
