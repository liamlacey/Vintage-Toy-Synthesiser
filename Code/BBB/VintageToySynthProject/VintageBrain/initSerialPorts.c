/*
 vintageBrain.c
 
 Created by Liam Lacey on 28/01/2016.
 Copyright 2016 Liam Lacey. All rights reserved.
 
 This is the main file for the vintageBrain application for the Vintage Toy Piano Synth project.
 This application is used provide communication between the synths keyboard, panel, sound engine, and MIDI I/O.
 It is also used to handle voice allocation and patch storage.
 
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sched.h>
#include <time.h>
#include <netdb.h>
#include <memory.h>
#include <ifaddrs.h>
#include <stdarg.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <linux/spi/spidev.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <termios.h>
#include <sys/termios.h>
//#include <asm/termbits.h>

#include "../globals.h"

#define KEYBOARD_SERIAL_PATH "/dev/ttyO1"
#define MIDI_SERIAL_PATH "/dev/ttyO2"
#define PANEL_SERIAL_PATH "/dev/ttyO4"

//==========================================================
//==========================================================
//==========================================================
//Function that sets up a serial port

int SetupSerialPort (const char path[], bool should_be_blocking)
{
    int fd;
    struct termios tty_attributes;
    
    // open device for read/write
    fd = open (path, O_RDWR);
    
    //if can't open file
    if (fd < 0)
    {
        //show error and exit
        perror (path);
        return (-1);
    }
    
    tcgetattr (fd, &tty_attributes);
    cfmakeraw (&tty_attributes);
    tty_attributes.c_cc[VMIN]=1;
    tty_attributes.c_cc[VTIME]=0;
    
    // setup bauds
    cfsetispeed (&tty_attributes, B38400);
    cfsetospeed (&tty_attributes, B38400);
    
    
    // apply changes now
    tcsetattr (fd, TCSANOW, &tty_attributes);
    
    if (should_be_blocking)
    {
        // set it to blocking
        fcntl (fd, F_SETFL, 0);
    }
    else
    {
        // set it to non-blocking
        fcntl (fd, F_SETFL, O_NONBLOCK);
    }
    
    printf("[ISP] %s comms set up\r\n", path);
    
    return fd;
}


//==========================================================
//==========================================================
//==========================================================
//main function

int main (void)
{
    printf ("[ISP] Running initSerialPort...\n");
    
    int keyboard_fd, midi_fd, panel_fd;
    
    //==========================================================
    //Set up serial connections
    
    printf ("[ISP] Setting up serial connections...\n");
    
    //open UART1 device file for read/write to keyboard
    keyboard_fd = SetupSerialPort (KEYBOARD_SERIAL_PATH, true);
    
    //open UART2 device file for read/write to MIDI interface
    midi_fd = SetupSerialPort (MIDI_SERIAL_PATH, true);
    
    //open UART4 device file for read/write to panel interface
    midi_fd = SetupSerialPort (PANEL_SERIAL_PATH, true);
    
    //==========================================================
    //close serial connections
    
    printf ("[ISP] Closing serial connections...\n");
    
    if (close (keyboard_fd) == -1)
        perror("[ISP] Closing keyboard_fd file descriptor");
    if (close (midi_fd) == -1)
        perror("[ISP] Closing midi_fd file descriptor");
    if (close (panel_fd) == -1)
        perror("[ISP] Closing panel_fd file descriptor");
    
    printf ("[ISP] Finished running initSerialPort\n");
    
    return 0;
}
