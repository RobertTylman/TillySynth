#include <iostream>
#include <fstream>
#include <regex>

int main() {
    std::ifstream file("PresetManager.cpp");
    std::string line;
    while(std::getline(file, line)) {
        // ...
    }
    return 0;
}
