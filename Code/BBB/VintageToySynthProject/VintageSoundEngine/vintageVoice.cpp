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

VintageVoice::VintageVoice (uint8_t voice_num)
{
    std::cout << "[VV] Initing vintageVoice num " << (int)voice_num << std::endl;
    
    //init variables
    
    for (uint8_t i = 0; i < 128; i++)
    {
        patchParameterData[i] = defaultPatchParameterData[i];
    }
    
    oscPitch = 200;
    
    //init objects
    envAmp.setAttack(0);
    envAmp.setDecay(200);
    envAmp.setSustain(0.2);
    envAmp.setRelease(2000);
}

VintageVoice::~VintageVoice()
{
    
}

//==========================================================
//==========================================================
//==========================================================
//This is called by the audio processing callback function,
//and is where all audio processing is done

void VintageVoice::processAudio (double *output)
{
    //FIXME: is there a way of only processing this function if the ampEnv is currently running?
    
    //process amplitude envelope
    envAmpOut = envAmp.adsr (patchParameterData[PARAM_AEG_AMOUNT].voice_val, envAmp.trigger);
    
    //process oscillators
    oscOut = oscSquare.square (oscPitch);
    
    //set final audio output, making both the L and R channels the same
    output[0] = oscOut  * envAmpOut;
    output[1] = output[0];
}

//==========================================================
//==========================================================
//==========================================================
//Sets the oscillator pitch based in the incoming MIDI note number

void VintageVoice::setOscPitch (uint8_t midi_note_num)
{
    convert mtof;
    oscPitch = mtof.mtof(midi_note_num);
}

//==========================================================
//==========================================================
//==========================================================
//Triggers the amplitude envelope of the voice to either start or stop,
//essentially starting or stopping a note.

void VintageVoice::triggerAmpEnvelope (uint8_t trigger_val)
{
    envAmp.trigger = trigger_val;
}

//==========================================================
//==========================================================
//==========================================================
//Sets the amp envelope amount.
//FIXME: amp envelope amount will eventually also be set with the amount control, not just velocity.
//also we will have velocity->amp depth which will affect this value

void VintageVoice::setNoteVelocity (uint8_t vel_val)
{
    setPatchParamVoiceValue (PARAM_AEG_AMOUNT, vel_val);
}

//==========================================================
//==========================================================
//==========================================================
//Sets a parameters voice value based on the parameters current user value

void VintageVoice::setPatchParamVoiceValue (uint8_t param_num, uint8_t param_user_val)
{
    patchParameterData[param_num].voice_val = scaleValue (param_user_val,
                                                          patchParameterData[param_num].user_min_val,
                                                          patchParameterData[param_num].user_max_val,
                                                          patchParameterData[param_num].voice_min_val,
                                                          patchParameterData[param_num].voice_max_val);
}
