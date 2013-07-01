{
	'targets': [
		{
			'target_name': 'gtest',
			'type': 'static_library',
			'sources': [
				'src/gtest-death-test.cc',
				'src/gtest-filepath.cc',
				'src/gtest-internal-inl.h',
				'src/gtest-port.cc',
				'src/gtest-printers.cc',
				'src/gtest-test-part.cc',
				'src/gtest-typed-test.cc',
				'src/gtest.cc',
			],
			'include_dirs': [
				'.',
				'include',
			],
			'direct_dependent_settings': {
				'include_dirs': [
					'include'
				],
			},
		},
		{
			'target_name': 'gtest_main',
			'type': 'static_library',
			'sources': [
				'src/gtest_main.cc',
			],
			'dependencies': [
				'gtest',
			],	
			'direct_dependent_settings': {
				'include_dirs': [
					'include'
				],
			},
		}
	],
}