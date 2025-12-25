#include <iostream>
#include <vector>
#include <stdexcept>
#include <array>
#include <cstdint>
#include <iomanip>
#include <cstdio>

// Emulating the clock cycle
/*
uint8_t clock_tick = 1;

void emulateCycle()
{
    while (clock_tick < 9)
    {
        process_tick(clock_tick);
        clock_tick++;
    }

    clock_tick = 1;
}*/

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

// Print Helpers

const char *to_string(HaltReason reason)
{
    switch (reason)
    {
        case HaltReason::NONE: return "NONE";
        case HaltReason::SUCCESS: return "SUCCESS (HALT EXECUTED)";
        case HaltReason::UNKNOWN_OPCODE: return "FAULT: UNKNOWN OPCODE";
        case HaltReason::DIVISION_BY_ZERO: return "FAULT: DIVISION BY ZERO";
        default: return "UNKNOWN";
    }
}

/*
This function was written using an LLM to streamline the debugging process.
*/
void print_cpu_state(const CPUState& cpu, bool final = false) {
    if (final) std::cout << "\n========== FINAL CPU STATE ==========\n";
    else       std::cout << "--- Step (PC: " << std::dec << cpu.pc << ") ---\n";

    // Format registers as 4-digit uppercase hex using I/O manipulators
    std::cout << "Registers: "
              << "R0:" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase << cpu.GenPR[0] << " "
              << "R1:" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase << cpu.GenPR[1] << " "
              << "R2:" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase << cpu.GenPR[2] << " "
              << "R3:" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase << cpu.GenPR[3] << "\n";

    // Format PC, Flags, and Halted status
    std::cout << "PC: " << std::setfill('0') << std::setw(4) << std::hex << cpu.pc 
              << " | SF: " << std::dec << (int)cpu.sf 
              << " | Halted: " << (cpu.halted ? "YES" : "NO") 
              << " | Reason: " << to_string(cpu.reason) << "\n";
    
    if (final) std::cout << "=====================================\n\n";
    
    // Reset to decimal for future regular output
    std::cout << std::dec;
}


// The Execution loop

void run(CPUState &cpu)
{

    std::cout << "Stating execution... \n";

    while (!cpu.halted)
    {

        //Print state before executing current instruction
        print_cpu_state(cpu);

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

        bool suppress_default_pc_increment = false;

        switch (opcode)
        {
            case 0x1: // LOAD Immediate: REG[dest] = operand
            {
                cpu.GenPR[dest_reg] = operand;
                break;
            }
            
            case 0x2: // ADD: REG[dest] = REG[dest] + REG[source]
                {
                    cpu.GenPR[dest_reg] += cpu.GenPR[source_reg];

                    //Update status flag is result is 0;
                    cpu.sf = (cpu.GenPR[dest_reg] == 0);
                }
                break;

            case 0x3: // STORE (to memory): RAM[source_reg] = REG[dest]
            {
            /*
            This case model register-indirect addressing.
            It allows us to access all 65,536 words as the registers are uint16_t.
            Safety checks are avoided as a valid address is always assumed to be true.
            */
                uint16_t reg_addr = instruction & 0x3; //using the bottom 2 bits to pick a register.
                uint16_t target_addr = cpu.GenPR[reg_addr];
                cpu.data[target_addr] = cpu.GenPR[dest_reg];
                break;
            }
            
            case 0x4: // SUB: GenPR[dest] = GenPR[dest] - GenPR[source]
                {
                cpu.GenPR[dest_reg] -= cpu.GenPR[source_reg];
                cpu.sf = (cpu.GenPR[dest_reg] == 0);
                break;
                }
            
            case 0x5: // MUL: GenPR[dest] = GenPR[dest] * GenPR[source]
                {
                cpu.GenPR[dest_reg] *= cpu.GenPR[source_reg];
                cpu.sf = (cpu.GenPR[dest_reg] == 0);
                break;
                }
            
            case 0x6: //DIV: GenPR[dest] = GenPR[dest] / GenPR[source]
                {
                if (cpu.GenPR[source_reg] == 0)
                {
                    cpu.halted = true;
                    cpu.reason = HaltReason::DIVISION_BY_ZERO;
                    suppress_default_pc_increment = true;
                }
                else
                {
                    //SUCCESS -> Atomic update of register and sf.
                    cpu.GenPR[dest_reg] /= cpu.GenPR[source_reg];
                    cpu.sf = (cpu.GenPR[dest_reg] == 0);
                }
                break;
                }

            case 0x7: //JMP: Set PC to immediate address.
                {
                cpu.pc = address;
                suppress_default_pc_increment = true;
                break;
                }
            
            case 0xF: //HALT
            {
                cpu.halted = true;
                cpu.reason = HaltReason::SUCCESS;
                suppress_default_pc_increment = true;
                break;
            }

            default:
            {
                cpu.halted = true;
                cpu.reason = HaltReason::UNKNOWN_OPCODE;
                suppress_default_pc_increment = true;
                break;
            }
        }

        if (!suppress_default_pc_increment) // Applies default PC increment rule.
        {
            cpu.pc += 1;
        }
    }

    print_cpu_state(cpu, true);
}

//Program loader (helper function)

void load_program(CPUState &cpu, const std::vector<uint16_t> &program)
{
    for (size_t i{0}; i < program.size(); ++i)
    {
        cpu.data[i] = program[i];
    }

    cpu.pc = 0;
    cpu.halted = false;
    cpu.reason = HaltReason::NONE;

}

int main()
{
    CPUState cpu;

    std::cout << "TEST CASE 1: HALT IMMEDIATELY" << '\n';
    load_program(cpu, {
        0xF000 //Opcode for halt
    });
    run(cpu);

    std::cout << "\nTEST CASE 2: ADD + STORE\n";
    cpu = CPUState(); // Reset
    load_program(cpu, { 0x100A, 0x1105, 0x2001, 0x3002, 0xF000 });
    run(cpu);

    std::cout << "\nTEST CASE 3: DIV BY ZERO FAULT\n";
    cpu = CPUState(); // Reset
    load_program(cpu, { 0x100A, 0x1100, 0x6001, 0xF000 });
    run(cpu);

    return 0;
}