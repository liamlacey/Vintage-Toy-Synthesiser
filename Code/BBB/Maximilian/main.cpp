#include "maximilian.h"

//This shows how to use maximilian to build a polyphonic synth.

#define POLYPHONY 6
#define POLYPHONY_2 6

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
