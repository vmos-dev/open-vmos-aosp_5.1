/* //device/system/toolbox/resetradio.c
**
** Copyright 2006, The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cutils/sockets.h>

#define SOCKET_NAME_RIL_DEBUG	"rild-debug"	/* from ril.cpp */
#define SOCKET_NAME_RIL2_DEBUG	"rild2-debug"

enum options {
    RADIO_RESET,
    RADIO_OFF,
    UNSOL_NETWORK_STATE_CHANGE,
    QXDM_ENABLE,
    QXDM_DISABLE,
    RADIO_ON,
    SETUP_PDP,
    DEACTIVATE_PDP,
    DIAL_CALL,
    ANSWER_CALL,
    END_CALL,
};


static void print_usage() {
    perror("Usage: radiooptions [option] [extra_socket_args]\n\
           0 - RADIO_RESET, \n\
           1 - RADIO_OFF, \n\
           2 - UNSOL_NETWORK_STATE_CHANGE, \n\
           3 - QXDM_ENABLE, \n\
           4 - QXDM_DISABLE, \n\
           5 - RADIO_ON, \n\
           6 apn- SETUP_PDP apn, \n\
           7 - DEACTIVE_PDP, \n\
           8 number - DIAL_CALL number, \n\
           9 - ANSWER_CALL, \n\
           10 - END_CALL \n\
          The argument before the last one must be SIM slot \n\
           0 - SIM1, \n\
           1 - SIM2, \n\
           2 - SIM3, \n\
           3 - SIM4, \n\
          The last argument must be modem-socket style \n\
           0 - one modem for one debug-socket, \n\
           1 - one modem for multiple debug socket \n");
}

static int error_check(int argc, char * argv[]) {
    if (argc < 2) {
        return -1;
    }
    const int option = atoi(argv[1]);
    if (option < 0 || option > 10) {
        return 0;
    } else if ((option == DIAL_CALL || option == SETUP_PDP) && argc == 5) {
        return 0;
    } else if ((option != DIAL_CALL && option != SETUP_PDP) && argc == 4) {
        return 0;
    }
    return -1;
}

static int get_number_args(char *argv[]) {
    const int option = atoi(argv[1]);
    if (option != DIAL_CALL && option != SETUP_PDP) {
        return 3;
    } else {
        return 4;
    }
}

int main(int argc, char *argv[])
{
    int fd;
    int num_socket_args = 0;
    int i  = 0;
    int modem_socket_type = 0;
    int sim_id = 0;
    char socket_name[20];

    if(error_check(argc, argv)) {
        print_usage();
        exit(-1);
    }

    num_socket_args = get_number_args(argv);
    modem_socket_type = atoi(argv[(num_socket_args-1)]);
    sim_id = atoi(argv[(num_socket_args-2)]);
    memset(socket_name, 0, sizeof(char)*20);
    if (sim_id == 0 || modem_socket_type == 1) {
        strncpy(socket_name, SOCKET_NAME_RIL_DEBUG, 19);
    } else if (sim_id == 1) {
        strncpy(socket_name, SOCKET_NAME_RIL2_DEBUG, 19);
    }
    
    fd = socket_local_client(socket_name,
                                 ANDROID_SOCKET_NAMESPACE_RESERVED,
                                 SOCK_STREAM);
    if (fd < 0) {
        perror ("opening radio debug socket");
        exit(-1);
    }

    
    /* It is not necessacry to pass the argument "modem-socket style" to rild */
    num_socket_args--;
    int ret = send(fd, (const void *)&num_socket_args, sizeof(int), 0);
    if(ret != sizeof(int)) {
        perror ("Socket write error when sending num args");
        close(fd);
        exit(-1);
    }

    for (i = 0; i < num_socket_args; i++) {
        // Send length of the arg, followed by the arg.
        int len = strlen(argv[1 + i]);
        ret = send(fd, &len, sizeof(int), 0);
        if (ret != sizeof(int)) {
            perror("Socket write Error: when sending arg length");
            close(fd);
            exit(-1);
        }
        ret = send(fd, argv[1 + i], sizeof(char) * len, 0);
        if (ret != len * sizeof(char)) {
            perror ("Socket write Error: When sending arg");
            close(fd);
            exit(-1);
        }
    }

    close(fd);
    return 0;
}
