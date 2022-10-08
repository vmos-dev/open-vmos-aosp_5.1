#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/dm-ioctl.h>
#include <stdlib.h>

#define DM_CRYPT_BUF_SIZE 4096

static void ioctl_init(struct dm_ioctl *io, size_t dataSize, const char *name, unsigned flags)
{
    memset(io, 0, dataSize);
    io->data_size = dataSize;
    io->data_start = sizeof(struct dm_ioctl);
    io->version[0] = 4;
    io->version[1] = 0;
    io->version[2] = 0;
    io->flags = flags;
    if (name) {
        strncpy(io->name, name, sizeof(io->name));
    }
}

int main(int argc, char *argv[])
{
    char buffer[DM_CRYPT_BUF_SIZE];
    struct dm_ioctl *io;
    struct dm_target_versions *v;
    int i;
    int fd;

    fd = open("/dev/device-mapper", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Cannot open /dev/device-mapper\n");
        exit(1);
    }

    io = (struct dm_ioctl *) buffer;

    ioctl_init(io, DM_CRYPT_BUF_SIZE, NULL, 0);

    if (ioctl(fd, DM_LIST_VERSIONS, io)) {
        fprintf(stderr, "ioctl(DM_LIST_VERSIONS) returned an error\n");
        exit(1);
    }

    /* Iterate over the returned versions, and print each subsystem's version */
    v = (struct dm_target_versions *) &buffer[sizeof(struct dm_ioctl)];
    while (v->next) {
        printf("%s: %d.%d.%d\n", v->name, v->version[0], v->version[1], v->version[2]);
        v = (struct dm_target_versions *)(((char *)v) + v->next);
    }

    close(fd);
    exit(0);
}

