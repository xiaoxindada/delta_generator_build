#!/usr/bin/env python3
#
# Copyright (C) 2024 The Android Open Source Project
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

import argparse
import functools
import pathlib
import subprocess
import re
from pathlib import Path
from typing import Callable, Dict, Optional, Tuple

import etm_types as etm
from simpleperf_report_lib import ReportLib
from simpleperf_utils import bytes_to_str, BinaryFinder, EtmContext, log_exit, ReadElf, Objdump, ToolFinder


class Tracer:
    def __init__(self, lib: ReportLib, binary_finder: BinaryFinder, objdump: Objdump) -> None:
        self.abort = False

        self.last_timestamp: Optional[int] = None
        self.lost_decoding = False

        self.context = EtmContext()

        self.instructions = 0
        self.cycles = 0

        self.lib = lib
        self.binary_finder = binary_finder
        self.objdump = objdump

        self.disassembly: Dict[str, Dict[int, str]] = {}

    def __call__(self, trace_id: int, elem: etm.GenericTraceElement) -> None:
        if self.abort:
            return

        try:
            self.process(trace_id, elem)
        except Exception as e:
            self.abort = True
            raise e

    def reset_trace(self) -> None:
        self.context.clear()
        self.lost_decoding = False
        self.last_timestamp = None

    def process(self, trace_id: int, elem: etm.GenericTraceElement) -> None:
        if elem.elem_type == etm.ElemType.TRACE_ON:
            self.reset_trace()
            return

        elif elem.elem_type == etm.ElemType.NO_SYNC:
            print("NO_SYNC: trace is lost, possibly due to overflow.")
            self.reset_trace()
            return

        elif elem.elem_type == etm.ElemType.PE_CONTEXT:
            if self.context.update(elem.context):
                print("New Context: ", end='')
                self.context.print()
                if self.context.tid:
                    process = self.lib.GetThread(self.context.tid)
                    if process:
                        print(f"PID: {process[0]}, TID: {process[1]}, comm: {process[2]}")
            return

        elif elem.elem_type == etm.ElemType.ADDR_NACC:
            if not self.lost_decoding:
                self.lost_decoding = True
                mapped = self.lib.ConvertETMAddressToVaddrInFile(trace_id, elem.st_addr)
                if mapped:
                    print(f'ADDR_NACC: path {mapped[0]} cannot be decoded!')
                else:
                    print(f'ADDR_NACC: trace address {hex(elem.st_addr)} is not mapped!')
            return

        elif elem.elem_type == etm.ElemType.EXCEPTION:
            print(f'Exception: "{elem.exception_type()}" ({elem.exception_number})!' +
                  (f" (Excepted return: {hex(elem.en_addr)})" if elem.excep_ret_addr else ""))
            if elem.exception_number == 3 and elem.excep_ret_addr:
                # For traps, output the instruction that it trapped on; it is usual to return to a
                # different address, to skip the trapping instruction.
                mapped = self.lib.ConvertETMAddressToVaddrInFile(trace_id, elem.en_addr)
                if mapped:
                    print("Trapped on:")
                    start_path, start_offset = mapped
                    b = str(self.find_binary(start_path))
                    self.print_disassembly(b, start_offset, start_offset)
                else:
                    print(f"Trapped on unmapped address {hex(elem.en_addr)}!")
            return

        elif elem.elem_type == etm.ElemType.TIMESTAMP:
            if self.last_timestamp != elem.timestamp:
                self.last_timestamp = elem.timestamp
                print(f'Current timestamp: {elem.timestamp}')
            return

        elif elem.elem_type == etm.ElemType.CYCLE_COUNT and elem.has_cc:
            print("Cycles: ", elem.cycle_count)
            self.cycles += elem.cycle_count
            return

        elif elem.elem_type != etm.ElemType.INSTR_RANGE:
            return

        self.lost_decoding = False
        self.instructions += elem.num_instr_range
        start_path, start_offset = self.lib.ConvertETMAddressToVaddrInFile(
            trace_id, elem.st_addr) or ("", 0)
        end_path, end_offset = self.lib.ConvertETMAddressToVaddrInFile(
            trace_id, elem.en_addr - elem.last_instr_sz) or ("", 0)

        error_messages = []
        if not start_path:
            error_messages.append(f"Couldn't determine start path for address {elem.st_addr}!")
        if not end_path:
            error_messages.append(
                f"Couldn't determine start path for address {elem.en_addr - elem.last_instr_sz}!")
        if error_messages:
            raise RuntimeError(' '.join(error_messages))

        if start_path == '[kernel.kallsyms]':
            start_path = 'vmlinux'

        cpu = (trace_id - 0x10) // 2
        print(f'CPU{cpu} {start_path}: {hex(start_offset)} -> {hex(end_offset)}')
        b = str(self.find_binary(start_path))
        self.print_disassembly(b, start_offset, end_offset)
        if not elem.last_instr_cond and not elem.last_instr_exec:
            raise RuntimeError(f'Wrong binary! Unconditional branch at {hex(end_offset)}'
                               f' in {start_path} was not taken!')

    @functools.lru_cache
    def find_binary(self, path: str) -> Optional[Path]:
        # binary_finder.find_binary opens the binary to check if it is an ELF, and runs readelf on
        # it to ensure that the build ids match. This is too much to do in our hot loop, therefore
        # its result should be cached.
        buildid = self.lib.GetBuildIdForPath(path)
        return self.binary_finder.find_binary(path, buildid)

    def print_disassembly(self, path: str, start: int, end: int) -> None:
        disassembly = self.disassemble(path)
        if not disassembly:
            log_exit(f"Failed to disassemble '{path}'!")

        for i in range(start, end + 4, 4):
            print(disassembly[i])

    def disassemble(self, path: str) -> Dict[int, str]:
        if path in self.disassembly:
            return self.disassembly[path]

        dso_info = self.objdump.get_dso_info(path, None)
        self.disassembly[path] = self.objdump.disassemble_whole(dso_info)
        return self.disassembly[path]


def get_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Generate instruction trace from ETM data.')
    parser.add_argument('-i', '--record_file', nargs=1, default=['perf.data'], help="""
                        Set profiling data file to process.""")
    parser.add_argument('--binary_cache', nargs=1, default=["binary_cache"], help="""
                        Set path to the binary cache.""")
    parser.add_argument('--ndk_path', nargs=1, help='Find tools in the ndk path.')
    return parser.parse_args()


def main() -> None:
    args = get_args()

    binary_cache_path = args.binary_cache[0]
    if not pathlib.Path(binary_cache_path).is_dir():
        log_exit(f"Binary cache '{binary_cache_path}' is not a directory!")
        return

    ndk_path = args.ndk_path[0] if args.ndk_path else None

    lib = ReportLib()
    try:
        lib.SetRecordFile(args.record_file[0])
        lib.SetSymfs(binary_cache_path)
        lib.SetLogSeverity('error')

        binary_finder = BinaryFinder(binary_cache_path, ReadElf(ndk_path))
        objdump = Objdump(ndk_path, binary_finder)

        callback = Tracer(lib, binary_finder, objdump)

        lib.SetETMCallback(callback)
        while not callback.abort and lib.GetNextSample():
            pass

        if callback.cycles:
            print("Total cycles:", callback.cycles)
        print("Total decoded instructions:", callback.instructions)

    finally:
        lib.Close()


if __name__ == '__main__':
    main()
