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

"""etm_types.py: ctypes based structures to handle ocsd_generic_trace_elem from OpenCSD.

   Please refer to the documentation of OpenCSD to see the fields available and their meaning:
   https://github.com/Linaro/OpenCSD/blob/master/decoder/docs/prog_guide/prog_guide_generic_pkts.md

"""

import ctypes as ct
from enum import Enum, auto
import sys

# On 32 bit machines, ocsd_vaddr_t is 32 bit only.
VADDR_TYPE = ct.c_uint64 if sys.maxsize > 2**32 else ct.c_uint32


class SecLevel(Enum):
    """ocsd_sec_level in OpenCSD."""
    SECURE = 0
    NONSECURE = 1
    ROOT = 2
    REALM = 3


class ExLevel(Enum):
    """ocsd_ex_level in OpenCSD."""
    UNKNOWN = -1
    EL0 = 0
    EL1 = 1
    EL2 = 2
    EL3 = 3


class PeContext(ct.Structure):
    """ocsd_pe_context in OpenCSD."""
    _fields_ = [("_security_level", ct.c_int),
                ("_exception_level", ct.c_int),
                ("context_id", ct.c_uint32),
                ("vmid", ct.c_uint32),
                ("bits64", ct.c_uint32, 1),
                ("ctxt_id_valid", ct.c_uint32, 1),
                ("vmid_valid", ct.c_uint32, 1),
                ("el_valid", ct.c_uint32, 1)]

    @property
    def security_level(self) -> SecLevel:
        return SecLevel(self._security_level)

    @property
    def exception_level(self) -> ExLevel:
        return ExLevel(self._exception_level)


class TraceEvent(ct.Structure):
    """trace_event_t in OpenCSD."""
    _fields_ = [("ev_type", ct.c_uint16),
                ("ev_number", ct.c_uint16)]


class TraceOnReason(Enum):
    """trace_on_reason_t in OpenCSD."""
    NORMAL = 0
    OVERFLOW = 1
    EX_DEBUG = 2


class _SwtInfoInnerStruct(ct.Structure):
    _fields_ = [("swt_payload_pkt_bitsize", ct.c_uint32, 8),
                ("swt_payload_num_packets", ct.c_uint32, 8),
                ("swt_marker_packet", ct.c_uint32, 1),
                ("swt_has_timestamp", ct.c_uint32, 1),
                ("swt_marker_first", ct.c_uint32, 1),
                ("swt_master_err", ct.c_uint32, 1),
                ("swt_global_err", ct.c_uint32, 1),
                ("swt_trigger_event", ct.c_uint32, 1),
                ("swt_frequency", ct.c_uint32, 1),
                ("swt_id_valid", ct.c_uint32, 1)]


class _SwtInfoInnerUnion(ct.Union):
    _anonymous_ = ("_s",)
    _fields_ = [("_s", _SwtInfoInnerStruct),
                ("swt_flag_bits", ct.c_uint32)]


class SwtInfo(ct.Structure):
    """ocsd_swt_info_t in OpenCSD."""
    _anonymous_ = ("_u",)
    _fields_ = [("swt_master_id", ct.c_uint16),
                ("swt_channel_id", ct.c_uint16),
                ("_u", _SwtInfoInnerUnion)]


class UnsyncInfo(Enum):
    """unsync_info_t in OpenCSD."""
    UNSYNC_UNKNOWN = 0
    UNSYNC_INIT_DECODER = 1
    UNSYNC_RESET_DECODER = 2
    UNSYNC_OVERFLOW = 3
    UNSYNC_DISCARD = 4
    UNSYNC_BAD_PACKET = 5
    UNSYNC_EOT = 6


class TraceSyncMarker(Enum):
    """trace_sync_marker_t in OpenCSD."""
    ELEM_MARKER_TS = 0


class TraceMarkerPayload(ct.Structure):
    """trace_marker_payload_t in OpenCSD."""
    _fields_ = [("_type", ct.c_int),
                ("value", ct.c_uint32)]

    @property
    def type(self) -> TraceSyncMarker:
        return TraceSyncMarker(self._type)


class TraceMemtrans(Enum):
    """trace_memtrans_t in OpenCSD."""
    MEM_TRANS_TRACE_INIT = 0
    MEM_TRANS_START = 1
    MEM_TRANS_COMMIT = 2
    MEM_TRANS_FAIL = 3


class TraceSwIte(ct.Structure):
    """trace_sw_ite_t in OpenCSD."""
    _fields_ = [("el", ct.c_uint8),
                ("value", ct.c_uint64)]


class _ElementFlags(ct.Structure):
    _fields_ = [("last_instr_exec", ct.c_uint32, 1),
                ("last_instr_sz", ct.c_uint32, 3),
                ("has_cc", ct.c_uint32, 1),
                ("cpu_freq_change", ct.c_uint32, 1),
                ("excep_ret_addr", ct.c_uint32, 1),
                ("excep_data_marker", ct.c_uint32, 1),
                ("extended_data", ct.c_uint32, 1),
                ("has_ts", ct.c_uint32, 1),
                ("last_instr_cond", ct.c_uint32, 1),
                ("excep_ret_addr_br_tgt", ct.c_uint32, 1),
                ("excep_M_tail_chain", ct.c_uint32, 1)]


class _ElementFlagsUnion(ct.Union):
    _anonymous_ = ("_s",)
    _fields_ = [("_s", _ElementFlags),
                ("flag_bits", ct.c_uint32)]


class _Payload(ct.Union):
    _fields_ = [("exception_number", ct.c_uint32),
                ("trace_event", TraceEvent),
                ("_trace_on_reason", ct.c_int),
                ("sw_trace_info", SwtInfo),
                ("num_instr_range", ct.c_uint32),
                ("_unsync_eot_info", ct.c_int),
                ("sync_marker", TraceMarkerPayload),
                ("_mem_trans", ct.c_int),
                ("sw_ite", TraceSwIte)]


class ElemType(Enum):
    """ocsd_gen_trc_elem_t in OpenCSD."""
    UNKNOWN = 0
    NO_SYNC = 1
    TRACE_ON = 2
    EO_TRACE = 3
    PE_CONTEXT = 4
    INSTR_RANGE = 5
    I_RANGE_NOPATH = 6
    ADDR_NACC = 7
    ADDR_UNKNOWN = 8
    EXCEPTION = 9
    EXCEPTION_RET = 10
    TIMESTAMP = 11
    CYCLE_COUNT = 12
    EVENT = 13
    SWTRACE = 14
    SYNC_MARKER = 15
    MEMTRANS = 16
    INSTRUMENTATION = 17
    CUSTOM = 18


class IsaType(Enum):
    """ocsd_isa in OpenCSD."""
    ARM = 0
    THUMB2 = 1
    AARCH64 = 2
    TEE = 3
    JAZELLE = 4
    CUSTOM = 5
    UNKNOWN = 6


class InstrType(Enum):
    """ocsd_instr_type in OpenCSD."""
    OTHER = 0
    BR = 1
    BR_INDIRECT = 2
    ISB = 3
    DSB_DMB = 4
    WFI_WFE = 5
    TSTART = 6


class InstrSubtype(Enum):
    """ocsd_instr_subtype in OpenCSD."""
    NONE = 0
    BR_LINK = 1
    V8_RET = 2
    V8_ERET = 3
    V7_IMPLIED_RET = 4


class GenericTraceElement(ct.Structure):
    """ocsd_generic_trace_elem in OpenCSD."""
    _anonymous_ = ("_payload", "_flags")
    _fields_ = [("_elem_type", ct.c_int),
                ("_isa", ct.c_int),
                ("st_addr", VADDR_TYPE),
                ("en_addr", VADDR_TYPE),
                ("context", PeContext),
                ("timestamp", ct.c_uint64),
                ("cycle_count", ct.c_uint32),
                ("_last_i_type", ct.c_int),
                ("_last_i_subtype", ct.c_int),
                ("_flags", _ElementFlagsUnion),
                ("_payload", _Payload),
                ("ptr_extended_data", ct.c_void_p)]

    @property
    def elem_type(self) -> ElemType:
        return ElemType(self._elem_type)

    @property
    def isa(self) -> IsaType:
        return IsaType(self._isa)

    @property
    def last_i_type(self) -> InstrType:
        return InstrType(self._last_i_type)

    @property
    def last_i_subtype(self) -> InstrSubtype:
        return InstrSubtype(self._last_i_subtype)

    @property
    def trace_on_reason(self) -> TraceOnReason:
        # This is from the anonymous structure that is called _Payload in _fields_.
        return TraceOnReason(self._trace_on_reason)

    @property
    def unsync_eot_info(self) -> UnsyncInfo:
        # This is from the anonymous structure that is called _Payload in _fields_.
        return UnsyncInfo(self._unsync_eot_info)

    @property
    def mem_trans(self) -> TraceMemtrans:
        # This is from the anonymous structure that is called _Payload in _fields_.
        return TraceMemtrans(self._mem_trans)

    def exception_type(self) -> str:
        """Return the exception type indicated by exception_number."""
        # The values are taken from the Arm Architecture Reference Manual for A-profile, "D5.3.30
        # Exception 64-bit Address IS0 Packet". The reasons for generating them are listed in
        # "D4.4.4 Exceptions to Exception element encoding".
        names = ["PE Reset", "Debug halt", "Call", "Trap", "System Error", "Unknown",
                 "Inst debug", "Data debug", "Unknown", "Unknown",
                 "Alignment", "Inst Fault", "Data Fault", "Unknown",
                 "IRQ", "FIQ",
                 "Implementation defined 0", "Implementation defined 1",
                 "Implementation defined 2", "Implementation defined 3",
                 "Implementation defined 4", "Implementation defined 5",
                 "Implementation defined 6", "Implementation defined 7",
                 "Reserved"]
        if self.exception_number < len(names):
            return names[self.exception_number]

        return "Reserved"
