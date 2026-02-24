#ifndef s5_vib_hpp
#define s5_vib_hpp

#include <random>
#include <algorithm>
#include <iostream>

struct Vibration {
    Vibration ();
    ~Vibration ();

    double uMax = 1.0;
    double uMin = -1.0;
    double std  = 0.1; // Standard deviation for normal distribution

    double sample_Normal ();
    double sample_Uniform ();

private:
    std::mt19937 gen;
};


#endif

