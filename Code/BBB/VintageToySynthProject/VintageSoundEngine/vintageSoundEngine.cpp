/*
 *  player.cpp
 *  rtaudiotest
 *
 *  Created by Chris on 23/08/2011.
 *  Copyright 2011 Goldsmiths Creative Computing. All rights reserved.
 *
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
#include "../globals.h"
#include "vintageVoice.h"

VintageVoice *vintageVoice[NUM_OF_VOICES];


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
    
    void play(double *output)
    {
        //for now just test processing 1 voice, as the test code in the voice object is essentially for two voices
        vintageVoice[0]->processAudio (output);
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
    
    std::cout << "[VSE] Setting up audio engine stuff..." << std::endl;
	setup();
	
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
							   paInt32/*paFloat32*/,  /* 64 bit floating point output */
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
    
    #endif
	
    //===============================================================
    //Start main loop (listening for messages from vintageBrain socket)
    
    std::cout << "[VSE] Starting main loop..." << std::endl;
    
    while (true)
    {
        //usleep (1000000);
        //std::cout << "[VSE] Waiting for messages from vintageBrain...\n";
        
        //Attempt to read data from socket, blocking on read
        ret = read (sock, socket_input_buf, sizeof(socket_input_buf));
        
        if (ret > 0)
        {
            //display the read data...
            std::cout << "[VSE] Data read from socket: ";
            for (int i = 0; i < ret; i++)
                std::cout << "[" << (int)socket_input_buf[i] << "] ";
            std::cout << std::endl;
            
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
