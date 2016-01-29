///
//  globals.h
//
//  Created by Liam Lacey on 28/01/2016.
//
//
//

//==========================================================================
//Socket stuff.
//I needed to create the vintageToySynthProject directory in order for these sockets to be setup

//server socket
#define SOCK_BRAIN_FILENAME "/var/vintageToySynthProject/vintageBrain.serv"

//client sockets
#define SOCK_SOUND_ENGINE_FILENAME "/var/vintageToySynthProject/vintageSoundEngine.cli"

//==========================================================================
#define NUM_OF_VOICES 2

//==========================================================================
//MIDI defines

#define MIDI_NOTEOFF 0x80
#define MIDI_NOTEON 0x90
#define MIDI_PAT 0xA0
#define MIDI_CC 0xB0
#define MIDI_PROGRAM_CHANGE 0xC0
#define MIDI_CAT 0xD0
#define MIDI_PITCH_BEND 0xE0

#define MIDI_NOTEOFF_MAX 0x8F
#define MIDI_NOTEON_MAX 0x9F
#define MIDI_PAT_MAX 0xAF
#define MIDI_CC_MAX 0xBF
#define MIDI_PROGRAM_CHANGE_MAX 0xCF
#define MIDI_CAT_MAX 0xDF
#define MIDI_PITCH_BEND_MAX 0xEF

#define MIDI_CLOCK 0xF8
#define MIDI_CLOCK_START 0xFA
#define MIDI_CLOCK_CONTINUE 0xFB
#define MIDI_CLOCK_STOP 0xFC

#define MIDI_SYSEX_START 0xF0
#define MIDI_SYSEX_END 0xF7

#define MIDI_CHANNEL_BITS 0b00001111
#define MIDI_STATUS_BITS 0b11110000