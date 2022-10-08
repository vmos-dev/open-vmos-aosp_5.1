/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include <cutils/uevent.h>
#include <stdio.h>

#define UEVENT_MSG_LEN  1024

int main(int argc, char *argv[])
{
    int device_fd;
    char msg[UEVENT_MSG_LEN+2];
    int n;
    int i;

    device_fd = uevent_open_socket(64*1024, true);
    if(device_fd < 0)
        return -1;

    while ((n = uevent_kernel_multicast_recv(device_fd, msg, UEVENT_MSG_LEN)) > 0) {
        msg[n] = '\0';
        msg[n+1] = '\0';

        for (i = 0; i < n; i++)
            if (msg[i] == '\0')
                msg[i] = ' ';

        printf("%s\n", msg);
    }

    return 0;
}
