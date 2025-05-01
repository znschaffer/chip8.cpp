#include <iostream>
#include "Emulator.h"

int main() {
    std::string filename = "ibm.ch8";
    Emulator emu = Emulator(filename);
    emu.run();
}
