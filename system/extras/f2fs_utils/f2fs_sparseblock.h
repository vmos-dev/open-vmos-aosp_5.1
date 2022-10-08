/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef F2FS_UTILS_F2F2_UTILS_H_
#define F2FS_UTILS_F2F2_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif





#define ver_after(a, b) (typecheck(unsigned long long, a) &&            \
    typecheck(unsigned long long, b) &&                     \
    ((long long)((a) - (b)) > 0))

#define ver_equal(a, b) (typecheck(unsigned long long, a) &&            \
    typecheck(unsigned long long, b) &&                     \
    ((long long)((a) - (b)) == 0))

struct f2fs_sit_block;
struct f2fs_summary_block;

struct f2fs_info {
    u_int64_t blocks_per_segment;
    u_int32_t block_size;

    char *sit_bmp;
    u_int32_t sit_bmp_size;
    u_int64_t blocks_per_sit;
    struct f2fs_sit_block *sit_blocks;
    struct f2fs_summary_block *sit_sums;

    u_int64_t cp_blkaddr;
    u_int64_t cp_valid_cp_blkaddr;

    u_int64_t sit_blkaddr;

    u_int64_t nat_blkaddr;

    u_int64_t ssa_blkaddr;

    u_int64_t main_blkaddr;

    u_int64_t total_user_used;
    u_int64_t total_blocks;
};

u64 get_num_blocks_used(struct f2fs_info *info);
struct f2fs_info *generate_f2fs_info(int fd);
void free_f2fs_info(struct f2fs_info *info);
unsigned int get_f2fs_filesystem_size_sec(char *dev);
int run_on_used_blocks(u64 startblock, struct f2fs_info *info, int (*func)(u64 pos, void *data), void *data);

#ifdef __cplusplus
}
#endif

#endif  // F2FS_UTILS_F2F2_UTILS_H_
