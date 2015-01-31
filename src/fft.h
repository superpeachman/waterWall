//
//  fft.h
//  samuraiApp
//
//  Created by Toshiya Momota on 12/28/14.
//
//

#pragma once
#include "ofMain.h"

class fft {
public:
    
    fft();
    ~fft();
    
    /* Calculate the power spectrum */
    void powerSpectrum(int start, int half, float *data, int windowSize,float *magnitude,float *phase, float *power, float *avg_power);
    /* ... the inverse */
    void inversePowerSpectrum(int start, int half, int windowSize, float *finalOut,float *magnitude,float *phase);
    
    
};
