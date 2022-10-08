/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include <linux/kdev_t.h>

#define LOG_TAG "Vold"

#include <openssl/md5.h>

#include <cutils/fs.h>
#include <cutils/log.h>

#include <selinux/android.h>

#include <sysutils/NetlinkEvent.h>

#include <private/android_filesystem_config.h>

#include "VolumeManager.h"
#include "DirectVolume.h"
#include "ResponseCode.h"
#include "Loop.h"
#include "Ext4.h"
#include "Fat.h"
#include "Devmapper.h"
#include "Process.h"
#include "Asec.h"
#include "cryptfs.h"

#define MASS_STORAGE_FILE_PATH  "/sys/class/android_usb/android0/f_mass_storage/lun/file"

#define ROUND_UP_POWER_OF_2(number, po2) (((!!(number & ((1U << po2) - 1))) << po2)\
                                         + (number & (~((1U << po2) - 1))))

/* writes superblock at end of file or device given by name */
static int writeSuperBlock(const char* name, struct asec_superblock *sb, unsigned int numImgSectors) {
    int sbfd = open(name, O_RDWR);
    if (sbfd < 0) {
        SLOGE("Failed to open %s for superblock write (%s)", name, strerror(errno));
        return -1;
    }

    if (lseek(sbfd, (numImgSectors * 512), SEEK_SET) < 0) {
        SLOGE("Failed to lseek for superblock (%s)", strerror(errno));
        close(sbfd);
        return -1;
    }

    if (write(sbfd, sb, sizeof(struct asec_superblock)) != sizeof(struct asec_superblock)) {
        SLOGE("Failed to write superblock (%s)", strerror(errno));
        close(sbfd);
        return -1;
    }
    close(sbfd);
    return 0;
}

static int adjustSectorNumExt4(unsigned numSectors) {
    // Ext4 started to reserve 2% or 4096 clusters, whichever is smaller for
    // preventing costly operations or unexpected ENOSPC error.
    // Ext4::format() uses default block size without clustering.
    unsigned clusterSectors = 4096 / 512;
    unsigned reservedSectors = (numSectors * 2)/100 + (numSectors % 50 > 0);
    numSectors += reservedSectors > (4096 * clusterSectors) ? (4096 * clusterSectors) : reservedSectors;
    return ROUND_UP_POWER_OF_2(numSectors, 3);
}

static int adjustSectorNumFAT(unsigned numSectors) {
    /*
    * Add some headroom
    */
    unsigned fatSize = (((numSectors * 4) / 512) + 1) * 2;
    numSectors += fatSize + 2;
    /*
    * FAT is aligned to 32 kb with 512b sectors.
    */
    return ROUND_UP_POWER_OF_2(numSectors, 6);
}

static int setupLoopDevice(char* buffer, size_t len, const char* asecFileName, const char* idHash, bool debug) {
    if (Loop::lookupActive(idHash, buffer, len)) {
        if (Loop::create(idHash, asecFileName, buffer, len)) {
            SLOGE("ASEC loop device creation failed for %s (%s)", asecFileName, strerror(errno));
            return -1;
        }
        if (debug) {
            SLOGD("New loop device created at %s", buffer);
        }
    } else {
        if (debug) {
            SLOGD("Found active loopback for %s at %s", asecFileName, buffer);
        }
    }
    return 0;
}

static int setupDevMapperDevice(char* buffer, size_t len, const char* loopDevice, const char* asecFileName, const char* key, const char* idHash , int numImgSectors, bool* createdDMDevice, bool debug) {
    if (strcmp(key, "none")) {
        if (Devmapper::lookupActive(idHash, buffer, len)) {
            if (Devmapper::create(idHash, loopDevice, key, numImgSectors,
                                  buffer, len)) {
                SLOGE("ASEC device mapping failed for %s (%s)", asecFileName, strerror(errno));
                return -1;
            }
            if (debug) {
                SLOGD("New devmapper instance created at %s", buffer);
            }
        } else {
            if (debug) {
                SLOGD("Found active devmapper for %s at %s", asecFileName, buffer);
            }
        }
        *createdDMDevice = true;
    } else {
        strcpy(buffer, loopDevice);
        *createdDMDevice = false;
    }
    return 0;
}

static void waitForDevMapper(const char *dmDevice) {
    /*
     * Wait for the device mapper node to be created. Sometimes it takes a
     * while. Wait for up to 1 second. We could also inspect incoming uevents,
     * but that would take more effort.
     */
    int tries = 25;
    while (tries--) {
        if (!access(dmDevice, F_OK) || errno != ENOENT) {
            break;
        }
        usleep(40 * 1000);
    }
}

VolumeManager *VolumeManager::sInstance = NULL;

VolumeManager *VolumeManager::Instance() {
    if (!sInstance)
        sInstance = new VolumeManager();
    return sInstance;
}

VolumeManager::VolumeManager() {
    mDebug = false;
    mVolumes = new VolumeCollection();
    mActiveContainers = new AsecIdCollection();
    mBroadcaster = NULL;
    mUmsSharingCount = 0;
    mSavedDirtyRatio = -1;
    // set dirty ratio to 0 when UMS is active
    mUmsDirtyRatio = 0;
    mVolManagerDisabled = 0;
}

VolumeManager::~VolumeManager() {
    delete mVolumes;
    delete mActiveContainers;
}

char *VolumeManager::asecHash(const char *id, char *buffer, size_t len) {
    static const char* digits = "0123456789abcdef";

    unsigned char sig[MD5_DIGEST_LENGTH];

    if (buffer == NULL) {
        SLOGE("Destination buffer is NULL");
        errno = ESPIPE;
        return NULL;
    } else if (id == NULL) {
        SLOGE("Source buffer is NULL");
        errno = ESPIPE;
        return NULL;
    } else if (len < MD5_ASCII_LENGTH_PLUS_NULL) {
        SLOGE("Target hash buffer size < %d bytes (%zu)",
                MD5_ASCII_LENGTH_PLUS_NULL, len);
        errno = ESPIPE;
        return NULL;
    }

    MD5(reinterpret_cast<const unsigned char*>(id), strlen(id), sig);

    char *p = buffer;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        *p++ = digits[sig[i] >> 4];
        *p++ = digits[sig[i] & 0x0F];
    }
    *p = '\0';

    return buffer;
}

void VolumeManager::setDebug(bool enable) {
    mDebug = enable;
    VolumeCollection::iterator it;
    for (it = mVolumes->begin(); it != mVolumes->end(); ++it) {
        (*it)->setDebug(enable);
    }
}

int VolumeManager::start() {
    return 0;
}

int VolumeManager::stop() {
    return 0;
}

int VolumeManager::addVolume(Volume *v) {
    mVolumes->push_back(v);
    return 0;
}

void VolumeManager::handleBlockEvent(NetlinkEvent *evt) {
    const char *devpath = evt->findParam("DEVPATH");

    /* Lookup a volume to handle this device */
    VolumeCollection::iterator it;
    bool hit = false;
    for (it = mVolumes->begin(); it != mVolumes->end(); ++it) {
        if (!(*it)->handleBlockEvent(evt)) {
#ifdef NETLINK_DEBUG
            SLOGD("Device '%s' event handled by volume %s\n", devpath, (*it)->getLabel());
#endif
            hit = true;
            break;
        }
    }

    if (!hit) {
#ifdef NETLINK_DEBUG
        SLOGW("No volumes handled block event for '%s'", devpath);
#endif
    }
}

int VolumeManager::listVolumes(SocketClient *cli, bool broadcast) {
    VolumeCollection::iterator i;
    char msg[256];

    for (i = mVolumes->begin(); i != mVolumes->end(); ++i) {
        char *buffer;
        asprintf(&buffer, "%s %s %d",
                 (*i)->getLabel(), (*i)->getFuseMountpoint(),
                 (*i)->getState());
        cli->sendMsg(ResponseCode::VolumeListResult, buffer, false);
        free(buffer);
        if (broadcast) {
            if((*i)->getUuid()) {
                snprintf(msg, sizeof(msg), "%s %s \"%s\"", (*i)->getLabel(),
                    (*i)->getFuseMountpoint(), (*i)->getUuid());
                mBroadcaster->sendBroadcast(ResponseCode::VolumeUuidChange,
                    msg, false);
            }
            if((*i)->getUserLabel()) {
                snprintf(msg, sizeof(msg), "%s %s \"%s\"", (*i)->getLabel(),
                    (*i)->getFuseMountpoint(), (*i)->getUserLabel());
                mBroadcaster->sendBroadcast(ResponseCode::VolumeUserLabelChange,
                    msg, false);
            }
        }
    }
    cli->sendMsg(ResponseCode::CommandOkay, "Volumes listed.", false);
    return 0;
}

int VolumeManager::formatVolume(const char *label, bool wipe) {
    Volume *v = lookupVolume(label);

    if (!v) {
        errno = ENOENT;
        return -1;
    }

    if (mVolManagerDisabled) {
        errno = EBUSY;
        return -1;
    }

    return v->formatVol(wipe);
}

int VolumeManager::getObbMountPath(const char *sourceFile, char *mountPath, int mountPathLen) {
    char idHash[33];
    if (!asecHash(sourceFile, idHash, sizeof(idHash))) {
        SLOGE("Hash of '%s' failed (%s)", sourceFile, strerror(errno));
        return -1;
    }

    memset(mountPath, 0, mountPathLen);
    int written = snprintf(mountPath, mountPathLen, "%s/%s", Volume::LOOPDIR, idHash);
    if ((written < 0) || (written >= mountPathLen)) {
        errno = EINVAL;
        return -1;
    }

    if (access(mountPath, F_OK)) {
        errno = ENOENT;
        return -1;
    }

    return 0;
}

int VolumeManager::getAsecMountPath(const char *id, char *buffer, int maxlen) {
    char asecFileName[255];

    if (!isLegalAsecId(id)) {
        SLOGE("getAsecMountPath: Invalid asec id \"%s\"", id);
        errno = EINVAL;
        return -1;
    }

    if (findAsec(id, asecFileName, sizeof(asecFileName))) {
        SLOGE("Couldn't find ASEC %s", id);
        return -1;
    }

    memset(buffer, 0, maxlen);
    if (access(asecFileName, F_OK)) {
        errno = ENOENT;
        return -1;
    }

    int written = snprintf(buffer, maxlen, "%s/%s", Volume::ASECDIR, id);
    if ((written < 0) || (written >= maxlen)) {
        SLOGE("getAsecMountPath failed for %s: couldn't construct path in buffer", id);
        errno = EINVAL;
        return -1;
    }

    return 0;
}

int VolumeManager::getAsecFilesystemPath(const char *id, char *buffer, int maxlen) {
    char asecFileName[255];

    if (!isLegalAsecId(id)) {
        SLOGE("getAsecFilesystemPath: Invalid asec id \"%s\"", id);
        errno = EINVAL;
        return -1;
    }

    if (findAsec(id, asecFileName, sizeof(asecFileName))) {
        SLOGE("Couldn't find ASEC %s", id);
        return -1;
    }

    memset(buffer, 0, maxlen);
    if (access(asecFileName, F_OK)) {
        errno = ENOENT;
        return -1;
    }

    int written = snprintf(buffer, maxlen, "%s", asecFileName);
    if ((written < 0) || (written >= maxlen)) {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

int VolumeManager::createAsec(const char *id, unsigned int numSectors, const char *fstype,
        const char *key, const int ownerUid, bool isExternal) {
    struct asec_superblock sb;
    memset(&sb, 0, sizeof(sb));

    if (!isLegalAsecId(id)) {
        SLOGE("createAsec: Invalid asec id \"%s\"", id);
        errno = EINVAL;
        return -1;
    }

    const bool wantFilesystem = strcmp(fstype, "none");
    bool usingExt4 = false;
    if (wantFilesystem) {
        usingExt4 = !strcmp(fstype, "ext4");
        if (usingExt4) {
            sb.c_opts |= ASEC_SB_C_OPTS_EXT4;
        } else if (strcmp(fstype, "fat")) {
            SLOGE("Invalid filesystem type %s", fstype);
            errno = EINVAL;
            return -1;
        }
    }

    sb.magic = ASEC_SB_MAGIC;
    sb.ver = ASEC_SB_VER;

    if (numSectors < ((1024*1024)/512)) {
        SLOGE("Invalid container size specified (%d sectors)", numSectors);
        errno = EINVAL;
        return -1;
    }

    if (lookupVolume(id)) {
        SLOGE("ASEC id '%s' currently exists", id);
        errno = EADDRINUSE;
        return -1;
    }

    char asecFileName[255];

    if (!findAsec(id, asecFileName, sizeof(asecFileName))) {
        SLOGE("ASEC file '%s' currently exists - destroy it first! (%s)",
                asecFileName, strerror(errno));
        errno = EADDRINUSE;
        return -1;
    }

    const char *asecDir = isExternal ? Volume::SEC_ASECDIR_EXT : Volume::SEC_ASECDIR_INT;

    int written = snprintf(asecFileName, sizeof(asecFileName), "%s/%s.asec", asecDir, id);
    if ((written < 0) || (size_t(written) >= sizeof(asecFileName))) {
        errno = EINVAL;
        return -1;
    }

    if (!access(asecFileName, F_OK)) {
        SLOGE("ASEC file '%s' currently exists - destroy it first! (%s)",
                asecFileName, strerror(errno));
        errno = EADDRINUSE;
        return -1;
    }

    unsigned numImgSectors;
    if (usingExt4)
        numImgSectors = adjustSectorNumExt4(numSectors);
    else
        numImgSectors = adjustSectorNumFAT(numSectors);

    // Add +1 for our superblock which is at the end
    if (Loop::createImageFile(asecFileName, numImgSectors + 1)) {
        SLOGE("ASEC image file creation failed (%s)", strerror(errno));
        return -1;
    }

    char idHash[33];
    if (!asecHash(id, idHash, sizeof(idHash))) {
        SLOGE("Hash of '%s' failed (%s)", id, strerror(errno));
        unlink(asecFileName);
        return -1;
    }

    char loopDevice[255];
    if (Loop::create(idHash, asecFileName, loopDevice, sizeof(loopDevice))) {
        SLOGE("ASEC loop device creation failed (%s)", strerror(errno));
        unlink(asecFileName);
        return -1;
    }

    char dmDevice[255];
    bool cleanupDm = false;

    if (strcmp(key, "none")) {
        // XXX: This is all we support for now
        sb.c_cipher = ASEC_SB_C_CIPHER_TWOFISH;
        if (Devmapper::create(idHash, loopDevice, key, numImgSectors, dmDevice,
                             sizeof(dmDevice))) {
            SLOGE("ASEC device mapping failed (%s)", strerror(errno));
            Loop::destroyByDevice(loopDevice);
            unlink(asecFileName);
            return -1;
        }
        cleanupDm = true;
    } else {
        sb.c_cipher = ASEC_SB_C_CIPHER_NONE;
        strcpy(dmDevice, loopDevice);
    }

    /*
     * Drop down the superblock at the end of the file
     */
    if (writeSuperBlock(loopDevice, &sb, numImgSectors)) {
        if (cleanupDm) {
            Devmapper::destroy(idHash);
        }
        Loop::destroyByDevice(loopDevice);
        unlink(asecFileName);
        return -1;
    }

    if (wantFilesystem) {
        int formatStatus;
        char mountPoint[255];

        int written = snprintf(mountPoint, sizeof(mountPoint), "%s/%s", Volume::ASECDIR, id);
        if ((written < 0) || (size_t(written) >= sizeof(mountPoint))) {
            SLOGE("ASEC fs format failed: couldn't construct mountPoint");
            if (cleanupDm) {
                Devmapper::destroy(idHash);
            }
            Loop::destroyByDevice(loopDevice);
            unlink(asecFileName);
            return -1;
        }

        if (usingExt4) {
            formatStatus = Ext4::format(dmDevice, numImgSectors, mountPoint);
        } else {
            formatStatus = Fat::format(dmDevice, numImgSectors, 0);
        }

        if (formatStatus < 0) {
            SLOGE("ASEC fs format failed (%s)", strerror(errno));
            if (cleanupDm) {
                Devmapper::destroy(idHash);
            }
            Loop::destroyByDevice(loopDevice);
            unlink(asecFileName);
            return -1;
        }

        if (mkdir(mountPoint, 0000)) {
            if (errno != EEXIST) {
                SLOGE("Mountpoint creation failed (%s)", strerror(errno));
                if (cleanupDm) {
                    Devmapper::destroy(idHash);
                }
                Loop::destroyByDevice(loopDevice);
                unlink(asecFileName);
                return -1;
            }
        }

        int mountStatus;
        if (usingExt4) {
            mountStatus = Ext4::doMount(dmDevice, mountPoint, false, false, false);
        } else {
            mountStatus = Fat::doMount(dmDevice, mountPoint, false, false, false, ownerUid, 0, 0000,
                    false);
        }

        if (mountStatus) {
            SLOGE("ASEC FAT mount failed (%s)", strerror(errno));
            if (cleanupDm) {
                Devmapper::destroy(idHash);
            }
            Loop::destroyByDevice(loopDevice);
            unlink(asecFileName);
            return -1;
        }

        if (usingExt4) {
            int dirfd = open(mountPoint, O_DIRECTORY);
            if (dirfd >= 0) {
                if (fchown(dirfd, ownerUid, AID_SYSTEM)
                        || fchmod(dirfd, S_IRUSR | S_IWUSR | S_IXUSR | S_ISGID | S_IRGRP | S_IXGRP)) {
                    SLOGI("Cannot chown/chmod new ASEC mount point %s", mountPoint);
                }
                close(dirfd);
            }
        }
    } else {
        SLOGI("Created raw secure container %s (no filesystem)", id);
    }

    mActiveContainers->push_back(new ContainerData(strdup(id), ASEC));
    return 0;
}

int VolumeManager::resizeAsec(const char *id, unsigned numSectors, const char *key) {
    char asecFileName[255];
    char mountPoint[255];
    bool cleanupDm = false;

    if (!isLegalAsecId(id)) {
        SLOGE("resizeAsec: Invalid asec id \"%s\"", id);
        errno = EINVAL;
        return -1;
    }

    if (findAsec(id, asecFileName, sizeof(asecFileName))) {
        SLOGE("Couldn't find ASEC %s", id);
        return -1;
    }

    int written = snprintf(mountPoint, sizeof(mountPoint), "%s/%s", Volume::ASECDIR, id);
    if ((written < 0) || (size_t(written) >= sizeof(mountPoint))) {
       SLOGE("ASEC resize failed for %s: couldn't construct mountpoint", id);
       return -1;
    }

    if (isMountpointMounted(mountPoint)) {
       SLOGE("ASEC %s mounted. Unmount before resizing", id);
       errno = EBUSY;
       return -1;
    }

    struct asec_superblock sb;
    int fd;
    unsigned int oldNumSec = 0;

    if ((fd = open(asecFileName, O_RDONLY)) < 0) {
        SLOGE("Failed to open ASEC file (%s)", strerror(errno));
        return -1;
    }

    struct stat info;
    if (fstat(fd, &info) < 0) {
        SLOGE("Failed to get file size (%s)", strerror(errno));
        close(fd);
        return -1;
    }

    oldNumSec = info.st_size / 512;

    unsigned numImgSectors;
    if (sb.c_opts & ASEC_SB_C_OPTS_EXT4)
        numImgSectors = adjustSectorNumExt4(numSectors);
    else
        numImgSectors = adjustSectorNumFAT(numSectors);
    /*
     *  add one block for the superblock
     */
    SLOGD("Resizing from %d sectors to %d sectors", oldNumSec, numImgSectors + 1);
    if (oldNumSec == numImgSectors + 1) {
        SLOGW("Size unchanged; ignoring resize request");
        return 0;
    } else if (oldNumSec > numImgSectors + 1) {
        SLOGE("Only growing is currently supported.");
        close(fd);
        return -1;
    }

    /*
     * Try to read superblock.
     */
    memset(&sb, 0, sizeof(struct asec_superblock));
    if (lseek(fd, ((oldNumSec - 1) * 512), SEEK_SET) < 0) {
        SLOGE("lseek failed (%s)", strerror(errno));
        close(fd);
        return -1;
    }
    if (read(fd, &sb, sizeof(struct asec_superblock)) != sizeof(struct asec_superblock)) {
        SLOGE("superblock read failed (%s)", strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);

    if (mDebug) {
        SLOGD("Container sb magic/ver (%.8x/%.2x)", sb.magic, sb.ver);
    }
    if (sb.magic != ASEC_SB_MAGIC || sb.ver != ASEC_SB_VER) {
        SLOGE("Bad container magic/version (%.8x/%.2x)", sb.magic, sb.ver);
        errno = EMEDIUMTYPE;
        return -1;
    }

    if (!(sb.c_opts & ASEC_SB_C_OPTS_EXT4)) {
        SLOGE("Only ext4 partitions are supported for resize");
        errno = EINVAL;
        return -1;
    }

    if (Loop::resizeImageFile(asecFileName, numImgSectors + 1)) {
        SLOGE("Resize of ASEC image file failed. Could not resize %s", id);
        return -1;
    }

    /*
     * Drop down a copy of the superblock at the end of the file
     */
    if (writeSuperBlock(asecFileName, &sb, numImgSectors))
        goto fail;

    char idHash[33];
    if (!asecHash(id, idHash, sizeof(idHash))) {
        SLOGE("Hash of '%s' failed (%s)", id, strerror(errno));
        goto fail;
    }

    char loopDevice[255];
    if (setupLoopDevice(loopDevice, sizeof(loopDevice), asecFileName, idHash, mDebug))
        goto fail;

    char dmDevice[255];

    if (setupDevMapperDevice(dmDevice, sizeof(dmDevice), loopDevice, asecFileName, key, idHash, numImgSectors, &cleanupDm, mDebug)) {
        Loop::destroyByDevice(loopDevice);
        goto fail;
    }

    /*
     * Wait for the device mapper node to be created.
     */
    waitForDevMapper(dmDevice);

    if (Ext4::resize(dmDevice, numImgSectors)) {
        SLOGE("Unable to resize %s (%s)", id, strerror(errno));
        if (cleanupDm) {
            Devmapper::destroy(idHash);
        }
        Loop::destroyByDevice(loopDevice);
        goto fail;
    }

    return 0;
fail:
    Loop::resizeImageFile(asecFileName, oldNumSec);
    return -1;
}

int VolumeManager::finalizeAsec(const char *id) {
    char asecFileName[255];
    char loopDevice[255];
    char mountPoint[255];

    if (!isLegalAsecId(id)) {
        SLOGE("finalizeAsec: Invalid asec id \"%s\"", id);
        errno = EINVAL;
        return -1;
    }

    if (findAsec(id, asecFileName, sizeof(asecFileName))) {
        SLOGE("Couldn't find ASEC %s", id);
        return -1;
    }

    char idHash[33];
    if (!asecHash(id, idHash, sizeof(idHash))) {
        SLOGE("Hash of '%s' failed (%s)", id, strerror(errno));
        return -1;
    }

    if (Loop::lookupActive(idHash, loopDevice, sizeof(loopDevice))) {
        SLOGE("Unable to finalize %s (%s)", id, strerror(errno));
        return -1;
    }

    unsigned int nr_sec = 0;
    struct asec_superblock sb;

    if (Loop::lookupInfo(loopDevice, &sb, &nr_sec)) {
        return -1;
    }

    int written = snprintf(mountPoint, sizeof(mountPoint), "%s/%s", Volume::ASECDIR, id);
    if ((written < 0) || (size_t(written) >= sizeof(mountPoint))) {
        SLOGE("ASEC finalize failed: couldn't construct mountPoint");
        return -1;
    }

    int result = 0;
    if (sb.c_opts & ASEC_SB_C_OPTS_EXT4) {
        result = Ext4::doMount(loopDevice, mountPoint, true, true, true);
    } else {
        result = Fat::doMount(loopDevice, mountPoint, true, true, true, 0, 0, 0227, false);
    }

    if (result) {
        SLOGE("ASEC finalize mount failed (%s)", strerror(errno));
        return -1;
    }

    if (mDebug) {
        SLOGD("ASEC %s finalized", id);
    }
    return 0;
}

int VolumeManager::fixupAsecPermissions(const char *id, gid_t gid, const char* filename) {
    char asecFileName[255];
    char loopDevice[255];
    char mountPoint[255];

    if (gid < AID_APP) {
        SLOGE("Group ID is not in application range");
        return -1;
    }

    if (!isLegalAsecId(id)) {
        SLOGE("fixupAsecPermissions: Invalid asec id \"%s\"", id);
        errno = EINVAL;
        return -1;
    }

    if (findAsec(id, asecFileName, sizeof(asecFileName))) {
        SLOGE("Couldn't find ASEC %s", id);
        return -1;
    }

    char idHash[33];
    if (!asecHash(id, idHash, sizeof(idHash))) {
        SLOGE("Hash of '%s' failed (%s)", id, strerror(errno));
        return -1;
    }

    if (Loop::lookupActive(idHash, loopDevice, sizeof(loopDevice))) {
        SLOGE("Unable fix permissions during lookup on %s (%s)", id, strerror(errno));
        return -1;
    }

    unsigned int nr_sec = 0;
    struct asec_superblock sb;

    if (Loop::lookupInfo(loopDevice, &sb, &nr_sec)) {
        return -1;
    }

    int written = snprintf(mountPoint, sizeof(mountPoint), "%s/%s", Volume::ASECDIR, id);
    if ((written < 0) || (size_t(written) >= sizeof(mountPoint))) {
        SLOGE("Unable remount to fix permissions for %s: couldn't construct mountpoint", id);
        return -1;
    }

    int result = 0;
    if ((sb.c_opts & ASEC_SB_C_OPTS_EXT4) == 0) {
        return 0;
    }

    int ret = Ext4::doMount(loopDevice, mountPoint,
            false /* read-only */,
            true  /* remount */,
            false /* executable */);
    if (ret) {
        SLOGE("Unable remount to fix permissions for %s (%s)", id, strerror(errno));
        return -1;
    }

    char *paths[] = { mountPoint, NULL };

    FTS *fts = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR | FTS_XDEV, NULL);
    if (fts) {
        // Traverse the entire hierarchy and chown to system UID.
        for (FTSENT *ftsent = fts_read(fts); ftsent != NULL; ftsent = fts_read(fts)) {
            // We don't care about the lost+found directory.
            if (!strcmp(ftsent->fts_name, "lost+found")) {
                continue;
            }

            /*
             * There can only be one file marked as private right now.
             * This should be more robust, but it satisfies the requirements
             * we have for right now.
             */
            const bool privateFile = !strcmp(ftsent->fts_name, filename);

            int fd = open(ftsent->fts_accpath, O_NOFOLLOW);
            if (fd < 0) {
                SLOGE("Couldn't open file %s: %s", ftsent->fts_accpath, strerror(errno));
                result = -1;
                continue;
            }

            result |= fchown(fd, AID_SYSTEM, privateFile? gid : AID_SYSTEM);

            if (ftsent->fts_info & FTS_D) {
                result |= fchmod(fd, 0755);
            } else if (ftsent->fts_info & FTS_F) {
                result |= fchmod(fd, privateFile ? 0640 : 0644);
            }

            if (selinux_android_restorecon(ftsent->fts_path, 0) < 0) {
                SLOGE("restorecon failed for %s: %s\n", ftsent->fts_path, strerror(errno));
                result |= -1;
            }

            close(fd);
        }
        fts_close(fts);

        // Finally make the directory readable by everyone.
        int dirfd = open(mountPoint, O_DIRECTORY);
        if (dirfd < 0 || fchmod(dirfd, 0755)) {
            SLOGE("Couldn't change owner of existing directory %s: %s", mountPoint, strerror(errno));
            result |= -1;
        }
        close(dirfd);
    } else {
        result |= -1;
    }

    result |= Ext4::doMount(loopDevice, mountPoint,
            true /* read-only */,
            true /* remount */,
            true /* execute */);

    if (result) {
        SLOGE("ASEC fix permissions failed (%s)", strerror(errno));
        return -1;
    }

    if (mDebug) {
        SLOGD("ASEC %s permissions fixed", id);
    }
    return 0;
}

int VolumeManager::renameAsec(const char *id1, const char *id2) {
    char asecFilename1[255];
    char *asecFilename2;
    char mountPoint[255];

    const char *dir;

    if (!isLegalAsecId(id1)) {
        SLOGE("renameAsec: Invalid asec id1 \"%s\"", id1);
        errno = EINVAL;
        return -1;
    }

    if (!isLegalAsecId(id2)) {
        SLOGE("renameAsec: Invalid asec id2 \"%s\"", id2);
        errno = EINVAL;
        return -1;
    }
#if 0
    if (findAsec(id1, asecFilename1, sizeof(asecFilename1), &dir)) {
        SLOGE("Couldn't find ASEC %s", id1);
        return -1;
    }
#endif
    sprintf(asecFilename1, "/data/app/%s", id1);    // Volume::ASECDIR -> /data/app/
    asprintf(&asecFilename2, "/data/app/%s", id2);  // Volume::ASECDIR -> /data/app/
#if 0
    int written = snprintf(mountPoint, sizeof(mountPoint), "%s/%s", Volume::ASECDIR, id1);
    if ((written < 0) || (size_t(written) >= sizeof(mountPoint))) {
        SLOGE("Rename failed: couldn't construct mountpoint");
        goto out_err;
    }

    if (isMountpointMounted(mountPoint)) {
        SLOGW("Rename attempt when src mounted");
        errno = EBUSY;
        goto out_err;
    }

    written = snprintf(mountPoint, sizeof(mountPoint), "%s/%s", Volume::ASECDIR, id2);
    if ((written < 0) || (size_t(written) >= sizeof(mountPoint))) {
        SLOGE("Rename failed: couldn't construct mountpoint2");
        goto out_err;
    }

    if (isMountpointMounted(mountPoint)) {
        SLOGW("Rename attempt when dst mounted");
        errno = EBUSY;
        goto out_err;
    }
#endif

    if (!access(asecFilename2, F_OK)) {
        SLOGE("Rename attempt when dst exists");
        errno = EADDRINUSE;
        goto out_err;
    }

    if (rename(asecFilename1, asecFilename2)) {
        SLOGE("Rename of '%s' to '%s' failed (%s)", asecFilename1, asecFilename2, strerror(errno));
        goto out_err;
    }

    free(asecFilename2);
    return 0;

out_err:
    free(asecFilename2);
    return -1;
}

#define UNMOUNT_RETRIES 5
#define UNMOUNT_SLEEP_BETWEEN_RETRY_MS (1000 * 1000)
int VolumeManager::unmountAsec(const char *id, bool force) {
    char asecFileName[255];
    char mountPoint[255];

    if (!isLegalAsecId(id)) {
        SLOGE("unmountAsec: Invalid asec id \"%s\"", id);
        errno = EINVAL;
        return -1;
    }

    if (findAsec(id, asecFileName, sizeof(asecFileName))) {
        SLOGE("Couldn't find ASEC %s", id);
        return -1;
    }

    int written = snprintf(mountPoint, sizeof(mountPoint), "%s/%s", Volume::ASECDIR, id);
    if ((written < 0) || (size_t(written) >= sizeof(mountPoint))) {
        SLOGE("ASEC unmount failed for %s: couldn't construct mountpoint", id);
        return -1;
    }

    char idHash[33];
    if (!asecHash(id, idHash, sizeof(idHash))) {
        SLOGE("Hash of '%s' failed (%s)", id, strerror(errno));
        return -1;
    }

    return unmountLoopImage(id, idHash, asecFileName, mountPoint, force);
}

int VolumeManager::unmountObb(const char *fileName, bool force) {
    char mountPoint[255];

    char idHash[33];
    if (!asecHash(fileName, idHash, sizeof(idHash))) {
        SLOGE("Hash of '%s' failed (%s)", fileName, strerror(errno));
        return -1;
    }

    int written = snprintf(mountPoint, sizeof(mountPoint), "%s/%s", Volume::LOOPDIR, idHash);
    if ((written < 0) || (size_t(written) >= sizeof(mountPoint))) {
        SLOGE("OBB unmount failed for %s: couldn't construct mountpoint", fileName);
        return -1;
    }

    return unmountLoopImage(fileName, idHash, fileName, mountPoint, force);
}

int VolumeManager::unmountLoopImage(const char *id, const char *idHash,
        const char *fileName, const char *mountPoint, bool force) {
    if (!isMountpointMounted(mountPoint)) {
        SLOGE("Unmount request for %s when not mounted", id);
        errno = ENOENT;
        return -1;
    }

    int i, rc;
    for (i = 1; i <= UNMOUNT_RETRIES; i++) {
        rc = umount(mountPoint);
        if (!rc) {
            break;
        }
        if (rc && (errno == EINVAL || errno == ENOENT)) {
            SLOGI("Container %s unmounted OK", id);
            rc = 0;
            break;
        }
        SLOGW("%s unmount attempt %d failed (%s)",
              id, i, strerror(errno));

        int action = 0; // default is to just complain

        if (force) {
            if (i > (UNMOUNT_RETRIES - 2))
                action = 2; // SIGKILL
            else if (i > (UNMOUNT_RETRIES - 3))
                action = 1; // SIGHUP
        }

        Process::killProcessesWithOpenFiles(mountPoint, action);
        usleep(UNMOUNT_SLEEP_BETWEEN_RETRY_MS);
    }

    if (rc) {
        errno = EBUSY;
        SLOGE("Failed to unmount container %s (%s)", id, strerror(errno));
        return -1;
    }

    int retries = 10;

    while(retries--) {
        if (!rmdir(mountPoint)) {
            break;
        }

        SLOGW("Failed to rmdir %s (%s)", mountPoint, strerror(errno));
        usleep(UNMOUNT_SLEEP_BETWEEN_RETRY_MS);
    }

    if (!retries) {
        SLOGE("Timed out trying to rmdir %s (%s)", mountPoint, strerror(errno));
    }

    for (i=1; i <= UNMOUNT_RETRIES; i++) {
        if (Devmapper::destroy(idHash) && errno != ENXIO) {
            SLOGE("Failed to destroy devmapper instance (%s)", strerror(errno));
            usleep(UNMOUNT_SLEEP_BETWEEN_RETRY_MS);
            continue;
        } else {
          break;
        }
    }

    char loopDevice[255];
    if (!Loop::lookupActive(idHash, loopDevice, sizeof(loopDevice))) {
        Loop::destroyByDevice(loopDevice);
    } else {
        SLOGW("Failed to find loop device for {%s} (%s)", fileName, strerror(errno));
    }

    AsecIdCollection::iterator it;
    for (it = mActiveContainers->begin(); it != mActiveContainers->end(); ++it) {
        ContainerData* cd = *it;
        if (!strcmp(cd->id, id)) {
            free(*it);
            mActiveContainers->erase(it);
            break;
        }
    }
    if (it == mActiveContainers->end()) {
        SLOGW("mActiveContainers is inconsistent!");
    }
    return 0;
}

int VolumeManager::destroyAsec(const char *id, bool force) {
    char asecFileName[255];
    char mountPoint[255];

    if (!isLegalAsecId(id)) {
        SLOGE("destroyAsec: Invalid asec id \"%s\"", id);
        errno = EINVAL;
        return -1;
    }

    if (findAsec(id, asecFileName, sizeof(asecFileName))) {
        SLOGE("Couldn't find ASEC %s", id);
        return -1;
    }

    int written = snprintf(mountPoint, sizeof(mountPoint), "%s/%s", Volume::ASECDIR, id);
    if ((written < 0) || (size_t(written) >= sizeof(mountPoint))) {
        SLOGE("ASEC destroy failed for %s: couldn't construct mountpoint", id);
        return -1;
    }

    if (isMountpointMounted(mountPoint)) {
        if (mDebug) {
            SLOGD("Unmounting container before destroy");
        }
        if (unmountAsec(id, force)) {
            SLOGE("Failed to unmount asec %s for destroy (%s)", id, strerror(errno));
            return -1;
        }
    }

    if (unlink(asecFileName)) {
        SLOGE("Failed to unlink asec '%s' (%s)", asecFileName, strerror(errno));
        return -1;
    }

    if (mDebug) {
        SLOGD("ASEC %s destroyed", id);
    }
    return 0;
}

/*
 * Legal ASEC ids consist of alphanumeric characters, '-',
 * '_', or '.'. ".." is not allowed. The first or last character
 * of the ASEC id cannot be '.' (dot).
 */
bool VolumeManager::isLegalAsecId(const char *id) const {
    size_t i;
    size_t len = strlen(id);

    if (len == 0) {
        return false;
    }
    if ((id[0] == '.') || (id[len - 1] == '.')) {
        return false;
    }

    for (i = 0; i < len; i++) {
        if (id[i] == '.') {
            // i=0 is guaranteed never to have a dot. See above.
            if (id[i-1] == '.') return false;
            continue;
        }
        if (id[i] == '_' || id[i] == '-') continue;
        if (id[i] >= 'a' && id[i] <= 'z') continue;
        if (id[i] >= 'A' && id[i] <= 'Z') continue;
        if (id[i] >= '0' && id[i] <= '9') continue;
        return false;
    }

    return true;
}

bool VolumeManager::isAsecInDirectory(const char *dir, const char *asecName) const {
    int dirfd = open(dir, O_DIRECTORY);
    if (dirfd < 0) {
        SLOGE("Couldn't open internal ASEC dir (%s)", strerror(errno));
        return -1;
    }

    bool ret = false;

    if (!faccessat(dirfd, asecName, F_OK, AT_SYMLINK_NOFOLLOW)) {
        ret = true;
    }

    close(dirfd);

    return ret;
}

int VolumeManager::findAsec(const char *id, char *asecPath, size_t asecPathLen,
        const char **directory) const {
    int dirfd, fd;
    const int idLen = strlen(id);
    char *asecName;

    if (!isLegalAsecId(id)) {
        SLOGE("findAsec: Invalid asec id \"%s\"", id);
        errno = EINVAL;
        return -1;
    }

    if (asprintf(&asecName, "%s.asec", id) < 0) {
        SLOGE("Couldn't allocate string to write ASEC name");
        return -1;
    }

    const char *dir;
    if (isAsecInDirectory(Volume::SEC_ASECDIR_INT, asecName)) {
        dir = Volume::SEC_ASECDIR_INT;
    } else if (isAsecInDirectory(Volume::SEC_ASECDIR_EXT, asecName)) {
        dir = Volume::SEC_ASECDIR_EXT;
    } else {
        free(asecName);
        return -1;
    }

    if (directory != NULL) {
        *directory = dir;
    }

    if (asecPath != NULL) {
        int written = snprintf(asecPath, asecPathLen, "%s/%s", dir, asecName);
        if ((written < 0) || (size_t(written) >= asecPathLen)) {
            SLOGE("findAsec failed for %s: couldn't construct ASEC path", id);
            free(asecName);
            return -1;
        }
    }

    free(asecName);
    return 0;
}

int VolumeManager::mountAsec(const char *id, const char *key, int ownerUid, bool readOnly) {
    char asecFileName[255];
    char mountPoint[255];

    if (!isLegalAsecId(id)) {
        SLOGE("mountAsec: Invalid asec id \"%s\"", id);
        errno = EINVAL;
        return -1;
    }

    if (findAsec(id, asecFileName, sizeof(asecFileName))) {
        SLOGE("Couldn't find ASEC %s", id);
        return -1;
    }

    int written = snprintf(mountPoint, sizeof(mountPoint), "%s/%s", Volume::ASECDIR, id);
    if ((written < 0) || (size_t(written) >= sizeof(mountPoint))) {
        SLOGE("ASEC mount failed for %s: couldn't construct mountpoint", id);
        return -1;
    }

    if (isMountpointMounted(mountPoint)) {
        SLOGE("ASEC %s already mounted", id);
        errno = EBUSY;
        return -1;
    }

    char idHash[33];
    if (!asecHash(id, idHash, sizeof(idHash))) {
        SLOGE("Hash of '%s' failed (%s)", id, strerror(errno));
        return -1;
    }

    char loopDevice[255];
    if (setupLoopDevice(loopDevice, sizeof(loopDevice), asecFileName, idHash, mDebug))
        return -1;

    char dmDevice[255];
    bool cleanupDm = false;
    int fd;
    unsigned int nr_sec = 0;
    struct asec_superblock sb;

    if (Loop::lookupInfo(loopDevice, &sb, &nr_sec)) {
        return -1;
    }

    if (mDebug) {
        SLOGD("Container sb magic/ver (%.8x/%.2x)", sb.magic, sb.ver);
    }
    if (sb.magic != ASEC_SB_MAGIC || sb.ver != ASEC_SB_VER) {
        SLOGE("Bad container magic/version (%.8x/%.2x)", sb.magic, sb.ver);
        Loop::destroyByDevice(loopDevice);
        errno = EMEDIUMTYPE;
        return -1;
    }
    nr_sec--; // We don't want the devmapping to extend onto our superblock

    if (setupDevMapperDevice(dmDevice, sizeof(dmDevice), loopDevice, asecFileName, key, idHash , nr_sec, &cleanupDm, mDebug)) {
        Loop::destroyByDevice(loopDevice);
        return -1;
    }

    if (mkdir(mountPoint, 0000)) {
        if (errno != EEXIST) {
            SLOGE("Mountpoint creation failed (%s)", strerror(errno));
            if (cleanupDm) {
                Devmapper::destroy(idHash);
            }
            Loop::destroyByDevice(loopDevice);
            return -1;
        }
    }

    /*
     * Wait for the device mapper node to be created.
     */
    waitForDevMapper(dmDevice);

    int result;
    if (sb.c_opts & ASEC_SB_C_OPTS_EXT4) {
        result = Ext4::doMount(dmDevice, mountPoint, readOnly, false, readOnly);
    } else {
        result = Fat::doMount(dmDevice, mountPoint, readOnly, false, readOnly, ownerUid, 0, 0222, false);
    }

    if (result) {
        SLOGE("ASEC mount failed (%s)", strerror(errno));
        if (cleanupDm) {
            Devmapper::destroy(idHash);
        }
        Loop::destroyByDevice(loopDevice);
        return -1;
    }

    mActiveContainers->push_back(new ContainerData(strdup(id), ASEC));
    if (mDebug) {
        SLOGD("ASEC %s mounted", id);
    }
    return 0;
}

Volume* VolumeManager::getVolumeForFile(const char *fileName) {
    VolumeCollection::iterator i;

    for (i = mVolumes->begin(); i != mVolumes->end(); ++i) {
        const char* mountPoint = (*i)->getFuseMountpoint();
        if (!strncmp(fileName, mountPoint, strlen(mountPoint))) {
            return *i;
        }
    }

    return NULL;
}

/**
 * Mounts an image file <code>img</code>.
 */
int VolumeManager::mountObb(const char *img, const char *key, int ownerGid) {
    char mountPoint[255];

    char idHash[33];
    if (!asecHash(img, idHash, sizeof(idHash))) {
        SLOGE("Hash of '%s' failed (%s)", img, strerror(errno));
        return -1;
    }

    int written = snprintf(mountPoint, sizeof(mountPoint), "%s/%s", Volume::LOOPDIR, idHash);
    if ((written < 0) || (size_t(written) >= sizeof(mountPoint))) {
        SLOGE("OBB mount failed for %s: couldn't construct mountpoint", img);
        return -1;
    }

    if (isMountpointMounted(mountPoint)) {
        SLOGE("Image %s already mounted", img);
        errno = EBUSY;
        return -1;
    }

    char loopDevice[255];
    if (setupLoopDevice(loopDevice, sizeof(loopDevice), img, idHash, mDebug))
        return -1;

    char dmDevice[255];
    bool cleanupDm = false;
    int fd;
    unsigned int nr_sec = 0;

    if ((fd = open(loopDevice, O_RDWR)) < 0) {
        SLOGE("Failed to open loopdevice (%s)", strerror(errno));
        Loop::destroyByDevice(loopDevice);
        return -1;
    }

    if (ioctl(fd, BLKGETSIZE, &nr_sec)) {
        SLOGE("Failed to get loop size (%s)", strerror(errno));
        Loop::destroyByDevice(loopDevice);
        close(fd);
        return -1;
    }

    close(fd);

    if (setupDevMapperDevice(dmDevice, sizeof(loopDevice), loopDevice, img,key, idHash , nr_sec, &cleanupDm, mDebug)) {
        Loop::destroyByDevice(loopDevice);
        return -1;
    }

    if (mkdir(mountPoint, 0755)) {
        if (errno != EEXIST) {
            SLOGE("Mountpoint creation failed (%s)", strerror(errno));
            if (cleanupDm) {
                Devmapper::destroy(idHash);
            }
            Loop::destroyByDevice(loopDevice);
            return -1;
        }
    }

    if (Fat::doMount(dmDevice, mountPoint, true, false, true, 0, ownerGid,
                     0227, false)) {
        SLOGE("Image mount failed (%s)", strerror(errno));
        if (cleanupDm) {
            Devmapper::destroy(idHash);
        }
        Loop::destroyByDevice(loopDevice);
        return -1;
    }

    mActiveContainers->push_back(new ContainerData(strdup(img), OBB));
    if (mDebug) {
        SLOGD("Image %s mounted", img);
    }
    return 0;
}

int VolumeManager::mountVolume(const char *label) {
    Volume *v = lookupVolume(label);

    if (!v) {
        errno = ENOENT;
        return -1;
    }

    return v->mountVol();
}

int VolumeManager::listMountedObbs(SocketClient* cli) {
    char device[256];
    char mount_path[256];
    char rest[256];
    FILE *fp;
    char line[1024];

    if (!(fp = fopen("/proc/mounts", "r"))) {
        SLOGE("Error opening /proc/mounts (%s)", strerror(errno));
        return -1;
    }

    // Create a string to compare against that has a trailing slash
    int loopDirLen = strlen(Volume::LOOPDIR);
    char loopDir[loopDirLen + 2];
    strcpy(loopDir, Volume::LOOPDIR);
    loopDir[loopDirLen++] = '/';
    loopDir[loopDirLen] = '\0';

    while(fgets(line, sizeof(line), fp)) {
        line[strlen(line)-1] = '\0';

        /*
         * Should look like:
         * /dev/block/loop0 /mnt/obb/fc99df1323fd36424f864dcb76b76d65 ...
         */
        sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);

        if (!strncmp(mount_path, loopDir, loopDirLen)) {
            int fd = open(device, O_RDONLY);
            if (fd >= 0) {
                struct loop_info64 li;
                if (ioctl(fd, LOOP_GET_STATUS64, &li) >= 0) {
                    cli->sendMsg(ResponseCode::AsecListResult,
                            (const char*) li.lo_file_name, false);
                }
                close(fd);
            }
        }
    }

    fclose(fp);
    return 0;
}

int VolumeManager::shareEnabled(const char *label, const char *method, bool *enabled) {
    Volume *v = lookupVolume(label);

    if (!v) {
        errno = ENOENT;
        return -1;
    }

    if (strcmp(method, "ums")) {
        errno = ENOSYS;
        return -1;
    }

    if (v->getState() != Volume::State_Shared) {
        *enabled = false;
    } else {
        *enabled = true;
    }
    return 0;
}

int VolumeManager::shareVolume(const char *label, const char *method) {
    Volume *v = lookupVolume(label);

    if (!v) {
        errno = ENOENT;
        return -1;
    }

    /*
     * Eventually, we'll want to support additional share back-ends,
     * some of which may work while the media is mounted. For now,
     * we just support UMS
     */
    if (strcmp(method, "ums")) {
        errno = ENOSYS;
        return -1;
    }

    if (v->getState() == Volume::State_NoMedia) {
        errno = ENODEV;
        return -1;
    }

    if (v->getState() != Volume::State_Idle) {
        // You need to unmount manually befoe sharing
        errno = EBUSY;
        return -1;
    }

    if (mVolManagerDisabled) {
        errno = EBUSY;
        return -1;
    }

    dev_t d = v->getShareDevice();
    if ((MAJOR(d) == 0) && (MINOR(d) == 0)) {
        // This volume does not support raw disk access
        errno = EINVAL;
        return -1;
    }

    int fd;
    char nodepath[255];
    int written = snprintf(nodepath,
             sizeof(nodepath), "/dev/block/vold/%d:%d",
             major(d), minor(d));

    if ((written < 0) || (size_t(written) >= sizeof(nodepath))) {
        SLOGE("shareVolume failed: couldn't construct nodepath");
        return -1;
    }

    if ((fd = open(MASS_STORAGE_FILE_PATH, O_WRONLY)) < 0) {
        SLOGE("Unable to open ums lunfile (%s)", strerror(errno));
        return -1;
    }

    if (write(fd, nodepath, strlen(nodepath)) < 0) {
        SLOGE("Unable to write to ums lunfile (%s)", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    v->handleVolumeShared();
    if (mUmsSharingCount++ == 0) {
        FILE* fp;
        mSavedDirtyRatio = -1; // in case we fail
        if ((fp = fopen("/proc/sys/vm/dirty_ratio", "r+"))) {
            char line[16];
            if (fgets(line, sizeof(line), fp) && sscanf(line, "%d", &mSavedDirtyRatio)) {
                fprintf(fp, "%d\n", mUmsDirtyRatio);
            } else {
                SLOGE("Failed to read dirty_ratio (%s)", strerror(errno));
            }
            fclose(fp);
        } else {
            SLOGE("Failed to open /proc/sys/vm/dirty_ratio (%s)", strerror(errno));
        }
    }
    return 0;
}

int VolumeManager::unshareVolume(const char *label, const char *method) {
    Volume *v = lookupVolume(label);

    if (!v) {
        errno = ENOENT;
        return -1;
    }

    if (strcmp(method, "ums")) {
        errno = ENOSYS;
        return -1;
    }

    if (v->getState() != Volume::State_Shared) {
        errno = EINVAL;
        return -1;
    }

    int fd;
    if ((fd = open(MASS_STORAGE_FILE_PATH, O_WRONLY)) < 0) {
        SLOGE("Unable to open ums lunfile (%s)", strerror(errno));
        return -1;
    }

    char ch = 0;
    if (write(fd, &ch, 1) < 0) {
        SLOGE("Unable to write to ums lunfile (%s)", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    v->handleVolumeUnshared();
    if (--mUmsSharingCount == 0 && mSavedDirtyRatio != -1) {
        FILE* fp;
        if ((fp = fopen("/proc/sys/vm/dirty_ratio", "r+"))) {
            fprintf(fp, "%d\n", mSavedDirtyRatio);
            fclose(fp);
        } else {
            SLOGE("Failed to open /proc/sys/vm/dirty_ratio (%s)", strerror(errno));
        }
        mSavedDirtyRatio = -1;
    }
    return 0;
}

extern "C" int vold_disableVol(const char *label) {
    VolumeManager *vm = VolumeManager::Instance();
    vm->disableVolumeManager();
    vm->unshareVolume(label, "ums");
    return vm->unmountVolume(label, true, false);
}

extern "C" int vold_getNumDirectVolumes(void) {
    VolumeManager *vm = VolumeManager::Instance();
    return vm->getNumDirectVolumes();
}

int VolumeManager::getNumDirectVolumes(void) {
    VolumeCollection::iterator i;
    int n=0;

    for (i = mVolumes->begin(); i != mVolumes->end(); ++i) {
        if ((*i)->getShareDevice() != (dev_t)0) {
            n++;
        }
    }
    return n;
}

extern "C" int vold_getDirectVolumeList(struct volume_info *vol_list) {
    VolumeManager *vm = VolumeManager::Instance();
    return vm->getDirectVolumeList(vol_list);
}

int VolumeManager::getDirectVolumeList(struct volume_info *vol_list) {
    VolumeCollection::iterator i;
    int n=0;
    dev_t d;

    for (i = mVolumes->begin(); i != mVolumes->end(); ++i) {
        if ((d=(*i)->getShareDevice()) != (dev_t)0) {
            (*i)->getVolInfo(&vol_list[n]);
            snprintf(vol_list[n].blk_dev, sizeof(vol_list[n].blk_dev),
                     "/dev/block/vold/%d:%d", major(d), minor(d));
            n++;
        }
    }

    return 0;
}

int VolumeManager::unmountVolume(const char *label, bool force, bool revert) {
    Volume *v = lookupVolume(label);

    if (!v) {
        errno = ENOENT;
        return -1;
    }

    if (v->getState() == Volume::State_NoMedia) {
        errno = ENODEV;
        return -1;
    }

    if (v->getState() != Volume::State_Mounted) {
        SLOGW("Attempt to unmount volume which isn't mounted (%d)\n",
             v->getState());
        errno = EBUSY;
        return UNMOUNT_NOT_MOUNTED_ERR;
    }

    cleanupAsec(v, force);

    return v->unmountVol(force, revert);
}

extern "C" int vold_unmountAllAsecs(void) {
    int rc;

    VolumeManager *vm = VolumeManager::Instance();
    rc = vm->unmountAllAsecsInDir(Volume::SEC_ASECDIR_EXT);
    if (vm->unmountAllAsecsInDir(Volume::SEC_ASECDIR_INT)) {
        rc = -1;
    }
    return rc;
}

#define ID_BUF_LEN 256
#define ASEC_SUFFIX ".asec"
#define ASEC_SUFFIX_LEN (sizeof(ASEC_SUFFIX) - 1)
int VolumeManager::unmountAllAsecsInDir(const char *directory) {
    DIR *d = opendir(directory);
    int rc = 0;

    if (!d) {
        SLOGE("Could not open asec dir %s", directory);
        return -1;
    }

    size_t dirent_len = offsetof(struct dirent, d_name) +
            fpathconf(dirfd(d), _PC_NAME_MAX) + 1;

    struct dirent *dent = (struct dirent *) malloc(dirent_len);
    if (dent == NULL) {
        SLOGE("Failed to allocate memory for asec dir");
        return -1;
    }

    struct dirent *result;
    while (!readdir_r(d, dent, &result) && result != NULL) {
        if (dent->d_name[0] == '.')
            continue;
        if (dent->d_type != DT_REG)
            continue;
        size_t name_len = strlen(dent->d_name);
        if (name_len > 5 && name_len < (ID_BUF_LEN + ASEC_SUFFIX_LEN - 1) &&
                !strcmp(&dent->d_name[name_len - 5], ASEC_SUFFIX)) {
            char id[ID_BUF_LEN];
            strlcpy(id, dent->d_name, name_len - 4);
            if (unmountAsec(id, true)) {
                /* Register the error, but try to unmount more asecs */
                rc = -1;
            }
        }
    }
    closedir(d);

    free(dent);

    return rc;
}

/*
 * Looks up a volume by it's label or mount-point
 */
Volume *VolumeManager::lookupVolume(const char *label) {
    VolumeCollection::iterator i;

    for (i = mVolumes->begin(); i != mVolumes->end(); ++i) {
        if (label[0] == '/') {
            if (!strcmp(label, (*i)->getFuseMountpoint()))
                return (*i);
        } else {
            if (!strcmp(label, (*i)->getLabel()))
                return (*i);
        }
    }
    return NULL;
}

bool VolumeManager::isMountpointMounted(const char *mp)
{
    char device[256];
    char mount_path[256];
    char rest[256];
    FILE *fp;
    char line[1024];

    if (!(fp = fopen("/proc/mounts", "r"))) {
        SLOGE("Error opening /proc/mounts (%s)", strerror(errno));
        return false;
    }

    while(fgets(line, sizeof(line), fp)) {
        line[strlen(line)-1] = '\0';
        sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
        if (!strcmp(mount_path, mp)) {
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false;
}

int VolumeManager::cleanupAsec(Volume *v, bool force) {
    int rc = 0;

    char asecFileName[255];

    AsecIdCollection removeAsec;
    AsecIdCollection removeObb;

    for (AsecIdCollection::iterator it = mActiveContainers->begin(); it != mActiveContainers->end();
            ++it) {
        ContainerData* cd = *it;

        if (cd->type == ASEC) {
            if (findAsec(cd->id, asecFileName, sizeof(asecFileName))) {
                SLOGE("Couldn't find ASEC %s; cleaning up", cd->id);
                removeAsec.push_back(cd);
            } else {
                SLOGD("Found ASEC at path %s", asecFileName);
                if (!strncmp(asecFileName, Volume::SEC_ASECDIR_EXT,
                        strlen(Volume::SEC_ASECDIR_EXT))) {
                    removeAsec.push_back(cd);
                }
            }
        } else if (cd->type == OBB) {
            if (v == getVolumeForFile(cd->id)) {
                removeObb.push_back(cd);
            }
        } else {
            SLOGE("Unknown container type %d!", cd->type);
        }
    }

    for (AsecIdCollection::iterator it = removeAsec.begin(); it != removeAsec.end(); ++it) {
        ContainerData *cd = *it;
        SLOGI("Unmounting ASEC %s (dependent on %s)", cd->id, v->getLabel());
        if (unmountAsec(cd->id, force)) {
            SLOGE("Failed to unmount ASEC %s (%s)", cd->id, strerror(errno));
            rc = -1;
        }
    }

    for (AsecIdCollection::iterator it = removeObb.begin(); it != removeObb.end(); ++it) {
        ContainerData *cd = *it;
        SLOGI("Unmounting OBB %s (dependent on %s)", cd->id, v->getLabel());
        if (unmountObb(cd->id, force)) {
            SLOGE("Failed to unmount OBB %s (%s)", cd->id, strerror(errno));
            rc = -1;
        }
    }

    return rc;
}

int VolumeManager::mkdirs(char* path) {
    // Require that path lives under a volume we manage and is mounted
    const char* emulated_source = getenv("EMULATED_STORAGE_SOURCE");
    const char* root = NULL;
    if (emulated_source && !strncmp(path, emulated_source, strlen(emulated_source))) {
        root = emulated_source;
    } else {
        Volume* vol = getVolumeForFile(path);
        if (vol && vol->getState() == Volume::State_Mounted) {
            root = vol->getMountpoint();
        }
    }

    if (!root) {
        SLOGE("Failed to find mounted volume for %s", path);
        return -EINVAL;
    }

    /* fs_mkdirs() does symlink checking and relative path enforcement */
    return fs_mkdirs(path, 0700);
}
