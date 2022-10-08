/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cryptfs_hw.h>
#include <stdlib.h>
#include <sys/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#define LOG_TAG "Cryptfs_HW"
#include "cutils/log.h"
#include "cutils/android_reboot.h"


// When device comes up or when user tries to change the password, user can
// try wrong password upto a certain number of times. If user enters wrong
// password further, HW would wipe all disk encryption related crypto data
// and would return an error ERR_MAX_PASSWORD_ATTEMPTS to VOLD. VOLD would
// wipe userdata partition once this error is received.
#define ERR_MAX_PASSWORD_ATTEMPTS -10
#define QSEECOM_DISK_ENCRYPTION 1
#define MAX_PASSWORD_LEN 32

/* Operations that be performed on HW based device encryption key */
#define SET_HW_DISK_ENC_KEY 1
#define UPDATE_HW_DISK_ENC_KEY 2

static int loaded_library = 0;
static unsigned char current_passwd[MAX_PASSWORD_LEN];
static int (*qseecom_create_key)(int, void*);
static int (*qseecom_update_key)(int, void*, void*);
static int (*qseecom_wipe_key)(int);

static unsigned char* get_tmp_passwd(const char* passwd)
{
    int passwd_len = 0;
    unsigned char * tmp_passwd = NULL;
    if(passwd) {
        tmp_passwd = (unsigned char*)malloc(MAX_PASSWORD_LEN);
        if(tmp_passwd) {
            memset(tmp_passwd, 0, MAX_PASSWORD_LEN);
            passwd_len = (strlen(passwd) > MAX_PASSWORD_LEN) ? MAX_PASSWORD_LEN : strlen(passwd);
            memcpy(tmp_passwd, passwd, passwd_len);
        } else {
            SLOGE("%s: Failed to allocate memory for tmp passwd \n", __func__);
        }
    } else {
        SLOGE("%s: Passed argument is NULL \n", __func__);
    }
    return tmp_passwd;
}

static int load_qseecom_library()
{
    const char *error = NULL;
    if (loaded_library)
        return loaded_library;

    void * handle = dlopen("libQSEEComAPI.so", RTLD_NOW);
    if(handle) {
        dlerror(); /* Clear any existing error */
        *(void **) (&qseecom_create_key) = dlsym(handle,"QSEECom_create_key");

        if((error = dlerror()) == NULL) {
            *(void **) (&qseecom_update_key) = dlsym(handle,"QSEECom_update_key_user_info");
            if((error = dlerror()) == NULL) {
                *(void **) (&qseecom_wipe_key) = dlsym(handle,"QSEECom_wipe_key");
                if ((error = dlerror()) == NULL)
                  loaded_library = 1;
                else
                  SLOGE("Error %s loading symbols for QSEECom APIs", error);
            }
        }
    } else {
        SLOGE("Could not load libQSEEComAPI.so");
    }

    if(error)
        dlclose(handle);

    return loaded_library;
}

static unsigned int set_key(const char* passwd, const char* enc_mode, int operation)
{
    int ret = 0;
    int err = -1;
    if (is_hw_disk_encryption(enc_mode) && load_qseecom_library()) {
        unsigned char* tmp_passwd = get_tmp_passwd(passwd);
        if(tmp_passwd) {

            if (operation == UPDATE_HW_DISK_ENC_KEY)
                err = qseecom_update_key(QSEECOM_DISK_ENCRYPTION, current_passwd, tmp_passwd);
            else if (operation == SET_HW_DISK_ENC_KEY)
                err = qseecom_create_key(QSEECOM_DISK_ENCRYPTION, tmp_passwd);

            if(!err) {
                memset(current_passwd, 0, MAX_PASSWORD_LEN);
                memcpy(current_passwd, tmp_passwd, MAX_PASSWORD_LEN);
                ret = 1;
            } else {
                if(ERR_MAX_PASSWORD_ATTEMPTS == err)
                    SLOGE("Maximum password attempts reached. Need factory data reset to recover.");
            }
            free(tmp_passwd);
        }
    }
    return ret;
}

unsigned int set_hw_device_encryption_key(const char* passwd, const char* enc_mode)
{
    SLOGI("Set key for HW disk encryption operation");
    return set_key(passwd, enc_mode, SET_HW_DISK_ENC_KEY);
}

unsigned int update_hw_device_encryption_key(const char* newpw, const char* enc_mode)
{
    SLOGI("Update key for HW disk encryption operation");
    return set_key(newpw, enc_mode, UPDATE_HW_DISK_ENC_KEY);
}

unsigned int is_hw_disk_encryption(const char* encryption_mode)
{
    int ret = 0;
    if(encryption_mode) {
        if (!strcmp(encryption_mode, "aes-xts")) {
            ret = 1;
        }
    }
    return ret;
}

unsigned int clear_hw_device_encryption_key()
{
    int err = -1;
    int rc = 0;
    SLOGI("Clear HW disk encryption key");
    if (load_qseecom_library()) {
        err = qseecom_wipe_key(1);
    }

    if (!err) rc = 1;

    return rc;
}
