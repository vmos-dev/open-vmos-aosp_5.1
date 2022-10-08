/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#define LOG_TAG "su"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <unistd.h>
#include <time.h>

#include <pwd.h>

#include <private/android_filesystem_config.h>


void pwtoid(const char *tok, uid_t *uid, gid_t *gid)
{
    struct passwd *pw;
    pw = getpwnam(tok);
    if (pw) {
        if (uid) *uid = pw->pw_uid;
        if (gid) *gid = pw->pw_gid;
    } else {
        uid_t tmpid = atoi(tok);
        if (uid) *uid = tmpid;
        if (gid) *gid = tmpid;
    }
}

void extract_uidgids(const char *uidgids, uid_t *uid, gid_t *gid, gid_t *gids,
                     int *gids_count)
{
    char *clobberablegids;
    char *nexttok;
    char *tok;
    int gids_found;

    if (!uidgids || !*uidgids) {
        *gid = *uid = 0;
        *gids_count = 0;
        return;
    }
    clobberablegids = strdup(uidgids);
    strcpy(clobberablegids, uidgids);
    nexttok = clobberablegids;
    tok = strsep(&nexttok, ",");
    pwtoid(tok, uid, gid);
    tok = strsep(&nexttok, ",");
    if (!tok) {
        /* gid is already set above */
        *gids_count = 0;
        free(clobberablegids);
        return;
    }
    pwtoid(tok, NULL, gid);
    gids_found = 0;
    while ((gids_found < *gids_count) && (tok = strsep(&nexttok, ","))) {
        pwtoid(tok, NULL, gids);
        gids_found++;
        gids++;
    }
    if (nexttok && gids_found == *gids_count) {
        fprintf(stderr, "too many group ids\n");
    }
    *gids_count = gids_found;
    free(clobberablegids);
}

/*
 * SU can be given a specific command to exec. UID _must_ be
 * specified for this (ie argc => 3).
 *
 * Usage:
 *   su 1000
 *   su 1000 ls -l
 *  or
 *   su [uid[,gid[,group1]...] [cmd]]
 *  E.g.
 *  su 1000,shell,net_bw_acct,net_bw_stats id
 * will return
 *  uid=1000(system) gid=2000(shell) groups=3006(net_bw_stats),3007(net_bw_acct)
 */
int main(int argc, char **argv)
{
    struct passwd *pw;
    uid_t uid, myuid;
    gid_t gid, gids[10];

    /* Until we have something better, only root and the shell can use su. */
    myuid = getuid();
    if (myuid != AID_ROOT && myuid != AID_SHELL) {
        fprintf(stderr,"su: uid %d not allowed to su\n", myuid);
        return 1;
    }

    if(argc < 2) {
        uid = gid = 0;
    } else {
        int gids_count = sizeof(gids)/sizeof(gids[0]);
        extract_uidgids(argv[1], &uid, &gid, gids, &gids_count);
        if(gids_count) {
            if(setgroups(gids_count, gids)) {
                fprintf(stderr, "su: failed to set groups\n");
                return 1;
            }
        }
    }

    if(setgid(gid) || setuid(uid)) {
        fprintf(stderr,"su: permission denied\n");
        return 1;
    }

    /* User specified command for exec. */
    if (argc == 3 ) {
        if (execlp(argv[2], argv[2], NULL) < 0) {
            int saved_errno = errno;
            fprintf(stderr, "su: exec failed for %s Error:%s\n", argv[2],
                    strerror(errno));
            return -saved_errno;
        }
    } else if (argc > 3) {
        /* Copy the rest of the args from main. */
        char *exec_args[argc - 1];
        memset(exec_args, 0, sizeof(exec_args));
        memcpy(exec_args, &argv[2], sizeof(exec_args));
        if (execvp(argv[2], exec_args) < 0) {
            int saved_errno = errno;
            fprintf(stderr, "su: exec failed for %s Error:%s\n", argv[2],
                    strerror(errno));
            return -saved_errno;
        }
    }

    /* Default exec shell. */
    execlp("/system/bin/sh", "sh", NULL);

    fprintf(stderr, "su: exec failed\n");
    return 1;
}
