#!/bin/bash
# Copyright 2009, The Android Open Source Project
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

# Run a bunch of test on the sdcard to establish a performance profile.

print_kernel() {
  adb shell cat /proc/version
}
print_sched_features() {
  adb shell cat /sys/kernel/debug/sched_features
}

# Use dd to get the raw speed of the card
block_level() {
  true
}

# Time to run a test vs number of processes
scalability() {
  local file="/tmp/sdcard-scalability.txt"
  rm -f ${file}
  echo "# Scalability tests" | tee -a ${file}
  echo "# Kernel: $(print_kernel)" | tee -a ${file}
  echo "# Sched features: $(print_sched_features)" | tee -a ${file}
  echo "# StopWatch scalability total/cumulative duration 0.0 Samples: 1" | tee -a ${file}
  echo "# Process Time" | tee -a ${file}
  for p in $(seq 1 8); do
    adb shell sdcard_perf_test --test=write --procnb=${p} --size=1000 --chunk-size=100 --iterations=50 >/tmp/tmp-sdcard.txt
    local t=$(grep 'write_total' /tmp/tmp-sdcard.txt | tail -n 1 | cut -f 6 -d ' ')
    echo "$p $t" | tee -a ${file}
  done

}

# Readers and writers should not starve each others.
fairness() {
  # Check readers finished before writers.
  # Find the time of the last read op.
  # Count how many writes and how many read happend
  # during that period, do the ratio.
  true
}

#######################################################################
# MAIN

echo "Make sure debugfs is mounted on the device."
block_level
scalability
fairness


