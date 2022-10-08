# Copyright (C) 2012 The Android Open Source Project
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

"""Emit extra commands needed for Group during OTA installation
(installing the bootloader)."""

import common

def FullOTA_InstallEnd(info):
  try:
    bootloader_bin = info.input_zip.read("RADIO/bootloader.raw")
  except KeyError:
    print "no bootloader.raw in target_files; skipping install"
  else:
    WriteBootloader(info, bootloader_bin)

def IncrementalOTA_InstallBegin(info):
  info.script.Unmount("/system")
  info.script.TunePartition("/system", "-O", "^has_journal")
  info.script.Mount("/system")

def IncrementalOTA_InstallEnd(info):
  try:
    target_bootloader_bin = info.target_zip.read("RADIO/bootloader.raw")
    try:
      source_bootloader_bin = info.source_zip.read("RADIO/bootloader.raw")
    except KeyError:
      source_bootloader_bin = None

    if source_bootloader_bin == target_bootloader_bin:
      print "bootloader unchanged; skipping"
    else:
      WriteBootloader(info, target_bootloader_bin)
  except KeyError:
    print "no bootloader.raw in target target_files; skipping install"


def WriteBootloader(info, bootloader_bin):
  common.ZipWriteStr(info.output_zip, "bootloader.raw", bootloader_bin)
  fstab = info.info_dict["fstab"]

  info.script.Print("Writing bootloader...")

  info.script.AppendExtra('''package_extract_file("bootloader.raw", "%s");''' %
                          (fstab["/staging"].device,))
