import common
import struct

# The target does not support OTA-flashing
# the partition table, so blacklist it.
DEFAULT_BOOTLOADER_OTA_BLACKLIST = [ 'partition' ]

class BadMagicError(Exception):
    __str__ = "bad magic value"

#
# Motoboot packed image format
#
# #define BOOTLDR_MAGIC "MBOOTV1"
# #define HEADER_SIZE 1024
# #define SECTOR_SIZE 512
# struct packed_images_header {
#         unsigned int num_images;
#         struct {
#                 char name[24];
#                 unsigned int start;  // start offset = HEADER_SIZE + start * SECTOR_SIZE
#                 unsigned int end;    // end offset = HEADER_SIZE + (end + 1) * SECTOR_SIZE - 1
#         } img_info[20];
#         char magic[8];  // set to BOOTLDR_MAGIC
# };
HEADER_SIZE = 1024
SECTOR_SIZE = 512
NUM_MAX_IMAGES = 20
MAGIC = "MBOOTV1\0"
class MotobootImage(object):

  def __init__(self, data, name = None):

    self.name = name
    self._unpack(data)

  def _unpack(self, data):
    """ Unpack the data blob as a motoboot image and return the list
    of contained image objects"""
    num_imgs_fmt = "<L"
    num_imgs_size = struct.calcsize(num_imgs_fmt)
    num_imgs, = struct.unpack(num_imgs_fmt, data[:num_imgs_size])

    img_info_format = "<24sLL"
    img_info_size = struct.calcsize(img_info_format)

    imgs = [ struct.unpack(img_info_format, data[num_imgs_size + i*img_info_size:num_imgs_size + (i+1)*img_info_size]) for i in range(num_imgs) ]

    magic_format = "<8s"
    magic_size = struct.calcsize(magic_format)
    magic, = struct.unpack(magic_format, data[num_imgs_size + NUM_MAX_IMAGES*img_info_size:num_imgs_size + NUM_MAX_IMAGES*img_info_size + magic_size])
    if magic != MAGIC:
      raise BadMagicError

    img_objs = []
    for name, start, end in imgs:
      start_offset = HEADER_SIZE + start * SECTOR_SIZE
      end_offset = HEADER_SIZE + (end + 1) * SECTOR_SIZE - 1
      img = common.File(trunc_to_null(name), data[start_offset:end_offset+1])
      img_objs.append(img)

    self.unpacked_images = img_objs

  def GetUnpackedImage(self, name):

    found_image = None
    for image in self.unpacked_images:
      if image.name == name:
          found_image = image
          break
    return found_image


def FindRadio(zipfile):
  try:
    return zipfile.read("RADIO/radio.img")
  except KeyError:
    return None

def FullOTA_InstallEnd(info):
  try:
    bootloader_img = info.input_zip.read("RADIO/bootloader.img")
  except KeyError:
    print "no bootloader.img in target_files; skipping install"
  else:
    WriteBootloader(info, bootloader_img)

  radio_img = FindRadio(info.input_zip)
  if radio_img:
    WriteRadio(info, radio_img)
  else:
    print "no radio.img in target_files; skipping install"

def IncrementalOTA_VerifyEnd(info):
  target_radio_img = FindRadio(info.target_zip)
  source_radio_img = FindRadio(info.source_zip)
  if not target_radio_img or not source_radio_img: return
  target_modem_img = MotobootImage(target_radio_img).GetUnpackedImage("modem")
  if not target_modem_img: return
  source_modem_img = MotobootImage(source_radio_img).GetUnpackedImage("modem")
  if not source_modem_img: return
  if target_modem_img.sha1 != source_modem_img.sha1:
    info.script.CacheFreeSpaceCheck(len(source_modem_img.data))
    radio_type, radio_device = common.GetTypeAndDevice("/modem", info.info_dict)
    info.script.PatchCheck("%s:%s:%d:%s:%d:%s" % (
        radio_type, radio_device,
        len(source_modem_img.data), source_modem_img.sha1,
        len(target_modem_img.data), target_modem_img.sha1))

def IncrementalOTA_InstallEnd(info):
  try:
    target_bootloader_img = info.target_zip.read("RADIO/bootloader.img")
    try:
      source_bootloader_img = info.source_zip.read("RADIO/bootloader.img")
    except KeyError:
      source_bootloader_img = None

    if source_bootloader_img == target_bootloader_img:
      print "bootloader unchanged; skipping"
    elif source_bootloader_img == None:
      print "no bootloader.img in source target_files; installing complete image"
      WriteBootloader(info, target_bootloader_img)
    else:
      tf = common.File("bootloader.img", target_bootloader_img)
      sf = common.File("bootloader.img", source_bootloader_img)
      WriteIncrementalBootloader(info, tf, sf)
  except KeyError:
    print "no bootloader.img in target target_files; skipping install"

  tf = FindRadio(info.target_zip)
  if not tf:
    # failed to read TARGET radio image: don't include any radio in update.
    print "no radio.img in target target_files; skipping install"
  else:
    tf = common.File("radio.img", tf)

    sf = FindRadio(info.source_zip)
    if not sf:
      # failed to read SOURCE radio image: include the whole target
      # radio image.
      print "no radio image in source target_files; installing complete image"
      WriteRadio(info, tf.data)
    else:
      sf = common.File("radio.img", sf)

      if tf.size == sf.size and tf.sha1 == sf.sha1:
        print "radio image unchanged; skipping"
      else:
        WriteIncrementalRadio(info, tf, sf)

def WriteIncrementalBootloader(info, target_imagefile, source_imagefile):
  try:
    tm = MotobootImage(target_imagefile.data, "bootloader")
  except BadMagicError:
    assert False, "bootloader.img bad magic value"
  try:
    sm = MotobootImage(source_imagefile.data, "bootloader")
  except BadMagicError:
    print "source bootloader image is not a motoboot image. Installing complete image."
    return WriteBootloader(info, target_imagefile.data)

  # blacklist any partitions that match the source image
  blacklist = DEFAULT_BOOTLOADER_OTA_BLACKLIST
  for ti in tm.unpacked_images:
    if ti not in blacklist:
      si = sm.GetUnpackedImage(ti.name)
      if not si:
        continue
      if ti.size == si.size and ti.sha1 == si.sha1:
        print "target bootloader partition image %s matches source; skipping" % ti.name
        blacklist.append(ti.name)

  # If there are any images to then write them
  whitelist = [ i.name for i in tm.unpacked_images if i.name not in blacklist ]
  if len(whitelist):
    # Install the bootloader, skipping any matching partitions
    WriteBootloader(info, target_imagefile.data, blacklist)

def WriteIncrementalRadio(info, target_imagefile, source_imagefile):
  try:
    target_radio_img = MotobootImage(target_imagefile.data, "radio")
  except BadMagicError:
    assert False, "radio.img bad magic value"

  try:
    source_radio_img = MotobootImage(source_imagefile.data, "radio")
  except BadMagicError:
    source_radio_img = None

  write_full_modem = True
  if source_radio_img:
    target_modem_img = target_radio_img.GetUnpackedImage("modem")
    if target_modem_img:
      source_modem_img = source_radio_img.GetUnpackedImage("modem")
      if source_modem_img:
        WriteIncrementalModemPartition(info, target_modem_img, source_modem_img)
        write_full_modem = False

  # Write the full images, skipping modem if so directed.
  #
  # NOTE: Some target flex radio images are zero-filled, and must
  #       be flashed to trigger the flex update "magic".  Do not
  #       skip installing target partition images that are identical
  #       to its corresponding source partition image.
  blacklist = []
  if not write_full_modem:
    blacklist.append('modem')
  WriteMotobootPartitionImages(info, target_radio_img, blacklist)

def WriteIncrementalModemPartition(info, target_modem_image, source_modem_image):
  tf = target_modem_image
  sf = source_modem_image

  b = common.BlockDifference("modem", common.DataImage(tf.data),
                             common.DataImage(sf.data))

  b.WriteScript(info.script, info.output_zip)


def WriteRadio(info, radio_img):
  info.script.Print("Writing radio...")

  try:
    motoboot_image = MotobootImage(radio_img, "radio")
  except BadMagicError:
      assert False, "radio.img bad magic value"

  WriteMotobootPartitionImages(info, motoboot_image)

def WriteMotobootPartitionImages(info, motoboot_image, blacklist = []):
  WriteGroupedImages(info, motoboot_image.name, motoboot_image.unpacked_images, blacklist)

def WriteGroupedImages(info, group_name, images, blacklist = []):
  """ Write a group of partition images to the OTA package,
  and add the corresponding flash instructions to the recovery
  script.  Skip any images that do not have a corresponding
  entry in recovery.fstab."""

  for i in images:
    if i.name not in blacklist:
      WritePartitionImage(info, i, group_name)

def WritePartitionImage(info, image, group_name = None):

  filename = "%s.img" % image.name
  if group_name:
    filename = "%s.%s" % (group_name,filename)

  try:
    _, device = common.GetTypeAndDevice("/"+image.name, info.info_dict)
  except KeyError:
    print "skipping flash of %s; not in recovery.fstab" % (image.name,)
    return

  common.ZipWriteStr(info.output_zip, filename, image.data)

  info.script.AppendExtra('package_extract_file("%s", "%s");' %
                          (filename, device))

def WriteBootloader(info, bootloader, blacklist = DEFAULT_BOOTLOADER_OTA_BLACKLIST):
  info.script.Print("Writing bootloader...")

  try:
    motoboot_image = MotobootImage(bootloader,"bootloader")
  except BadMagicError:
      assert False, "bootloader.img bad magic value"

  common.ZipWriteStr(info.output_zip, "bootloader-flag.txt",
                     "updating-bootloader" + "\0" * 13)
  common.ZipWriteStr(info.output_zip, "bootloader-flag-clear.txt", "\0" * 32)

  _, misc_device = common.GetTypeAndDevice("/misc", info.info_dict)

  info.script.AppendExtra(
      'package_extract_file("bootloader-flag.txt", "%s");' %
      (misc_device,))

  # OTA does not support partition changes, so
  # do not bundle the partition image in the OTA package.
  WriteMotobootPartitionImages(info, motoboot_image, blacklist)

  info.script.AppendExtra(
      'package_extract_file("bootloader-flag-clear.txt", "%s");' %
      (misc_device,))

def trunc_to_null(s):
  if '\0' in s:
    return s[:s.index('\0')]
  else:
    return s
