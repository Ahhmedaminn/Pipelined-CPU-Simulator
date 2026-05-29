#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "processor.h"
#include "instructions.h"

/* Sign-extend an 18-bit value to a full 32-bit signed integer */
static int signExtend18(int value) {
    if (value & (1 << 17))
        value |= ~0x3FFFF;   /* fill upper bits with 1s */
    return value;
}

/* Return the assembly name for a given opcode */
static const char *getMnemonic(int opcode) {
    switch (opcode) {
        case 0:  return "ADD";
        case 1:  return "SUB";
        case 2:  return "MUL";
        case 3:  return "MOVI";
        case 4:  return "JEQ";
        case 5:  return "AND";
        case 6:  return "XORI";
        case 7:  return "JMP";
        case 8:  return "LSL";
        case 9:  return "LSR";
        case 10: return "MOVR";
        case 11: return "MOVM";
        default: return "UNKNOWN";
    }
}

/* Decode a raw 32-bit instruction into all its fields.
   instrPC is the PC value at the time of fetch (needed for branch target calculation). */
DecodedInstruction decodeInstruction(Processor *cpu, int32_t rawInstruction, int instrPC) {
    DecodedInstruction d;
    uint32_t instr = (uint32_t)rawInstruction;

    d.rawInstruction = rawInstruction;
    d.instructionPC  = instrPC;

    /* Extract fields from the 32-bit encoding */
    d.opcode    = (instr >> 28) & 0xF;
    d.r1        = (instr >> 23) & 0x1F;
    d.r2        = (instr >> 18) & 0x1F;
    d.r3        = (instr >> 13) & 0x1F;
    d.shamt     =  instr        & 0x1FFF;
    d.immediate = signExtend18(instr & 0x3FFFF);
    d.address   =  instr        & 0x0FFFFFFF;

    /* Read current register values */
    d.r1Value = read_register(cpu, d.r1);
    d.r2Value = read_register(cpu, d.r2);
    d.r3Value = read_register(cpu, d.r3);

    /* Initialize output fields */
    d.result        = 0;
    d.memoryAddress = 0;
    d.memoryData    = 0;
    d.branchTaken   = 0;
    d.branchTarget  = -1;

    strcpy(d.mnemonic, getMnemonic(d.opcode));

    return d;
}

/* Execute an already-decoded instruction (used in the sequential Part C test) */
void executeInstruction(Processor *cpu, DecodedInstruction *d, int instrPC) {
    switch (d->opcode) {
        case 0:  /* ADD */
            d->result = d->r2Value + d->r3Value;
            write_register(cpu, d->r1, d->result);
            break;
        case 1:  /* SUB */
            d->result = d->r2Value - d->r3Value;
            write_register(cpu, d->r1, d->result);
            break;
        case 2:  /* MUL */
            d->result = d->r2Value * d->r3Value;
            write_register(cpu, d->r1, d->result);
            break;
        case 3:  /* MOVI */
            d->result = d->immediate;
            write_register(cpu, d->r1, d->result);
            break;
        case 4:  /* JEQ */
            if (d->r1Value == d->r2Value) {
                d->branchTaken  = 1;
                d->branchTarget = instrPC + 1 + d->immediate;
                cpu->PC = d->branchTarget;
            }
            break;
        case 5:  /* AND */
            d->result = d->r2Value & d->r3Value;
            write_register(cpu, d->r1, d->result);
            break;
        case 6:  /* XORI */
            d->result = d->r2Value ^ d->immediate;
            write_register(cpu, d->r1, d->result);
            break;
        case 7:  /* JMP */
            d->branchTaken  = 1;
            d->branchTarget = (cpu->PC & 0xF0000000) | d->address;
            cpu->PC = d->branchTarget;
            break;
        case 8:  /* LSL */
            d->result = d->r2Value << d->shamt;
            write_register(cpu, d->r1, d->result);
            break;
        case 9:  /* LSR */
            d->result = (int32_t)((uint32_t)d->r2Value >> d->shamt);
            write_register(cpu, d->r1, d->result);
            break;
        case 10: /* MOVR - load from memory */
            d->memoryAddress = d->r2Value + d->immediate;
            d->memoryData    = read_memory(cpu, d->memoryAddress);
            write_register(cpu, d->r1, d->memoryData);
            break;
        case 11: /* MOVM - store to memory */
            d->memoryAddress = d->r2Value + d->immediate;
            write_memory(cpu, d->memoryAddress, d->r1Value);
            break;
        default:
            printf("[ERROR] Unknown opcode: %d\n", d->opcode);
            break;
    }
}
