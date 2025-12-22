#!/usr/bin/env python3
#
# Copyright 2025, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import gzip
import os
from os.path import abspath, dirname
import tempfile
import unittest
import smaps_pprof
from smaps_pprof import profile_pb2


class SmapsPprofTest(unittest.TestCase):

  def test_smaps_pprof(self):
    smaps_data = """>>PID_START: 1111 system_server
7f8c8c0000-7f8c8c2000 r--p 00000000 103:0a 1234  /system/lib64/libart.so
Size:               16 kB
Rss:                12 kB
Pss:                10 kB
Shared_Clean:       8 kB
Shared_Dirty:       0 kB
Private_Clean:      4 kB
Private_Dirty:      0 kB
Swap:               0 kB
SwapPss:            0 kB
VmFlags: rd ex mr mw me sd
7f8c8c2000-7f8c8c4000 r-xp 00002000 103:0a 5678  /system/lib64/libart.so
Size:               8 kB
Rss:                8 kB
Pss:                8 kB
Shared_Clean:       8 kB
Shared_Dirty:       0 kB
Private_Clean:      0 kB
Private_Dirty:      0 kB
Swap:               0 kB
SwapPss:            0 kB
VmFlags: rd ex mr mw me sd
>>PID_END: 1111
>>PID_START: 2222 com.android.systemui
12c00000-12e00000 r--p 00000000 103:0b 9012  [heap]
Size:               128 kB
Rss:                100 kB
Pss:                90 kB
Shared_Clean:       0 kB
Shared_Dirty:       0 kB
Private_Clean:      100 kB
Private_Dirty:      0 kB
Swap:               0 kB
SwapPss:            0 kB
VmFlags: rd wr mr mw me sd
>>PID_END: 2222
"""
    with tempfile.NamedTemporaryFile(
        mode='w', delete=False, suffix='.txt'
    ) as smaps_file:
      smaps_file.write(smaps_data)
      smaps_file.close()
      with tempfile.NamedTemporaryFile(
          mode='rb', delete=False, suffix='.pprof'
      ) as output_file:
        output_file.close()
        smaps_pprof.main(
            ['--smaps', smaps_file.name, '--output', output_file.name]
        )

        # Validate the generated pprof file
        self.assertTrue(os.path.exists(output_file.name))
        self.assertGreater(os.path.getsize(output_file.name), 0)

        with gzip.open(output_file.name, 'rb') as f:
          profile = profile_pb2.Profile()
          profile.ParseFromString(f.read())

          string_table = profile.string_table
          self.assertIn('system_server', string_table)
          self.assertIn('com.android.systemui', string_table)
          self.assertIn('/system/lib64/libart.so', string_table)
          self.assertIn('[heap]', string_table)
          self.assertIn('Pss', string_table)
          self.assertIn('Rss', string_table)

          # Check for a specific sample
          found_system_server_pss = False
          for sample in profile.sample:
            location = profile.location[sample.location_id[0] - 1]
            function = profile.function[location.line[0].function_id - 1]
            if string_table[function.name] == '/system/lib64/libart.so':
              pss_index = -1
              for i, sample_type in enumerate(profile.sample_type):
                if string_table[sample_type.type] == 'Pss':
                  pss_index = i
                  break
              if pss_index != -1 and sample.value[pss_index] == 10:
                found_system_server_pss = True
                break
          self.assertTrue(found_system_server_pss)

        os.unlink(output_file.name)
      os.unlink(smaps_file.name)


if __name__ == '__main__':
  unittest.main()
