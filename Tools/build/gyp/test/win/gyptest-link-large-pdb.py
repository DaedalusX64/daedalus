#!/usr/bin/env python

# Copyright (c) 2013 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Make sure msvs_large_pdb works correctly.
"""

import TestGyp

import struct
import sys


CHDIR = 'large-pdb'


def CheckImageAndPdb(test, image_basename, expected_page_size):
  pdb_basename = image_basename + '.pdb'
  test.built_file_must_exist(image_basename, chdir=CHDIR)
  test.built_file_must_exist(pdb_basename, chdir=CHDIR)

  # We expect the PDB to have the given page size. For full details of the
  # header look here: https://code.google.com/p/pdbparser/wiki/MSF_Format
  # We read the little-endian 4-byte unsigned integer at position 32 of the
  # file.
  pdb_path = test.built_file_path(pdb_basename, chdir=CHDIR)
  pdb_file = open(pdb_path, 'rb')
  pdb_file.seek(32, 0)
  page_size = struct.unpack('<I', pdb_file.read(4))[0]
  if page_size != expected_page_size:
    print "Expected page size of %d, got %d for PDB file `%s'." % (
        expected_page_size, page_size, pdb_path)


if sys.platform == 'win32':
  test = TestGyp.TestGyp(formats=['msvs', 'ninja'])

  test.run_gyp('large-pdb.gyp', chdir=CHDIR)
  test.build('large-pdb.gyp', test.ALL, chdir=CHDIR)

  CheckImageAndPdb(test, 'large_pdb_exe.exe', 4096)
  CheckImageAndPdb(test, 'small_pdb_exe.exe', 1024)
  CheckImageAndPdb(test, 'large_pdb_dll.dll', 4096)
  CheckImageAndPdb(test, 'small_pdb_dll.dll', 1024)

  test.pass_test()
