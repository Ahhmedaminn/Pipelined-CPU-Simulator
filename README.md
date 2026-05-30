# Team_8_CA26

## Team Information

| Name                | ID   | Tutorial |
|---------------------|------|----------|
| Ahmed Amin          | 19002035 | 1        |
| Abd el Rahman Gamal | 19002030 | 2        |
| Ziad Tarek          | 22001257 | 2        |
| Mohamed Waleed      | 19002146 | 2        |
 | Karma El Sabahy | 22001230 | 1 | 

---

## Project Overview

A 5-stage pipelined processor simulator written in C, implementing a fictional ISA as described in the CSEN B501 project specification (Spring 2026).

The processor uses a **Von Neumann architecture** with a single shared memory of 2048 × 32-bit words:
- Addresses **0–1023**: instruction memory
- Addresses **1024–2047**: data memory

The pipeline has 5 stages: **IF** (2 cycles) → **ID** (2 cycles) → **EX** (2 cycles) → **MEM** (1 cycle) → **WB** (1 cycle)

---

## Instruction Set

| Opcode | Mnemonic | Format | Operation |
|--------|----------|--------|-----------|
| 0 | ADD  | R | R1 = R2 + R3 |
| 1 | SUB  | R | R1 = R2 - R3 |
| 2 | MUL  | R | R1 = R2 * R3 |
| 3 | MOVI | I | R1 = IMM |
| 4 | JEQ  | I | if (R1 == R2) PC = PC+1+IMM |
| 5 | AND  | R | R1 = R2 & R3 |
| 6 | XORI | I | R1 = R2 ^ IMM |
| 7 | JMP  | J | PC = PC[31:28] \|\| ADDRESS |
| 8 | LSL  | R | R1 = R2 << SHAMT |
| 9 | LSR  | R | R1 = R2 >>> SHAMT |
| 10 | MOVR | I | R1 = MEM[R2 + IMM] |
| 11 | MOVM | I | MEM[R2 + IMM] = R1 |

---

## File Structure

```
Team_[NUMBER]_CA26/
├── main.c           Entry point. Loads program.txt, runs pipeline, prints final state.
├── processor.h/.c   Processor struct (memory, registers, PC). Read/write functions for
│                    memory and registers. R0 is hardwired to 0 and cannot be overwritten.
├── parser.h/.c      Reads assembly from program.txt, encodes each instruction as a
│                    32-bit integer, stores it in instruction memory.
├── instructions.h/.c Decodes a raw 32-bit instruction into all its fields (opcode, r1,
│                    r2, r3, shamt, immediate, address). Sign-extends the 18-bit immediate.
├── pipeline.h/.c    5-stage pipeline simulator. Tracks up to 5 in-flight instructions.
│                    Handles IF/MEM conflict, branch/jump flushing, and cycle counting.
├── printer.h/.c     All console output lives here. Prints cycle info, stage activity,
│                    register/memory updates, pipeline diagram, and final state.
├── program.txt      The assembly program to simulate (edit this to change the input).
└── test_programs/   Additional test programs covering all instruction types.
```

---

## How to Compile and Run

```bash
gcc -o sim main.c processor.c parser.c instructions.c pipeline.c printer.c
./sim
```

The program reads `program.txt` from the same directory and prints the full pipeline execution to the console.

---

## Pipeline Design Notes

**Stage durations:** IF=2, ID=2, EX=2, MEM=1, WB=1

**IF/MEM conflict:** The spec states IF and MEM cannot run in parallel as they share physical memory. The hardware resolves this by interleaving — in practice, the spec's own table (page 4) shows them coexisting on the same cycle, so no stall is introduced.

**Control hazards:** When a branch (JEQ) or jump (JMP) is taken, instructions that entered the pipeline after it are **flushed** — they pass through remaining stages with a `flushed` flag set, producing no register or memory writes. Flushing happens after the 2nd EX cycle when the branch outcome is known.

**Data hazards:** Not handled, as per the project specification. If an instruction reads a register that a previous instruction has not yet written back, it will read the stale value. The programmer is responsible for avoiding this.

**Stopping condition:** The simulation stops when all instructions have been fetched and the pipeline is fully empty — not by using the cycle count formula.
