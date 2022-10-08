/*
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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
 *   * Neither the name of Code Aurora Forum, Inc. nor the names of its
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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "alsa_ucm.h"
#include "msm8960_use_cases.h"

/* Function prototypes */
static void print_help_menu(void);
static void alsaucm_test_cmd_svr(void);
static int process_cmd(char *cmdStr);

/* Global data */
snd_use_case_mgr_t *uc_mgr;

/* Defines */
enum ucm_cmd_id {
    UCM_OPEN = 0,
    UCM_SET,
    UCM_LISTCARDS,
    UCM_LIST,
    UCM_GET,
    UCM_GETI,
    UCM_RESET,
    UCM_RELOAD,
    UCM_HELP,
    UCM_QUIT,
    UCM_UNKNOWN
};

struct cmd {
    enum ucm_cmd_id code;
    const char *cmd_str;
};

static struct cmd cmds[] = {
    { UCM_OPEN, "open" },
    { UCM_SET,  "set" },
    { UCM_LISTCARDS,  "listcards" },
    { UCM_LIST,  "list" },
    { UCM_GET,  "get" },
    { UCM_GETI,  "geti" },
    { UCM_RESET,  "reset" },
    { UCM_RELOAD,  "reload" },
    { UCM_HELP,  "help" },
    { UCM_QUIT,  "quit" },
    { UCM_UNKNOWN, NULL }
};

static void alsaucm_test_cmd_svr(void)
{
    int fd;
    ssize_t read_count;
    char cmdstr[256] = {'\0'};
    char ch;
    char *exit_str = "quit";

    if (mknod("/data/alsaucm_test", S_IFIFO | 0666, 0) == 0) {
        fd = open("/data/alsaucm_test", O_RDONLY);
        while (1) {
            read_count = read(fd, &ch, 1);
            if (read_count == 0) {
                sleep(2);
                continue;
            } else if (read_count < 0) {
                fprintf(stderr, "alsaucm_test: error reading cmd\n");
                break;
            }

            if (ch != '\n') {
                strlcat(cmdstr, &ch , (2+strlen(cmdstr)));
                continue;
            } else {
                if (!strncmp(cmdstr, exit_str, strlen(cmdstr))) {
                    /* free UCM instace */
                    if (uc_mgr) {
                        snd_use_case_mgr_close(uc_mgr);
                        uc_mgr = NULL;
                    }
                    break;
                } else {
                    process_cmd(cmdstr);
                    memset(cmdstr, 0, sizeof(cmdstr));
                }
            }
        }
        printf("alsaucm_test: exit server mode\n");
        close(fd);
        remove("/data/alsaucm_test");
    } else {
        fprintf(stderr, "alsaucm_test: Failed to create server\n");
    }
}


static void print_help_menu(void)
{
    printf("\nAvailable commands:\n"
           "  open NAME                  open card NAME\n"
           "  reset                      reset sound card to default state\n"
           "  reload                     reload configuration\n"
           "  listcards                  list available cards\n"
           "  list IDENTIFIER            list command\n"
           "  get IDENTIFIER             get string value\n"
           "  geti IDENTIFIER            get integer value\n"
           "  set IDENTIFIER VALUE       set string value\n"
           "  help                     help\n"
           "  quit                     quit\n");
}

int main(int argc, char **argv)
{
    char *help_str = "help";
    argc--;
    argv++;

    if (argc > 0) {
        if (!strncmp(argv[0], help_str, strlen(argv[0])))
            print_help_menu();
    } else
	    alsaucm_test_cmd_svr();
    return 0;
}

static int process_cmd(char *cmdStr)
{
    const char **list = NULL , *str = NULL;
    long lval;
    int err, i;
    char *command = NULL;
    int count = 0;
    char *identifier = NULL, *value = NULL;
    struct cmd *cmd = NULL;

    command = strtok_r(cmdStr, " ", &value);
    identifier = strtok_r(NULL, " ", &value);

    if (command == NULL) {
        fprintf(stderr, "NULL pointer encountered. Invalid value for command");
        return -1;
    }

    for (cmd = cmds; cmd->cmd_str != NULL; cmd++) {
        if (strncmp(cmd->cmd_str, command, strlen(cmd->cmd_str)) == 0)
            break;
    }

    if (cmd->cmd_str == NULL) {
        fprintf(stderr, "Unknown command '%s'\n", command);
        return -EINVAL;
    }

    if ((identifier == NULL) && ((cmd->code != UCM_HELP) &&
        (cmd->code != UCM_LISTCARDS) && (cmd->code != UCM_RESET) &&
        (cmd->code != UCM_RELOAD)))
    {
        fprintf(stderr, "NULL pointer encountered. Invalid value for identifier");
        return -1;
    }

    switch (cmd->code) {
    case UCM_HELP:
        print_help_menu();
        break;

    case UCM_OPEN:
        if (uc_mgr) {
            snd_use_case_mgr_close(uc_mgr);
            uc_mgr = NULL;
        }

        err = snd_use_case_mgr_open(&uc_mgr, identifier);
        if (err < 0) {
            fprintf(stderr, "%s: error failed to open sound card %s: %d\n", cmd->cmd_str, identifier, err);
            return err;
        }
        snd_use_case_mgr_wait_for_parsing(uc_mgr);
        break;

    case UCM_LISTCARDS:
        err = snd_use_case_card_list(&list);
        if (err < 0) {
            fprintf(stderr, "%s: error failed to get card list: %d\n", cmd->cmd_str, err);
            return err;
        }
        if (err == 0) {
            printf("list is empty\n");
            return 0;
        }

        for (i = 0; i < err; i++)
            printf("  %i: %s\n", i+1, list[i]);
        snd_use_case_free_list(list, err);
        break;

    case UCM_RESET:
        if (!uc_mgr) {
            fprintf(stderr, "No card is opened before. %s command can't be executed\n", cmd->cmd_str);
            return -EINVAL;
        }

        err = snd_use_case_mgr_reset(uc_mgr);
        if (err < 0) {
            fprintf(stderr, "%s: error failed to reset sound card %d\n", cmd->cmd_str, err);
            return err;
        }
        break;

    case UCM_RELOAD:
        if (!uc_mgr) {
            fprintf(stderr, "No card is opened before. %s command can't be executed\n", cmd->cmd_str);
            return -EINVAL;
        }

        err = snd_use_case_mgr_reload(uc_mgr);
        if (err < 0) {
            fprintf(stderr, "%s: error failed to reload manager %d\n", cmd->cmd_str, err);
            return err;
        }
        break;

    case UCM_LIST:
        if (!uc_mgr) {
            fprintf(stderr, "No card is opened before. %s command can't be executed\n", cmd->cmd_str);
            return -EINVAL;
        }

        err = snd_use_case_get_list(uc_mgr, identifier, &list);
        if (err < 0) {
            fprintf(stderr, "%s: error failed to get list %s: %d\n", cmd->cmd_str, identifier, err);
            return err;
        }
        if (err == 0) {
           printf("list is empty\n");
           return 0;
        }
        for (i = 0; i < err; i++) {
            printf("  %i: %s\n", i+1, list[i]);
        }
        snd_use_case_free_list(list, err);
        break;

    case UCM_SET:
        if (!uc_mgr) {
            fprintf(stderr, "No card is opened before. %s command can't be executed\n", cmd->cmd_str);
            return -EINVAL;
        }

        err = snd_use_case_set(uc_mgr, identifier, value);
        if (err < 0) {
            fprintf(stderr, "%s: error failed to set %s=%s: %d\n", cmd->cmd_str, identifier, value, err);
            return err;
        }
        break;

    case UCM_GET:
        if (!uc_mgr) {
            fprintf(stderr, "No card is opened before. %s command can't be executed\n", cmd->cmd_str);
            return -EINVAL;
        }

        err = snd_use_case_get(uc_mgr, identifier, &str);
        if (err < 0) {
            fprintf(stderr, "%s: error failed to get %s: %d\n", cmd->cmd_str, identifier, err);
            return err;
        }
        printf("  %s=%s\n", identifier, str);
        free((void *)str);
        break;

    case UCM_GETI:
        if (!uc_mgr) {
           fprintf(stderr, "No card is opened before. %s command can't be executed\n", cmd->cmd_str);
           return -EINVAL;
        }

        err = snd_use_case_geti(uc_mgr, identifier, &lval);
        if (err < 0) {
            fprintf(stderr, "%s: error failed to get integer %s: %d\n", cmd->cmd_str, identifier, err);
            return lval;
        }
        printf("  %s=%li\n", identifier, lval);
        break;

    default:
        break;
    }
    return 0;
}

