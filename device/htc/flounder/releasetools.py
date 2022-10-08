import common
import struct

def FullOTA_InstallEnd(info):
  try:
    firmware_img = info.input_zip.read("RADIO/bootloader.img")
  except KeyError:
    print "no bootloader.img in target_files; skipping install"
  else:
    info.script.Print("Writing bootloader.img...")
    common.ZipWriteStr(info.output_zip, "bootloader.img", firmware_img)
    info.script.AppendExtra(
        'package_extract_file("bootloader.img", "/dev/block/platform/sdhci-tegra.3/by-name/OTA");')


def IncrementalOTA_InstallEnd(info):
  try:
    source_firmware_img = info.source_zip.read("RADIO/bootloader.img")
  except KeyError:
    print "no bootloader.img in source_files; skipping install"
  else:
    try:
      target_firmware_img = info.target_zip.read("RADIO/bootloader.img")
    except KeyError:
      print "no bootloader.img in target_files; skipping install"
    else:
      if source_firmware_img == target_firmware_img:
        return
      info.script.Print("Writing bootloader.img...")
      common.ZipWriteStr(info.output_zip, "bootloader.img", target_firmware_img)
      info.script.AppendExtra(
          'package_extract_file("bootloader.img", "/dev/block/platform/sdhci-tegra.3/by-name/OTA");')
