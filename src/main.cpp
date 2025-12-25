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

/*
We treat division by zero as an architectural fault. 
This is because in the previously implemented model, the CPU halted silently 
when division by zero occured.
The architectural state was left partially updated.
This method fixes those issues.
*/

enum class HaltReason : uint8_t
{
    NONE = 0,
    SUCCESS,
    DIVISION_BY_ZERO,
    UNKNOWN_OPCODE
};

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
    size_t pc{}; // Program Counter
    
    uint8_t sf{}; // Status flag

    /*
    We model memory as a dumb, plain architectural state.
    Instructions are word-encoded (16 bit)
    The memory isn't a separate device, it is owned by the CPU wrapper,
    and accessed via free functions.
    */

    static constexpr size_t RAM_SIZE = 64 * 1024; // 128KB RAM for 16-bit architecture.
    std::array<RAMType, RAM_SIZE> data{};

    // Halted is a CPU architectural state, not a local control variable.
    bool halted = false;
    HaltReason reason = HaltReason::NONE; //Single Source of Truth.
    
};

/*
This project is a sequential interpreter and does not require an explicit state machine
to model the atomic cycle of computation, hence it is omitted.
The control flow encodes the state.
*/

// The Execution loop

void run(CPUState &cpu)
{

    while (!cpu.halted)
    {
        // 1. FETCH -> this fetches a full 16-bit word.
        uint16_t instruction = cpu.data[cpu.pc];

        //2. DECODE AND EXECUTE
        // The high 4 bits are the opcode, the middle 2 are the destination register
        //The lower bits are the source registers/immediate values.

        uint16_t opcode = (instruction >> 12) & 0xF;
        uint16_t dest_reg = (instruction >> 8) & 0x3;
        uint16_t source_reg = instruction & 0x3;
        uint16_t operand = instruction & 0xFF; // 8-bit intermediate.
        uint16_t address = instruction & 0xFFF;

        bool branched = false;

        switch (opcode)
        {
            case 0x1: // LOAD Immediate: REG[dest] = operand
                cpu.GenPR[dest_reg] = operand;
                break;
            
            case 0x2: // ADD: REG[dest] = REG[dest] + REG[source]
                {
                    cpu.GenPR[dest_reg] += cpu.GenPR[source_reg];

                    //Update status flag is result is 0;
                    cpu.sf = (cpu.GenPR[dest_reg] == 0);
                }
                break;

            case 0x3: // STORE (to memory): RAM[source_reg] = REG[dest]
            /*
            This case model register-indirect addressing.
            It allows us to access all 65,536 words as the registers are uint16_t.
            Safety checks are avoided as a valid address is always assumed to be true.
            */
                uint16_t reg_addr = instruction & 0x3; //using the bottom 2 bits to pick a register.
                uint16_t target_addr = cpu.GenPR[reg_addr];
                cpu.data[target_addr] = cpu.GenPR[dest_reg];
                break;
            
            case 0x4: // SUB: GenPR[dest] = GenPR[dest] - GenPR[source]
                
                cpu.GenPR[dest_reg] -= cpu.GenPR[source_reg];
                cpu.sf = (cpu.GenPR[dest_reg] == 0);
                break;
            
            case 0x5: // MUL: GenPR[dest] = GenPR[dest] * GenPR[source]
                
                cpu.GenPR[dest_reg] *= cpu.GenPR[source_reg];
                cpu.sf = (cpu.GenPR[dest_reg] == 0);
                break;
            
            case 0x6: //DIV: GenPR[dest] = GenPR[dest] / GenPR[source]
                
                if (cpu.GenPR[source_reg] == 0)
                {
                    cpu.halted = true;
                    cpu.reason = HaltReason::DIVISION_BY_ZERO;
                    branched = true;
                }
                else
                {
                    //SUCCESS -> Atomic update of register and sf.
                    cpu.GenPR[dest_reg] /= cpu.GenPR[source_reg];
                    cpu.sf = (cpu.GenPR[dest_reg] == 0);
                }
                break;

            case 0x7: //JMP: Set PC to immediate address.
                
                cpu.pc = address;
                branched = true;
                break;
            
            case 0xF: //HALT
                cpu.halted = true;
                cpu.reason = HaltReason::SUCCESS;
                branched = true;
                break;

            default:
                cpu.halted = true;
                cpu.reason = HaltReason::UNKNOWN_OPCODE;
                branched = true;
                break;
        }

        if (!branched) // Applies default PC increment rule.
        {
            cpu.pc += 1;
        }
    }
}