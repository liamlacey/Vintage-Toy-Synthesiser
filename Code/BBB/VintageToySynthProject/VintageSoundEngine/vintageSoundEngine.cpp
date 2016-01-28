/*
 *  player.cpp
 *  rtaudiotest
 *
 *  Created by Chris on 23/08/2011.
 *  Copyright 2011 Goldsmiths Creative Computing. All rights reserved.
 *
 */

#include "vintageSoundEngine.h"
#include "Maximilian/maximilian.h"
#include <iostream>


#ifdef MAXIMILIAN_PORTAUDIO
#include "Maximilian/portaudio.h"
//#include "pa_mac_core.h"
#elif defined(MAXIMILIAN_RT_AUDIO)
	#if defined( __WIN32__ ) || defined( _WIN32 )
		#include <dsound.h>
	#endif
#include "Maximilian/RtAudio.h"
#endif

//use this to do any initialisation if you want.
void setup();

//run dac! Very very often. Too often in fact. er...
void play(double *output);

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


    
    
    
    
    //This shows how to use maximilian to build a polyphonic synth.
    
#define POLYPHONY 4
#define POLYPHONY_2 4
    
    //These are the synthesiser bits
    maxiOsc VCO1[POLYPHONY],VCO2[POLYPHONY],LFO1[POLYPHONY]/*,LFO2[POLYPHONY]*/;
    //maxiFilter VCF[POLYPHONY];
    maxiSVF svfVcf[POLYPHONY];
    maxiEnv ADSR[POLYPHONY];
    
    //This is a bunch of control signals so that we can hear something
    
    maxiOsc timer;//this is the metronome
    int currentCount,lastCount,voice=0;//these values are used to check if we have a new beat this sample
    
    //and these are some variables we can use to pass stuff around
    
    double VCO1out[POLYPHONY],VCO2out[POLYPHONY],LFO1out[POLYPHONY],/*LFO2out[POLYPHONY],*/VCFout[POLYPHONY],ADSRout[POLYPHONY],mix,pitch[POLYPHONY];
    
    
    void setup() {//some inits
        
        for (int i=0;i<POLYPHONY;i++)
        {
            
            ADSR[i].setAttack(0);
            ADSR[i].setDecay(200);
            ADSR[i].setSustain(0.2);
            ADSR[i].setRelease(2000);
            
        }
    }
    
    void play(double *output)
    {
        mix=0;//we're adding up the samples each update and it makes sense to clear them each time first.
        
        //so this first bit is just a basic metronome so we can hear what we're doing.
        
        currentCount=(int)timer.phasor(8);//this sets up a metronome that ticks 8 times a second
        
        if (lastCount!=currentCount) {//if we have a new timer int this sample, play the sound
            
            if (voice==POLYPHONY_2)
            {
                voice=0;
            }
            
            ADSR[voice].trigger=1;//trigger the envelope from the start
            pitch[voice]=voice+1;
            voice++;
            
        }
        
        //and this is where we build the synth
        
        for (int i=0; i<POLYPHONY_2; i++)
        {
            ADSRout[i]=ADSR[i].adsr(1.,ADSR[i].trigger);//our ADSR env is passed a constant signal of 1 to generate the transient.
            
            LFO1out[i]=LFO1[i].sinebuf(0.2);//this lfo is a sinewave at 0.2 hz
            
            VCO1out[i]=VCO1[i].pulse(55*pitch[i],0.6);//here's VCO1. it's a pulse wave at 55 hz, with a pulse width of 0.POLYPHONY
            VCO2out[i]=VCO2[i].pulse((110*pitch[i])+LFO1out[i],0.2);//here's VCO2. it's a pulse wave at 110hz with LFO modulation on the frequency, and width of 0.2
            
            //now we stick the VCO's into the VCF, using the ADSR as the filter cutoff
            //VCFout[i]=VCF[i].lores((VCO1out[i]+VCO2out[i])*0.5, 250+((pitch[i]+LFO1out[i])*1000), 10);
            //VCFout[i]=VCF[i].lopass((VCO1out[i]+VCO2out[i])*0.5, 1000);
            //VCFout[i]=(VCO1out[i]+VCO2out[i]);
            
            svfVcf[i].setCutoff(250+((pitch[i]+LFO1out[i])*1000));
            svfVcf[i].setResonance(4);
            VCFout[i] = svfVcf[i].play ((VCO1out[i]+VCO2out[i])*0.5, 1.0, 0.0, 1.0, 0.0);
            
            
            mix+=VCFout[i]*ADSRout[i]/POLYPHONY_2;//finally we add the ADSR as an amplitude modulator
            
        }
        
        output[0]=mix*0.5;//left channel
        output[1]=mix*0.5;//right channel
        
        
        // This just sends note-off messages.
        for (int i=0; i<POLYPHONY_2; i++) {
            ADSR[i].trigger=0;
        }
        
    }


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    //This is main()
int main()
{
    std::cout << "[VSE] Setting up audio engine stuff..." << std::endl;
	setup();
	
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
	
	
	char input;
	std::cout << "\nMaximilian is playing (using PortAudio) ... press <enter> to quit.\n";
	std::cin.get( input );
	
	
	
	err = Pa_Terminate();
	if( err != paNoError )
		std::cout <<  "PortAudio error: "<< Pa_GetErrorText( err ) << std::endl;
	
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
	
    std::cout << "[VSE] Starting main loop..." << std::endl;
    
    while (true)
    {
        usleep (1000000);
        std::cout << "[VSE] Waiting for messages from vintageBrain...\n";
    };
	
	if ( dac.isStreamOpen() ) dac.closeStream();
#endif
	
	return 0;
}
