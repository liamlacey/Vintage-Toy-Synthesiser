//Using BPF equation from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
//Example contributed by Rebecca Fiebrink

#include "maximilian.h"

float xs[3], ys[3];
float a0, a1, a2, b0, b1, b2;
float f0 = 400; //THE FREQUENCY
float Q = 1.0;

maxiOsc mySwitchableOsc;

void setup() {//some inits
    double w0 = 2*PI*f0/44100;
    double alpha = sin(w0)/(2*Q);
    //Band-pass reson:
    //    b0 =   alpha;
    //     b1 =   0;
    //     b2 =  -1 * alpha;
    //     a0 =   1 + alpha;
    //     a1 =  -2*cos(w0);
    //     a2 =   1 - alpha;
    
    //Notch:
    b0 =   1;
    b1 =  -2*cos(w0);
    b2 =   1;
    a0 =   1 + alpha;
    a1 =  -2*cos(w0);
    a2 =   1 - alpha;
    
    //LPF:
    //    b0 =  (1 - cos(w0))/2;
    //    b1 =   1 - cos(w0);
    //    b2 =  (1 - cos(w0))/2;
    //    a0 =   1 + alpha;
    //    a1 =  -2*cos(w0);
    //    a2 =   1 - alpha;
}

void play(double *output) {
    xs[0] = mySwitchableOsc.sawn(400);
    ys[0] = (b0/a0)*xs[0] + (b1/a0)*xs[1] + (b2/a0)*xs[2]
    - (a1/a0)*ys[1] - (a2/a0)*ys[2];
    
    *output = ys[0];
    
    ys[2] = ys[1]; ys[1] = ys[0];
    xs[2] = xs[1]; xs[1] = xs[0];
}
