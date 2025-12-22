#!/bin/sh
#
# Copyright 2023 - The Android Open Source Project
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

# Setups the device before running liblog tests. Recovers the state after the
# tests are done. The setup and the tead-down phases are distinguished via the
# first argument: [setup|teardown].

if [ "$1" != setup -a "$1" != teardown ]; then
    echo "Usage: $0 [setup|teardown]"
    exit 1
fi

MODE=$1

save_or_restore () {
    local PROP=$1
    local SAVED=/data/local/tests/${PROP}.saved
    if [ "$MODE" = setup ]; then
        if [ -n "$(getprop ${PROP})" ]; then
            getprop ${PROP} > ${SAVED}
            setprop ${PROP} ""
        fi
    elif [ "$MODE" = teardown ]; then
        if [ -e ${SAVED} ]; then
            setprop ${PROP} $(cat ${SAVED})
            rm ${SAVED}
        fi
    fi
}

# b/279123901: If persist.log.tag is set, remove the sysprop during the test.
# b/379667769: do the same as above for log.tag as well
PROPS=(persist.log.tag log.tag)
for PROP in "${PROPS[@]}"
do
    save_or_restore ${PROP}
done
