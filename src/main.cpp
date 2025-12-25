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
    using RAMType = uint16_t;
    /*
    Here we treat the registers as a trusted architectural state.
    We use uint16_t (2 byte registers) to avoid problems that may arise due to overflow.
    The CPU state is always created in a known, deterministic manner, 
    so we can avoid using constructors here.
    */
    std::array<RegisterType, 4> GenPR{}; // General Purpose Registers.

    //We use size_t here (host agnostic) keeping in mind that PC Width >= memory index width
    size_t pc; // Program Counter
    
    uint8_t sf; // Status flag

    /*
    We model memory as a dumb, plain architectural state.
    Instructions are word-encoded (16 bit)
    The memory isn't a separate device, it is owned by the CPU wrapper,
    and accessed via free functions.
    */

    static constexpr size_t RAM_SIZE = 64 * 1024; // 128KB RAM for 16-bit architecture.
    std::array<RAMType, RAM_SIZE> data{};
    
};

/*
This project is a sequential interpreter and does not require an explicit state machine
to model the atomic cycle of computation, hence it is omitted.
The control flow encodes the state.
*/

