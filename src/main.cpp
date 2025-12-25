#include <iostream>
#include <string>
#include <stdexcept>
#include <array>
#include <cstdint>

// Emulating the clock cycle

uint8_t clock_tick = 1;

void emulateCycle()
{
    while (clock_tick < 9)
    {
        process_tick(clock_tick);
        clock_tick++;
    }

    clock_tick = 1;
}

struct CPUState
{
    using RegisterType = uint16_t;
    /*
    Here we treat the registers as a trusted architectural state.
    We use uint16_t (2 byte registers) to avoid problems that may arise due to overflow.
    The CPU is always created in a known, deterministic manner, so we can avoid using constructors here.
    */
    std::array<RegisterType, 4> GenPR; // General Purpose Registers.

    uint8_t pc; // Program Counter
    uint8_t sf; // Status flag
    
};
