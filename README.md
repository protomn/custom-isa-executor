# Deterministic Word-Addressed CPU Interpreter

A clean-room 16-bit CPU interpreter demonstrating explicit architectural state management, deterministic fault handling, and observational cycle accounting.

## Overview

This project implements a sequential interpreter for a simple 16-bit instruction set architecture. It emphasizes architectural correctness over performance, treating the CPU state as a first-class, observable entity throughout execution.

## Key Features

* **Explicit fetch–decode–execute model** with side-effect-free fetch phase
* **Program Counter ownership** handled by instruction semantics, not centralized control
* **Register-indirect addressing** enabling full 64K-word memory access
* **Deterministic fault handling** for division by zero and invalid opcodes
* **Observational cycle model** decoupled from correctness logic
* **Stress-tested** with backward jumps, infinite loops, and fault dominance scenarios

## Quick Start

### Build
```bash
g++ -std=c++17 -Wall -Wextra -O2 main.cpp -o cpu_interpreter
```

### Run
```bash
./cpu_interpreter
```

The interpreter will execute the loaded test case and print detailed execution traces.

## Architecture Highlights

### CPU Components
- **4 general-purpose registers** (R0-R3, 16-bit each)
- **64K-word unified memory** (128KB total)
- **Program counter** (word-addressed)
- **Status flag** (zero flag)
- **Halt mechanism** with reason tracking

### Instruction Set
| Opcode | Instruction | Description |
|--------|-------------|-------------|
| 0x1 | LOAD | Load immediate into register |
| 0x2 | ADD | Add registers |
| 0x3 | STORE | Store register to memory (indirect) |
| 0x4 | SUB | Subtract registers |
| 0x5 | MUL | Multiply registers |
| 0x6 | DIV | Divide registers (faults on zero) |
| 0x7 | JMP | Unconditional jump |
| 0xF | HALT | Stop execution |

## Example Program

```cpp
CPUState cpu;
load_program(cpu, {
    0x100A,  // R0 = 10
    0x1105,  // R1 = 5
    0x2001,  // R0 = R0 + R1  (R0 = 15)
    0x3002,  // MEM[R2] = R0  (store result)
    0xF000   // HALT
});
run(cpu);
```

## Design Philosophy

### Architectural Clarity
Every piece of CPU state is explicit in `CPUState`. No hidden globals, no magic registers, no implicit state machines.

### Fault Atomicity
When division by zero or an invalid opcode occurs, execution halts immediately without partial state updates. The architectural state reflects the exact moment of failure.

### Observational Metrics
The cycle counter tracks performance but **never influences control flow**. This demonstrates how to add instrumentation without compromising determinism.

### Instruction Autonomy
Each instruction explicitly controls whether the program counter advances, making control flow semantics clear and testable.

## Testing

The implementation includes comprehensive test cases:

1. Immediate halt
2. Arithmetic sequences
3. Memory operations
4. Division by zero fault
5. Backward jumps
6. Infinite loops
7. Unknown opcode handling
8. Fault dominance in loops
9. PC boundary conditions

Uncomment test cases in `main()` to run them (They have been left uncommented in the final commit).

## Output Format

```
--- Step (PC: 0) ---
Registers: R0:0000 R1:0000 R2:0000 R3:0000
PC: 0000 | SF: 0 | Cycles: 0 | Halted: NO | Reason: NONE

--- Step (PC: 1) ---
Registers: R0:000A R1:0000 R2:0000 R3:0000
PC: 0001 | SF: 0 | Cycles: 1 | Halted: NO | Reason: NONE

========== FINAL CPU STATE ==========
Registers: R0:000F R1:0005 R2:0000 R3:0000
PC: 0004 | SF: 0 | Cycles: 7 | Halted: YES | Reason: SUCCESS (HALT EXECUTED)
=====================================
```

## Documentation

See the full documentation for:
- Complete instruction encoding details
- Memory model specification
- Cycle cost breakdown
- API reference
- Design rationale

## Requirements

- C++17 or later
- Standard library (`<iostream>`, `<vector>`, `<array>`, `<cstdint>`)
- No external dependencies

## Implementation Notes

### Why This Approach?

This interpreter was built to explore architectural state as a first-class design concern. Many emulators and interpreters scatter state across globals, implicit flags, and control variables. This implementation consolidates everything into `CPUState`, making the system's behavior completely transparent and reproducible.

### Differences from Previous Design

The earlier model had a critical flaw: division by zero caused silent partial state corruption. This version treats faults as atomic architectural events—when a fault occurs, the CPU state reflects the exact moment of failure with no side effects.

### Performance Considerations

This is an interpreter, not a JIT or optimizing VM. Performance is deliberately sacrificed for clarity. Every instruction executes in its own switch case with no attempt at optimization.

## Future Work

- Conditional jump instructions (BEQ, BNE, etc.)
- Bitwise operations (AND, OR, XOR, shift)
- Stack operations (PUSH, POP)
- Assembler for human-readable programs
- Disassembler for debugging
- Interactive debugger with breakpoints

Educational implementation. Free to use for learning and experimentation.

## Acknowledgments

The development of this project was informed by a range of publicly available resources on emulator and instruction set simulator design. These materials were used primarily for conceptual guidance, architectural patterns, and understanding common implementation pitfalls when building execution engines. They served as reference points rather than direct implementation sources; all architectural decisions, core logic, testing, and validation were independently designed and implemented. Additionally, LLMs were used selectively to assist with README drafting and documentation clarity, but not for the design or implementation of the execution engine itself.

Referenced resources:

EMUL8 HOWTO — http://fms.komkon.org/EMUL8/HOWTO.html

Gecko05, BlueFPGA repository — https://github.com/Gecko05/BlueFPGA/tree/master

Gecko05, Writing Your First Emulator (Part 1) — https://gecko05.github.io/2022/10/22/first-emulator-part1.html

Gecko05, personal website repository — https://github.com/Gecko05/gecko05.github.io/tree/master

Stack Overflow discussion: What are the main steps to write an instruction set simulator? — https://stackoverflow.com/questions/4587291/what-are-the-main-steps-to-write-an-instruction-set-simulator

---

**Note**: This interpreter prioritizes architectural correctness and educational clarity over execution speed. It's designed to demonstrate clean state management and deterministic behavior, making it ideal for learning about CPU internals and instruction set architecture.
