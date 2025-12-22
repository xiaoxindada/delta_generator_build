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
"""A script to collect smaps data from an Android device and format it as a pprof profile.

The script organizes the smaps data hierarchically in the generated pprof
report.
The structure is as follows:
- The top level represents the process name.
- The next level represents the memory mapping name (e.g., '[heap]',
'/system/lib/libart.so').

The various memory metrics from smaps (e.g., Pss, Rss, Private_Dirty) are stored
as different sample types in the pprof profile. This allows you to select and
visualize different metrics in the pprof UI.

Additionally, the 'VmFlags' and 'Permissions' for each mapping are stored as
labels in the pprof report, which can be used for filtering and analysis.
"""

import argparse
import gzip
import io
import os
import re
import subprocess
import sys

# BEGIN profile_pb2 import
try:
  import profile_pb2
except ImportError:
  # Allow running the script from a checkout, not as a python_binary_host
  ANDROID_BUILD_TOP = os.environ.get('ANDROID_BUILD_TOP')
  if ANDROID_BUILD_TOP:
    import_path = ANDROID_BUILD_TOP + '/system/extras/simpleperf/scripts'
  else:
    import_path = os.getcwd() + '/system/extras/simpleperf/scripts'
  sys.path.append(import_path)
  import profile_pb2
# END profile_pb2 import


# Constants for marking process data boundaries.
PID_START_MARKER = '>>PID_START:'
PID_END_MARKER = '>>PID_END:'


HEADER_RE = re.compile(
    r'^[0-9a-f]+-[0-9a-f]+\s+(....)\s+\w+\s+\w+:\w+\s+\d+\s*(.*)'
)
VMFLAGS_RE = re.compile(r'VmFlags:\s+(.*)')
KB_VALUE_RE = re.compile(r'(\w+):\s+(\d+)\s+kB')


# Constants for keys in the mapping dictionary
MAPPING_NAME_KEY = 'name'
MAPPING_PERMISSIONS_KEY = 'Permissions'
MAPPING_VMFLAGS_KEY = 'VmFlags'
MAPPING_PROCESS_NAME_KEY = 'process_name'


class PprofReportGenerator:
  """A class to incrementally build a pprof report."""

  def __init__(self):
    self.profile = profile_pb2.Profile()
    self.string_table = ['']
    self.string_map = {'': 0}  # Maps string to its ID for fast lookups
    self.functions = {}
    self.locations = {}
    self.mappings_dict = {}
    self.sample_type_map = {}  # Maps metric name to its index

  def get_string_id(self, s):
    """Adds a string to the string table and returns its ID.

    Optimized for performance.
    """
    if s in self.string_map:
      return self.string_map[s]

    new_id = len(self.string_table)
    self.string_table.append(s)
    self.string_map[s] = new_id
    return new_id

  def add_mappings(self, mappings_data):
    """Adds a list of parsed mapping data to the profile."""
    for mapping_data in mappings_data:
      if not mapping_data:
        continue

      # Dynamically discover and define sample types
      for key in mapping_data.keys():
        if key not in [
            MAPPING_PROCESS_NAME_KEY,
            MAPPING_NAME_KEY,
            MAPPING_PERMISSIONS_KEY,
            MAPPING_VMFLAGS_KEY,
        ]:
          if key not in self.sample_type_map:
            st = self.profile.sample_type.add()
            st.type = self.get_string_id(key)
            st.unit = self.get_string_id('kilobytes')
            self.sample_type_map[key] = len(self.profile.sample_type) - 1

      sample = self.profile.sample.add()
      # Ensure the value list is the correct size, initialized to 0
      sample.value.extend([0] * len(self.profile.sample_type))

      # Populate values based on the dynamic map
      for key, value in mapping_data.items():
        if key in self.sample_type_map:
          index = self.sample_type_map[key]
          sample.value[index] = value

      proc_name = mapping_data[MAPPING_PROCESS_NAME_KEY]
      mapping_name = mapping_data.get(MAPPING_NAME_KEY, '[anon]')

      stack = [proc_name, mapping_name]
      location_ids = []

      if mapping_name not in self.mappings_dict:
        mapping = self.profile.mapping.add()
        mapping.id = len(self.profile.mapping)
        mapping.filename = self.get_string_id(mapping_name)
        self.mappings_dict[mapping_name] = mapping.id
      mapping_id = self.mappings_dict[mapping_name]

      for i, frame in enumerate(stack):
        is_leaf = i == len(stack) - 1
        func_unique_id = f"{'/'.join(stack[:i])}/{frame}"

        if func_unique_id not in self.functions:
          func = self.profile.function.add()
          func.id = len(self.profile.function)
          func.name = self.get_string_id(frame)
          self.functions[func_unique_id] = func.id

        func_id = self.functions[func_unique_id]

        loc_key = (func_id, mapping_id if is_leaf else 0)
        if loc_key not in self.locations:
          loc = self.profile.location.add()
          loc.id = len(self.profile.location)
          if is_leaf:
            loc.mapping_id = mapping_id
          line = loc.line.add()
          line.function_id = func_id
          self.locations[loc_key] = loc.id

        location_ids.append(self.locations[loc_key])

      sample.location_id.extend(reversed(location_ids))

      if MAPPING_VMFLAGS_KEY in mapping_data:
        label = sample.label.add()
        label.key = self.get_string_id('VmFlags')
        label.str = self.get_string_id(mapping_data[MAPPING_VMFLAGS_KEY])

      if MAPPING_PERMISSIONS_KEY in mapping_data:
        label = sample.label.add()
        label.key = self.get_string_id('Permission')
        label.str = self.get_string_id(mapping_data[MAPPING_PERMISSIONS_KEY])

  def write_report(self, output_path, argv):
    """Writes the final report to disk."""
    comment = f"{os.path.basename(argv[0])} {' '.join(argv[1:])}"
    self.profile.comment.append(self.get_string_id(comment))
    self.profile.string_table.extend(self.string_table)

    with gzip.open(output_path, 'wb') as f:
      f.write(self.profile.SerializeToString())


def run_command(command):
  """Helper to run a shell command with common parameters."""
  try:
    return subprocess.run(
        command,
        shell=True,
        capture_output=True,
        text=True,
        check=True,
        errors='ignore',
    )
  except subprocess.CalledProcessError as e:
    sys.stderr.write(
        f'Error: Command failed with exit code {e.returncode}.\n'
    )
    sys.stderr.write(f'Command: {e.cmd}\n')
    sys.stderr.write(f'Stderr: {e.stderr}\n')
    sys.exit(1)
  except FileNotFoundError:
    sys.stderr.write(
        f"Error: Command '{command.split()[0]}' not found. Please ensure it is in your PATH.\n"
    )
    sys.exit(1)


def parse_process_chunk(process_chunk, proc_name):
  """Parses a chunk of smaps data for a single process.

  Args:
    process_chunk: A string containing the smaps data for a single process.
    proc_name: The name of the process.

  Returns:
    A tuple containing:
      - The process name.
      - A list of dictionaries, where each dictionary represents a memory
        mapping and contains its associated smaps metrics.
  """
  lines = process_chunk.splitlines()
  if not lines:
    return None, []

  mappings = []
  current_mapping = {}

  def process_previous_mapping():
    if current_mapping:
      mappings.append(current_mapping)

  for line in lines:
    header_match = HEADER_RE.match(line)
    if header_match:
      process_previous_mapping()
      permissions_str, name = header_match.groups()
      current_mapping = {
          MAPPING_NAME_KEY: name.strip() if name else '[anon]',
          MAPPING_PERMISSIONS_KEY: permissions_str,
          MAPPING_VMFLAGS_KEY: '',
          MAPPING_PROCESS_NAME_KEY: proc_name,
      }
      continue

    if not current_mapping:
      continue

    vmflags_match = VMFLAGS_RE.match(line)
    if vmflags_match:
      current_mapping[MAPPING_VMFLAGS_KEY] = vmflags_match.group(1).strip()
      continue

    kb_value_match = KB_VALUE_RE.match(line)
    if kb_value_match:
      key, value = kb_value_match.groups()
      current_mapping[key] = int(value)

  process_previous_mapping()
  return proc_name, mappings


def process_smaps_input(input_stream, report_generator):
  """Processes smaps data from a given input stream."""
  process_count = 0
  current_process_chunk = []
  proc_name = ''
  for line in input_stream:
    if line.startswith(PID_START_MARKER):
      current_process_chunk = []
      parts = line.split(None, 2)
      proc_name = parts[2].strip() if len(parts) > 2 else f'PID_{parts[1]}'
    elif line.startswith(PID_END_MARKER):
      process_count += 1
      _, parsed_mappings = parse_process_chunk(
          '\n'.join(current_process_chunk), proc_name
      )
      if parsed_mappings:
        report_generator.add_mappings(parsed_mappings)
        sys.stdout.write(f'Processed: {process_count} | Current: {proc_name}\r')
        sys.stdout.flush()
    else:
      current_process_chunk.append(line)
  print()


def ensure_adb_root():
  """Ensures adb connection has root access.

  Raises:
    subprocess.CalledProcessError: If an adb command fails.
    FileNotFoundError: If 'adb' command is not found.
    SystemExit: If root access cannot be obtained.
  """ # Attempt to restart adbd as root. This command will block until adbd restarts.
  run_command('adb root')
  # Wait for the device to reconnect after adb root.
  run_command('adb wait-for-device')
  # Verify root access by checking the user ID.
  result_id = run_command('adb shell id -u')
  if not result_id or result_id.stdout.strip() != '0':
    sys.stderr.write(
        'Error: Failed to get root access via adb. Please ensure your device'
        ' is rooted or adbd can be restarted as root.\n'
    )
    sys.exit(1)


def main(argv):
  parser = argparse.ArgumentParser(
      description=(
          'Generate a pprof file of all smaps metrics for all processes.'
      )
  )
  parser.add_argument(
      '--output',
      default='smaps_report.pprof',
      help='Path to save the generated pprof file.',
  )
  group = parser.add_mutually_exclusive_group()
  group.add_argument('--smaps', help='Path to the local smaps output file.')
  group.add_argument('--pid', help='Analyze a single PID for debugging.')
  args = parser.parse_args(argv)

  report_generator = PprofReportGenerator()

  if args.smaps:
    try:
      with open(args.smaps, 'r', errors='ignore') as f:
        process_smaps_input(f, report_generator)
    except FileNotFoundError:
      sys.stderr.write(f'Error: smaps file not found at {args.smaps}.\n')
      sys.exit(1)
  # If --smaps is not provided, we need to connect to a device via adb.
  else:
    ensure_adb_root()

    if args.pid:
      device_script = f"""
      pid={args.pid}
      name=$(ps -p $pid -o ARGS | tail -n 1)
      echo "{PID_START_MARKER} $pid $name"
      if [ -f /proc/$pid/smaps ]; then cat /proc/$pid/smaps; fi
      echo "{PID_END_MARKER} $pid"
      """
    else:
      # Get all usermode processes and their smaps data.
      # Skip kernel processes (have names like "[kthreadd]") since they don't have smaps.
      device_script = rf"""
      ps -A -o PID,ARGS | tail -n +2 | while read -r pid name; do
        if [[ "$name" == \[*\] ]]; then
          continue
        fi
        echo "{PID_START_MARKER} $pid $name"
        if [ -f /proc/$pid/smaps ]; then cat /proc/$pid/smaps; fi
        echo "{PID_END_MARKER} $pid"
      done
      """
    command = f"adb shell '{device_script}'"

    result = run_command(command)
    process_smaps_input(io.StringIO(result.stdout), report_generator)

  print(f'Generating pprof report to {args.output}...')
  report_generator.write_report(args.output, sys.argv)

  print('Done.')
  print(f'Report saved to: {os.path.abspath(args.output)}')
  print(f'Tip: To upload to pprof web UI, run:')
  print(f'pprof -flame {os.path.abspath(args.output)}')


if __name__ == '__main__':
  main(sys.argv[1:])
