/*
 vintageVoice.cpp
 
 Created by Liam Lacey on 29/01/2016.
 Copyright 2016 Liam Lacey. All rights reserved.
 
 This is a file for the vintageSoundEngine application for the Vintage Toy Piano Synth project.
 This application is the audio synthesis engine for the synthesiser, using the Maximilian synthesis library.
 
 This particular file/class handles the audio processing for each voice.
 */

//FIXME: do I need to do any thread locking when setting variables that are accessed in processAudio (called by the audio callback function)?

//TODO: when implementing modulation, try and do it in such a way so that, if possible, it would be easy to add further modulation destinations.

#include "vintageVoice.h"

//==========================================================
//==========================================================
//==========================================================

VintageVoice::VintageVoice (uint8_t voice_num)
{
    voiceNum = voice_num;
    
    std::cout << "[VV] Initing vintageVoice num " << (int)voiceNum << std::endl;
    
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
    
    //TODO: implement vintage mode amount.
    //This will involve adding things like:
    // - random frequency and amounts of osc detuning when a note is pressed
    // - random frequency and amounts of filter cutoff offset when a note is pressed
    // - random frequency and amounts of noise added when a note is pressed
    // - detuning of the 5 oscillators on each voice
    // - changing the phase of the oscs so they're different
    //possible the above three but added periodically throughout a note, not just when pressed.
    
    //process LFO...
    //FIXME: for LFO rate it would be better if we used an LFO rate table (an array of 128 different rates).
    //FIXME: Does the LFO osc always need to be halved and offset like being doing with lfo->amp mod? If so do that here.
    if (patchParameterData[PARAM_LFO_SHAPE].voice_val == 0)
        lfoOut = lfo.sinewave (patchParameterData[PARAM_LFO_RATE].voice_val) * patchParameterData[PARAM_LFO_DEPTH].voice_val;
    else if (patchParameterData[PARAM_LFO_SHAPE].voice_val == 1)
        lfoOut = lfo.triangle (patchParameterData[PARAM_LFO_RATE].voice_val) * patchParameterData[PARAM_LFO_DEPTH].voice_val;
    else if (patchParameterData[PARAM_LFO_SHAPE].voice_val == 2)
        lfoOut = lfo.saw (patchParameterData[PARAM_LFO_RATE].voice_val) * patchParameterData[PARAM_LFO_DEPTH].voice_val;
    else if (patchParameterData[PARAM_LFO_SHAPE].voice_val == 3)
        lfoOut = lfo.square (patchParameterData[PARAM_LFO_RATE].voice_val) * patchParameterData[PARAM_LFO_DEPTH].voice_val;
    
    //process amp envelope with note velocity
    envAmpOut = envAmp.adsr (patchParameterData[PARAM_AEG_AMOUNT].voice_val * voiceVelocityValue, envAmp.trigger);
    
    //process filter envelope
    envFilterOut = envFilter.adsr (1.0, envFilter.trigger);
    
    //process lfo->amp env depth modulation
    //Don't forget that the LFO wave has to be halved and offset by 0.5 to act as an amplitude modulator.
    double offset_lfo_val = ((lfoOut * 0.5) + 0.5);
    //FIXME: if patchParameterData[PARAM_MOD_LFO_AMP].voice_val is negative it will double the signal which not what we want.
    //What do we want a negative depth value to do, and how should it do it?
    envAmpOut = envAmpOut * (1.0 - (offset_lfo_val * patchParameterData[PARAM_MOD_LFO_AMP].voice_val));
    
    //TODO: implement all lfo modulation
    
    //TODO: implement all aftertouch modulation
    
    //process oscillators
    //FIXME: do we want the oscillators to have the same phase? If not this should be set in the contructor
    oscSineOut = oscSine.sinewave (oscPitch) * patchParameterData[PARAM_OSC_SINE_LEVEL].voice_val;
    oscTriOut = (oscTri.triangle (oscPitch) * patchParameterData[PARAM_OSC_TRI_LEVEL].voice_val);
    oscSawOut = (oscSaw.saw (oscPitch) * patchParameterData[PARAM_OSC_SAW_LEVEL].voice_val);
    oscPulseOut = (oscPulse.pulse (oscPitch, patchParameterData[PARAM_OSC_PULSE_AMOUNT].voice_val) * patchParameterData[PARAM_OSC_PULSE_LEVEL].voice_val);
    oscSquareOut = (oscSquare.square (oscSubPitch) * patchParameterData[PARAM_OSC_SQUARE_LEVEL].voice_val);
    
    //mix oscillators toehether
    oscMixOut = (oscSineOut + oscTriOut + oscSawOut + oscPulseOut + oscSquareOut) / 5.;
    
    //process filter (pass in oscOut, return filterOut)
    //TODO: implement cutoff modulation (LFO and PAT) and reso modulation (LFO)
    filterSvf.setCutoff (patchParameterData[PARAM_FILTER_FREQ].voice_val * envFilterOut);
    filterSvf.setResonance (patchParameterData[PARAM_FILTER_RESO].voice_val);
    filterOut = filterSvf.play (oscMixOut,
                                patchParameterData[PARAM_FILTER_LP_MIX].voice_val,
                                patchParameterData[PARAM_FILTER_BP_MIX].voice_val,
                                patchParameterData[PARAM_FILTER_HP_MIX].voice_val,
                                patchParameterData[PARAM_FILTER_NOTCH_MIX].voice_val);
    
    
    //process distortion...
    //FIXME: should PARAM_FX_DISTORTION_AMOUNT also change the shape of the distortion?
    distortionOut = distortion.atanDist (filterOut, 200.0);
    
    //process distortion mix
    //FIXME: probably need to reduce the disortionOut value so bringing in disortion doesn't increase the overall volume too much
    effectsMixOut = (distortionOut * patchParameterData[PARAM_FX_DISTORTION_AMOUNT].voice_val) + (filterOut * (1.0 - patchParameterData[PARAM_FX_DISTORTION_AMOUNT].voice_val));
    
    //apply amp envelope, making both the L and R channels the same (pass in filterOut, return output)
    output[0] = effectsMixOut * envAmpOut;
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
//FIXME: change this function to a more appropriate name, such as processNoteMessage.
//FIXME: why aren't the processing of note pitch and velocity done here either? Everything could be done in one function. Though make sure if it is called due a a note-off message (trigger val set to 0) the pitch and velocity aren't changed

void VintageVoice::triggerEnvelopes (uint8_t trigger_val)
{
    envAmp.trigger = trigger_val;
    envFilter.trigger = trigger_val;
    
    //TODO: reset phase of LFO using the function phaseReset().
}

//==========================================================
//==========================================================
//==========================================================
//Sets the amp envelope amount.

void VintageVoice::setNoteVelocity (uint8_t vel_val)
{
    //FIXME: amp envelope amount will eventually also be set with multiple params, not just velocity.
    //These params will be AEG amount control, vel to amp mod depth, and LFO to amp mod delth
    //Therefore I think we need to start using a new varialbe here which is set based on
    //the velocity value, the mod depths, and the AEG amount value. Then use
    //this new variable in play() instead of the patchParam value. This variable
    //will need to be set whenever each of these four states change.
    //Though I think because LFO changes in realtime within play(), this may need to be processed in play(),
    //which would mean we would need a global vel val, and other vals, that are used in play().
    
    //convert user velocity value into a voice velocity value
    voiceVelocityValue = scaleValue (vel_val,
                                     0,
                                     127,
                                     0.,
                                     1.);
    
    //TODO: change the velocity->amp depth (voiceVelocityValue) using the PARAM_MOD_VEL_AMP param value, so that it equals 1 with no depth.
    //HOW DO I DO THIS?
    //FIXME: if we start modulating other parameters with velocity we will either needed individual vel variables
    //for each mod destination, or just do all the depth processesing within play
    
    //setPatchParamVoiceValue (PARAM_AEG_AMOUNT, vel_val);
}

//==========================================================
//==========================================================
//==========================================================
//Sets a parameters voice value based on the parameters current user value

void VintageVoice::setPatchParamVoiceValue (uint8_t param_num, uint8_t param_user_val)
{
    patchParameterData[param_num].user_val = param_user_val;
    //FIXME: this could probably be done within vintageSoundEngine.cpp instead of within the voice object,
    //as each voice will probably be given the same value most of the time, so it would save CPU
    //to only have to do this once instead of for each voice.
    patchParameterData[param_num].voice_val = scaleValue (patchParameterData[param_num].user_val,
                                                          patchParameterData[param_num].user_min_val,
                                                          patchParameterData[param_num].user_max_val,
                                                          patchParameterData[param_num].voice_min_val,
                                                          patchParameterData[param_num].voice_max_val);
    
    //==========================================================
    //Set certain things based on the recieved param num
    
    if (param_num == PARAM_AEG_ATTACK)
    {
        envAmp.setAttack (patchParameterData[param_num].voice_val);
    }
    
    else if (param_num == PARAM_AEG_DECAY)
    {
        envAmp.setDecay (patchParameterData[param_num].voice_val);
    }
    
    else if (param_num == PARAM_AEG_SUSTAIN)
    {
        envAmp.setSustain (patchParameterData[param_num].voice_val);
    }
    
    else if (param_num == PARAM_AEG_RELEASE)
    {
        envAmp.setRelease (patchParameterData[param_num].voice_val);
    }
    
    else if (param_num == PARAM_FEG_ATTACK)
    {
        envFilter.setAttack (patchParameterData[param_num].voice_val);
    }
    
    else if (param_num == PARAM_FEG_DECAY)
    {
        envFilter.setDecay (patchParameterData[param_num].voice_val);
    }
    
    else if (param_num == PARAM_FEG_SUSTAIN)
    {
        envFilter.setSustain (patchParameterData[param_num].voice_val);
    }
    
    else if (param_num == PARAM_FEG_RELEASE)
    {
        envFilter.setRelease (patchParameterData[param_num].voice_val);
    }
}
