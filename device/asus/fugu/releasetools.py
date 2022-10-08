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

import struct
import common


def WriteIfwi(info):
  info.script.AppendExtra('package_extract_file("ifwi.bin", "/tmp/ifwi.bin");')
  info.script.AppendExtra("""fugu.flash_ifwi("/tmp/ifwi.bin");""")

def WriteDroidboot(info):
  info.script.WriteRawImage("/fastboot", "droidboot.img")

def WriteSplashscreen(info):
  info.script.WriteRawImage("/splashscreen", "splashscreen.img")

def WriteBootloader(info, bootloader):
  header_fmt = "<8sHHI"
  header_size = struct.calcsize(header_fmt)
  magic, revision, reserved, reserved = struct.unpack(
    header_fmt, bootloader[:header_size])

  assert magic == "BOOTLDR!", "bootloader.img bad magic value"

  if revision == 1:
    offset = header_size;
    header_v1_fmt = "II"
    header_v1_size = struct.calcsize(header_v1_fmt)
    ifwi_size, droidboot_size = struct.unpack(header_v1_fmt, bootloader[offset:offset + header_v1_size])
    offset += header_v1_size
    ifwi = bootloader[offset:offset + ifwi_size]
    offset += ifwi_size
    droidboot = bootloader[offset:]
    common.ZipWriteStr(info.output_zip, "droidboot.img", droidboot)
    common.ZipWriteStr(info.output_zip, "ifwi.bin", ifwi)
    WriteIfwi(info)
    WriteDroidboot(info)
    return

  offset = header_size;
  while offset < len(bootloader):
    c_header_fmt = "<8sIBBBB"
    c_header_size = struct.calcsize(c_header_fmt)
    c_magic, size, flags, _, _ , _ = struct.unpack(c_header_fmt, bootloader[offset:offset + c_header_size])
    buf = bootloader[offset + c_header_size: offset + c_header_size + size]
    offset += c_header_size + size

    if not flags & 1:
      continue

    if c_magic == "IFWI!!!!":
      common.ZipWriteStr(info.output_zip, "ifwi.bin", buf)
      WriteIfwi(info);
      continue
    if c_magic == "DROIDBT!":
      common.ZipWriteStr(info.output_zip, "droidboot.img", buf)
      WriteDroidboot(info);
      continue
    if c_magic == "SPLASHS!":
      common.ZipWriteStr(info.output_zip, "splashscreen.img", buf)
      WriteSplashscreen(info);
      continue

def FullOTA_InstallEnd(info):
  try:
    bootloader_img = info.input_zip.read("RADIO/bootloader.img")
  except KeyError:
    print "no bootloader.img in target_files; skipping install"
  else:
    WriteBootloader(info, bootloader_img)


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
      print "bootloader changed; adding it"
      WriteBootloader(info, target_bootloader_img)
  except KeyError:
    print "no bootloader.img in target target_files; skipping install"
