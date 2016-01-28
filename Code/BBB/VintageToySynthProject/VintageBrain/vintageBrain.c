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
#include <termios.h>
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
#include <sys/termios.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <linux/spi/spidev.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "../globals.h"

#define KEYBOARD_SERIAL_PATH "/dev/ttyO1"

//==========================================================
//==========================================================
//==========================================================
//Function that sends data to the vintageSoundEngine application using a socket

void SendToSoundEngine (uint8_t data_buffer[], int data_buffer_size, int sock, struct sockaddr_un sock_addr)
{
    if (sendto (sock, data_buffer, data_buffer_size, 0, (struct sockaddr *) &sock_addr, sizeof(struct sockaddr_un)) == -1)
        perror("[VB] Sending to sound engine");
    
    //PrintArray(data_buffer, data_buffer_size, "Data to Clock");
}

//==========================================================
//==========================================================
//==========================================================
//Function that sets up a serial port

int SetupSerialPort (const char path[], speed_t speed, bool should_be_blocking)
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
    
    tcgetattr(fd,&tty_attributes);
    cfmakeraw(&tty_attributes);
    tty_attributes.c_cc[VMIN]=1;
    tty_attributes.c_cc[VTIME]=0;
    
    // setup bauds
    cfsetispeed (&tty_attributes, speed);
    cfsetospeed (&tty_attributes, speed);

    // apply changes now
    tcsetattr(fd, TCSANOW, &tty_attributes);
    
    if (should_be_blocking)
    {
        // set it to blocking
        fcntl(fd, F_SETFL, 0);
    }
    else
    {
        // set it to non-blocking
        fcntl(fd, F_SETFL, O_NONBLOCK);
    }
    
    return fd;
}

//==========================================================
//==========================================================
//==========================================================
//main function

int main (void)
{
    printf ("[VB] Running vintageBrain...\n");
    
    int keyboard_fd;
    uint8_t keyboard_input_buf[1] = {0};
    
    struct sockaddr_un brain_sock_addr, sound_engine_sock_addr;
    int sock;
    
    //==========================================================
    //Set up serial connections
    
    printf ("[VB] Setting up serial connections...\n");
    
    //open UART1 device file for read/write to keyboard with baud rate of 38400
    keyboard_fd = SetupSerialPort (KEYBOARD_SERIAL_PATH, B38400, true);
    
    //===============================================================
    //Set up sockets for comms with the vintageSoundEngine application
    
    printf ("[VB] Setting up sockets...\n");

    //sound engine socket
    sound_engine_sock_addr.sun_family = AF_UNIX;
    strncpy (sound_engine_sock_addr.sun_path, SOCK_SOUND_ENGINE_FILENAME, sizeof(sound_engine_sock_addr.sun_path) - 1);
    
    sock = socket (AF_UNIX, SOCK_DGRAM, 0);
    
    //brain socket
    brain_sock_addr.sun_family = AF_UNIX;
    strcpy (brain_sock_addr.sun_path, SOCK_BRAIN_FILENAME);
    // Unlink (in case sock already exists)
    unlink (brain_sock_addr.sun_path);
    
    if (bind (sock, (struct sockaddr *)&brain_sock_addr, sizeof(brain_sock_addr)) == -1)
    {
        perror("[VB] socket bind");
    }
    
    //==========================================================
    //Enter main loop, and just read any data that comes in over the serial port
    
    printf ("[VB] Starting reading data from key mech...\n");
    
    while (true)
    {
        //attempt to read a byte from the serial device file
        int ret = read (keyboard_fd, keyboard_input_buf, 1);
        
        //if read something
        if (ret != -1)
        {
            //display the read byte
            printf ("[VB] Byte read from keyboard: %d\n", keyboard_input_buf[0]);
            
            //Send data to vintageSoundEngine app
            SendToSoundEngine (keyboard_input_buf, 1, sock, sound_engine_sock_addr);
            
        } //if (ret)
        
    } ///while (true)
    
    return 0;
    
}
