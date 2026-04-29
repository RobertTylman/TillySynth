#include <iostream>
#include <cmath>
using namespace std;
int main() {
    float filterEnvValue = 1.0f;
    float filterEnvAmount = 1.0f;
    float envModOctaves = filterEnvValue * filterEnvAmount * 10.0f;
    float baseCutoff = 20.0f;
    cout << baseCutoff * std::exp2(envModOctaves) << endl;
}
