/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "cutils/misc.h"
#include "cutils/properties.h"
#include "edify/expr.h"
#include "mincrypt/sha.h"
#include "minzip/DirUtil.h"
#include "mtdutils/mounts.h"
#include "mtdutils/mtdutils.h"
#include "downloadFN.h"

Value* DownloadModemFn(const char* name, State* state, int argc, Expr* argv[]) {

    char* modemImg;
    int result = -1;

    if (argc != 1)
        return ErrorAbort(state, "%s() expects 1 arg, got %d", name, argc);

    if (ReadArgs(state, argv, 1, &modemImg) != 0) {
        return NULL;
    }

    printf("DownloadModemFn: %s\n", modemImg);

    result = DownloadFiles(modemImg);

    if (result < 0) {
        printf("Download failed!\n");
    } else {
        printf("Download success!\n");
        ResetModem();
        sleep(6);
    }
    return StringValue(strdup(result >= 0 ? "t" : ""));
}

void Register_librecovery_updater_tilapia() {
    printf("Register_librecovery_updater_tilapia is called\n");
    RegisterFunction("bach.update_modem", DownloadModemFn);
}
