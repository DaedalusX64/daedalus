# Copyright (c) 2013 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
 'targets': [
    {
      'target_name': 'large_pdb_exe',
      'type': 'executable',
      'msvs_large_pdb': 1,
      'sources': [
        'main.cc',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          'GenerateDebugInformation': 'true',
          'ProgramDatabaseFile': '<(PRODUCT_DIR)/large_pdb_exe.exe.pdb',
        },
      },
    },
    {
      'target_name': 'small_pdb_exe',
      'type': 'executable',
      'msvs_large_pdb': 0,
      'sources': [
        'main.cc',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          'GenerateDebugInformation': 'true',
          'ProgramDatabaseFile': '<(PRODUCT_DIR)/small_pdb_exe.exe.pdb',
        },
      },
    },
    {
      'target_name': 'large_pdb_dll',
      'type': 'shared_library',
      'msvs_large_pdb': 1,
      'sources': [
        'dllmain.cc',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          'GenerateDebugInformation': 'true',
          'ProgramDatabaseFile': '<(PRODUCT_DIR)/large_pdb_dll.dll.pdb',
        },
      },
    },
    {
      'target_name': 'small_pdb_dll',
      'type': 'shared_library',
      'msvs_large_pdb': 0,
      'sources': [
        'dllmain.cc',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          'GenerateDebugInformation': 'true',
          'ProgramDatabaseFile': '<(PRODUCT_DIR)/small_pdb_dll.dll.pdb',
        },
      },
    },
  ]
}
