/*
 vintageSoundEngine.cpp
 
 Created by Liam Lacey on 28/01/2016.
 Copyright 2016 Liam Lacey. All rights reserved.
 
 This is the main file for the vintageSoundEngine application for the Vintage Toy Piano Synth project.
 This application is the audio synthesis engine for the synthesiser, using the Maximilian synthesis library.
 
 This particular file is heavily based on Maximilians player.cpp file which connects to RtAudio or PortAudio,
 attaching the play() function as the audio processing callback function. It also listens from messages
 from the vintageBrain socket.
 
 The audio processing for each voice is handled within the VintageVoice objects.
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
#include <iostream>

#include "vintageSoundEngine.h"
#include "Maximilian/maximilian.h"
#include "vintageVoice.h"

//FIXME: am I able to declare these in main and pass it into routing and play?
//I think I pass into routing using the *userData variable.
VintageVoice *vintageVoice[NUM_OF_VOICES];
//maxiDistortion distortion;

#ifdef MAXIMILIAN_PORTAUDIO
#include "Maximilian/portaudio.h"
//#include "pa_mac_core.h"
#elif defined(MAXIMILIAN_RT_AUDIO)
	#if defined( __WIN32__ ) || defined( _WIN32 )
		#include <dsound.h>
	#endif
#include "Maximilian/RtAudio.h"
#endif

//==========================================================
//==========================================================
//==========================================================

//use this to do any initialisation if you want.
void setup();

//run dac! Very very often. Too often in fact. er...
void play(double *output);

//==========================================================
//==========================================================
//==========================================================
//Sends data to the vintageBrain application via a socket

void sendToVintageBrainSocket (int sock, uint8_t data_buffer, uint8_t data_buffer_size)
{
    if (send (sock, data_buffer, data_buffer_size, 0) == -1)
    {
        printf ("[VSE] ERROR: Sending data to vintageBrain socket\r\n");
    }
}

//==========================================================
//==========================================================
//==========================================================

#ifdef MAXIMILIAN_PORTAUDIO
int routing(const void *inputBuffer,
		void *outputBuffer,
		unsigned long nBufferFrames,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags status,
		void *userData )
{
#elif defined(MAXIMILIAN_RT_AUDIO)
int routing	(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
			 double streamTime, RtAudioStreamStatus status, void *userData )
    {
#endif
	
	unsigned int i, j;
	
#ifdef MAXIMILIAN_PORTAUDIO
	float *buffer = (float *) outputBuffer;
#elif defined(MAXIMILIAN_RT_AUDIO)
	double *buffer = (double *) outputBuffer;
#endif
	double *lastValues = (double *) userData;
	//	double currentTime = (double) streamTime; Might come in handy for control
	if ( status )
		std::cout << "Stream underflow detected!" << std::endl;
	for ( i=0; i<nBufferFrames; i++ ) {	
	}
	// Write interleaved audio data.
	for ( i=0; i<nBufferFrames; i++ ) {
		play(lastValues);			
		for ( j=0; j<maxiSettings::channels; j++ ) {
			*buffer++=lastValues[j];
		}
	}
        
	return 0;
}

    //==========================================================
    //==========================================================
    //==========================================================
    
    void setup()
    {
        //don't think I need this function - init stuff will be done in the constructor of vintageVoice
    }
    
    //==========================================================
    //==========================================================
    //==========================================================
    
    void play (double *output)
    {
        double voice_out[NUM_OF_VOICES];
        double mix = 0;
        //double distortionOut;
        
        //process each voice
        for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
        {
            vintageVoice[voice]->processAudio (&voice_out[voice]);
        }
        
        //mix all voices together (for some reason this won't work if done in the above for loop...)
        for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
        {
            mix += voice_out[voice];
        }
        
        //set output
        for (uint8_t i = 0; i < maxiSettings::channels; i++)
        {
            output[i] = mix / (double)NUM_OF_VOICES;
        }
        
        //Trying out individual distortions per each voice to see if this sounds better.
        //May be too CPU intensive though.
        
//        distortionOut = distortion.atanDist (mix, 200.0);
//        
//        //set output
//        output[0] = distortionOut;
//        output[1] = distortionOut;
    }
    
    //==========================================================
    //==========================================================
    //==========================================================
    //Function that processes MIDI bytes coming from the vintageBrain socket
    
    uint8_t ProcInputByte (uint8_t input_byte, uint8_t message_buffer[], uint8_t *byte_counter)
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
        
        static uint8_t running_status_value = 0;
        static uint8_t prev_cc_num = 127; //don't init this to 0, incase the first CC we get is 32, causing it to be ignored!
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
                
                running_status_value = message_buffer[0];
            }
            
            //If it's a clock message
            else if (input_byte >= MIDI_CLOCK && input_byte <= MIDI_CLOCK_STOP)
            {
                //Don't do anything with MidiInCount or running_status_value here,
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
                    
                    //Check running_status_value here instead of message_buffer[0], as it could be possible
                    //that we are receiving running status messages entwined with clock messages, where
                    //message_buffer[0] will actually be equal to MIDI_CLOCK.
                    
                    //TODO: process NRPNs, correctly (e.g. process 9 byte NRPN, and then as 12 bytes if the following CC is 0x26)
                    
                    //if it's a channel aftertouch message
                    if (running_status_value >= MIDI_CAT && running_status_value <= MIDI_CAT_MAX)
                    {
                        message_buffer[1] = input_byte;
                        result = MIDI_CAT;
                        
                        //set the correct status value
                        message_buffer[0] = running_status_value;
                        
                        //wait for next data byte if running status
                        *byte_counter = 1;
                    }
                    
                    //if it's a program change message
                    else if (running_status_value >= MIDI_PROGRAM_CHANGE && running_status_value <= MIDI_PROGRAM_CHANGE_MAX)
                    {
                        message_buffer[1] = input_byte;
                        result = MIDI_PROGRAM_CHANGE;
                        
                        //set the correct status value
                        message_buffer[0] = running_status_value;
                        
                        //wait for next data byte if running status
                        *byte_counter = 1;
                    }
                    
                    //else it's a 3+ byte MIDI message
                    else
                    {
                        message_buffer[1] = input_byte;
                        *byte_counter = 2;
                        
                        //set the correct status value
                        message_buffer[0] = running_status_value;
                    }
                    
                    break;
                }
                    
                case 2:
                {
                    //Process the third byte...
                    
                    result = 0;
                    
                    //TODO: process NRPNs, correctly
                    
                    //if it's not zero it's a note on
                    if (input_byte && (running_status_value >= MIDI_NOTEON && running_status_value <= MIDI_NOTEON_MAX))
                    {
                        //3rd byte is velocity
                        message_buffer[2] = input_byte;
                        result = MIDI_NOTEON;
                        
                        //set the correct status value
                        message_buffer[0] = running_status_value;
                    }
                    
                    //if it's a note off
                    else if ((running_status_value >= MIDI_NOTEOFF && running_status_value <= MIDI_NOTEOFF_MAX) ||
                             (!input_byte && (running_status_value >= MIDI_NOTEON && running_status_value <= MIDI_NOTEON_MAX)))
                    {
                        //3rd byte should be zero
                        message_buffer[2] = 0;
                        result = MIDI_NOTEOFF;
                        
                        //set the correct status value
                        message_buffer[0] = running_status_value;
                    }
                    
                    //if it's a CC
                    else if (running_status_value >= MIDI_CC && running_status_value <= MIDI_CC_MAX)
                    {
                        //if we have got a 32-63 CC (0-31 LSB/fine CC),
                        //and the last CC we received was the MSB/coarse CC pair
                        if (message_buffer[1] >= 32 && message_buffer[1] <= 63 && (prev_cc_num == (message_buffer[1] - 32)))
                        {
                            //Don't do anything. Right now if this is the case we just want to ignore it.
                            //However in the future we may want to process coarse/fine CC pairs to
                            //control parameters at a higher resolution.
                            std::cout << "[VSE] Received CC num " << (int)message_buffer[1] << " directly after CC num " << (int)prev_cc_num << ", so ignoring it" << std::endl;
                        }
                        
                        else
                        {
                            message_buffer[2] = input_byte;
                            result = MIDI_CC;
                            
                            //set the correct status value
                            message_buffer[0] = running_status_value;
                            
                        } //else
                        
                        //store this CC num as the previously received CC
                        prev_cc_num = message_buffer[1];
                    }
                    
                    //if it's a poly aftertouch message
                    else if (running_status_value >= MIDI_PAT && running_status_value <= MIDI_PAT_MAX)
                    {
                        message_buffer[2] = input_byte;
                        result = MIDI_PAT;
                        
                        //set the correct status value
                        message_buffer[0] = running_status_value;
                    }
                    
                    //if it's a pitch bend message
                    else if (running_status_value >= MIDI_PITCH_BEND && running_status_value <= MIDI_PITCH_BEND_MAX)
                    {
                        message_buffer[2] = input_byte;
                        result = MIDI_PITCH_BEND;
                        
                        //set the correct status value
                        message_buffer[0] = running_status_value;
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

    //==========================================================
    //==========================================================
    //==========================================================
    //This is main()
    
int main()
{
    std::cout << "[VSE] Running vintageSoundEngine..." << std::endl;
    
    uint8_t socket_input_buf[16] = {0};
    int ret = 0;
    
    for (uint8_t i = 0; i < NUM_OF_VOICES; i++)
    {
        vintageVoice[i] = new VintageVoice(i);
    }
    
    //'patch' parameter data
    PatchParameterData patchParameterData[128];
    
    for (uint8_t i = 0; i < 128; i++)
    {
        patchParameterData[i] = defaultPatchParameterData[i];
    }
    
    //===============================================================
    //Setup socket for comms with vintageBrain
    
    std::cout << "[VSE] Setting up sockets..." << std::endl;
    
    int sock;
    struct sockaddr_un client, server;
    
    if ((sock = socket (AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
        fprintf(stderr,"[VSE] Socket");
    }
    
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, SOCK_BRAIN_FILENAME);
    client.sun_family = AF_UNIX;
    strcpy(client.sun_path, SOCK_SOUND_ENGINE_FILENAME);
    unlink(client.sun_path);
    
    if (bind (sock, (struct sockaddr *) &client, sizeof(client)) == -1)
    {
        std::cout << "[VSE] ERROR: Bind client" << std::endl;
    }
    
    if (connect (sock, (struct sockaddr *) &server, sizeof(server)) == -1)
    {
        std::cout << "[VSE] ERROR: Connect client" << std::endl;
    }
    
    //===============================================================
    //Setup my audio synth engine
    
//    std::cout << "[VSE] Setting up audio engine stuff..." << std::endl;
//	setup();
	
    //===============================================================
    //Setup PortAudio or RtAudio, depending in which one is defined/chosen,
    //and start the audio stream processing
    
#ifdef MAXIMILIAN_PORTAUDIO
	PaStream *stream;
	PaError err;
	err = Pa_Initialize();
	if( err != paNoError )
		std::cout <<   "PortAudio error: " << Pa_GetErrorText( err ) << std::endl;
	
	double data[maxiSettings::channels];
	
	err = Pa_OpenDefaultStream( &stream,
							   0,          /* no input channels */
							   maxiSettings::channels,          /* stereo output */
							   paFloat64,  /* 64 bit floating point output */
							   maxiSettings::sampleRate,
							   paFramesPerBufferUnspecified /*maxiSettings::bufferSize*/,        /* frames per buffer, i.e. the number
												   of sample frames that PortAudio will
												   request from the callback. Many apps
												   may want to use
												   paFramesPerBufferUnspecified, which
												   tells PortAudio to pick the best,
												   possibly changing, buffer size.*/
							   &routing, /* this is your callback function */
							   &data ); /*This is a pointer that will be passed to
										 your callback*/
	
	//PaAlsa_EnableRealtimeScheduling(stream,true);
	
	err = Pa_StartStream( stream );
	if( err != paNoError )
		std::cout <<   "PortAudio error: " << Pa_GetErrorText( err ) << std::endl;
	
	std::cout << "[VSE] Maximilian sound engine has started (using PortAudio)" << std::endl;
	
#elif defined(MAXIMILIAN_RT_AUDIO)
	RtAudio dac(RtAudio::WINDOWS_DS);
	if ( dac.getDeviceCount() < 1 ) {
		std::cout << "\nNo audio devices found!\n";
		char input;
		std::cin.get( input );
		exit( 0 );
	}
	
    std::cout << "[VSE] Setting up RtAudio..." << std::endl;
    
	RtAudio::StreamParameters parameters;
	parameters.deviceId = dac.getDefaultOutputDevice();
	parameters.nChannels = maxiSettings::channels;
	parameters.firstChannel = 0;
	unsigned int sampleRate = maxiSettings::sampleRate;
	unsigned int bufferFrames = maxiSettings::bufferSize; 
	//double data[maxiSettings::channels];
	vector<double> data(maxiSettings::channels,0);
	
	try
    {
        std::cout << "[VSE] Starting RtAudio stream..." << std::endl;
        
		dac.openStream( &parameters, NULL, RTAUDIO_FLOAT64,
					   sampleRate, &bufferFrames, &routing, (void *)&(data[0]));
		
		dac.startStream();
	}
	catch ( RtError& e )
    {
		e.printMessage();
		exit( 0 );
	}
    
    std::cout << "[VSE] Maximilian sound engine has started (using RTAudio)" << std::endl;
    
    //===============================================================
    //Send a command CC to vintageBrain to request current panel parameter settings
    
    uint8_t ready_cc_buf[3] = {MIDI_CC, PARAM_CMD, CMD_REQUEST_PANEL_PARAM_DATA};
    sendToVintageBrainSocket (sock, ready_cc_buf, 3);
    
    #endif
	
    //===============================================================
    //Start main loop (listening for messages from vintageBrain socket)
    
    std::cout << "[VSE] Starting main loop..." << std::endl;
    
    uint8_t input_message_flag = 0;
    uint8_t input_message_buffer[32] = {0};
    uint8_t input_message_counter = 0;
    
    while (true)
    {
        //usleep (1000000);
        //std::cout << "[VSE] Waiting for messages from vintageBrain...\n";
        
        //Attempt to read data from socket, blocking on read
        ret = read (sock, socket_input_buf, sizeof(socket_input_buf));
        
        //if we have read something
        if (ret > 0)
        {
            //display the read data...
//            std::cout << "[VSE] Data read from socket: ";
//            for (int i = 0; i < ret; i++)
//                std::cout << "[" << (int)socket_input_buf[i] << "] ";
//            std::cout << std::endl;
            
            //for each read byte
            for (int i = 0; i < ret; i++)
            {
                //process the read byte
                input_message_flag = ProcInputByte (socket_input_buf[i], input_message_buffer, &input_message_counter);
                
                //if we have received a full MIDI message
                if (input_message_flag)
                {
                    //std::cout << "[VSE] input_message_flag: " << (int)input_message_flag << std::endl;
                   
//                    std::cout << "[VSE] Processed MIDI message: ";
//                    for (int i = 0; i < ret; i++)
//                        std::cout << "[" << (int)input_message_buffer[i] << "] ";
//                    std::cout << std::endl;
                    
                    //================================
                    //Process note-on messages
                    if (input_message_flag == MIDI_NOTEON)
                    {
                        //channel relates to voice number
                        uint8_t voice_num = input_message_buffer[0] & MIDI_CHANNEL_BITS;
                        
                        vintageVoice[voice_num]->processNoteMessage (1, input_message_buffer[1], input_message_buffer[2]);

                    } //if (input_message_flag == MIDI_NOTEON)
                    
                    //================================
                    //Process note-off messages
                    else if (input_message_flag == MIDI_NOTEOFF)
                    {
                        //channel relates to voice number
                        uint8_t voice_num = input_message_buffer[0] & MIDI_CHANNEL_BITS;
                        
                        vintageVoice[voice_num]->processNoteMessage (0, 0, 0);
                        
                    } //if (input_message_flag == MIDI_NOTEOFF)
                    
                    //================================
                    //Process CC/param messages
                    else if (input_message_flag == MIDI_CC)
                    {
                        //if a patch parameter
                        if (input_message_buffer[1] != PARAM_CMD)
                        {
                            //channel relates to voice number. Channel 15 means send to all voices
                            uint8_t voice_num = input_message_buffer[0] & MIDI_CHANNEL_BITS;
                            
                            for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
                            {
                                //if we want to send this message to voice number 'voice'
                                if (voice_num == 15 || voice_num == voice)
                                {
                                    //TODO: check if this param/CC num is a sound param, and in range.
                                    //At this point it always should be, but it may be best to check anyway.
                                    
                                    //set the paramaters voice value
                                    vintageVoice[voice]->setPatchParamVoiceValue (input_message_buffer[1], input_message_buffer[2]);
                                    
                                } //if (voice_num == 15 || voice_num == voice)
                                
                            } //for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
                            
                        } //if (input_message_buffer[1] != PARAM_CMD)
                        
                        //if a command message
                        else
                        {
                            //if 'kill all voices' command
                            if (input_message_buffer[2] == CMD_KILL_ALL_VOICES)
                            {
                                for (uint8_t voice = 0; voice < NUM_OF_VOICES; voice++)
                                {
                                    vintageVoice[voice]->processNoteMessage (0, 0, 0);
                                }
                            }
                            
                        } //if (input_message_buffer[1] == PARAM_CMD)
                        
                    } //if (input_message_flag == MIDI_CC)
                    
                    //================================
                    //Process poly aftertouch messages

                    else if (input_message_flag == MIDI_PAT)
                    {
                        //channel relates to voice number
                        uint8_t voice_num = input_message_buffer[0] & MIDI_CHANNEL_BITS;
                        
                        vintageVoice[voice_num]->processAftertouchMessage (input_message_buffer[2]);
                        
                    } //if (input_message_flag == MIDI_NOTEOFF)
                    
                    
                } //if (input_message_flag)
                
            } //for (int i = 0; i < ret; i++)
            
        } //if (ret > 0)
        
    } //while (true)
	
    //===============================================================
    //Stop PortAudio/RtAudio
    
    #ifdef MAXIMILIAN_PORTAUDIO
    
    err = Pa_Terminate();
    if( err != paNoError )
        std::cout <<  "PortAudio error: "<< Pa_GetErrorText( err ) << std::endl;
    
    #elif defined(MAXIMILIAN_RT_AUDIO)
    
	if ( dac.isStreamOpen() ) dac.closeStream();
    
    #endif

	
	return 0;
}
