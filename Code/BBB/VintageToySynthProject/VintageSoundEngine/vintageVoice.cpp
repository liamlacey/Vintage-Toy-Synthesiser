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
    oscSubPitch = 100;
    
    //init objects
    envAmp.setAttack (patchParameterData[PARAM_AEG_ATTACK].voice_val);
    envAmp.setDecay (patchParameterData[PARAM_AEG_DECAY].voice_val);
    envAmp.setSustain (patchParameterData[PARAM_AEG_SUSTAIN].voice_val);
    envAmp.setRelease (patchParameterData[PARAM_AEG_RELEASE].voice_val);
    
    envFilter.setAttack (patchParameterData[PARAM_FEG_ATTACK].voice_val);
    envFilter.setDecay (patchParameterData[PARAM_FEG_DECAY].voice_val);
    envFilter.setSustain (patchParameterData[PARAM_FEG_SUSTAIN].voice_val);
    envFilter.setRelease (patchParameterData[PARAM_FEG_RELEASE].voice_val);
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
    
    //process envelopes
    envAmpOut = envAmp.adsr (patchParameterData[PARAM_AEG_AMOUNT].voice_val, envAmp.trigger);
    envFilterOut = envFilter.adsr (1.0, envFilter.trigger);
    
    //process oscillators
    //FIXME: should I be setting each osc output to it's own variable, and then mixing them at the end? I don't think that'll make a difference.
    oscOut = oscSine.sinewave (oscPitch) * patchParameterData[PARAM_OSC_SINE_LEVEL].voice_val;
    oscOut += (oscTri.triangle (oscPitch) * patchParameterData[PARAM_OSC_TRI_LEVEL].voice_val);
    oscOut += (oscSaw.saw (oscPitch) * patchParameterData[PARAM_OSC_SAW_LEVEL].voice_val);
    oscOut += (oscPulse.pulse (oscPitch, patchParameterData[PARAM_OSC_PULSE_AMOUNT].voice_val) * patchParameterData[PARAM_OSC_PULSE_LEVEL].voice_val);
    oscOut += (oscSquare.square (oscSubPitch) * patchParameterData[PARAM_OSC_SQUARE_LEVEL].voice_val);
    oscOut /= 4.;
    
    //process filter (pass in oscOut, return filterOut)
    filterSvf.setCutoff (patchParameterData[PARAM_FILTER_FREQ].voice_val * envFilterOut);
    filterSvf.setResonance (patchParameterData[PARAM_FILTER_RESO].voice_val);
    filterOut = filterSvf.play (oscOut,
                                patchParameterData[PARAM_FILTER_LP_MIX].voice_val,
                                patchParameterData[PARAM_FILTER_BP_MIX].voice_val,
                                patchParameterData[PARAM_FILTER_HP_MIX].voice_val,
                                patchParameterData[PARAM_FILTER_NOTCH_MIX].voice_val);
    
    //apply amp envelope, making both the L and R channels the same (pass in filterOut, return output)
    output[0] = filterOut * envAmpOut;
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
    oscSubPitch = mtof.mtof(midi_note_num - 12);
}


//==========================================================
//==========================================================
//==========================================================
//Triggers the envelopes of the voice to either start or stop,
//essentially starting or stopping a note.

void VintageVoice::triggerEnvelopes (uint8_t trigger_val)
{
    envAmp.trigger = trigger_val;
    envFilter.trigger = trigger_val;
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
    patchParameterData[param_num].user_val = param_user_val;
    patchParameterData[param_num].voice_val = scaleValue (patchParameterData[param_num].user_val,
                                                          patchParameterData[param_num].user_min_val,
                                                          patchParameterData[param_num].user_max_val,
                                                          patchParameterData[param_num].voice_min_val,
                                                          patchParameterData[param_num].voice_max_val);
}
