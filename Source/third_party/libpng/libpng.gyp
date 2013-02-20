{
	'targets': [
		{
			'target_name': 'libpng',
			'type': 'static_library',
			'sources': [
				'png.c',
				'png.h',
				'pngerror.c',
				'pngget.c',
				'pngmem.c',
				'pngpread.c',
				'pngread.c',
				'pngrio.c',
				'pngrtran.c',
				'pngrutil.c',
				'pngset.c',
				'pngtrans.c',
				'pngwio.c',
				'pngwrite.c',
				'pngwtran.c',
				'pngwutil.c',
			],
      'dependencies': [
        '../zlib/zlib.gyp:zlib',
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
