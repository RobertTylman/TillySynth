#include <iostream>
#include <cmath>

void makeCoeffs(float cutoff, float sampleRate, float& b0, float& b1, float& b2, float& a1, float& a2) {
    float wc = 2.0f * M_PI * cutoff / sampleRate;
    float alpha = std::sin(wc) / (2.0f * 0.7071f);
    float a0 = 1.0f + alpha;
    b0 = (1.0f - std::cos(wc)) / 2.0f / a0;
    b1 = (1.0f - std::cos(wc)) / a0;
    b2 = (1.0f - std::cos(wc)) / 2.0f / a0;
    a1 = -2.0f * std::cos(wc) / a0;
    a2 = (1.0f - alpha) / a0;
}

int main() {
    float b0, b1, b2, a1, a2;
    makeCoeffs(20.0f, 44100.0f, b0, b1, b2, a1, a2);
    
    // Test frequency
    float freq = 65.0f; // C2
    float w = 2.0f * M_PI * freq / 44100.0f;
    
    // Magnitude response of 1 biquad
    float num = b0*b0 + b1*b1 + b2*b2 + 2.0f*(b0*b1 + b1*b2)*std::cos(w) + 2.0f*b0*b2*std::cos(2.0f*w);
    float den = 1.0f + a1*a1 + a2*a2 + 2.0f*(a1 + a1*a2)*std::cos(w) + 2.0f*a2*std::cos(2.0f*w);
    float mag = std::sqrt(num / den);
    
    std::cout << "Magnitude at 65Hz for 20Hz cutoff: " << 20.0f * std::log10(mag) << " dB\n";
    std::cout << "Magnitude for 2 stages (24dB/oct): " << 20.0f * std::log10(mag*mag) << " dB\n";
}
