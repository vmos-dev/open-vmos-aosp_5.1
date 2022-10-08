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
 *
 * Simple cpu eater busy loop. Runs as a daemon. prints the child PID to
 * std so you can easily kill it later.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


int main(int argc, char *argv[])
{
    pid_t pid;
    int life_universe_and_everything;
    int fd;

    switch(fork()) {
        case -1:
            perror(argv[0]);
            exit(1);
            break;
        case 0: /* child */
            chdir("/");
            umask(0);
            setpgrp();
            setsid();
            /* fork again to fully detach from controlling terminal. */
            switch(pid = fork()) { 
                case -1:
                    break;
                case 0: /* second child */
                    /* redirect to /dev/null */
                    close(0); 
                    open("/dev/null", 0);
                    close(1);
                    if(open("/dev/null", O_WRONLY) < 0) {
                        perror("/dev/null");
                        exit(1);
                    }
                    fflush(stdout);
                    close(2); 
                    dup(1);
                    for (fd = 3; fd < 256; fd++) {
                        close(fd);
                    }
                    /* busy looper */
                    while (1) {
                        life_universe_and_everything = 42 * 2;
                    }
                  default:
                      /* so caller can easily kill it later. */
                      printf("%d\n", pid);
                      exit(0);
                      break;
                }
                break;
          default:
              exit(0);
              break;
    }
  return 0;
}


/* vim:ts=4:sw=4:softtabstop=4:smarttab:expandtab */

