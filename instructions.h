#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdint.h>
#include "processor.h"

/* Holds all decoded fields of a 32-bit instruction, plus runtime values */
typedef struct {
    int32_t rawInstruction;

    int opcode;
    int r1, r2, r3;
    int shamt;
    int immediate;
    int address;

    int32_t r1Value, r2Value, r3Value;

    int32_t result;
    int32_t memoryAddress;
    int32_t memoryData;

    int branchTaken;
    int branchTarget;

    char mnemonic[10];
    int  instructionPC;   /* PC value at the time this instruction was fetched */
} DecodedInstruction;

DecodedInstruction decodeInstruction(Processor *cpu, int32_t raw, int instrPC);
void               executeInstruction(Processor *cpu, DecodedInstruction *d, int instrPC);

#endif
