/*
 *    Allows to set arbitrary speed for the serial device on Linux.
 * stty allows to set only predefined values: 9600, 19200, 38400, 57600, 115200, 230400, 460800.
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>

#define error_exit(f, args...)				       		       \
printf_exit("%s: " f ": %s (code: %d)\n", __func__, ##args,	       \
strerror(errno), errno)

static void printf_exit(char *f, ...) {
    va_list va;
    
    va_start(va, f);
    vfprintf(stderr, f, va);
    va_end(va);
    
    exit(EXIT_FAILURE);
}

static void usage(char* argv[]) {
    printf("%s device speed\n\n"
           "Set speed for a serial device.\n"
           "For instance:\n"
           "    %s /dev/ttyUSB0 75000\n", argv[0], argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    
    int fd, speed;
    struct termios2 tio;
    
    setbuf(stdout, NULL);
    
    if (argc != 3)
        usage(argv);
    
    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
        error_exit("opening tty");
    
    speed = atoi(argv[2]);
    
    if (ioctl(fd, TCGETS2, &tio) < 0)
        error_exit("TCGETS2 ioctl");
    
    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER;
    tio.c_ispeed = speed;
    tio.c_ospeed = speed;
    
    if (ioctl(fd, TCSETS2, &tio) < 0)
        error_exit("TCSETS2 ioctl");
    close(fd);
    
    printf("%s speed changed to %d baud\n", argv[1], speed);
    return 0;
}