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
//No longer want these, otherwise you get a redefinition error within asm/termbits.h
//#include <termios.h>
//#include <sys/termios.h>
#include <asm/termbits.h>

#include "../globals.h"

//#define DEBUG 1

#define KEYBOARD_SERIAL_PATH "/dev/ttyO1"
#define MIDI_SERIAL_PATH "/dev/ttyO2"

enum InputSourceTypes
{
    INPUT_SRC_KEYBOARD = 0,
    INPUT_SRC_MIDI_IN,

    NUM_OF_INPUT_SRC_TYPES
};

#define NUM_OF_POLL_FDS NUM_OF_INPUT_SRC_TYPES
#define POLL_TIME -1

int keyboard_fd, midi_fd;

//TODO: parameters that need implementing here - all key ones, the voice one, and the global volume one

//==========================================================
//==========================================================
//==========================================================

void WriteToMidiOutFd (uint8_t data_buffer[], int data_buffer_size)
{
    if (write (midi_fd, data_buffer, data_buffer_size) == -1)
        perror("[VB] Writing to MIDI output");
}

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

int SetupSerialPort (const char path[], int speed, bool should_be_blocking)
{
    //Below is some new code for setting up serial comms which allows me
    //to use non-standard baud rates (such as 31250 for MIDI interface comms).
    //This code was provided in this thread:
    //https://groups.google.com/forum/#!searchin/beagleboard/Peter$20Hurdley%7Csort:date/beagleboard/GC0rKe6rM0g/lrHWS_e2_poJ
    //This is a direct link to the example code:
    //https://gist.githubusercontent.com/peterhurley/fbace59b55d87306a5b8/raw/220cfc2cb1f2bf03ce662fe387362c3cc21b65d7/anybaud.c

    int fd;
    struct termios2 tio;
    
    // open device for read/write
    fd = open (path, O_RDWR);
    
    //if can't open file
    if (fd < 0)
    {
        //show error and exit
        perror (path);
        return (-1);
    }
    
    if (ioctl (fd, TCGETS2, &tio) < 0)
        perror("TCGETS2 ioctl");
    
    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER;
    tio.c_ispeed = speed;
    tio.c_ospeed = speed;
    
    if (ioctl( fd, TCSETS2, &tio) < 0)
        perror("TCGETS2 ioctl");
    
    printf("[VB] %s speed set to %d baud\r\n", path, speed);

    return fd;
    
    //Original serial setup code (which can only set standard baud rates)
    
//        int fd;
//        struct termios tty_attributes;
//    
//        // open device for read/write
//        fd = open (path, O_RDWR);
//    
//        //if can't open file
//        if (fd < 0)
//        {
//            //show error and exit
//            perror (path);
//            return (-1);
//        }
//    
//        tcgetattr (fd, &tty_attributes);
//        cfmakeraw (&tty_attributes);
//        tty_attributes.c_cc[VMIN]=1;
//        tty_attributes.c_cc[VTIME]=0;
//    
//        // setup bauds
//        cfsetispeed (&tty_attributes, B38400);
//        cfsetospeed (&tty_attributes, B38400);
//    
//    
//        // apply changes now
//        tcsetattr (fd, TCSANOW, &tty_attributes);
//    
//        if (should_be_blocking)
//        {
//            // set it to blocking
//            fcntl (fd, F_SETFL, 0);
//        }
//        else
//        {
//            // set it to non-blocking
//            fcntl (fd, F_SETFL, O_NONBLOCK);
//        }
//    
//        return fd;
}

//==========================================================
//==========================================================
//==========================================================
//Function that processes MIDI bytes coming from any of the serial ports

uint8_t ProcInputByte (uint8_t input_byte, uint8_t message_buffer[], uint8_t *byte_counter, uint8_t *running_status_value, uint8_t *prev_cc_num)
{
    /*
     A recommended approach for a receiving device is to maintain its "running status buffer" as so:
     Buffer is cleared (ie, set to 0) at power up.
     Buffer stores the status when a Voice Category Status (ie, 0x80 to 0xEF) is received.
     Buffer is cleared when a System Common Category Status (ie, 0xF0 to 0xF7) is received - need to implement this fully!?
     Nothing is done to the buffer when a RealTime Category message (ie, 0xF8 to 0xFF, which includes clock messages) is received.
     Any data bytes are ignored when the buffer is 0.
     
     (http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midispec/run.htm)
     
     */
    
    //static uint8_t running_status_value = 0;
    //static uint8_t prev_cc_num = 127; //don't init this to 0, incase the first CC we get is 32, causing it to be ignored!
    uint8_t result = 0;
    
    //=====================================================================
    //If we've received the start of a new MIDI message (a status byte)...
    
    if (input_byte >= MIDI_NOTEOFF)
    {
        //If it's a Voice Category message
        if (input_byte >= MIDI_NOTEOFF && input_byte <= MIDI_PITCH_BEND_MAX)
        {
            message_buffer[0] = input_byte;
            *byte_counter = 1;
            result = 0;
            
            *running_status_value = message_buffer[0];
        }
        
        //If it's a clock message
        else if (input_byte >= MIDI_CLOCK && input_byte <= MIDI_CLOCK_STOP)
        {
            //Don't do anything with MidiInCount or *running_status_value here,
            //so that running status works correctly.
            
            message_buffer[0] = input_byte;
            result = input_byte;
        }
        
        //If it's the start of a sysex message
        else if (input_byte == MIDI_SYSEX_START)
        {
            message_buffer[0] = input_byte;
            *byte_counter = 1;
        }
        
        //If it's the end of a sysex
        else if (input_byte == MIDI_SYSEX_END)
        {
            message_buffer[*byte_counter] = input_byte;
            *byte_counter = 0;
            
            result = MIDI_SYSEX_START;
        }
        
        // If any other status byte, don't do anything
        
    } //if (input_byte >= MIDI_NOTEOFF)
    
    //=====================================================================
    //If we're received a data byte of a non-sysex MIDI message...
    //FIXME: do I actually need to check *byte_counter here?
    
    else if (input_byte < MIDI_NOTEOFF && message_buffer[0] != MIDI_SYSEX_START && *byte_counter != 0)
    {
        switch (*byte_counter)
        {
            case 1:
            {
                //Process the second byte...
                
                //Check *running_status_value here instead of message_buffer[0], as it could be possible
                //that we are receiving running status messages entwined with clock messages, where
                //message_buffer[0] will actually be equal to MIDI_CLOCK.
                
                //TODO: process NRPNs, correctly (e.g. process 9 byte NRPN, and then as 12 bytes if the following CC is 0x26)
                
                //if it's a channel aftertouch message
                if (*running_status_value >= MIDI_CAT && *running_status_value <= MIDI_CAT_MAX)
                {
                    message_buffer[1] = input_byte;
                    result = MIDI_CAT;
                    
                    //set the correct status value
                    message_buffer[0] = *running_status_value;
                    
                    //wait for next data byte if running status
                    *byte_counter = 1;
                }
                
                //if it's a program change message
                else if (*running_status_value >= MIDI_PROGRAM_CHANGE && *running_status_value <= MIDI_PROGRAM_CHANGE_MAX)
                {
                    message_buffer[1] = input_byte;
                    result = MIDI_PROGRAM_CHANGE;
                    
                    //set the correct status value
                    message_buffer[0] = *running_status_value;
                    
                    //wait for next data byte if running status
                    *byte_counter = 1;
                }
                
                //else it's a 3+ byte MIDI message
                else
                {
                    message_buffer[1] = input_byte;
                    *byte_counter = 2;
                    
                    //set the correct status value
                    message_buffer[0] = *running_status_value;
                }
                
                break;
            }
                
            case 2:
            {
                //Process the third byte...
                
                result = 0;
                
                //TODO: process NRPNs, correctly
                
                //if it's not zero it's a note on
                if (input_byte && (*running_status_value >= MIDI_NOTEON && *running_status_value <= MIDI_NOTEON_MAX))
                {
                    //3rd byte is velocity
                    message_buffer[2] = input_byte;
                    result = MIDI_NOTEON;
                    
                    //set the correct status value
                    message_buffer[0] = *running_status_value;
                }
                
                //if it's a note off
                else if ((*running_status_value >= MIDI_NOTEOFF && *running_status_value <= MIDI_NOTEOFF_MAX) ||
                         (!input_byte && (*running_status_value >= MIDI_NOTEON && *running_status_value <= MIDI_NOTEON_MAX)))
                {
                    //3rd byte should be zero
                    message_buffer[2] = 0;
                    result = MIDI_NOTEOFF;
                    
                    //set the correct status value
                    message_buffer[0] = *running_status_value;
                }
                
                //if it's a CC
                else if (*running_status_value >= MIDI_CC && *running_status_value <= MIDI_CC_MAX)
                {
                    //if we have got a 32-63 CC (0-31 LSB/fine CC),
                    //and the last CC we received was the MSB/coarse CC pair
                    if (message_buffer[1] >= 32 && message_buffer[1] <= 63 && (*prev_cc_num == (message_buffer[1] - 32)))
                    {
                        //Don't do anything. Right now if this is the case we just want to ignore it.
                        //However in the future we may want to process coarse/fine CC pairs to
                        //control parameters at a higher resolution.
                        printf ("[VB] Received CC num %d directly after CC num %d, so ignoring it\r\n", message_buffer[1], *prev_cc_num);
                    }
                    
                    else
                    {
                        message_buffer[2] = input_byte;
                        result = MIDI_CC;
                        
                        //set the correct status value
                        message_buffer[0] = *running_status_value;
                        
                    } //else
                    
                    //store this CC num as the previously received CC
                    *prev_cc_num = message_buffer[1];
                }
                
                //if it's a poly aftertouch message
                else if (*running_status_value >= MIDI_PAT && *running_status_value <= MIDI_PAT_MAX)
                {
                    message_buffer[2] = input_byte;
                    result = MIDI_PAT;
                    
                    //set the correct status value
                    message_buffer[0] = *running_status_value;
                }
                
                //if it's a pitch bend message
                else if (*running_status_value >= MIDI_PITCH_BEND && *running_status_value <= MIDI_PITCH_BEND_MAX)
                {
                    message_buffer[2] = input_byte;
                    result = MIDI_PITCH_BEND;
                    
                    //set the correct status value
                    message_buffer[0] = *running_status_value;
                }
                
                // wait for next data byte (if running status)
                *byte_counter = 1;
                
                break;
            }
                
            default:
            {
                break;
            }
                
        } //switch (*byte_counter)
        
    } //else if (input_byte < MIDI_NOTEOFF && message_buffer[0] != MIDI_SYSEX_START && *byte_counter != 0)
    
    //if we're currently receiving a sysex message
    else if (message_buffer[0] == MIDI_SYSEX_START)
    {
        //add data to the sysex buffer
        message_buffer[*byte_counter] = input_byte;
        *byte_counter++;
    }
    
    return result;
}

//====================================================================================
//====================================================================================
//====================================================================================
//Processes a note message recieived from any source, sending it to the needed places

void ProcessNoteMessage (uint8_t message_buffer[],
                         bool send_to_midi_out,
                         int sock,
                         struct sockaddr_un sound_engine_sock_addr)
{
    //TODO: proper voice allocation
    
    //Send to the sound engine
    SendToSoundEngine (message_buffer, 3, sock, sound_engine_sock_addr);
    
    //Send to MIDI out if needed
    if (send_to_midi_out)
    {
        WriteToMidiOutFd (message_buffer, 3);
    }
}

//====================================================================================
//====================================================================================
//====================================================================================
//Processes a CC/param message from any source, sending it to the needed places

void ProcessCcMessage (uint8_t message_buffer[],
                       PatchParameterData patch_param_data[],
                       bool send_to_midi_out,
                       int sock,
                       struct sockaddr_un sound_engine_sock_addr)
{
    uint8_t param_num = message_buffer[1];
    uint8_t param_val = message_buffer[2];
    
    //Store the new param value, and bounding it if needed...
    
    if (param_val > patch_param_data[param_num].user_max_val)
        param_val = patch_param_data[param_num].user_max_val;
    else if (param_val < patch_param_data[param_num].user_min_val)
        param_val = patch_param_data[param_num].user_min_val;
    
    patch_param_data[param_num].user_val = param_val;
    message_buffer[2] = param_val;
    
    //Send to the sound engine if needed
    if (patch_param_data[param_num].sound_param)
    {
        SendToSoundEngine (message_buffer, 3, sock, sound_engine_sock_addr);
    }
    
    //Send to MIDI out if needed
    if (send_to_midi_out && patch_param_data[param_num].patch_param)
    {
        WriteToMidiOutFd (message_buffer, 3);
    }
}

//====================================================================================
//====================================================================================
//====================================================================================
//Handles signals
static void SignalHandler (int sig)
{
    printf("[VB] caught signal %d. Killing process.\r\n", sig);
    
    //Close connnections to serial ports
    if (close (keyboard_fd) == -1)
        perror("[UPDATE] Closing keyboard_fd file descriptor");
    if (close (midi_fd) == -1)
        perror("[UPDATE] Closing midi_fd file descriptor");
    
    exit(0);
}

//==========================================================
//==========================================================
//==========================================================
//main function

int main (void)
{
    printf ("[VB] Running vintageBrain...\n");
    
    uint8_t serial_input_buf[1] = {0};
    
    struct sockaddr_un brain_sock_addr, sound_engine_sock_addr;
    int sock;
    
    struct pollfd poll_fds[NUM_OF_POLL_FDS];
    
    //FIXME: group these variables into a struct, and have an array of that struct instead,
    //passing in a reference to the struct instance into ProcInputByte.
    //If I get this working, do the same within vintageSoundEngine
    uint8_t input_message_buffer[NUM_OF_INPUT_SRC_TYPES][1024] = {{0}}; //stores any MIDI message (including NRPN and sysex).
    uint8_t input_message_counter[NUM_OF_INPUT_SRC_TYPES] = {0};
    uint8_t input_message_running_status_value[NUM_OF_INPUT_SRC_TYPES] = {0};
    //don't init this to 0, incase the first CC we get is 32, causing it to be ignored!
    uint8_t input_message_prev_cc_num[NUM_OF_INPUT_SRC_TYPES] = {127};
    
    //'patch' parameter data
    PatchParameterData patchParameterData[128];
    
    for (uint8_t i = 0; i < 128; i++)
    {
        patchParameterData[i] = defaultPatchParameterData[i];
    }
    
    //==========================================================
    //Set up SIGINT and SIGTERM so that the process can be shutdown
    //correctly when it asked to quit.
    
    if(signal(SIGINT, SignalHandler) == SIG_ERR)	// Setup SIGINT handler
        fprintf(stderr, "[DR] [ERROR] Cannot setup SIGINT handler");
    if(signal(SIGTERM, SignalHandler) == SIG_ERR)	// Setup SIGTERM handler
        fprintf(stderr, "[DR] [ERROR] Cannot setup SIGTERM handler");
    
    //==========================================================
    //Set up serial connections
    
    printf ("[VB] Setting up serial connections...\n");
    
    //open UART1 device file for read/write to keyboard with baud rate of 38400
    keyboard_fd = SetupSerialPort (KEYBOARD_SERIAL_PATH, 38400, true);
    
    //open UART2 device file for read/write to MIDI interface with baud rate of 31250
    midi_fd = SetupSerialPort (MIDI_SERIAL_PATH, 31250, true);
    
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
    
    //===============================================================
    //Set up poll() stuff for waiting for events on file descriptors
    
    //Change #define NUM_OF_POLL_FDS if adding more things to poll
    poll_fds[INPUT_SRC_KEYBOARD].fd = keyboard_fd;
    poll_fds[INPUT_SRC_KEYBOARD].events = POLLIN;
    poll_fds[INPUT_SRC_MIDI_IN].fd = midi_fd;
    poll_fds[INPUT_SRC_MIDI_IN].events = POLLIN;
    
    //==========================================================
    //Enter main loop, and just read any data that comes in over the serial ports
    
    printf ("[VB] Starting reading data from serial ports...\n");
    
//    uint8_t test_num = 48;
//    uint8_t test_vel = 20;
    
    while (true)
    {
        //Wait for an event to happen on one of the file descriptors
        int ret = poll (poll_fds, NUM_OF_POLL_FDS, POLL_TIME);
        
        //==========================================================
        //Reading data from the keyboard serial port
        
        //if an event has happened on the keyboard serial file descriptor
        if (poll_fds[INPUT_SRC_KEYBOARD].revents & POLLIN)
        {
            //attempt to read a byte from the keyboard
            ret = read (keyboard_fd, serial_input_buf, 1);
            
            //if read something
            if (ret != -1)
            {
                //display the read byte
                //printf ("[VB] Byte read from keyboard: %d\n", serial_input_buf[0]);
                
                //process the read byte
                uint8_t input_message_flag = ProcInputByte (serial_input_buf[0],
                                                            input_message_buffer[INPUT_SRC_KEYBOARD],
                                                            &input_message_counter[INPUT_SRC_KEYBOARD],
                                                            &input_message_running_status_value[INPUT_SRC_KEYBOARD],
                                                            &input_message_prev_cc_num[INPUT_SRC_KEYBOARD]);
                
                //if we have received a full MIDI message
                if (input_message_flag)
                {
                    //printf ("[VB] Received full MIDI message from keyboard with status byte %d\n", input_message_buffer[INPUT_SRC_KEYBOARD][0]);
                    
                    if (input_message_flag == MIDI_NOTEON)
                    {
                        #ifdef DEBUG
                        printf ("[VB] Received note-on message from keyboard\r\n");
                        #endif
                        
                        ProcessNoteMessage (input_message_buffer[INPUT_SRC_KEYBOARD], true, sock, sound_engine_sock_addr);
                        
                    } //if (input_message_flag == MIDI_NOTEON)
                    
                    else if (input_message_flag == MIDI_NOTEOFF)
                    {
                        #ifdef DEBUG
                        printf ("[VB] Received note-off message from keyboard\r\n");
                        #endif
                        
                        ProcessNoteMessage (input_message_buffer[INPUT_SRC_KEYBOARD], true, sock, sound_engine_sock_addr);
                        
                    } //else if (input_message_flag == MIDI_NOTEOFF)
                    
                } //if (input_message_flag)
                
            } //if (ret)

        } //if (poll_fds[INPUT_SRC_KEYBOARD].revents & POLLIN)
        
        //==========================================================
        //Reading data from the MIDI serial port
        
        //if an event has happened on the MIDI serial file descriptor
        //FIXME: can this be an else if?
        if (poll_fds[INPUT_SRC_MIDI_IN].revents & POLLIN)
        {
            //attempt to read a byte from the MIDI interface device file
            ret = read (midi_fd, serial_input_buf, 1);
            
            //if read something
            if (ret != -1)
            {
                //display the read byte
                //printf ("[VB] Byte read from MIDI interface: %d\n", serial_input_buf[0]);
                
                //process the read byte
                uint8_t input_message_flag = ProcInputByte (serial_input_buf[0],
                                                            input_message_buffer[INPUT_SRC_MIDI_IN],
                                                            &input_message_counter[INPUT_SRC_MIDI_IN],
                                                            &input_message_running_status_value[INPUT_SRC_MIDI_IN],
                                                            &input_message_prev_cc_num[INPUT_SRC_MIDI_IN]);
                
                //if we have received a full MIDI message
                if (input_message_flag)
                {
                     //printf ("[VB] Received full MIDI message from MIDI interface with status byte %d\n", input_message_buffer[INPUT_SRC_MIDI_IN][0]);
                   
                    if (input_message_flag == MIDI_NOTEON)
                    {
                        #ifdef DEBUG
                        printf ("[VB] Received note-on message from MIDI\r\n");
                        #endif
                        
                        //set the MIDI channel to 0
                        input_message_buffer[INPUT_SRC_MIDI_IN][0] = MIDI_NOTEON;
                        
                        ProcessNoteMessage (input_message_buffer[INPUT_SRC_MIDI_IN], false, sock, sound_engine_sock_addr);
                        
                    } //if (input_message_flag == MIDI_NOTEON)
                    
                    else if (input_message_flag == MIDI_NOTEOFF)
                    {
                        #ifdef DEBUG
                        printf ("[VB] Received note-off message from MIDI\r\n");
                        #endif
                        
                        //set the MIDI channel to 0
                        input_message_buffer[INPUT_SRC_MIDI_IN][0] = MIDI_NOTEOFF;
                        
                        ProcessNoteMessage (input_message_buffer[INPUT_SRC_MIDI_IN], false, sock, sound_engine_sock_addr);
                        
                    } //else if (input_message_flag == MIDI_NOTEOFF)
                    
                    else if (input_message_flag == MIDI_CC)
                    {
                        #ifdef DEBUG
                        printf ("[VB] Received CC message from MIDI\r\n");
                        #endif
                        
                        //set the MIDI channel to 0
                        input_message_buffer[INPUT_SRC_MIDI_IN][0] = MIDI_CC;
                        
                        ProcessCcMessage (input_message_buffer[INPUT_SRC_MIDI_IN], patchParameterData, false, sock, sound_engine_sock_addr);
                        
                    } //else if (input_message_flag == MIDI_CC)
                    
                } //if (input_message_flag)

            } //if (ret)
            
        } //if (poll_fds[INPUT_SRC_MIDI_IN].revents & POLLIN)
        
        
        //test sending data to socket
//        uint8_t test_buf[3] = {MIDI_NOTEON, test_num, test_vel};
//        SendToSoundEngine (test_buf, 3, sock, sound_engine_sock_addr);
//        
//        usleep (250000);
//        
//        uint8_t test_buf_2[3] = {MIDI_NOTEOFF, test_num, 0};
//        SendToSoundEngine (test_buf_2, 3, sock, sound_engine_sock_addr);
//        
//        usleep (250000);
//        
//        test_num++;
//        if (test_num > 72)
//            test_num = 48;
//        
//        test_vel += 5;
//        if (test_vel > 127)
//            test_vel = 20;
        
    } ///while (true)
    
    return 0;
}
