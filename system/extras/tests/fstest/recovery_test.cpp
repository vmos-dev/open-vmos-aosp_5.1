/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless requied by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/*
 * These file system recovery tests ensure the ability to recover from
 * filesystem crashes in key blocks (e.g. superblock).
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fs_mgr.h>
#include <gtest/gtest.h>
#include <logwrap/logwrap.h>
#include <sys/types.h>
#include <unistd.h>

#include "cutils/properties.h"
#include "ext4.h"
#include "ext4_utils.h"

#define LOG_TAG "fsRecoveryTest"
#include <utils/Log.h>
#include <testUtil.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define FSTAB_PREFIX "/fstab."
#define SB_OFFSET 1024
#define UMOUNT_BIN "/system/bin/umount"
#define VDC_BIN "/system/bin/vdc"

enum Fs_Type { FS_UNKNOWN, FS_EXT4, FS_F2FS };

namespace android {

class DataFileVerifier {
 public:
  DataFileVerifier(const char* file_name) {
    strncpy(test_file_, file_name, FILENAME_MAX);
  }

  void verify_write() {
    int write_fd = open(test_file_, O_CREAT | O_WRONLY, 0666);
    ASSERT_TRUE(write_fd);
    ASSERT_EQ(write(write_fd, "TEST", 4), 4);
    close(write_fd);
  }

  void verify_read() {
    char read_buff[4];
    int read_fd = open(test_file_, O_RDONLY);
    ASSERT_TRUE(read_fd);
    ASSERT_EQ(read(read_fd, read_buff, sizeof(read_buff)), 4);
    ASSERT_FALSE(strncmp(read_buff, "TEST", 4));
    close(read_fd);
  }

  ~DataFileVerifier() {
    unlink(test_file_);
  }

 private:
  char test_file_[FILENAME_MAX];
};

namespace ext4 {
bool getSuperBlock(const int blk_fd, struct ext4_super_block* sb) {
  if (lseek(blk_fd, SB_OFFSET, SEEK_SET) == -1) {
    testPrintE("Cannot lseek to ext4 superblock to read");
    return false;
  }

  if (read(blk_fd, sb, sizeof(*sb)) != sizeof(*sb)) {
    testPrintE("Cannot read ext4 superblock");
    return false;
  }

  if (sb->s_magic != 0xEF53) {
    testPrintE("Invalid ext4 superblock magic");
    return false;
  }

  return true;
}

bool setSbErrorBit(const int blk_fd) {
  // Read super block.
  struct ext4_super_block sb;
  if (!getSuperBlock(blk_fd, &sb)) {
    return false;
  }

  // Check that the detected errors bit is not set.
  if (sb.s_state & 0x2) {
    testPrintE("Ext4 superblock already corrupted");
    return false;
  }

  // Set the detected errors bit.
  sb.s_state |= 0x2;

  // Write superblock.
  if (lseek(blk_fd, SB_OFFSET, SEEK_SET) == -1) {
      testPrintE("Cannot lseek to superblock to write\n");
      return false;
  }

  if (write(blk_fd, &sb, sizeof(sb)) != sizeof(sb)) {
      testPrintE("Cannot write superblock\n");
      return false;
  }

  return true;
}

bool corruptGdtFreeBlock(const int blk_fd) {
  // Read super block.
  struct ext4_super_block sb;
  if (!getSuperBlock(blk_fd, &sb)) {
    return false;
  }
  // Make sure the block size is 2K or 4K.
  if ((sb.s_log_block_size != 1) && (sb.s_log_block_size != 2)) {
      testPrintE("Ext4 block size not 2K or 4K\n");
      return false;
  }
  int block_size = 1 << (10 + sb.s_log_block_size);
  int num_bgs = DIV_ROUND_UP(sb.s_blocks_count_lo, sb.s_blocks_per_group);

  if (sb.s_desc_size != sizeof(struct ext2_group_desc)) {
    testPrintE("Can't handle ext4 block group descriptor size of %d",
               sb.s_desc_size);
    return false;
  }

  // Read first block group descriptor, decrement free block count, and
  // write it back out.
  if (lseek(blk_fd, block_size, SEEK_SET) == -1) {
    testPrintE("Cannot lseek to ext4 block group descriptor table to read");
    return false;
  }

  // Read in block group descriptors till we read one that has at least one free
  // block.
  struct ext2_group_desc gd;
  for (int i = 0; i < num_bgs; i++) {
    if (read(blk_fd, &gd, sizeof(gd)) != sizeof(gd)) {
      testPrintE("Cannot read ext4 group descriptor %d", i);
      return false;
    }
    if (gd.bg_free_blocks_count) {
      break;
    }
  }

  gd.bg_free_blocks_count--;

  if (lseek(blk_fd, -sizeof(gd), SEEK_CUR) == -1) {
    testPrintE("Cannot lseek to ext4 block group descriptor table to write");
    return false;
  }

  if (write(blk_fd, &gd, sizeof(gd)) != sizeof(gd)) {
    testPrintE("Cannot write modified ext4 group descriptor");
    return false;
  }
  return true;
}

}  // namespace ext4

class FsRecoveryTest : public ::testing::Test {
 protected:
  FsRecoveryTest() : fs_type(FS_UNKNOWN), blk_fd_(-1) {}

  bool setCacheInfoFromFstab() {
    fs_type = FS_UNKNOWN;
    char propbuf[PROPERTY_VALUE_MAX];
    property_get("ro.hardware", propbuf, "");
    char fstab_filename[PROPERTY_VALUE_MAX + sizeof(FSTAB_PREFIX)];
    snprintf(fstab_filename, sizeof(fstab_filename), FSTAB_PREFIX"%s", propbuf);

    struct fstab *fstab = fs_mgr_read_fstab(fstab_filename);
    if (!fstab) {
      testPrintE("failed to open %s\n", fstab_filename);
    } else {
      // Loop through entries looking for cache.
      for (int i = 0; i < fstab->num_entries; ++i) {
        if (!strcmp(fstab->recs[i].mount_point, "/cache")) {
          strcpy(blk_path_, fstab->recs[i].blk_device);
          if (!strcmp(fstab->recs[i].fs_type, "ext4")) {
            fs_type = FS_EXT4;
            break;
          } else if (!strcmp(fstab->recs[i].fs_type, "f2fs")) {
            fs_type = FS_F2FS;
            break;
          }
        }
      }
      fs_mgr_free_fstab(fstab);
    }
    return fs_type != FS_UNKNOWN;
  }

  bool unmountCache() {
    char *umount_argv[] = {
      UMOUNT_BIN,
      "/cache"
    };
    int status;
    return android_fork_execvp_ext(ARRAY_SIZE(umount_argv), umount_argv,
                                   NULL, true, LOG_KLOG, false, NULL) >= 0;
  }

  bool mountAll() {
    char *mountall_argv[] = {
      VDC_BIN,
      "storage",
      "mountall"
    };
    int status;
    return android_fork_execvp_ext(ARRAY_SIZE(mountall_argv), mountall_argv,
                                   NULL, true, LOG_KLOG, false, NULL) >= 0;
  }

  int getCacheBlkFd() {
    if (blk_fd_ == -1) {
      blk_fd_ = open(blk_path_, O_RDWR);
    }
    return blk_fd_;
  }

  void closeCacheBlkFd() {
    if (blk_fd_ > -1) {
      close(blk_fd_);
    }
    blk_fd_ = -1;
  }

  void assertCacheHealthy() {
    const char* test_file = "/cache/FsRecoveryTestGarbage.txt";
    DataFileVerifier file_verify(test_file);
    file_verify.verify_write();
    file_verify.verify_read();
  }

  virtual void SetUp() {
    assertCacheHealthy();
    ASSERT_TRUE(setCacheInfoFromFstab());
  }

  virtual void TearDown() {
    // Ensure /cache partition is accessible, mounted and healthy for other
    // tests.
    closeCacheBlkFd();
    ASSERT_TRUE(mountAll());
    assertCacheHealthy();
  }

  Fs_Type fs_type;

 private:
  char blk_path_[FILENAME_MAX];
  int blk_fd_;
};

TEST_F(FsRecoveryTest, EXT4_CorruptGdt) {
  if (fs_type != FS_EXT4) {
    return;
  }
  // Setup test file in /cache.
  const char* test_file = "/cache/CorruptGdtGarbage.txt";
  DataFileVerifier file_verify(test_file);
  file_verify.verify_write();
  // Unmount and corrupt /cache gdt.
  ASSERT_TRUE(unmountCache());
  ASSERT_TRUE(ext4::corruptGdtFreeBlock(getCacheBlkFd()));
  closeCacheBlkFd();
  ASSERT_TRUE(mountAll());

  // Verify results.
  file_verify.verify_read();
}

TEST_F(FsRecoveryTest, EXT4_SetErrorBit) {
  if (fs_type != FS_EXT4) {
    return;
  }
  // Setup test file in /cache.
  const char* test_file = "/cache/ErrorBitGarbagetxt";
  DataFileVerifier file_verify(test_file);
  file_verify.verify_write();

  // Unmount and set /cache super block error bit.
  ASSERT_TRUE(unmountCache());
  ASSERT_TRUE(ext4::setSbErrorBit(getCacheBlkFd()));
  closeCacheBlkFd();
  ASSERT_TRUE(mountAll());

  // Verify results.
  file_verify.verify_read();
  struct ext4_super_block sb;
  ASSERT_TRUE(ext4::getSuperBlock(getCacheBlkFd(), &sb));
  // Verify e2fsck has recovered the error bit of sb.
  ASSERT_FALSE(sb.s_state & 0x2);
}
}  // namespace android
