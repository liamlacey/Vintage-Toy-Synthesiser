/*
 vintageVoice.cpp
 
 Created by Liam Lacey on 29/01/2016.
 Copyright 2016 Liam Lacey. All rights reserved.
 
 This is a file for the vintageSoundEngine application for the Vintage Toy Piano Synth project.
 This application is the audio synthesis engine for the synthesiser, using the Maximilian synthesis library.
 
 This particular file/class handles the audio processing for each voice.
 */

//FIXME: do I need to do any thread locking when setting variables that are accessed in processAudio (called by the audio callback function)?

#include "vintageVoice.h"

//==========================================================
//==========================================================
//==========================================================

VintageVoice::VintageVoice (int voice_num)
{
    std::cout << "[VV] Initing vintageVoice num " << voice_num << std::endl;
    
    oscPitch = 200;
    
//    for (int i = 0; i < NUM_OF_VOICES; i++)
//    {
//        ADSR[i].setAttack(0);
//        ADSR[i].setDecay(200);
//        ADSR[i].setSustain(0.2);
//        ADSR[i].setRelease(2000);
//    }
//    
//    voice = 0;
}

VintageVoice::~VintageVoice()
{
    
}

//==========================================================
//==========================================================
//==========================================================

void VintageVoice::processAudio (double *output)
{
    output[0] = oscSquare.square (oscPitch);
    output[1] = output[0];
    
//    mix = 0;//we're adding up the samples each update and it makes sense to clear them each time first.
//    
//    //so this first bit is just a basic metronome so we can hear what we're doing.
//    
//    currentCount = (int)timer.phasor(8);//this sets up a metronome that ticks 8 times a second
//    
//    if (lastCount != currentCount)
//    {//if we have a new timer int this sample, play the sound
//        
//        if (voice == NUM_OF_VOICES)
//        {
//            voice=0;
//        }
//        
//        ADSR[voice].trigger = 1;//trigger the envelope from the start
//        pitch[voice] = voice + 1;
//        voice++;
//        
//    }
//    
//    //and this is where we build the synth
//    
//    for (int i = 0; i < NUM_OF_VOICES; i++)
//    {
//        ADSRout[i]=ADSR[i].adsr(1.,ADSR[i].trigger);//our ADSR env is passed a constant signal of 1 to generate the transient.
//        
//        LFO1out[i]=LFO1[i].sinebuf(0.2);//this lfo is a sinewave at 0.2 hz
//        
//        VCO1out[i]=VCO1[i].pulse(55*pitch[i],0.6);//here's VCO1. it's a pulse wave at 55 hz, with a pulse width of 0.POLYPHONY
//        VCO2out[i]=VCO2[i].pulse((110*pitch[i])+LFO1out[i],0.2);//here's VCO2. it's a pulse wave at 110hz with LFO modulation on the frequency, and width of 0.2
//        
//        //now we stick the VCO's into the VCF, using the ADSR as the filter cutoff
//        //VCFout[i]=VCF[i].lores((VCO1out[i]+VCO2out[i])*0.5, 250+((pitch[i]+LFO1out[i])*1000), 10);
//        //VCFout[i]=VCF[i].lopass((VCO1out[i]+VCO2out[i])*0.5, 1000);
//        //VCFout[i]=(VCO1out[i]+VCO2out[i]);
//        
//        svfVcf[i].setCutoff(250+((pitch[i]+LFO1out[i])*1000));
//        svfVcf[i].setResonance(4);
//        VCFout[i] = svfVcf[i].play ((VCO1out[i]+VCO2out[i])*0.5, 1.0, 0.0, 1.0, 0.0);
//        
//        
//        mix+=VCFout[i]*ADSRout[i] / NUM_OF_VOICES;//finally we add the ADSR as an amplitude modulator
//        
//    }
//    
//    output[0] = mix * 0.5;//left channel
//    output[1] = mix * 0.5;//right channel
//    
//    
//    // This just sends note-off messages.
//    for (int i = 0; i < NUM_OF_VOICES; i++)
//    {
//        ADSR[i].trigger = 0;
//    }
}

//==========================================================
//==========================================================
//==========================================================
//Sets the oscillator pitch based in the incoming MIDI note number

void VintageVoice::setNotePitch (uint8_t midi_note_num)
{
    convert mtof;
    oscPitch = mtof.mtof(midi_note_num);
}
