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

import unittest

from report_fuchsia import Stacker
from etm_types import InstrSubtype
from . test_utils import TestBase


class Result:
    def __init__(self):
        self.events = []

    def duration(self, begin, thread, category, name, timestamp):
        self.events.append(('call' if begin else 'ret', name, timestamp))


class TestEtmStacker(TestBase):
    def setUp(self):
        super().setUp()
        self.r = Result()
        self.s = Stacker(self.r, 0)

    def assertResult(self, sequence):
        self.s.flush()
        self.assertEqual(self.r.events, sequence)

    # Primitives.
    def test_primitives_call(self):
        self.s.call('main', 1)
        self.assertResult([('call', 'main', 1)])

    def test_primitives_ret(self):
        self.s.call('main', 1)
        self.s.ret('main', 2)
        self.assertResult([('call', 'main', 1), ('ret', 'main', 2)])

    def test_primitives_no_timestamp(self):
        self.s.call('main', None)
        self.s.ret('main', None)
        self.assertResult([])

    # Simple stacks.
    def test_simple_start(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, InstrSubtype.V8_RET)
        self.assertResult([('call', 'main', 1), ('ret', 'main', 1)])

    def test_simple_return(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, InstrSubtype.V8_RET)
        self.assertResult([('call', 'main', 1), ('ret', 'main', 1)])

    def test_simple_inner(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, InstrSubtype.BR_LINK)
        self.s.instr_range(2, 'inner', 0, 'inner', 0, InstrSubtype.V8_RET)
        self.assertResult([('call', 'main', 1), ('call', 'inner', 2), ('ret', 'inner', 2)])

    def test_simple_recursion(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, InstrSubtype.BR_LINK)
        self.s.instr_range(2, 'main', 0, 'main', 0, InstrSubtype.V8_RET)
        self.s.instr_range(3, 'main', 0, 'main', 0, InstrSubtype.V8_RET)
        self.assertResult([('call', 'main', 1), ('call', 'main', 2),
                           ('ret', 'main', 2), ('ret', 'main', 3)])

    def test_simple_tailcall(self):
        # If not a return or a call, the jump is considered a tail call.
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.instr_range(2, 'next', 0, 'next', 0, None)
        self.assertResult([('call', 'main', 1), ('ret', 'main', 2), ('call', 'next', 2)])

    def test_simple_tailcall_single(self):
        # If the symbol is different between the start and the end, it changed without a jump.
        # Pretend that it is a tail-call.
        self.s.instr_range(1, 'main', 0, 'next', 0, None)
        self.assertResult([('call', 'main', 1), ('ret', 'main', 1), ('call', 'next', 1)])

    def test_simple_no_call(self):
        # If execution returns below the known stack, pretend that we have known about the function
        # running since the first known timestamp.
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.instr_range(2, 'main', 0, 'main', 0, InstrSubtype.V8_RET)
        self.s.instr_range(3, 'next', 0, 'next', 0, None)
        self.assertResult([('call', 'next', 1),
                           ('call', 'main', 1), ('ret', 'main', 2)])

    # Delayed events (i.e., no timestamps).
    def test_delay_later(self):
        self.s.instr_range(None, 'main', 0, 'main', 0, None)
        self.s.timestamp(1)
        self.assertResult([('call', 'main', 1)])

    def test_delay_later_ret(self):
        self.s.instr_range(None, 'main', 0, 'main', 0, InstrSubtype.V8_RET)
        self.s.timestamp(1)
        self.assertResult([('call', 'main', 1), ('ret', 'main', 1)])

    def test_delay_inner(self):
        self.s.instr_range(None, 'main', 0, 'main', 0, InstrSubtype.BR_LINK)
        self.s.instr_range(2, 'inner', 0, 'inner', 0, InstrSubtype.V8_RET)
        self.assertResult([('call', 'main', 2), ('call', 'inner', 2), ('ret', 'inner', 2)])

    def test_delay_no_call(self):
        self.s.instr_range(None, 'main', 0, 'main', 0, InstrSubtype.V8_RET)
        self.s.instr_range(2, 'next', 0, 'next', 0, None)
        self.assertResult([('call', 'next', 2),
                           ('call', 'main', 2), ('ret', 'main', 2)])

    # Gaps.
    def test_gaps_gap(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.gap(2)
        self.s.instr_range(3, 'next', 0, 'next', 0, None)
        self.assertResult([('call', 'main', 1), ('ret', 'main', 2),
                           ('call', 'next', 3)])

    def test_gaps_exception_start(self):
        self.s.exception(1, 'exception', 0)
        self.assertResult([('call', 'exception', 1)])

    def test_gaps_exception_twice(self):
        self.s.exception(1, 'exception', 0)
        self.s.gap(2)
        self.s.exception(2, 'another', 0)
        self.assertResult([('call', 'exception', 1), ('ret', 'exception', 2),
                           ('call', 'another', 2)])

    def test_gaps_exception_ret(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.exception(2, 'exception', 1)
        self.s.gap(2)
        self.s.instr_range(3, 'main', 1, 'main', 1, InstrSubtype.V8_RET)
        self.assertResult([('call', 'main', 1), ('call', 'exception', 2),
                           ('ret', 'exception', 3), ('ret', 'main', 3)])

    def test_gaps_exception_ret_twice(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.exception(2, 'exception', 1)
        self.s.gap(2)
        self.s.exception(3, 'another', 1)
        self.s.gap(3)
        self.s.instr_range(4, 'main', 1, 'main', 1, InstrSubtype.V8_RET)
        self.assertResult([('call', 'main', 1), ('call', 'exception', 2),
                           ('ret', 'exception', 3), ('call', 'another', 3),
                           ('ret', 'another', 4), ('ret', 'main', 4)])

    def test_gaps_exception_ret_bad(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.exception(2, 'exception', 100)
        self.s.gap(2)
        self.s.instr_range(3, 'next', 1, 'next', 1, None)
        self.assertResult([('call', 'main', 1), ('call', 'exception', 2),
                           ('ret', 'exception', 3), ('ret', 'main', 3),
                           ('call', 'next', 3)])

    def test_gaps_exception_ret_twice_bad(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.exception(2, 'exception', 1)
        self.s.gap(2)
        self.s.exception(3, 'another', 2)
        self.s.gap(3)
        self.s.instr_range(4, 'next', 3, 'next', 3, None)
        self.assertResult([('call', 'main', 1), ('call', 'exception', 2),
                           ('ret', 'exception', 3), ('ret', 'main', 3),
                           ('call', 'another', 3), ('ret', 'another', 4), ('call', 'next', 4)])

    def test_gaps_exception_call_preserved(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, InstrSubtype.BR_LINK)
        self.s.exception(2, 'exception', 1)
        self.s.gap(2)
        self.s.instr_range(3, 'next', 1, 'next', 2, None)
        self.assertResult([('call', 'main', 1), ('call', 'exception', 2),
                           ('ret', 'exception', 3), ('call', 'next', 3)])

    def test_gaps_exception_ret_preserved(self):
        # The actual ret is not preserved, but the starting point of elements below the known stack
        # should be.
        self.s.instr_range(1, 'main', 0, 'main', 0, InstrSubtype.V8_RET)
        self.s.exception(2, 'exception', 1)
        self.s.gap(2)
        self.s.instr_range(3, 'next', 1, 'next', 2, None)
        self.assertResult([('call', 'next', 1),
                           ('call', 'main', 1), ('ret', 'main', 1),
                           ('call', 'exception', 2), ('ret', 'exception', 3)])

    def test_gaps_plt(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, InstrSubtype.BR_LINK)
        self.s.instr_range(2, 'func@plt', 0, 'func@plt', 0, None)
        self.s.gap(3)
        self.s.instr_range(4, 'main', 0, 'main', 0, None)
        self.s.instr_range(5, 'main', 0, 'main', 0, InstrSubtype.V8_RET)
        self.assertResult([('call', 'main', 1), ('call', 'func@plt', 2),
                           ('ret', 'func@plt', 4), ('ret', 'main', 5)])

    def test_gaps_plt_wrong(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, InstrSubtype.BR_LINK)
        self.s.instr_range(2, 'func@plt', 0, 'func@plt', 0, None)
        self.s.gap(3)
        self.s.instr_range(4, 'next', 0, 'next', 0, None)
        self.assertResult([('call', 'main', 1), ('call', 'func@plt', 2),
                           ('ret', 'func@plt', 4), ('ret', 'main', 4),
                           ('call', 'next', 4)])

    def test_gaps_danger(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.exception(2, 'exception', 100)
        self.s.gap(2)
        self.s.instr_range(3, 'main', 0, 'main', 0, InstrSubtype.V8_RET)
        self.assertResult([('call', 'main', 1), ('call', 'exception', 2),
                           ('ret', 'exception', 3), ('ret', 'main', 3)])

    # Stack was lost.
    def test_lost_lost(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, InstrSubtype.BR_LINK)
        self.s.instr_range(2, 'next', 0, 'next', 0, None)
        self.s.lost_stack(3)
        self.assertResult([('call', 'main', 1), ('call', 'next', 2),
                           ('ret', 'next', 3), ('ret', 'main', 3)])

    def test_lost_ordering(self):
        self.s.exception(1, 'exception', 0)
        self.s.gap(None)
        self.s.lost_stack(None)
        self.s.timestamp(2)
        self.assertResult([('call', 'exception', 1), ('ret', 'exception', 2)])

    def test_lost_assumption(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.exception(2, 'exception', 0)
        self.s.gap(2)
        self.s.instr_range(3, 'main', 0, 'main', 0, None)
        self.assertResult([('call', 'main', 1), ('call', 'exception', 2),
                           ('ret', 'exception', 3)])

    def test_lost_late(self):
        self.s.instr_range(None, 'main', 0, 'main', 0, InstrSubtype.BR_LINK)
        self.s.exception(None, 'exception', 0)
        self.s.gap(None)
        self.s.instr_range(1, 'next', 0, 'next', 0, None)
        self.assertResult([('call', 'main', 1), ('call', 'exception', 1), ('ret', 'exception', 1),
                           ('call', 'next', 1)])

    # Timestamps.
    def test_timestamps_updates(self):
        self.s.timestamp(10)
        self.assertEqual(self.s.last_timestamp, 10)

    def test_timestamps_gap_removes(self):
        self.s.timestamp(10)
        self.s.gap(11)
        self.assertEqual(self.s.last_timestamp, None)

    def test_timestamps_gap_removes_exception(self):
        self.s.timestamp(10)
        self.s.exception(10, "exception", 0)
        self.s.gap(10)
        self.assertEqual(self.s.last_timestamp, None)

    def test_timestamps_gap_removes_plt(self):
        self.s.instr_range(1, 'foo@plt', 0, 'foo@plt', 0, None)
        self.s.gap(10)
        self.assertEqual(self.s.last_timestamp, None)

    def test_timestamps_gap_again(self):
        self.s.timestamp(10)
        self.s.gap(11)
        self.s.timestamp(11)
        self.assertEqual(self.s.last_timestamp, 11)

    def test_timestamps_first(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.instr_range(2, 'main', 0, 'main', 0, None)
        self.assertEqual(self.s.first_timestamp, 1)
        self.assertEqual(self.s.last_timestamp, 2)

    def test_timestamps_first_gap(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.gap(10)
        self.s.instr_range(11, 'main', 0, 'main', 0, None)
        self.assertEqual(self.s.first_timestamp, 11)

    def test_timestamps_first_exception_good(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.exception(2, 'exception', 100)
        self.s.gap(10)
        self.s.instr_range(11, 'main', 100, 'main', 100, None)
        self.assertEqual(self.s.first_timestamp, 1)

    def test_timestamps_first_exception_bad(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, None)
        self.s.exception(2, 'exception', 100)
        self.s.gap(10)
        self.s.instr_range(11, 'next', 200, 'next', 200, None)
        self.assertEqual(self.s.first_timestamp, 11)

    def test_timestamps_first_plt_good(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, InstrSubtype.BR_LINK)
        self.s.instr_range(2, 'foo@plt', 0, 'foo@plt', 0, None)
        self.s.gap(10)
        self.s.instr_range(11, 'main', 200, 'main', 200, None)
        self.assertEqual(self.s.first_timestamp, 1)

    def test_timestamps_first_plt_bad(self):
        self.s.instr_range(1, 'main', 0, 'main', 0, InstrSubtype.BR_LINK)
        self.s.instr_range(2, 'foo@plt', 0, 'foo@plt', 0, None)
        self.s.gap(10)
        self.s.instr_range(11, 'next', 200, 'next', 200, None)
        self.assertEqual(self.s.first_timestamp, 11)

    def test_timestamps_exception_fake_start(self):
        self.s.exception(1, 'exception', 100)
        self.s.lost_stack(None)
        self.s.instr_range(None, 'main', 0, 'main', 0, InstrSubtype.V8_RET)
        self.s.instr_range(2, 'deep', 0, 'deep', 0, None)
        self.assertResult([('call', 'exception', 1), ('ret', 'exception', 2),
                           ('call', 'deep', 2), ('call', 'main', 2), ('ret', 'main', 2)])
