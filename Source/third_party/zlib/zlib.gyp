{
  'targets': [
    {
      'target_name': 'zlib',
      'type': 'static_library',
      'xcode_settings': {
        'OTHER_CFLAGS': [
          #'-Werror',
          '-O3',
          '-g'
        ],
      },
      'defines': [
        'Z_HAVE_UNISTD_H',
      ],
      'sources': [
        'adler32.c',
        'compress.c',
        'crc32.c',
        'crc32.h',
        'deflate.c',
        'deflate.h',
        'gzclose.c',
        'gzread.c',
        'gzwrite.c',
        'gzlib.c',
        'infback.c',
        'inffast.c',
        'inffast.h',
        'inffixed.h',
        'inflate.c',
        'inflate.h',
        'inftrees.c',
        'inftrees.h',
        'mozzconf.h',
        'trees.c',
        'trees.h',
        'uncompr.c',
        'zconf.h',
        'zlib.h',
        'zutil.c',
        'zutil.h',
      ],
      'include_dirs': [
        '.',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '.',
        ],
      },
    }, {
      'target_name': 'minizip',
      'type': 'static_library',
      'xcode_settings': {
        'OTHER_CFLAGS': [
          #'-Werror',
          '-O3',
          '-g'
        ],
      },
      'sources': [
        'contrib/minizip/ioapi.c',
        'contrib/minizip/ioapi.h',
        'contrib/minizip/iowin32.c',
        'contrib/minizip/iowin32.h',
        'contrib/minizip/unzip.c',
        'contrib/minizip/unzip.h',
        'contrib/minizip/zip.c',
        'contrib/minizip/zip.h',
      ],
      'include_dirs': [
        '.',
        'contrib/minizip/'
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '.',
          'contrib/minizip/'
        ],
      },
      'conditions': [
        ['OS!="win"', {
          'sources!': [
            'contrib/minizip/iowin32.c'
          ],
        }],
      ],
    },
  ],
}
