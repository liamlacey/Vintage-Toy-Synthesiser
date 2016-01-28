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

#define KEYBOARD_SERIAL_PATH "/dev/ttyO1"

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
    printf ("Running vintageBrain...\n");
    
    int keyboard_fd;
    uint8_t keyboard_input_buf[1] = {0};
    
    //==========================================================
    //Set up serial connection
    
    printf ("Setting up key mech serial connection...\n");
    
    //open UART1 device file for read/write with baus rate of 38400
    keyboard_fd = SetupSerialPort (KEYBOARD_SERIAL_PATH, B38400, true);
    
    //==========================================================
    //Enter main loop, and just read any data that comes in over the serial port
    
    printf ("Starting reading data from key mech...\n");
    
    while (true)
    {
        //attempt to read a byte from the serial device file
        int ret = read (keyboard_fd, keyboard_input_buf, 1);
        
        //if read something
        if (ret != -1)
        {
            //display the read byte
            printf ("Byte read from keyboard: %d\n", keyboard_input_buf[0]);
            
        } //if (ret)
        
    } ///while (true)
    
    return 0;
    
}
