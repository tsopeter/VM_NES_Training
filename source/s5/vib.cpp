#include "vib.hpp"


Vibration::Vibration()
: gen(std::random_device{}()) {}

Vibration::~Vibration() {}

double Vibration::sample_Normal() {
    std::normal_distribution<double> d(0.0, std);
    double x = d(gen);
    return std::clamp(x, uMin, uMax);
}

double Vibration::sample_Uniform() {
    std::uniform_real_distribution<double> d(uMin, uMax);
    return d(gen);
}