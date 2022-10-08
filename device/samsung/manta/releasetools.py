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

"""Emit commands needed for Mantaray during OTA installation
(installing the bootloader)."""

import common

def FullOTA_InstallEnd(info):
  try:
    bootloader_img = info.input_zip.read("RADIO/bootloader.img")
  except KeyError:
    print "no bootloader.img in target_files; skipping install"
  else:
    WriteBootloader(info, bootloader_img)

def IncrementalOTA_InstallBegin(info):
  info.script.Unmount("/system")
  info.script.TunePartition("/system", "-O", "^has_journal")
  info.script.Mount("/system")

def IncrementalOTA_VerifyEnd(info):
  # try:
  #   target_radio_img = info.target_zip.read("RADIO/radio.img")
  #   source_radio_img = info.source_zip.read("RADIO/radio.img")
  # except KeyError:
  #   # No source or target radio. Nothing to verify
  #   pass
  # else:
  #   if source_radio_img != target_radio_img:
  #     info.script.CacheFreeSpaceCheck(len(source_radio_img))
  #     radio_type, radio_device = common.GetTypeAndDevice("/radio", info.info_dict)
  #     info.script.PatchCheck("%s:%s:%d:%s:%d:%s" % (
  #         radio_type, radio_device,
  #         len(source_radio_img), common.sha1(source_radio_img).hexdigest(),
  #         len(target_radio_img), common.sha1(target_radio_img).hexdigest()))
  pass

def IncrementalOTA_InstallEnd(info):
  try:
    target_bootloader_img = info.target_zip.read("RADIO/bootloader.img")
    try:
      source_bootloader_img = info.source_zip.read("RADIO/bootloader.img")
    except KeyError:
      source_bootloader_img = None

    if source_bootloader_img == target_bootloader_img:
      print "bootloader unchanged; skipping"
    else:
      WriteBootloader(info, target_bootloader_img)
  except KeyError:
    print "no bootloader.img in target target_files; skipping install"

def WriteBootloader(info, bootloader_img):
  common.ZipWriteStr(info.output_zip, "bootloader.img", bootloader_img)
  bl_type, bl_device = common.GetTypeAndDevice("/bootloader", info.info_dict)

  fstab = info.info_dict["fstab"]

  info.script.Print("Writing bootloader...")
  force_ro = "/sys/block/" + bl_device.split("/")[-1] + "/force_ro"
  info.script.AppendExtra(
      ('samsung.manta.write_bootloader(package_extract_file('
       '"bootloader.img"), "%s", "%s");') %
      (bl_device, force_ro))
