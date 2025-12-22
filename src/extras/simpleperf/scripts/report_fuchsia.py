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
import pathlib
import subprocess
from bisect import bisect
from collections import deque
from dataclasses import dataclass
from itertools import chain
from math import ceil
from typing import BinaryIO, Callable, Deque, Dict, List, Optional, Tuple

import etm_types as etm
from simpleperf_report_lib import ReportLib
from simpleperf_utils import bytes_to_str, BinaryFinder, EtmContext, log_exit, ReadElf, Objdump, ToolFinder


class TraceWriter:
    """A writer for the Fuchsia trace format:
       https://fuchsia.dev/fuchsia-src/reference/tracing/trace-format
    """

    def __init__(self, output: BinaryIO):
        self.output = output

        # The following dictionaries are for keeping track of the fuchsia records already made, to
        # use them as references. They map the "value" of the record to their index that can be used
        # as the reference.

        # Strings are strings.
        self.strings: Dict[str, int] = {}
        # Providers are just strings.
        self.providers: Dict[str, int] = {}
        # Kernel objects are a type and a name.
        self.kernel_objects: Dict[Tuple[int, str], int] = {}
        # Threads are made of two kernel object ids, the first is for the process, the second for
        # the thread.
        self.threads: Dict[Tuple[int, int], int] = {}

    def _write_bits(self, desc: List[Tuple[int, int, int]]) -> None:
        v = 0
        for (start, end, value) in desc:
            old_value = value
            # Check if the value fits into its field.
            value &= (1 << (end+1)) - 1
            assert old_value == value
            v |= value << start
        self.output.write(v.to_bytes(8, 'little'))

    def _write_padded(self, array: bytearray) -> None:
        pad = 8 - (len(array) % 8)
        if pad == 8:
            pad = 0

        self.output.write(array)
        self.output.write(bytearray(pad))

    def magic(self) -> None:
        self._write_bits([(0, 3, 0),
                          (4, 15, 1),
                          (16, 19, 4),
                          (20, 23, 0),
                          (24, 55, 0x16547846),
                          (56, 63, 0)])

    def switch_provider(self, name: str) -> None:
        if name not in self.providers:
            n = bytearray(name, 'utf-8')
            length = ceil(len(n) / 8)
            provider_id = len(self.providers)
            self._write_bits([(0, 3, 0),
                              (4, 15, 1 + length),
                              (20, 51, provider_id),
                              (52, 59, len(n)),
                              (60, 63, 0)])
            self._write_padded(n)
            self.providers[name] = provider_id

        provider_id = self.providers[name]
        self._write_bits([(0, 3, 0),
                          (4, 15, 1),
                          (16, 19, 2),
                          (20, 51, provider_id),
                          (52, 63, 0)])

    def encode_string(self, value: str) -> int:
        if value in self.strings:
            return self.strings[value]

        if len(self.strings) >= 0x7fff - 1:
            raise RuntimeError("Ran out of space in the string table!")

        n = bytearray(value, 'utf-8')
        length = ceil(len(n) / 8)
        string_id = len(self.strings) + 1
        self._write_bits([(0, 3, 2),
                          (4, 15, 1 + length),
                          (16, 30, string_id),
                          (31, 31, 0),
                          (32, 46, len(n)),
                          (47, 47, 0),
                          (48, 63, 0)])
        self._write_padded(n)
        self.strings[value] = string_id
        return string_id

    def kernel_object(self, kobj_type: int, name: str, process: Optional[int] = None) -> int:
        if (kobj_type, name) in self.kernel_objects:
            return self.kernel_objects[(kobj_type, name)]

        name_ref = self.encode_string(name)
        arg_name = 0
        if process:
            arg_name = self.encode_string("process")

        self._write_bits([(0, 3, 7),
                          (4, 15, 4 if process else 2),
                          (16, 23, kobj_type),
                          (24, 39, name_ref),
                          (40, 43, 1 if process else 0),
                          (44, 63, 0)])

        koid = len(self.kernel_objects)
        self._write_bits([(0, 63, koid)])

        if process:
            self._write_bits([(0, 3, 8),
                              (4, 15, 2),
                              (16, 31, arg_name),
                              (32, 63, 0)])
            self._write_bits([(0, 63, process)])

        self.kernel_objects[(kobj_type, name)] = koid
        return koid

    def thread(self, process_koid: int, thread_koid: int) -> int:
        if (process_koid, thread_koid) in self.threads:
            return self.threads[(process_koid, thread_koid)]

        thread_index = len(self.threads) + 1
        self._write_bits([(0, 3, 3),
                          (4, 15, 3),
                          (16, 23, thread_index),
                          (24, 63, 0)])
        self._write_bits([(0, 63, process_koid)])
        self._write_bits([(0, 63, thread_koid)])
        self.threads[(process_koid, thread_koid)] = thread_index
        return thread_index

    def duration(self, begin: bool, thread: int, category: Optional[str],
                 name: Optional[str], timestamp: int) -> None:
        category_ref = self.encode_string(category) if category else 0
        name_ref = self.encode_string(name) if name else 0
        self._write_bits([(0, 3, 4),
                          (4, 15, 2),
                          (16, 19, 2 if begin else 3),
                          (20, 23, 0),
                          (24, 31, thread),
                          (32, 47, category_ref),
                          (48, 63, name_ref)])
        self._write_bits([(0, 63, timestamp)])


@dataclass
class Event:
    call: bool
    name: str
    timestamp: Optional[int]


class Stacker:
    """Stacker tries to keep track of how a thread's stack changes, and uses TraceWriter to
       generate the corresponding Fuchsia trace.
    """

    def __init__(self, writer: TraceWriter, thread: int):
        self.writer = writer
        self.thread = thread
        self.stack: List[str] = []
        self.events: Deque[Event] = deque()
        self.early_events: List[Event] = []
        self.waiting: List[Event] = []

        self.reset()

    def reset(self) -> None:
        self.was_call = False
        self.was_ret = False
        self.was_plt = False
        self.last_symbol: Optional[str] = None
        self.exception_name: Optional[str] = None
        self.excepted_return: Optional[int] = None
        self.first_timestamp: Optional[int] = None
        self.last_timestamp: Optional[int] = None

    def flush(self) -> None:
        last = -1
        for event in chain(self.early_events, self.events):
            if event.timestamp:
                assert last <= event.timestamp
                last = event.timestamp
                self.writer.duration(event.call, self.thread, None, event.name, event.timestamp)

        self.early_events.clear()
        self.events.clear()
        self.events.extend(self.waiting)

    def check_waiting(self, timestamp: Optional[int]) -> None:
        if not timestamp:
            return

        if not self.first_timestamp:
            self.first_timestamp = timestamp
        self.last_timestamp = timestamp

        for event in self.waiting:
            event.timestamp = timestamp

        self.waiting.clear()

    def call(self, symbol: str, timestamp: Optional[int], front: bool = False) -> None:
        event = Event(True, symbol, timestamp)

        if not timestamp:
            self.waiting.append(event)

        for entry in self.stack:
            if entry.startswith('Exception: '):
                raise RuntimeError("New call while an exception is still on the stack!",
                                   symbol, self.stack)

        if front and self.events and not self.events[0].call:
            # If the front of the events is a return, and we need to fake a call at the start, the
            # returns will have to come before the call, otherwise Perfetto can't deal with it.
            assert(
                not timestamp or not self.events[0].timestamp or self.events[0].timestamp <=
                timestamp)
            while self.events and not self.events[0].call:
                self.early_events.append(self.events.popleft())

        self.stack.append(symbol)
        insert = self.events.appendleft if front else self.events.append
        insert(event)

    def ret(self, symbol: str, timestamp: Optional[int]) -> None:
        event = Event(False, symbol, timestamp)
        if not timestamp:
            self.waiting.append(event)

        if self.stack and self.stack[-1] != symbol:
            raise RuntimeError('Top of the stack does not match the function being returned from! '
                               f'Current function: {symbol}, top of the stack: {self.stack[-1]}')

        if self.stack:
            self.stack.pop()
        self.events.append(event)

    def lost_stack(self, timestamp: Optional[int]) -> None:
        self.check_waiting(timestamp)

        for symbol in reversed(self.stack):
            self.ret(symbol, timestamp)
        self.stack.clear()
        self.flush()
        self.reset()

    def gap(self, timestamp: Optional[int]) -> None:
        self.last_timestamp = None
        if self.exception_name:
            return

        if self.last_symbol and self.last_symbol.endswith('@plt'):
            self.was_plt = True
            return

        self.lost_stack(timestamp)

    def timestamp(self, timestamp: Optional[int]) -> None:
        self.check_waiting(timestamp)

    def exception(self, timestamp: Optional[int],
                  name: str, excepted_return: Optional[int]) -> None:
        self.check_waiting(timestamp)

        if self.exception_name:
            if excepted_return is not None and self.excepted_return == excepted_return:
                # Same return, mark the end of the previous but carry on:
                self.ret(self.exception_name, timestamp)
            else:
                # Interrupted, but for a different return. This most likely means some trace was
                # lost. Drop the stack.
                self.lost_stack(timestamp)

        for e in self.stack:
            if e.startswith('Exception '):
                raise RuntimeError("Exception while another exception is already on the stack!",
                                   name, self.stack)

        self.exception_name = name
        self.excepted_return = excepted_return
        self.call(name, timestamp)
        if timestamp:
            self.last_timestamp = timestamp

    def instr_range(self, timestamp: Optional[int], start: str, start_addr: int,
                    end: str, _end_addr: int, isubtype: etm.InstrSubtype) -> None:
        self.check_waiting(timestamp)

        if self.exception_name:
            self.ret(self.exception_name, timestamp)
            self.exception_name = None

            if start_addr != self.excepted_return:
                # If we are in the same symbol, assume we managed to get back safe and sound.
                # Otherwise, drop the stack.
                if not self.stack or self.stack[-1] != start:
                    self.lost_stack(self.last_timestamp)

        if self.was_plt:
            self.was_plt = False
            if len(self.stack) > 1 and self.stack[-2] == start:
                # When was_plt was set, last_symbol should have been too.
                assert self.last_symbol
                self.ret(self.last_symbol, timestamp)
                self.last_symbol = start
            else:
                self.lost_stack(timestamp)

        if self.last_symbol == 'art_quick_do_long_jump' and self.last_symbol != start:
            # Art's long jump rewinds the stack to a previous state.
            self.lost_stack(timestamp)

        # check_waiting already set the timestamps, but lost_stack above might have nulled them. Set
        # them again to make sure everything below can use the timestamps correctly.
        self.first_timestamp = self.first_timestamp or timestamp
        self.last_timestamp = timestamp or self.last_timestamp

        if not self.was_call and not self.was_ret and self.last_symbol != start:
            # If the symbol changed without us detecting a call or a return, we recognize two cases:
            # if there's a last symbol saved, this was a tail call, and we emit a return and a call.
            # Otherwise, this is the beginning of a new reconstruction of a stack, and we emit only
            # the call.
            if self.last_symbol:
                # Tail call.
                self.ret(self.last_symbol, timestamp)
            self.call(start, timestamp)
        if self.was_ret and not self.stack:
            # If we have just returned into a function but the stack is empty, pretend that the
            # current (new) function has been running since the first known timestamp.
            self.call(start, self.first_timestamp, front=True)
        if self.was_call:
            # If the last instruction of the previous instruction range was a call (branch with
            # link), emit a call.
            self.call(start, timestamp)
        if start != end:
            # If for some reason the symbol changes inside an instruction range, pretend it was a
            # tail call, even though this really shouldn't happen.
            self.ret(start, timestamp)
            self.call(end, timestamp)
        if isubtype == etm.InstrSubtype.V8_RET:
            # If the last instruction of the range is a return, we are returning from the current
            # symbol.
            self.ret(end, timestamp)

        self.was_ret = isubtype == etm.InstrSubtype.V8_RET
        self.was_call = isubtype == etm.InstrSubtype.BR_LINK
        self.last_symbol = end
        if timestamp:
            self.last_timestamp = timestamp


class Tracer:
    def __init__(self, lib: ReportLib, binary_finder: BinaryFinder, readelf: ReadElf,
                 objdump: Objdump, w: TraceWriter) -> None:
        self.abort: bool = False

        self.last_timestamp: Optional[int] = None
        self.lost_decoding: bool = False

        self.context: EtmContext = EtmContext()

        self.lib: ReportLib = lib
        self.binary_finder: BinaryFinder = binary_finder
        self.readelf: ReadElf = readelf
        self.objdump: Objdump = objdump

        self.processes: Dict[int, str] = {}

        self.symbols: Dict[str, Tuple[List[Tuple[int, int, str]], List[int]]] = {}
        self.symbol_cache: Optional[Tuple[str, Tuple[int, int, str]]] = None

        self.stacks: Dict[int, Stacker] = {}
        self.w: TraceWriter = w
        w.magic()
        w.switch_provider('etm')

        self.s: Optional[Stacker] = None

    def __call__(self, trace_id: int, elem: etm.GenericTraceElement) -> None:
        if self.abort:
            return

        try:
            self.process(trace_id, elem)
        except Exception as e:
            self.abort = True
            self.gap()
            self.flush()
            raise e

    def reset_trace(self) -> None:
        self.context = EtmContext()
        self.lost_decoding = False
        self.last_timestamp = None

    def process(self, trace_id: int, elem: etm.GenericTraceElement) -> None:
        if elem.elem_type == etm.ElemType.TRACE_ON or elem.elem_type == etm.ElemType.NO_SYNC:
            if self.s:
                self.s.gap(self.last_timestamp)
            self.reset_trace()
            return

        if elem.elem_type == etm.ElemType.PE_CONTEXT:
            if self.context.update(elem.context):
                tid = self.context.tid
                if tid:
                    process = self.lib.GetThread(tid)
                    if process:
                        pid, tid, comm = process
                    else:
                        pid = tid
                        comm = f'Unknown process'
                else:
                    pid = -1
                    tid = -1
                    comm = 'Unknown process'

                kernel = "" if self.context.ex_level == etm.ExLevel.EL0 else " (kernel)"
                thread_name = f'{tid} {comm or ""}{kernel}'

                if pid in self.processes:
                    process_name = self.processes[pid]
                else:
                    if pid == tid:
                        parent_name = comm
                    else:
                        t = self.lib.GetThread(pid)
                        parent_name = t[2] if t else "Unknown process"
                    process_name = f'{pid} {parent_name or ""}'
                    self.processes[pid] = process_name

                pobj = self.w.kernel_object(1, process_name)
                tobj = self.w.kernel_object(2, thread_name, process=pobj)
                thread = self.w.thread(pobj, tobj)
                # It is possible that the thread_name changed and we have the old one saved here.
                # The Fuchsia trace does not seem to have any nice ways to handle changing names,
                # therefore we are sticking with the first name we saw.
                if tid not in self.stacks:
                    self.stacks[tid] = Stacker(self.w, thread)

                self.s = self.stacks[tid]
            return

        if elem.elem_type == etm.ElemType.TIMESTAMP:
            if self.last_timestamp != elem.timestamp:
                self.last_timestamp = elem.timestamp
                if self.s:
                    self.s.timestamp(elem.timestamp)

        # For the other elements, a context should have happened before and must have set s.
        assert self.s

        if elem.elem_type == etm.ElemType.ADDR_NACC:
            if not self.lost_decoding:
                self.lost_decoding = True
                mapped = self.lib.ConvertETMAddressToVaddrInFile(trace_id, elem.st_addr)
                if mapped:
                    print(f'ADDR_NACC: path {mapped[0]} cannot be decoded! ({hex(elem.st_addr)})')
                else:
                    print(f'ADDR_NACC: trace address {hex(elem.st_addr)} is not mapped!')

                # We have lost trace. Give up on the stack.
                # print(self.s.stack)
                # print(self.last_timestamp, self.s.last_timestamp, self.s.waiting)
                self.s.lost_stack(self.last_timestamp)
                # print(self.last_timestamp, self.s.last_timestamp, self.s.waiting)

            return

        if elem.elem_type == etm.ElemType.EXCEPTION:
            name = f'Exception: "{elem.exception_type()}" ({elem.exception_number})!'
            self.s.exception(self.last_timestamp, name,
                             elem.en_addr if elem.excep_ret_addr else None)
            return

        if elem.elem_type != etm.ElemType.INSTR_RANGE:
            return

        self.lost_decoding = False

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

        start = self.get_symbol(start_path, start_offset) or "Unknown"
        end = self.get_symbol(end_path, end_offset) or "Unknown"

        if not elem.last_instr_cond and not elem.last_instr_exec:
            buildid = self.lib.GetBuildIdForPath(end_path)
            raise RuntimeError(f"Wrong binary! Unconditional branch at {hex(end_offset)}"
                               f" in {end_path} (build id: {buildid}) was not taken!")

        self.s.instr_range(self.last_timestamp,
                           start, elem.st_addr,
                           end, elem.en_addr,
                           elem.last_i_subtype)

    def gap(self) -> None:
        for stack in self.stacks.values():
            stack.lost_stack(stack.last_timestamp)

    def flush(self) -> None:
        for stack in self.stacks.values():
            stack.flush()

    def get_symbol(self, path: str, offset: int) -> Optional[str]:
        if (self.symbol_cache and self.symbol_cache[0] == path and
                (self.symbol_cache[1][0]) <= offset < (self.symbol_cache[1][0] + self.symbol_cache[1][1])):
            return self.symbol_cache[1][2]

        (symbols, offsets) = self.get_symbols(path)
        if symbols:
            i = bisect(offsets, offset)
            if i:
                i -= 1
                if symbols[i][0] <= offset < symbols[i][0] + symbols[i][1]:
                    self.symbol_cache = (path, symbols[i])
                    return symbols[i][2]

        return None

    def get_symbols(self, path: str) -> Tuple[List[Tuple[int, int, str]], List[int]]:
        if path not in self.symbols:
            s = self.lib.GetSymbols(path)

            if not s:
                log_exit(f"Can't find symbols for unknown binary '{path}'!")
                return ([], [])

            # Since other use cases don't care about the PLT, simpleperf represents it only as a
            # single symbol called '@plt'. If it is present, try to get the actual names by parsing
            # objdump's output.
            if s[-1][2] == '@plt':
                dso_info = self.objdump.get_dso_info(path, self.lib.GetBuildIdForPath(path))
                plts = self.objdump.get_plt_symbols(dso_info)
                if plts:
                    del s[-1]
                    s.extend(plts)

            self.symbols[path] = (s, [e[0] for e in s])

        return self.symbols[path]


def get_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Generate Fuchsia trace from ETM data.')
    parser.add_argument('-i', '--record_file', nargs=1, default=['perf.data'], help="""
                        Set profiling data file to process.""")
    parser.add_argument('--binary_cache', nargs=1, default=["binary_cache"], help="""
                        Set path to the binary cache.""")
    parser.add_argument('--ndk_path', nargs=1, help='Find tools in the ndk path.')
    parser.add_argument('-o', '--output', nargs=1, help="""
                        Output path for the trace. If not specified, the record file's name is
                        used, with .fxt appended.""")
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

        readelf = ReadElf(ndk_path)
        binary_finder = BinaryFinder(binary_cache_path, readelf)
        objdump = Objdump(ndk_path, binary_finder)

        filename = args.output[0] if args.output else f'{args.record_file[0]}.fxt'

        with open(filename, 'wb') as f:
            callback = Tracer(lib, binary_finder, readelf, objdump, TraceWriter(f))
            lib.SetETMCallback(callback)
            while not callback.abort and lib.GetNextSample():
                pass

            # Trace has ended, make sure every call has a corresponding return. Use the largest
            # timestamp and end everything there.
            last_timestamps = [stacker.last_timestamp for stacker
                               in callback.stacks.values()
                               if stacker.last_timestamp]
            last_timestamp = max(last_timestamps, default=None)
            for stacker in callback.stacks.values():
                stacker.last_timestamp = last_timestamp
            callback.gap()
            callback.flush()

    finally:
        lib.Close()


if __name__ == '__main__':
    main()
