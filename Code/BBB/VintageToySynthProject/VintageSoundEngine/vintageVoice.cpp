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
    voiceNum = voice_num;
    
    std::cout << "[VV] Initing vintageVoice num " << (int)voiceNum << std::endl;
    
    //init variables
    
    for (uint8_t i = 0; i < 128; i++)
    {
        patchParameterData[i] = defaultPatchParameterData[i];
    }
    
    oscSinePitch = oscTriPitch = oscSawPitch = oscPulsePitch = oscSquarePitch = 200;
    
    voiceVelocityValue = 1.0;
    rootNoteNum = 60;
    aftertouchValue = 0;
    
    velAmpModVal = velFreqModVal = velResoModVal = 0;
    
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
    
    //FIXME: make sure only stuff that NEEDS to be done in here is done in here.
    //If possibly do stuff on note and param input events instead
    
    //==========================================================
    //process LFO...
    
    //set shape and rate
        //FIXME: for LFO rate it would be better if we used an LFO rate table (an array of 128 different rates).
    if (patchParameterData[PARAM_LFO_SHAPE].voice_val == 0)
        lfoOut = lfo.sinewave (patchParameterData[PARAM_LFO_RATE].voice_val);
    else if (patchParameterData[PARAM_LFO_SHAPE].voice_val == 1)
        lfoOut = lfo.triangle (patchParameterData[PARAM_LFO_RATE].voice_val);
    else if (patchParameterData[PARAM_LFO_SHAPE].voice_val == 2)
        lfoOut = lfo.saw (patchParameterData[PARAM_LFO_RATE].voice_val);
    else if (patchParameterData[PARAM_LFO_SHAPE].voice_val == 3)
        lfoOut = lfo.square (patchParameterData[PARAM_LFO_RATE].voice_val);
    
    //convert the osc wave into an lfo wave (multiply and offset)
    lfoOut = ((lfoOut * 0.5) + 0.5);
    
    //set depth
    lfoOut = lfoOut * patchParameterData[PARAM_LFO_DEPTH].voice_val;
    
    //==========================================================
    //Amp envelope stuff...
    
    //process LFO->amp env modulation
    double amp_lfo_mod_val = getModulatedParamValue (PARAM_MOD_LFO_AMP, PARAM_AEG_AMOUNT, lfoOut);
    
    //Add the amp modulation values to the patch value, making sure the produced value is in range
    double amp_val = patchParameterData[PARAM_AEG_AMOUNT].voice_val + amp_lfo_mod_val + velAmpModVal;
    amp_val = boundValue (amp_val, patchParameterData[PARAM_AEG_AMOUNT].voice_min_val, patchParameterData[PARAM_AEG_AMOUNT].voice_max_val);
    
    //generate the amp evelope output using amp_val as the envelope amount
    envAmpOut = envAmp.adsr (amp_val, envAmp.trigger);
    
    //if the output of the amp envelope is above 0, continue processing audio.
    //else don't, as it is just silence.
    //FIXME: will this work? What if LFO is creating tremolo that causes this to be true even when the note isn't over - will this make weird things happen?
    
    if (envAmpOut > 0)
    {
        //==========================================================
        //process filter envelope
        envFilterOut = envFilter.adsr (1.0, envFilter.trigger);
        
        //==========================================================
        //process oscillators
        oscSineOut = oscSine.sinewave (oscSinePitch) * patchParameterData[PARAM_OSC_SINE_LEVEL].voice_val;
        oscTriOut = (oscTri.triangle (oscTriPitch) * patchParameterData[PARAM_OSC_TRI_LEVEL].voice_val);
        oscSawOut = (oscSaw.saw (oscSawPitch) * patchParameterData[PARAM_OSC_SAW_LEVEL].voice_val);
        oscPulseOut = (oscPulse.pulse (oscPulsePitch, patchParameterData[PARAM_OSC_PULSE_AMOUNT].voice_val) * patchParameterData[PARAM_OSC_PULSE_LEVEL].voice_val);
        oscSquareOut = (oscSquare.square (oscSquarePitch) * patchParameterData[PARAM_OSC_SQUARE_LEVEL].voice_val);
        
        //mix oscillators together
        oscMixOut = (oscSineOut + oscTriOut + oscSawOut + oscPulseOut + oscSquareOut) / 5.;
        
        //==========================================================
        //process filter (pass in oscOut, return filterOut)
        
        //================================
        //process LFO->cutoff modulation
        double cutoff_lfo_mod_val = getModulatedParamValue (PARAM_MOD_LFO_FREQ, PARAM_FILTER_FREQ, lfoOut);
        
        //Add the cutoff modulation values to the patch value, making sure the produced value is in range
        double cutoff_val = patchParameterData[PARAM_FILTER_FREQ].voice_val + cutoff_lfo_mod_val + velFreqModVal;
        cutoff_val = boundValue (cutoff_val, patchParameterData[PARAM_FILTER_FREQ].voice_min_val, patchParameterData[PARAM_FILTER_FREQ].voice_max_val);
        
        //set cutoff value, multipled by filter envelope
        filterSvf.setCutoff (cutoff_val * envFilterOut);
        
        //================================
        //process LFO->reso modulation
        double reso_lfo_mod_val = getModulatedParamValue (PARAM_MOD_LFO_RESO, PARAM_FILTER_RESO, lfoOut);
        
        //Add the reso modulation values to the patch value, making sure the produced value is in range
        double reso_val = patchParameterData[PARAM_FILTER_RESO].voice_val + reso_lfo_mod_val + velResoModVal;
        reso_val = boundValue (reso_val, patchParameterData[PARAM_FILTER_RESO].voice_min_val, patchParameterData[PARAM_FILTER_RESO].voice_max_val);
        
        //set resonance value
        filterSvf.setResonance (reso_val);
        
        //================================
        //Apply the filter
        
        filterOut = filterSvf.play (oscMixOut,
                                    patchParameterData[PARAM_FILTER_LP_MIX].voice_val,
                                    patchParameterData[PARAM_FILTER_BP_MIX].voice_val,
                                    patchParameterData[PARAM_FILTER_HP_MIX].voice_val,
                                    patchParameterData[PARAM_FILTER_NOTCH_MIX].voice_val);
        
        
        //==========================================================
        //process distortion...
        //FIXME: should PARAM_FX_DISTORTION_AMOUNT also change the shape of the distortion?
        distortionOut = distortion.atanDist (filterOut, 200.0);
        
        //process distortion mix
        //FIXME: is this (mixing dry and wet) the best way to apply distortion? Or should I just always be running the main output through the distortion function?
        //FIXME: probably need to reduce the disortionOut value so bringing in disortion doesn't increase the overall volume too much
        effectsMixOut = (distortionOut * patchParameterData[PARAM_FX_DISTORTION_AMOUNT].voice_val) + (filterOut * (1.0 - patchParameterData[PARAM_FX_DISTORTION_AMOUNT].voice_val));
        
        //==========================================================
        //apply amp envelope, making all channels the same (pass in effectsMixOut, return output)
        for (uint8_t i = 0; i < maxiSettings::channels; i++)
        {
            output[i] = effectsMixOut * envAmpOut;
        }
        
    } // if (envAmpOut > 0)
    
}

//==========================================================
//==========================================================
//==========================================================
//Function that does everything that needs to be done when a new
//note-on or note-off message is sent to the voice.

void VintageVoice::processNoteMessage (bool note_status, uint8_t note_num, uint8_t note_vel)
{
    //==========================================================
    //if a note-on
    if (note_status == true)
    {
        //============================
        //store the root note num
        rootNoteNum = note_num;
        
        //============================
        //Set 'vintage amount' pitch offset
        //TODO: implement seperate pitches for each osc too/instead?
        int16_t vintage_pitch_offset = 0;
        
        //if there is a vintage value
        if (patchParameterData[PARAM_GLOBAL_VINTAGE_AMOUNT].voice_val != 0)
        {
            //get a random pitch value using the vintage amount as the max possible value
            vintage_pitch_offset = rand() % (int)patchParameterData[PARAM_GLOBAL_VINTAGE_AMOUNT].voice_val;
            //offset the random pitch value so that the offset could be negative
            vintage_pitch_offset -= patchParameterData[PARAM_GLOBAL_VINTAGE_AMOUNT].voice_val / 2;
            
            //FIXME: the above algorithm will make higher notes sound less out of tune than higher notes - fix this.
            
        } //if (patchParameterData[PARAM_GLOBAL_VINTAGE_AMOUNT].voice_val != 0)
        
        //============================
        //set the oscillator pitches
        convert mtof;
        oscSinePitch = mtof.mtof (rootNoteNum + (patchParameterData[PARAM_OSC_SINE_NOTE].voice_val - 64)) + vintage_pitch_offset;
        oscTriPitch = mtof.mtof (rootNoteNum + (patchParameterData[PARAM_OSC_TRI_NOTE].voice_val - 64)) + vintage_pitch_offset;
        oscSawPitch = mtof.mtof (rootNoteNum + (patchParameterData[PARAM_OSC_SAW_NOTE].voice_val - 64)) + vintage_pitch_offset;
        oscPulsePitch = mtof.mtof (rootNoteNum + (patchParameterData[PARAM_OSC_PULSE_NOTE].voice_val - 64)) + vintage_pitch_offset;
        oscSquarePitch = mtof.mtof (rootNoteNum + (patchParameterData[PARAM_OSC_SQUARE_NOTE].voice_val - 64)) + vintage_pitch_offset;
        
        //============================
        //set the note velocity
        voiceVelocityValue = scaleValue (note_vel, 0, 127, 0., 1.);
        
        //============================
        //work out velocity modulation values
        
        //vel->amp env modulation
        velAmpModVal = getModulatedParamValue (PARAM_MOD_VEL_AMP, PARAM_AEG_AMOUNT, voiceVelocityValue);
        
        //vel->cutoff modulation
        velFreqModVal = getModulatedParamValue (PARAM_MOD_VEL_FREQ, PARAM_FILTER_FREQ, voiceVelocityValue);
        
        //vel->resonance modulation
        velResoModVal = getModulatedParamValue (PARAM_MOD_VEL_RESO, PARAM_FILTER_RESO, voiceVelocityValue);
        
        //============================
        //Reset LFO osc phase, but only if the amp envelope is currently closed.
        //This means that the phase won't be reset in mono mode when triggering stacked notes.
        
        if (envAmp.trigger == 0)
            lfo.phaseReset(0.0);
        
    } //if (note_status == true)
    
    //==========================================================
    //if a note-off
    else if (note_status == false)
    {
        //reset aftertouch value
        aftertouchValue = 0;
    }
    
    //==========================================================
    //set trigger value of envelopes
    envAmp.trigger = note_status;
    envFilter.trigger = note_status;
}

//==========================================================
//==========================================================
//==========================================================
//Converts a MIDI aftertouch value into a voice aftertouch value

void VintageVoice::processAftertouchMessage (uint8_t aftertouch_val)
{
    aftertouchValue = scaleValue (aftertouch_val, 0, 127, 0., 1.);
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
    
    else if (param_num == PARAM_OSC_SINE_NOTE)
    {
        convert mtof;
        oscSinePitch = mtof.mtof (rootNoteNum + (patchParameterData[param_num].voice_val - 64));
    }
    
    else if (param_num == PARAM_OSC_TRI_NOTE)
    {
        convert mtof;
        oscTriPitch = mtof.mtof (rootNoteNum + (patchParameterData[param_num].voice_val - 64));
    }
    
    else if (param_num == PARAM_OSC_SAW_NOTE)
    {
        convert mtof;
        oscSawPitch = mtof.mtof (rootNoteNum + (patchParameterData[param_num].voice_val - 64));
    }
    
    else if (param_num == PARAM_OSC_PULSE_NOTE)
    {
        convert mtof;
        oscPulsePitch = mtof.mtof (rootNoteNum + (patchParameterData[param_num].voice_val - 64));
    }
    
    else if (param_num == PARAM_OSC_SQUARE_NOTE)
    {
        convert mtof;
        oscSquarePitch = mtof.mtof (rootNoteNum + (patchParameterData[param_num].voice_val - 64));
    }
    
    else if (param_num == PARAM_OSC_PHASE_SPREAD)
    {
        //FIXME: I need to properly understand what the phase value represents in order to implement a definitive algorithm here.
        //But basically what it does is, the higher the param value, the more spread the phases are of each oscillator from one another.
        //Sine will always stay at 0, tri will change of a small range, saw over a slightly bigger range, and so on.
        
        oscSine.phaseReset(0.0);
        oscTri.phaseReset (patchParameterData[param_num].voice_val * 0.002);
        oscSaw.phaseReset (patchParameterData[param_num].voice_val * 0.004);
        oscPulse.phaseReset (patchParameterData[param_num].voice_val * 0.006);
        oscSquare.phaseReset (patchParameterData[param_num].voice_val * 0.008);
    }
    
    else if (param_num == PARAM_MOD_VEL_AMP)
    {
        //vel->amp env modulation
        velAmpModVal = getModulatedParamValue (param_num, PARAM_AEG_AMOUNT, voiceVelocityValue);
    }
    
    else if (param_num == PARAM_MOD_VEL_FREQ)
    {
        //vel->amp env modulation
        velAmpModVal = getModulatedParamValue (param_num, PARAM_FILTER_FREQ, voiceVelocityValue);
    }
    
    else if (param_num == PARAM_MOD_VEL_RESO)
    {
        //vel->amp env modulation
        velAmpModVal = getModulatedParamValue (param_num, PARAM_FILTER_RESO, voiceVelocityValue);
    }
}

//==========================================================
//==========================================================
//==========================================================

double VintageVoice::getModulatedParamValue (uint8_t mod_depth_param, uint8_t source_param, double realtime_mod_val)
{
    double modulated_param_val = 0;
    
    if (patchParameterData[mod_depth_param].voice_val > 0)
    {
        modulated_param_val = ((patchParameterData[source_param].voice_max_val - patchParameterData[source_param].voice_val) * (realtime_mod_val * patchParameterData[mod_depth_param].voice_val));
    }
    else if (patchParameterData[mod_depth_param].voice_val < 0)
    {
        modulated_param_val = ((patchParameterData[source_param].voice_val - patchParameterData[source_param].voice_min_val) * (realtime_mod_val * patchParameterData[mod_depth_param].voice_val));
    }
    
    return modulated_param_val;
}

//==========================================================
//==========================================================
//==========================================================

double VintageVoice::boundValue (double val, double min_val, double max_val)
{
    if (val > max_val)
        val = max_val;
    
    else if (val < min_val)
        val = min_val;
    
    return val;
}

