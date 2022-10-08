#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <sys/wait.h>
#include <cutils/android_reboot.h>
#include <cutils/partition_utils.h>

const char *mkfs = "/system/bin/make_ext4fs";

int setup_fs(const char *blockdev)
{
    char buf[256], path[128];
    pid_t child;
    int status, n;
    pid_t pid;

        /* we might be looking at an indirect reference */
    n = readlink(blockdev, path, sizeof(path) - 1);
    if (n > 0) {
        path[n] = 0;
        if (!memcmp(path, "/dev/block/", 11))
            blockdev = path + 11;
    }

    if (strchr(blockdev,'/')) {
        fprintf(stderr,"not a block device name: %s\n", blockdev);
        return 0;
    }

    snprintf(buf, sizeof(buf), "/sys/fs/ext4/%s", blockdev);
    if (access(buf, F_OK) == 0) {
        fprintf(stderr,"device %s already has a filesystem\n", blockdev);
        return 0;
    }
    snprintf(buf, sizeof(buf), "/dev/block/%s", blockdev);

    if (!partition_wiped(buf)) {
        fprintf(stderr,"device %s not wiped, probably encrypted, not wiping\n", blockdev);
        return 0;
    }

    fprintf(stderr,"+++\n");

    child = fork();
    if (child < 0) {
        fprintf(stderr,"error: setup_fs: fork failed\n");
        return 0;
    }
    if (child == 0) {
        execl(mkfs, mkfs, buf, NULL);
        exit(-1);
    }

    while ((pid=waitpid(-1, &status, 0)) != child) {
        if (pid == -1) {
            fprintf(stderr, "error: setup_fs: waitpid failed!\n");
            return 1;
        }
    }

    fprintf(stderr,"---\n");
    return 1;
}


int main(int argc, char **argv)
{
    int need_reboot = 0;

    while (argc > 1) {
        if (strlen(argv[1]) < 128)
            need_reboot |= setup_fs(argv[1]);
        argv++;
        argc--;
    }

    if (need_reboot) {
        fprintf(stderr,"REBOOT!\n");
        android_reboot(ANDROID_RB_RESTART, 0, 0);
        exit(-1);
    }
    return 0;
}
