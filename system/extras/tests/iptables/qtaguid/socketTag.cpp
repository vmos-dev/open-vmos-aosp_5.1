/*
 * Copyright (C) 2011 The Android Open Source Project
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
 * This socket tagging test is to ensure that the
 * netfilter/xt_qtaguid kernel module somewhat behaves as expected
 * with respect to tagging sockets.
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>

#define LOG_TAG "socketTagTest"
#include <utils/Log.h>
#include <testUtil.h>

namespace android {

class SockInfo {
public:
    SockInfo() : fd(-1), addr(NULL) {};
    int setup(uint64_t tag);
    bool checkTag(uint64_t tag, uid_t uid);
    int fd;
    void *addr;
};


int openCtrl() {
    int ctrl;
    ctrl = open("/proc/net/xt_qtaguid/ctrl", O_RDWR);
    if (!ctrl) {
       testPrintE("qtaguid ctrl open failed: %s", strerror(errno));
    }
    return ctrl;
}

int doCtrlCommand(const char *fmt, ...)
    __attribute__((__format__(__printf__, 1, 2)));

int doCtrlCommand(const char *fmt, ...) {
    char *buff;
    int ctrl;
    int res;
    va_list argp;

    va_start(argp, fmt);
    ctrl = openCtrl();
    vasprintf(&buff, fmt, argp);
    errno = 0;
    res = write(ctrl, buff, strlen(buff));
    testPrintI("cmd: '%s' res=%d %d/%s", buff, res, errno, strerror(errno));
    close(ctrl);
    free(buff);
    va_end(argp);
    return res;
}


int writeModuleParam(const char *param, const char *data) {
    int param_fd;
    int res;
    std::string filename("/sys/module/xt_qtaguid/parameters/");

    filename += param;
    param_fd = open(filename.c_str(), O_WRONLY);
    if (param_fd < 0) {
        testPrintE("qtaguid param open failed: %s", strerror(errno));
        return -1;
    }
    res = write(param_fd, data, strlen(data));
    if (res < 0) {
        testPrintE("qtaguid param write failed: %s", strerror(errno));
    }
    close(param_fd);
    return res;
}

/*----------------------------------------------------------------*/
int SockInfo::setup(uint64_t tag) {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        testPrintE("socket creation failed: %s", strerror(errno));
        return -1;
    }
    if (doCtrlCommand("t %d %" PRIu64, fd, tag) < 0) {
        testPrintE("socket setup: failed to tag");
        close(fd);
        return -1;
    }
    if (!checkTag(tag, getuid())) {
        testPrintE("socket setup: Unexpected results: tag not found");
        close(fd);
        return -1;
    }
    if (doCtrlCommand("u %d", fd) < 0) {
        testPrintE("socket setup: Unexpected results");
        close(fd);
        return -1;
    }
    return 0;
}

/* checkTag() also tries to lookup the socket address in the kernel and
 * return it when *addr  == NULL.
 * This allows for better look ups when another process is also setting the same
 * tag + uid. But it is not fool proof.
 * Without the kernel reporting more info on who setup the socket tag, it is
 * not easily verifiable from user-space.
 * Returns: true if tag found.
 */
bool SockInfo::checkTag(uint64_t acct_tag, uid_t uid) {
    int ctrl_fd;
    ctrl_fd = openCtrl();
    char ctrl_data[1024];
    ssize_t read_size;
    char *buff;
    char *pos;
    int res;
    char *match_template;
    uint64_t k_tag;
    uint32_t k_uid;
    uint64_t full_tag;
    long dummy_count;
    pid_t dummy_pid;

    read_size = read(ctrl_fd, ctrl_data, sizeof(ctrl_data));
    if (read_size < 0) {
       testPrintE("Unable to read active tags from ctrl %d/%s",
                  errno, strerror(errno));
    }
    ctrl_data[read_size] = '\0';
    testPrintI("<ctrl_raw_data>\n%s</ctrl_raw_data>", ctrl_data);

    if (addr) {
        assert(sizeof(void*) == sizeof(long int));  // Why does %p use 0x? grrr. %lx.
        asprintf(&match_template, "sock=%" PRIxPTR " %s", (uintptr_t)addr, "tag=0x%" PRIx64" (uid=%u)");
    }
    else {
        /* Allocate for symmetry */
        asprintf(&match_template, "%s", " tag=0x%" PRIx64 " (uid=%u)");
    }

    full_tag = acct_tag | uid;

    asprintf(&buff, match_template, full_tag | uid, uid);
    testPrintI("looking for '%s'", buff);
    pos = strstr(ctrl_data, buff);

    if (pos && !addr) {
        assert(sizeof(void*) == sizeof(long int));  // Why does %p use 0x? grrr. %lx.
        res = sscanf(pos - strlen("sock=1234abcd"),
                     "sock=%" SCNxPTR " tag=0x%" SCNx64 " (uid=%" SCNu32 ") pid=%u f_count=%lu",
                     (uintptr_t *)&addr, &k_tag, &k_uid, &dummy_pid, &dummy_count );
        if (!(res == 5 && k_tag == full_tag && k_uid == uid)) {
            testPrintE("Unable to read sock addr res=%d", res);
           addr = 0;
        }
        else {
            testPrintI("Got sock_addr %lx", addr);
        }
    }
    free(buff);
    free(match_template);
    close(ctrl_fd);
    return pos != NULL;
}


class SocketTaggingTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        ctrl_fd = -1;
        dev_fd = -1;
        my_uid = getuid();
        my_pid = getpid();
        srand48(my_pid * my_uid);
        // Adjust fake UIDs and tags so that multiple instances can run in parallel.
        fake_uid = testRand();
        fake_uid2 = testRand();
        valid_tag1 = ((uint64_t)my_pid << 48) | ((uint64_t)testRand() << 32);
        valid_tag2 = ((uint64_t)my_pid << 48) | ((uint64_t)testRand() << 32);
        valid_tag2 &= 0xffffff00ffffffffllu;  // Leave some room to make counts visible.
        testPrintI("* start: pid=%lu uid=%lu uid1=0x%lx/%lu uid2=0x%lx/%lu"
                   " tag1=0x%" PRIx64 "/%" PRIu64 " tag2=0x%" PRIx64 "/% "PRIu64,
                   (unsigned long)my_pid, (unsigned long)my_uid,
                   (unsigned long)fake_uid, (unsigned long)fake_uid,
                   (unsigned long)fake_uid2, (unsigned long)fake_uid2,
                   valid_tag1, valid_tag1, valid_tag2, valid_tag2);
        max_uint_tag = 0xffffffff00000000llu;
        max_uint_tag = 1llu << 63 | (((uint64_t)my_pid << 48) ^ max_uint_tag);

        testPrintI("kernel has qtaguid");
        ctrl_fd = openCtrl();
        ASSERT_GE(ctrl_fd, 0) << "qtaguid ctrl open failed";
        close(ctrl_fd);
        dev_fd = open("/dev/xt_qtaguid", O_RDONLY);
        EXPECT_GE(dev_fd, 0) << "qtaguid dev open failed";

        // We want to clean up any previous faulty test runs.
        testPrintI("delete command does not fail");
        EXPECT_GE(doCtrlCommand("d 0 %u", fake_uid), 0) << "Failed to delete fake_uid";
        EXPECT_GE(doCtrlCommand("d 0 %u", fake_uid2), 0) << "Failed to delete fake_uid2";
        EXPECT_GE(doCtrlCommand("d 0 %u", my_uid), 0) << "Failed to delete my_uid";

        testPrintI("setup sock0 and addr via tag");
        ASSERT_FALSE(sock0.setup(valid_tag1))  << "socket0 setup failed";
        testPrintI("setup sock1 and addr via tag");
        ASSERT_FALSE(sock1.setup(valid_tag1))  << "socket1 setup failed";
    }

   virtual void TearDown() {
       if (dev_fd >= 0) {
           close(dev_fd);
       }
       if (ctrl_fd >= 0) {
           close(ctrl_fd);
       }
   }

   SockInfo sock0;
   SockInfo sock1;
   int ctrl_fd;
   int dev_fd;
   uid_t fake_uid;
   uid_t fake_uid2;
   uid_t my_uid;
   pid_t my_pid;
   uint64_t valid_tag1;
   uint64_t valid_tag2;
   uint64_t max_uint_tag;
   static const uint64_t invalid_tag1 = 0x0000000100000001llu;
   static const int max_tags = 5;
};

TEST_F(SocketTaggingTest, TagData) {
    max_uint_tag = 0xffffffff00000000llu;
    char *max_tags_str;

    testPrintI("setup tag limit");
    asprintf(&max_tags_str, "%d", max_tags);
    ASSERT_GE(writeModuleParam("max_sock_tags", max_tags_str), 0) << "Failed to setup tag limit";

    testPrintI("tag quota reach limit");
    for (int cnt = 0; cnt < max_tags; cnt++ ) {
        uint64_t tag = valid_tag2 + ((uint64_t)cnt << 32);
        EXPECT_GE(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, tag , fake_uid2), 0)
            << "Tagging within limit failed";
        EXPECT_TRUE(sock0.checkTag(tag, fake_uid2))<<  "Unexpected results: tag not found";
    }

    testPrintI("tag quota go over limit");
    uint64_t new_tag = valid_tag2 + ((uint64_t)max_tags << 32);
    EXPECT_LT(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, new_tag, fake_uid2), 0);
    EXPECT_TRUE(sock0.checkTag(valid_tag2 + (((uint64_t)max_tags - 1) << 32),
                               fake_uid2)) << "Unexpected results: tag not found";

    testPrintI("valid untag");
    EXPECT_GE(doCtrlCommand("u %d", sock0.fd), 0);
    EXPECT_FALSE(sock0.checkTag(valid_tag2 + (((uint64_t)max_tags - 1) << 32), fake_uid2))
        << "Untagged tag should not be there";

    testPrintI("tag after untag should not free up max tags");
    uint64_t new_tag2 = valid_tag2 + ((uint64_t)max_tags << 32);
    EXPECT_LT(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, new_tag2 , fake_uid2), 0);
    EXPECT_FALSE(sock0.checkTag(valid_tag2 + ((uint64_t)max_tags << 32), fake_uid2))
        << "Tag should not be there";

    testPrintI("delete one tag");
    uint64_t new_tag3 = valid_tag2 + (((uint64_t)max_tags / 2) << 32);
    EXPECT_GE(doCtrlCommand("d %" PRIu64 " %u", new_tag3, fake_uid2), 0);

    testPrintI("2 tags after 1 delete pass/fail");
    uint64_t new_tag4;
    new_tag4 = valid_tag2 + (((uint64_t)max_tags + 1 ) << 32);
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, new_tag4 , fake_uid2), 0);
    EXPECT_TRUE(sock0.checkTag(valid_tag2 + (((uint64_t)max_tags + 1) << 32), fake_uid2))
        << "Tag not found";
    new_tag4 = valid_tag2 + (((uint64_t)max_tags + 2 ) << 32);
    EXPECT_LT(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, new_tag4 , fake_uid2), 0);
    EXPECT_FALSE(sock0.checkTag(valid_tag2 + (((uint64_t)max_tags + 2) << 32), fake_uid2))
        << "Tag should not be there";

    /* TODO(jpa): test tagging two different sockets with same tags and
     * check refcounts  the tag_node should be +2
     */
}

TEST_F(SocketTaggingTest, InsufficientArgsFails) {
    // Insufficient args. Expected failure
    EXPECT_LE(doCtrlCommand("t"), 0) << "Insufficient args, should fail.";
}

TEST_F(SocketTaggingTest, BadCommandFails) {
    // Bad command. Expected failure";
    EXPECT_LE(doCtrlCommand("?"), 0) << "Bad command, should fail";
}

TEST_F(SocketTaggingTest, NoTagNoUid) {
    // no tag, no uid
    EXPECT_GE(doCtrlCommand("t %d", sock0.fd), 0);
    ASSERT_TRUE(sock0.checkTag(0, my_uid))  << "Tag not found";
}

TEST_F(SocketTaggingTest, InvalidTagFail) {
    // Invalid tag. Expected failure
    EXPECT_LE(doCtrlCommand("t %d %" PRIu64, sock0.fd, invalid_tag1), 0);
    ASSERT_FALSE(sock0.checkTag(invalid_tag1, my_uid)) << "Tag should not be there";
}

TEST_F(SocketTaggingTest, ValidTagWithNoUid) {
    // Valid tag with no uid
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64, sock0.fd, valid_tag1), 0);
    EXPECT_TRUE(sock0.checkTag(valid_tag1, my_uid)) << "Tag not found";
}

TEST_F(SocketTaggingTest, ValidUntag) {
    // Valid untag
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64, sock0.fd, valid_tag1), 0);
    EXPECT_TRUE(sock0.checkTag(valid_tag1, my_uid)) << "Tag not found";
    EXPECT_GE(doCtrlCommand("u %d", sock0.fd), 0);
    EXPECT_FALSE(sock0.checkTag(valid_tag1, my_uid)) << "Tag should be removed";
}

TEST_F(SocketTaggingTest, ValidFirsttag) {
    // Valid 1st tag
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, valid_tag2, fake_uid), 0);
    EXPECT_TRUE(sock0.checkTag(valid_tag2, fake_uid)) << "Tag not found.";
}

TEST_F(SocketTaggingTest, ValidReTag) {
    // Valid re-tag
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, valid_tag2, fake_uid), 0);
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, valid_tag2, fake_uid), 0);
    EXPECT_TRUE(sock0.checkTag(valid_tag2, fake_uid)) << "Tag not found.";
}

TEST_F(SocketTaggingTest, ValidReTagWithAcctTagChange) {
    // Valid re-tag with acct_tag change
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, valid_tag2, fake_uid), 0);
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, valid_tag1, fake_uid), 0);
    EXPECT_TRUE(sock0.checkTag(valid_tag1, fake_uid)) << "Tag not found.";
}

TEST_F(SocketTaggingTest, ReTagWithUidChange) {
    // Re-tag with uid change
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, valid_tag1, fake_uid), 0);
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, valid_tag2, fake_uid2), 0);
}

TEST_F(SocketTaggingTest, Valid64BitAcctTag) {
    // Valid 64bit acct tag
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64, sock0.fd, max_uint_tag), 0);
    EXPECT_TRUE(sock0.checkTag(max_uint_tag, my_uid)) << "Tag not found.";
}

TEST_F(SocketTaggingTest, TagAnotherSocket) {
    testPrintI("Tag two sockets");
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64, sock0.fd, max_uint_tag), 0);
    EXPECT_GE(doCtrlCommand("t %d %" PRIu64 " %u", sock1.fd, valid_tag1, fake_uid2), 0);
    EXPECT_TRUE(sock1.checkTag(valid_tag1, fake_uid2)) << "Tag not found.";
    testPrintI("Untag socket0 of them only.");
    EXPECT_GE(doCtrlCommand("u %d", sock0.fd), 0);
    EXPECT_FALSE(sock0.checkTag(max_uint_tag, fake_uid)) << "Tag should not be there";
    EXPECT_TRUE(sock1.checkTag(valid_tag1, fake_uid2)) << "Tag not found";
    testPrintI("Now untag socket1 as well.");
    EXPECT_GE(doCtrlCommand("u %d", sock1.fd), 0);
    EXPECT_FALSE(sock1.checkTag(valid_tag1, fake_uid2)) << "Tag should not be there";
}

TEST_F(SocketTaggingTest, TagInvalidSocketFail) {
    // Invalid tag. Expected failure
    close(sock0.fd);
    EXPECT_LE(doCtrlCommand("t %d %" PRIu64 " %u", sock0.fd, valid_tag1, my_uid), 0);
    EXPECT_FALSE(sock0.checkTag(valid_tag1, my_uid)) << "Tag should not be there";
}

TEST_F(SocketTaggingTest, UntagInvalidSocketFail) {
    // Invalid untag. Expected failure";
    close(sock1.fd);
    EXPECT_LE(doCtrlCommand("u %d", sock1.fd), 0);
}

}  // namespace android
