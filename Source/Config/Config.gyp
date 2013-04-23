 {
    'targets': [
      {
        'target_name': 'Config',
        'type': 'static_library',
        'xcode_settings': {
          'OTHER_CFLAGS': [
            '-Werror',
            '-Wformat=0',
            #'-O4',
            #'-g'
          ],
        },
        'direct_dependent_settings': {
          'include_dirs': [
            '../',
            '../Config/Dev',
          ],
        },
      },
    ],
  }
