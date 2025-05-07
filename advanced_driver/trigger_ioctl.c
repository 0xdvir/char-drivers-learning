#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sysexits.h>

#define SUCCESS 0
#define MY_IOCTL_CMD_CREATE_DEVICE _IO('a', 1)
#define DEVICE_PATH "/dev/advanced_driver0"

int main() {
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -EX_NOINPUT;
    }

    printf("Sebding ioctl command to create device\n");

    if (ioctl(fd, MY_IOCTL_CMD_CREATE_DEVICE) < 0) {
        perror("Failed to send ioctl command");
        close(fd);
        return -EX_IOERR;
    }

    printf("ioctl command sent successfully\n");
    close(fd);
    return SUCCESS;
}
