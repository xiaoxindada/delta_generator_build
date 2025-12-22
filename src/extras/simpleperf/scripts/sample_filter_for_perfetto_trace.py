#!/usr/bin/env python3
#
# Copyright (C) 2025 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Generates a simpleperf sample filter file based on event timings from a Perfetto trace.

This script uses trace_processor to query a Perfetto trace for specific slice events
matching a regex. It then creates a filter file containing the time ranges of these
events. This filter can be used with tools like pprof_proto_generator.py to focus
performance analysis on specific periods of interest.

Filter file format is documented in docs/sample_filter.md.

Example: Find samples that occurred on the same threads only during the specified events.
$ sample_filter_for_perfetto_trace.py trace.perfetto-trace \
    --event-filter-regex "CriticalEventRegex"
$ pprof_proto_generator.py --filter-file filter.txt

Example: Find samples that occurred on all threads only during the specified events.
$ sample_filter_for_perfetto_trace.py trace.perfetto-trace \
    --event-filter-regex "GlobalCriticalEventRegex" --global-event
$ pprof_proto_generator.py --filter-file filter.txt
"""

import argparse
import csv
import shutil
import subprocess
from pathlib import Path
from typing import Dict, List, Optional

from simpleperf_utils import BaseArgumentParser


def get_trace_processor_path() -> Optional[str]:
    """
    Finds the path to the trace_processor executable, downloading it if necessary.

    Searches for 'trace_processor_shell', 'trace_processor' in PATH, and
    './trace_processor' in the current directory before attempting to download.

    Returns:
        The path to the executable or None if not found and couldn't be downloaded.
    """
    for executable in ["trace_processor_shell", "trace_processor"]:
        path = shutil.which(executable)
        if path:
            return path

    local_path = Path("./trace_processor")
    if local_path.is_file():
        return "./trace_processor"

    print("trace_processor not found. Attempting to download...")
    try:
        download_cmd = "curl -LO https://get.perfetto.dev/trace_processor"
        subprocess.run(download_cmd, shell=True, check=True, capture_output=True)
        chmod_cmd = "chmod +x ./trace_processor"
        subprocess.run(chmod_cmd, shell=True, check=True)
        print("Download complete.")
        return "./trace_processor"
    except subprocess.CalledProcessError as e:
        print(f"Error downloading trace_processor: {e.stderr.decode().strip()}")
    except FileNotFoundError:
        print("Error: 'curl' is not installed. Please install it to download trace_processor.")
    return None


def run_perfetto_query(
    trace_processor_path: str, trace_file: Path, query: str
) -> List[Dict[str, str]]:
    """
    Executes a SQL query on a Perfetto trace file using trace_processor.

    Args:
        trace_processor_path: Path to the trace_processor executable.
        trace_file: Path to the .perfetto-trace file.
        query: The SQL query to execute.

    Returns:
        A list of dictionaries, where each dictionary is a row from the query result.
        Returns an empty list if an error occurs.
    """
    if not trace_file.is_file():
        print(f"Error: Trace file not found at '{trace_file}'")
        return []

    command = [trace_processor_path, str(trace_file), "--query-string", query]
    print(f"\n--- Running Query ---\n{query.strip()}\n---------------------")

    try:
        result = subprocess.run(
            command, capture_output=True, text=True, check=True, encoding="utf-8"
        )
        lines = result.stdout.strip().splitlines()
        if not lines:
            return []

        # The header can have quotes; remove them for clean keys.
        header = [h.strip().replace('"', "") for h in lines[0].split(",")]
        reader = csv.DictReader(lines[1:], fieldnames=header)
        return list(reader)

    except FileNotFoundError:
        print(f"Error: '{trace_processor_path}' not found.")
    except subprocess.CalledProcessError as e:
        print("Error running trace_processor:")
        print(f"Return Code: {e.returncode}")
        print(f"Stderr:\n{e.stderr.strip()}")
    return []

def main():
    """Main entry point of the script."""
    parser = BaseArgumentParser(description=__doc__)
    parser.add_argument("trace_file", type=Path, help="Path to the Perfetto trace file.")
    parser.add_argument(
        "--event-filter-regex",
        type=str,
        required=True,
        help="Regular expression to filter slice names.",
    )
    parser.add_argument(
        "--global-event",
        action="store_true",
        help="""
        Create a global time range for all threads. By default, filters are
        per-thread within the event's time range.
        """,
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default="filter.txt",
        help="Path to the output filter file (default: filter.txt).",
    )
    args = parser.parse_args()

    if not args.trace_file.exists():
        print(f"Error: Trace file not found at '{args.trace_file}'")
        exit(1)

    trace_processor_path = get_trace_processor_path()
    if not trace_processor_path:
        print("Could not find or download trace_processor. Exiting.")
        exit(1)

    # This query joins slices with their corresponding threads and calculates the
    # BOOTTIME timestamp by applying the offset between MONOTONIC and BOOTTIME clocks.
    query = f"""
    WITH clock_offset AS (
      SELECT
        (SELECT MIN(clock_value) FROM clock_snapshot WHERE clock_name = 'MONOTONIC') -
        (SELECT MIN(clock_value) FROM clock_snapshot WHERE clock_name = 'BOOTTIME')
        AS val
    )
    SELECT
      s.name,
      t.name AS thread_name,
      t.tid,
      s.ts + (SELECT val FROM clock_offset) AS start_ts,
      s.ts + s.dur + (SELECT val FROM clock_offset) AS end_ts
    FROM
      slice s
    JOIN
      thread_track tr ON s.track_id = tr.id
    JOIN
      thread t ON tr.utid = t.utid
    WHERE
      s.name REGEXP '{args.event_filter_regex}';
    """

    slices = run_perfetto_query(trace_processor_path, args.trace_file, query)

    if not slices:
        print("No matching slices found.")
        return

    output_lines = [f"// Found {len(slices)} matching slices."]
    for s in slices:
        output_lines.append(f"// Name: {s['name']}")
        output_lines.append(f"// Thread: {s['thread_name']} ({s['tid']})")
        if args.global_event:
            output_lines.append(f"GLOBAL_BEGIN {s['start_ts']}")
            output_lines.append(f"GLOBAL_END {s['end_ts']}")
        else:
            output_lines.append(f"THREAD_BEGIN {s['tid']} {s['start_ts']}")
            output_lines.append(f"THREAD_END {s['tid']} {s['end_ts']}")
        output_lines.append("")

    output_text = "\n".join(output_lines)
    with open(args.output, "w") as f:
        f.write(output_text)
    print(f"Filter file written to {args.output}")


if __name__ == "__main__":
    main()
