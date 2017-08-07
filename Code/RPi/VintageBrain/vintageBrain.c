/*
 vintageBrain.c
 
 Created by Liam Lacey on 28/01/2016.
 Copyright 2016 Liam Lacey. All rights reserved.
 
 This is the main file for the vintageBrain application for the Vintage Toy Piano Synth project.
 This application is used provide communication between the synths keyboard, panel, sound engine, and MIDI I/O.
 It is also used to handle voice allocation and patch storage.
 
 */

//FIXME: TRY MOVING ALL OF THIS CODE INTO THE MAIN THREAD OF VINTAGESOUNDENGINE TO SEE IF THAT STOPS THE GLITCHES

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
#define PANEL_SERIAL_PATH "/dev/ttyO4"

enum InputSources
{
    //Add any source TYPES here
    INPUT_SRC_KEYBOARD = 0,
    INPUT_SRC_MIDI_IN,
    INPUT_SRC_PANEL,
    INPUT_SRC_SOCKETS,

    //use this when setting up polling stuff
    NUM_OF_INPUT_SRC_TYPES,
    
    //add SOCKET types here
    INPUT_SOCK_SRC_VOICE,
    
    //use this when setting up input buffers
    NUM_OF_INPUT_SRCS
};

#define NUM_OF_POLL_FDS NUM_OF_INPUT_SRC_TYPES
#define POLL_TIME -1

int keyboard_fd, midi_fd, panel_fd;

//FIXME: make these variables not global!
bool panelIsEnabled = true;

//Struct that stores info about playing notes.
//In poly mode this is for each voice, but in mono mode this
//is for each note in the note stack.
typedef struct
{
    int16_t note_num;
    int16_t note_vel;
    
    //for handling realtime keyboard setting changes
    bool from_internal_keyboard;
    int16_t keyboard_key_num;
    
} NoteData;

//Struct that stores voice allocation data for both poly and mono mode.
typedef struct
{
    //for poly mode...
    uint8_t free_voices[NUM_OF_VOICES];
    
    //for mono mode...
    uint8_t mono_note_stack_pointer;
    
    //for both modes...
    NoteData note_data[VOICE_ALLOC_NOTE_BUFFER_SIZE];
    
    //for note stealing (last voice)
    uint8_t last_voice;
    
    
    
} VoiceAllocData;

//==========================================================
//==========================================================
//==========================================================

void WriteToMidiOutFd (uint8_t data_buffer[], int data_buffer_size)
{
    //for (int i = 0; i < data_buffer_size; i++)
    //    printf ("[VB] Byte to MIDI-output: %d\r\n", data_buffer[i]);
    
    if (write (midi_fd, data_buffer, data_buffer_size) == -1)
        perror("[VB] Writing to MIDI output");
}

//==========================================================
//==========================================================
//==========================================================

void WriteToPanelFd (uint8_t data_buffer[], int data_buffer_size)
{
    if (write (panel_fd, data_buffer, data_buffer_size) == -1)
        perror("[VB] Writing to panel");
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

//====================================================================================
//====================================================================================
//====================================================================================
//Kills all voices

void KillAllVoices (PatchParameterData patch_param_data[],
                    VoiceAllocData *voice_alloc_data,
                    int sock,
                    struct sockaddr_un sound_engine_sock_addr)
{
    //kill all voices in the sound engine...
    
    uint8_t cc_buf[3] = {MIDI_CC, PARAM_CMD, CMD_KILL_ALL_VOICES};
    SendToSoundEngine (cc_buf, 3, sock, sound_engine_sock_addr);
    
    //reset voice allocation and note data...
    
    for (uint8_t i = 0; i < NUM_OF_VOICES; i++)
    {
        voice_alloc_data->free_voices[i] = i + 1;
    }
    
    for (uint8_t i = 0; i < VOICE_ALLOC_NOTE_BUFFER_SIZE; i++)
    {
        voice_alloc_data->note_data[i].note_num = VOICE_NO_NOTE;
        voice_alloc_data->note_data[i].note_vel = VOICE_NO_NOTE;
        
        voice_alloc_data->note_data[i].from_internal_keyboard = false;
        voice_alloc_data->note_data[i].keyboard_key_num = VOICE_NO_NOTE;
    }
    
    voice_alloc_data->mono_note_stack_pointer = 0;
    voice_alloc_data->last_voice = 0;
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
    //FIXME: do I actually need to check byte_counter here?
    
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
//Gets the next free voice (the oldest played voice) from the voice_alloc_data.free_voices buffer,
//or steals the oldest voice if not voices are currently free.

uint8_t GetNextFreeVoice (VoiceAllocData *voice_alloc_data, int sock, struct sockaddr_un sound_engine_sock_addr)
{
    uint8_t free_voice = 0;
    
    //get the next free voice number from first index of the free_voices array
    free_voice = voice_alloc_data->free_voices[0];
    
    //if got a free voice
    if (free_voice != 0)
    {
        //shift all voices forwards, removing the first value, and adding 0 on the end...
        
        for (uint8_t voice = 0; voice < NUM_OF_VOICES - 1; voice++)
        {
            voice_alloc_data->free_voices[voice] = voice_alloc_data->free_voices[voice + 1];
        }
        
        voice_alloc_data->free_voices[NUM_OF_VOICES - 1] = 0;
        
    } //if (free_voice != 0)
    
    else
    {
        //use the oldest voice
        free_voice = voice_alloc_data->last_voice;
        
        //Send a note-off message to the stolen voice so that when
        //sending the new note-on it enters the attack phase....
        uint8_t note_buffer[3] = {MIDI_NOTEOFF + (free_voice - 1), voice_alloc_data->note_data[free_voice - 1].note_num, 0};
        SendToSoundEngine (note_buffer, 3, sock, sound_engine_sock_addr);
        
    } //else ((free_voice != 0))
    
    return free_voice;
}

//====================================================================================
//====================================================================================
//====================================================================================
//Adds a new free voice to the voice_alloc_data.free_voices buffer

uint8_t FreeVoiceOfNote (uint8_t note_num, VoiceAllocData *voice_alloc_data)
{
    //first, find which voice note_num is currently being played on
    
    uint8_t free_voice = 0;
    
    for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
    {
        if (note_num == voice_alloc_data->note_data[voice].note_num)
        {
            free_voice = voice + 1;
            break;
        }
        
    } //for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
    
    
    //if we have a voice to free up
    if (free_voice > 0)
    {
        //find space in voice buffer
        
        for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
        {
            //if we find zero put the voice in that place
            if (voice_alloc_data->free_voices[voice] == 0)
            {
                voice_alloc_data->free_voices[voice] = free_voice;
                break;
            }
            
        } //for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
        
    } //if (free_voice > 0)
    
    return free_voice;
}

//====================================================================================
//====================================================================================
//====================================================================================
//Returns a list of voices that are currently playing note note_num (using the voice_list array)
//as well as returning the number of voices.
//Even though at the moment it will probably only ever be 1 voice here, I'm implementing
//it to be able to return multiple voices incase in the future I allow the same note
//to play multiple voices.

uint8_t GetVoicesOfNote (uint8_t note_num, VoiceAllocData *voice_alloc_data, uint8_t voice_list[])
{
    uint8_t num_of_voices = 0;
    
    for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
    {
        if (note_num == voice_alloc_data->note_data[voice].note_num)
        {
            voice_list[num_of_voices] = voice + 1;
            num_of_voices++;
        }
        
    } //for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
    
    return num_of_voices;
}

//====================================================================================
//====================================================================================
//====================================================================================
///Removes a note from the mono stack by shuffling a set of notes down

void RemoveNoteFromMonoStack (uint8_t start_index, uint8_t end_index, VoiceAllocData *voice_alloc_data)
{
    //shuffle the notes in the stack down to remove the note
    for (uint8_t index = start_index; index < end_index; index++)
    {
        voice_alloc_data->note_data[index].note_num = voice_alloc_data->note_data[index + 1].note_num;
    }
    
    //set top of stack to empty
    voice_alloc_data->note_data[voice_alloc_data->mono_note_stack_pointer].note_num = VOICE_NO_NOTE;
    voice_alloc_data->note_data[voice_alloc_data->mono_note_stack_pointer].note_vel = VOICE_NO_NOTE;
    
    //set internal keyboard note stuff
    voice_alloc_data->note_data[voice_alloc_data->mono_note_stack_pointer].keyboard_key_num = VOICE_NO_NOTE;
    
    //decrement pointer if above 0
    if (voice_alloc_data->mono_note_stack_pointer)
    {
        voice_alloc_data->mono_note_stack_pointer--;
    }
}

//====================================================================================
//====================================================================================
//====================================================================================
//Adds a note to mono mode stack

void AddNoteToMonoStack (uint8_t note_num, uint8_t note_vel, VoiceAllocData *voice_alloc_data, bool from_internal_keyboard, uint8_t keyboard_key_num)
{
    //add note to the top of the stack
    voice_alloc_data->note_data[voice_alloc_data->mono_note_stack_pointer].note_num = note_num;
    voice_alloc_data->note_data[voice_alloc_data->mono_note_stack_pointer].note_vel = note_vel;
    
    //set internal keyboard note stuff
    voice_alloc_data->note_data[voice_alloc_data->mono_note_stack_pointer].from_internal_keyboard = from_internal_keyboard;
    if (from_internal_keyboard)
        voice_alloc_data->note_data[voice_alloc_data->mono_note_stack_pointer].keyboard_key_num = keyboard_key_num;
    
    //increase stack pointer
    voice_alloc_data->mono_note_stack_pointer++;
    
    //if the stack is full
    if (voice_alloc_data->mono_note_stack_pointer >= VOICE_MONO_BUFFER_SIZE)
    {
        //remove the oldest note from the stack
        RemoveNoteFromMonoStack (0, VOICE_MONO_BUFFER_SIZE, voice_alloc_data);
    }
}

//====================================================================================
//====================================================================================
//====================================================================================
//Pulls a note from the mono stack

void PullNoteFromMonoStack (uint8_t note_num, VoiceAllocData *voice_alloc_data)
{
    uint8_t note_index;
    
    //find the note in the stack buffer
    for (uint8_t i = 0; i < voice_alloc_data->mono_note_stack_pointer; i++)
    {
        //if it matches
        if (voice_alloc_data->note_data[i].note_num == note_num)
        {
            //store index
            note_index = i;
            
            //break from loop
            break;
        }
        
    } //if (uint8_t i = 0; i < voice_alloc_data->mono_note_stack_pointer; i++)
    
    //remove the note from the stack
    RemoveNoteFromMonoStack (note_index, voice_alloc_data->mono_note_stack_pointer, voice_alloc_data);
}

//====================================================================================
//====================================================================================
//====================================================================================
//Processes a note message recieived from any source, sending it to the needed places

void ProcessNoteMessage (uint8_t message_buffer[],
                         PatchParameterData patch_param_data[],
                         VoiceAllocData *voice_alloc_data,
                         bool send_to_midi_out,
                         int sock,
                         struct sockaddr_un sound_engine_sock_addr,
                         bool from_internal_keyboard,
                         uint8_t keyboard_key_num)
{
    
    //====================================================================================
    //Voice allocation for sound engine
    
    //FIXME: it is kind of confusing how in mono mode the seperate functions handle the setting
    //of voice_alloc_data, however in poly mode all of that is done within this function. It
    //may be a good idea to rewrite the voice allocation stuff to make this neater.
    
    //=========================================
    //if a note-on message
    if ((message_buffer[0] & MIDI_STATUS_BITS) == MIDI_NOTEON)
    {
        //====================
        //if in poly mode
        if (patch_param_data[PARAM_VOICE_MODE].user_val > 0)
        {
            //get next voice we can use
            uint8_t free_voice = GetNextFreeVoice (voice_alloc_data, sock, sound_engine_sock_addr);
            
            #ifdef DEBUG
            printf ("[VB] Next free voice: %d\r\n", free_voice);
            #endif
            
            //if we have a voice to use
            if (free_voice > 0)
            {
                //put free_voice into the correct range
                free_voice -= 1;
                
                //store the note info for this voice
                voice_alloc_data->note_data[free_voice].note_num = message_buffer[1];
                voice_alloc_data->note_data[free_voice].note_vel = message_buffer[2];
                
                //set the last played voice (for note stealing)
                voice_alloc_data->last_voice = free_voice + 1;
                
                //set internal keyboard note stuff
                voice_alloc_data->note_data[free_voice].from_internal_keyboard = from_internal_keyboard;
                if (from_internal_keyboard)
                    voice_alloc_data->note_data[free_voice].keyboard_key_num = keyboard_key_num;
                
                //Send to the sound engine...
                
                uint8_t note_buffer[3] = {MIDI_NOTEON + free_voice, message_buffer[1], message_buffer[2]};
                SendToSoundEngine (note_buffer, 3, sock, sound_engine_sock_addr);
                
            } //if (free_voice > 0)
            
        } //if (patch_param_data[PARAM_VOICE_MODE].user_val > 0)
        
        //====================
        //if in mono mode
        else
        {
            AddNoteToMonoStack (message_buffer[1], message_buffer[2], voice_alloc_data, from_internal_keyboard, keyboard_key_num);
            
            //Send to the sound engine for voice 0...
            uint8_t note_buffer[3] = {MIDI_NOTEON, message_buffer[1], message_buffer[2]};
            SendToSoundEngine (note_buffer, 3, sock, sound_engine_sock_addr);
            
        } //else (mono mode)
        
    } //((message_buffer[0] & MIDI_STATUS_BITS) == MIDI_NOTEON)
    
    //=========================================
    //if a note-off message
    else
    {
        //====================
        //if in poly mode
        if (patch_param_data[PARAM_VOICE_MODE].user_val > 0)
        {
            //free used voice of this note
            uint8_t freed_voice = FreeVoiceOfNote (message_buffer[1], voice_alloc_data);
            
            #ifdef DEBUG
            printf ("[VB] freed voice: %d\r\n", freed_voice);
            #endif
            
            //if we sucessfully freed a voice
            if (freed_voice > 0)
            {
                //put freed_voice into the correct range
                freed_voice -= 1;
                
                //reset the note info for this voice
                voice_alloc_data->note_data[freed_voice].note_num = VOICE_NO_NOTE;
                voice_alloc_data->note_data[freed_voice].note_vel = VOICE_NO_NOTE;
                voice_alloc_data->note_data[freed_voice].keyboard_key_num = VOICE_NO_NOTE;
                
                //Send to the sound engine...
                
                uint8_t note_buffer[3] = {MIDI_NOTEOFF + freed_voice, message_buffer[1], message_buffer[2]};
                SendToSoundEngine (note_buffer, 3, sock, sound_engine_sock_addr);
                
            } //if (freed_voice > 0)
            
        } //if (patch_param_data[PARAM_VOICE_MODE].user_val > 0)
        
        //====================
        //if in mono mode
        else
        {
            PullNoteFromMonoStack (message_buffer[1], voice_alloc_data);
            
            //if there is still atleast one note on the stack
            if (voice_alloc_data->mono_note_stack_pointer != 0)
            {
                //Send a note-on message to the sound engine with the previous note on the stack...
                
                uint8_t note_num = voice_alloc_data->note_data[voice_alloc_data->mono_note_stack_pointer - 1].note_num;
                uint8_t note_vel = voice_alloc_data->note_data[voice_alloc_data->mono_note_stack_pointer - 1].note_vel;
                
                uint8_t note_buffer[3] = {MIDI_NOTEON, note_num, note_vel};
                SendToSoundEngine (note_buffer, 3, sock, sound_engine_sock_addr);
                
            } //if (prev_stack_note != VOICE_NO_NOTE)
            
            //if this was the last note in the stack
            else
            {
                //Send to the sound engine as a note off...
                
                uint8_t note_buffer[3] = {MIDI_NOTEOFF, message_buffer[1], message_buffer[2]};
                SendToSoundEngine (note_buffer, 3, sock, sound_engine_sock_addr);
            }
            
        } //else (mono mode)
        
    } //else (note-off message)
    
    //====================================================================================
    //Sending to MIDI-out
    
    //Send to MIDI out if needed
    if (send_to_midi_out)
    {
        WriteToMidiOutFd (message_buffer, 3);
    }
}

//====================================================================================
//====================================================================================
//====================================================================================
//Returns a note number for a keyboard key based on the current scale, octave, and transpose settings

int16_t GetNoteForKeyboardKey (uint8_t keyboard_key_num, PatchParameterData patch_param_data[])
{
    int16_t note_num;
    
    //apply scale value
    //Note numbers come from the keyboard in the range of 0 - KEYBOARD_NUM_OF_KEYS-1,
    //and are used to select an index of keyboardScales[patchParameterData[PARAM_KEYS_SCALE].user_val]
    note_num = keyboardScales[patch_param_data[PARAM_KEYS_SCALE].user_val][keyboard_key_num];
    
    //apply octave value
    //if octave value is 64 (0) bottom key is note 64 (middle E, as E is the first key)
    note_num = (note_num + 64) + ((patch_param_data[PARAM_KEYS_OCTAVE].user_val - 64) * 12);
    
    //apply tranpose
    //a value of 64 (0) means no transpose
    note_num += patch_param_data[PARAM_KEYS_TRANSPOSE].user_val - 64;
    
    //make sure note number is still in range
    if (note_num > 127)
        note_num = 127;
    else if (note_num < 0)
        note_num = 0;
    
    return note_num;
}

//====================================================================================
//====================================================================================
//====================================================================================
//Processes a poly aftertouch message, sending it to the needed places

void ProcessPolyAftertouchMessage (uint8_t message_buffer[],
                                   VoiceAllocData *voice_alloc_data,
                                   bool send_to_midi_out,
                                   int sock,
                                   struct sockaddr_un sound_engine_sock_addr)
{
    //====================================================================================
    //Sending to sound engine
    //Currently the sound engine doesn't do anything with aftertouch, so disabled sending it
    //to save CPU usage
    
//    uint8_t voice_list[NUM_OF_VOICES];
//    uint8_t num_of_voices;
//    
//    //get all voices that the note for this aftertouch message are currently playing on
//    num_of_voices = GetVoicesOfNote (message_buffer[1], voice_alloc_data, voice_list);
//    
//    //for each found voice
//    for (uint8_t voice = 0; voice < num_of_voices; voice++)
//    {
//        //send the aftertouch message to the sound engine, using channel to signify voice number
//        uint8_t pat_buffer[3] = {MIDI_PAT + voice_list[voice] - 1, message_buffer[1], message_buffer[2]};
//        SendToSoundEngine (pat_buffer, 3, sock, sound_engine_sock_addr);
//        
//    } //for (uint8_t voice = 0; voice < num_of_voices; voice++)
    
    //====================================================================================
    //Sending to MIDI-out
    
    //Send to MIDI out if needed
    if (send_to_midi_out)
    {
        //make sure channel is set to 0
        message_buffer[0] = MIDI_PAT;
        
        WriteToMidiOutFd (message_buffer, 3);
        
    } //if (send_to_midi_out)
}

//====================================================================================
//====================================================================================
//====================================================================================
//Processes a CC/param message from any source, sending it to the needed places

void ProcessCcMessage (uint8_t message_buffer[],
                       PatchParameterData patch_param_data[],
                       VoiceAllocData *voice_alloc_data,
                       bool send_to_midi_out,
                       int sock,
                       struct sockaddr_un sound_engine_sock_addr)
{
    uint8_t param_num = message_buffer[1];
    uint8_t param_val = message_buffer[2];
    
    //bound the param val if needed
    if (param_val > patch_param_data[param_num].user_max_val)
        param_val = patch_param_data[param_num].user_max_val;
    else if (param_val < patch_param_data[param_num].user_min_val)
        param_val = patch_param_data[param_num].user_min_val;
    
    //====================================================================================
    //Process certain parameter changes
    
    //if voice mode has changed
    if (param_num == PARAM_VOICE_MODE && patch_param_data[param_num].user_val != param_val)
    {
        //kill all voices
        KillAllVoices (patch_param_data, voice_alloc_data, sock, sound_engine_sock_addr);

    } //if (param_num == PARAM_VOICE_MODE && patch_param_data[param_num].user_val != param_val)
    
    //if keyboard scale, octave, or tranpose value has changed
    else if ((param_num == PARAM_KEYS_SCALE || param_num == PARAM_KEYS_OCTAVE || param_num == PARAM_KEYS_TRANSPOSE) &&
             patch_param_data[param_num].user_val != param_val)
    {
        //update pitch of playing notes/voices...
        
        //set the new param value instantly
        patch_param_data[param_num].user_val = param_val;
        message_buffer[2] = param_val;
        
        //for each voice
        for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
        {
            //if this voice is currently playing a note from the internal keyboard
            if (voice_alloc_data->note_data[voice].from_internal_keyboard == true &&
                voice_alloc_data->note_data[voice].keyboard_key_num != VOICE_NO_NOTE)
            {
                //get the new note for the key playing this voice
                uint8_t note_num = GetNoteForKeyboardKey (voice_alloc_data->note_data[voice].keyboard_key_num, patch_param_data);
                
                //update the note pitch of the voice
                uint8_t cc_buf[3] = {MIDI_CC + voice, PARAM_UPDATE_NOTE_PITCH, note_num};
                SendToSoundEngine (cc_buf, 3, sock, sound_engine_sock_addr);
                
                //Update voice allocation note value...
                
                //if in poly mode
                if (patch_param_data[PARAM_VOICE_MODE].user_val > 0)
                    voice_alloc_data->note_data[voice].note_num = note_num;

                //if in mono mode
                else
                    voice_alloc_data->note_data[voice_alloc_data->mono_note_stack_pointer].note_num = note_num;
                
            } //if (playing note from internal keyboard)
            
        } //for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
        
    } //if (keyboard scale, octave, or transpose changed)
    
    //if global volume has changed
    else if (param_num == PARAM_GLOBAL_VOLUME && patch_param_data[param_num].user_val != param_val)
    {
        //set the Linux system volume...
        
        //create start of amixer command to set 'Speaker' control value
        //See http://linux.die.net/man/1/amixer for more options
        uint8_t volume_cmd[64] = {"amixer -q sset Speaker "};
        
        //turn the param value into a percentage string
        uint8_t volume_string[16];
        sprintf(volume_string, "%d%%", param_val);
        
        //append the value string onto the command
        strcat (volume_cmd, volume_string);
        
        //printf ("[VB] - volume command: %s\r\n", volume_cmd);
        
        //Send the command to the system
        system (volume_cmd);
        
    } //if (param_num == PARAM_GLOBAL_VOLUME && patch_param_data[param_num].user_val != param_val)
    
    //if a command CC (not a patch param)
    else if (param_num == PARAM_CMD)
    {
        //if got a request for all current patch data from the VTS Editor via MIDI-out
        if (param_val == CMD_REQUEST_ALL_PATCH_DATA)
        {
            printf ("[VB] Got a request for all patch data from the VTS Editor. Sending to MIDI-out...\r\n");
            
            //send back all patch data to the MIDI-out
            for (uint8_t i = 0; i < 127; i++)
            {
                if (patch_param_data[i].patch_param == true)
                {
                    uint8_t cc_buf[3] = {MIDI_CC, i, patch_param_data[i].user_val};
                    WriteToMidiOutFd (cc_buf, 3);
                }
                
            } //for (uint8_t i = 0; i < 127; i++)
            
            //send a command CC to flag that all patch data has been sent
            uint8_t cc_buf[3] = {MIDI_CC, PARAM_CMD, CMD_SENT_ALL_PATCH_DATA};
            WriteToMidiOutFd (cc_buf, 3);
            
        } //if (param_val == CMD_REQUEST_ALL_PATCH_DATA)
        
        //if got a request for all current panel patch data settings, most probably from the vintageSoundEngine
        else if (param_val == CMD_REQUEST_PANEL_PARAM_DATA)
        {
            printf ("[VB] Got a request for current panel settings from vintageSoundEngine\r\n");
            
            //forward the message to the panel
            WriteToPanelFd (message_buffer, 3);
            
        } //else if (param_val == CMD_REQUEST_PANEL_PARAM_DATA)
        
        //if got a disabled panel command
        else if (param_val == CMD_DISABLE_PANEL)
        {
            printf ("[VB] Disabling panel\r\n");
            panelIsEnabled = false;
            
        } //else if (param_val == CMD_DISABLE_PANEL)
        
        //if got a disabled panel command
        else if (param_val == CMD_ENABLE_PANEL)
        {
            printf ("[VB] Enabling panel\r\n");
            panelIsEnabled = true;
            
        } //else if (param_val == CMD_ENABLE_PANEL)
        
    } //else if (param_num == PARAM_CMD)
    
    
    //====================================================================================
    //Store the new param value

    if (param_num != PARAM_CMD)
    {
        patch_param_data[param_num].user_val = param_val;
        message_buffer[2] = param_val;
    }
    
    //====================================================================================
    //Send to the sound engine if needed
    if (patch_param_data[param_num].sound_param)
    {
        //set channel to 15 to signify that this message is ment for all voices
        message_buffer[0] = MIDI_CC + 15;
        
        SendToSoundEngine (message_buffer, 3, sock, sound_engine_sock_addr);
    }
    
    //====================================================================================
    //Send to MIDI out if needed
    if (send_to_midi_out && patch_param_data[param_num].patch_param)
    {
        //make sure channel is set to 0
        message_buffer[0] = MIDI_CC;
        
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
        perror("[VB] Closing keyboard_fd file descriptor");
    if (close (midi_fd) == -1)
        perror("[VB] Closing midi_fd file descriptor");
    if (close (panel_fd) == -1)
        perror("[VB] Closing panel_fd file descriptor");
    
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
    uint8_t socket_input_buf[64] = {0};
    
    struct sockaddr_un brain_sock_addr, sound_engine_sock_addr, from_addr;
    int sock;
    
    struct pollfd poll_fds[NUM_OF_POLL_FDS];
    
    //FIXME: group these variables into a struct, and have an array of that struct instead,
    //passing in a reference to the struct instance into ProcInputByte.
    //If I get this working, do the same within vintageSoundEngine
    uint8_t input_message_buffer[NUM_OF_INPUT_SRCS][1024] = {{0}}; //stores any MIDI message (including NRPN and sysex).
    uint8_t input_message_counter[NUM_OF_INPUT_SRCS] = {0};
    uint8_t input_message_running_status_value[NUM_OF_INPUT_SRCS] = {0};
    //don't init this to 0, incase the first CC we get is 32, causing it to be ignored!
    uint8_t input_message_prev_cc_num[NUM_OF_INPUT_SRCS] = {127};
    
    bool send_to_midi_out = false;
    
    //==========================================================
    //'patch' parameter data stuff
    
    PatchParameterData patchParameterData[128];
    
    for (uint8_t i = 0; i < 128; i++)
    {
        patchParameterData[i] = defaultPatchParameterData[i];
    }
    
    //==========================================================
    //voice alloc stuff
    
    VoiceAllocData voice_alloc_data;
    
    for (uint8_t i = 0; i < NUM_OF_VOICES; i++)
    {
        voice_alloc_data.free_voices[i] = i + 1;
    }
    
    for (uint8_t i = 0; i < VOICE_ALLOC_NOTE_BUFFER_SIZE; i++)
    {
        voice_alloc_data.note_data[i].note_num = VOICE_NO_NOTE;
        voice_alloc_data.note_data[i].note_vel = VOICE_NO_NOTE;
        
        voice_alloc_data.note_data[i].from_internal_keyboard = false;
        voice_alloc_data.note_data[i].keyboard_key_num = VOICE_NO_NOTE;
    }
    
    voice_alloc_data.mono_note_stack_pointer = 0;
    voice_alloc_data.last_voice = 0;
    
    
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
    
    //open UART4 device file for read/write to panel with baud rate of 38400
    panel_fd = SetupSerialPort (PANEL_SERIAL_PATH, 38400, true);
    
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
    //Set up poll() stuff for waiting for events on file descriptors and sockets
    
    poll_fds[INPUT_SRC_KEYBOARD].fd = keyboard_fd;
    poll_fds[INPUT_SRC_KEYBOARD].events = POLLIN;
    poll_fds[INPUT_SRC_MIDI_IN].fd = midi_fd;
    poll_fds[INPUT_SRC_MIDI_IN].events = POLLIN;
    poll_fds[INPUT_SRC_PANEL].fd = panel_fd;
    poll_fds[INPUT_SRC_PANEL].events = POLLIN;
    poll_fds[INPUT_SRC_SOCKETS].fd = sock;
    poll_fds[INPUT_SRC_SOCKETS].events = POLLIN;
    
    //Variable to hold the size of a struct sockaddr_un.
    socklen_t len = (socklen_t)sizeof(struct sockaddr_un);
    
    //==========================================================
    //Enter main loop, and just read any data that comes in over the serial ports or sockets
    
    printf ("[VB] Starting reading data from serial ports...\n");
    
//    uint8_t test_num = 48;
//    uint8_t test_vel = 20;
    
    while (true)
    {
        //for now only send data from keyboard/panel to MIDI-out if global volume is 0,
        //due to the issue where sending to MIDI-out causes glitches in the audio.
        //FIXME: this MIDI-out audio glitch issue
        if (patchParameterData[PARAM_GLOBAL_VOLUME].user_val == 0)
            send_to_midi_out = true;
        else
            send_to_midi_out = false;
        
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
                    
                    if (input_message_flag == MIDI_NOTEON || input_message_flag == MIDI_NOTEOFF)
                    {
                        #ifdef DEBUG
                        printf ("[VB] Received note-on/off message from keyboard\r\n");
                        #endif
                        
                        //Set note number based on keyboard scale, octave, and transpose settings...
                        uint8_t note_index = input_message_buffer[INPUT_SRC_KEYBOARD][1];
                        int16_t note_num = GetNoteForKeyboardKey (note_index, patchParameterData);
                        
                        //put new note number back into input_message_buffer buffer
                        input_message_buffer[INPUT_SRC_KEYBOARD][1] = note_num;
                        
                        //Process the note message
                        ProcessNoteMessage (input_message_buffer[INPUT_SRC_KEYBOARD],
                                            patchParameterData,
                                            &voice_alloc_data,
                                            send_to_midi_out,
                                            sock,
                                            sound_engine_sock_addr,
                                            true,
                                            note_index);
                        
                    } //if (input_message_flag == MIDI_NOTEON)
                    
                    else if (input_message_flag == MIDI_PAT)
                    {
                        #ifdef DEBUG
                        printf ("[VB] Received poly aftertouch message from keyboard\r\n");
                        #endif
                        
                        //Set note number based on keyboard scale, octave, and transpose settings...
                        uint8_t note_index = input_message_buffer[INPUT_SRC_KEYBOARD][1];
                        int16_t note_num = GetNoteForKeyboardKey (note_index, patchParameterData);
                        
                        //put new note number back into input_message_buffer buffer
                        input_message_buffer[INPUT_SRC_KEYBOARD][1] = note_num;
                        
                        //Process the aftertouch message
                        ProcessPolyAftertouchMessage (input_message_buffer[INPUT_SRC_KEYBOARD],
                                                      &voice_alloc_data,
                                                      send_to_midi_out,
                                                      sock,
                                                      sound_engine_sock_addr);
                        
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
                #ifdef DEBUG
                //display the read byte
                printf ("[VB] Byte read from MIDI interface: %d\n", serial_input_buf[0]);
                #endif
                
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
                        
                        ProcessNoteMessage (input_message_buffer[INPUT_SRC_MIDI_IN], patchParameterData, &voice_alloc_data, false, sock, sound_engine_sock_addr, false, 0);
                        
                    } //if (input_message_flag == MIDI_NOTEON)
                    
                    else if (input_message_flag == MIDI_NOTEOFF)
                    {
                        #ifdef DEBUG
                        printf ("[VB] Received note-off message from MIDI\r\n");
                        #endif
                        
                        //set the MIDI channel to 0
                        input_message_buffer[INPUT_SRC_MIDI_IN][0] = MIDI_NOTEOFF;
                        
                        ProcessNoteMessage (input_message_buffer[INPUT_SRC_MIDI_IN], patchParameterData, &voice_alloc_data, false, sock, sound_engine_sock_addr, false, 0);
                        
                    } //else if (input_message_flag == MIDI_NOTEOFF)
                    
                    else if (input_message_flag == MIDI_CC)
                    {
                        #ifdef DEBUG
                        printf ("[VB] Received CC message from MIDI\r\n");
                        #endif
                        
                        //set the MIDI channel to 0
                        input_message_buffer[INPUT_SRC_MIDI_IN][0] = MIDI_CC;
                        
                        ProcessCcMessage (input_message_buffer[INPUT_SRC_MIDI_IN], patchParameterData, &voice_alloc_data, false, sock, sound_engine_sock_addr);
                        
                    } //else if (input_message_flag == MIDI_CC)
                    
                } //if (input_message_flag)

            } //if (ret)
            
        } //if (poll_fds[INPUT_SRC_MIDI_IN].revents & POLLIN)
        
        //==========================================================
        //Reading data from the panel serial port
        
        //if an event has happened on the panel serial file descriptor
        //FIXME: can this be an else if?
        if (poll_fds[INPUT_SRC_PANEL].revents & POLLIN)
        {
            //attempt to read a byte from the panel device file
            ret = read (panel_fd, serial_input_buf, 1);
            
            //if read something
            if (ret != -1)
            {
                //display the read byte
                //printf ("[VB] Byte read from panel: %d\n", serial_input_buf[0]);
                
                //process the read byte
                uint8_t input_message_flag = ProcInputByte (serial_input_buf[0],
                                                            input_message_buffer[INPUT_SRC_PANEL],
                                                            &input_message_counter[INPUT_SRC_PANEL],
                                                            &input_message_running_status_value[INPUT_SRC_PANEL],
                                                            &input_message_prev_cc_num[INPUT_SRC_PANEL]);
                
                //if we have received a full MIDI message
                if (input_message_flag)
                {
                    //printf ("[VB] Received full MIDI message from panel with status byte %d\n", input_message_buffer[INPUT_SRC_PANEL][0]);
                    
                    //if it is a CC, and the panel is currently enabled or it is a volume control message
                    if (input_message_flag == MIDI_CC &&
                        (panelIsEnabled || input_message_buffer[INPUT_SRC_PANEL][1] == PARAM_GLOBAL_VOLUME))
                    {
                        #ifdef DEBUG
                        printf ("[VB] %s change from panel - CC num: %d, CC val %d\r\n",
                                patchParameterData[input_message_buffer[INPUT_SRC_PANEL][1]].param_name,
                                input_message_buffer[INPUT_SRC_PANEL][1],
                                input_message_buffer[INPUT_SRC_PANEL][2]);
                        #endif
                        
                        ProcessCcMessage (input_message_buffer[INPUT_SRC_PANEL], patchParameterData, &voice_alloc_data, send_to_midi_out, sock, sound_engine_sock_addr);
                        
                    } //else if (input_message_flag == MIDI_CC)
                    
                } //if (input_message_flag)
                
            } //if (ret)
            
        } //if (poll_fds[INPUT_SRC_PANEL].revents & POLLIN)
        
        //====================================================================================
        // Data available to be read from socket
        if (poll_fds[INPUT_SRC_SOCKETS].revents & POLLIN)
        {
            //reset from_addr.sun_path so that I'll get a clean path string below
            //I think 104 is the max size of sun_path
            memset (from_addr.sun_path, 0, 104);
            
            // fill the buffer and get the source
            ret = recvfrom (sock, socket_input_buf, sizeof(socket_input_buf), 0, (struct sockaddr *) &from_addr, &len);
            
            //====================================================================================
            // Data available to be read from vintageSoundEngine
            
            if (strncmp (from_addr.sun_path, SOCK_SOUND_ENGINE_FILENAME, strlen(from_addr.sun_path)) == 0)
            {
                //for each received byte
                for (uint8_t i = 0; i < ret; i++)
                {
                    //display the read byte
                    //printf ("[VB] Byte read from vintageSoundEngine: %d\n", socket_input_buf[i]);
                    
                    //process the read byte
                    uint8_t input_message_flag = ProcInputByte (socket_input_buf[i],
                                                                input_message_buffer[INPUT_SOCK_SRC_VOICE],
                                                                &input_message_counter[INPUT_SOCK_SRC_VOICE],
                                                                &input_message_running_status_value[INPUT_SOCK_SRC_VOICE],
                                                                &input_message_prev_cc_num[INPUT_SOCK_SRC_VOICE]);
                    
                    //if we have received a full MIDI message
                    if (input_message_flag)
                    {
                        if (input_message_flag == MIDI_CC)
                        {
                            #ifdef DEBUG
                            printf ("[VB] Received CC message from vintageSoundEngine\r\n");
                            #endif
                            
                            ProcessCcMessage (input_message_buffer[INPUT_SOCK_SRC_VOICE],
                                              patchParameterData,
                                              &voice_alloc_data,
                                              send_to_midi_out,
                                              sock,
                                              sound_engine_sock_addr);
                            
                        } //if (input_message_flag == MIDI_CC)
                        
                    } // if (input_message_flag)
                    
                } //for (uint8_t i = 0; i < nr; i++)
                
            } //if (strncmp (from_addr.sun_path, SOCK_SOUND_ENGINE_FILENAME, strlen(from_addr.sun_path)) == 0)
            
        } //if (poll_fds[INPUT_SRC_SOCKETS].revents & POLLIN)
    
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
