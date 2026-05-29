#include <stdio.h>
#include <string.h>
#include "processor.h"

/* Set all memory, registers, and PC to zero */
void init_processor(Processor *cpu) {
    memset(cpu->memory,    0, sizeof(cpu->memory));
    memset(cpu->registers, 0, sizeof(cpu->registers));
    cpu->PC = 0;
}

/* Read a value from memory. Returns 0 and prints an error if out of range. */
int32_t read_memory(Processor *cpu, int32_t addr) {
    if (addr < 0 || addr >= MEMORY_SIZE) {
        printf("[ERROR] read_memory: address %d out of range\n", addr);
        return 0;
    }
    return cpu->memory[addr];
}

/* Write a value to memory. Prints an error if out of range. */
void write_memory(Processor *cpu, int32_t addr, int32_t value) {
    if (addr < 0 || addr >= MEMORY_SIZE) {
        printf("[ERROR] write_memory: address %d out of range\n", addr);
        return;
    }
    cpu->memory[addr] = value;
}

/* Read a register value. R0 always returns 0. */
int32_t read_register(Processor *cpu, int reg_num) {
    if (reg_num < 0 || reg_num >= NUM_REGISTERS) {
        printf("[ERROR] read_register: R%d out of range\n", reg_num);
        return 0;
    }
    if (reg_num == ZERO_REGISTER) return 0;
    return cpu->registers[reg_num];
}

/* Write a value to a register. R0 is protected and cannot be changed. */
void write_register(Processor *cpu, int reg_num, int32_t value) {
    if (reg_num < 0 || reg_num >= NUM_REGISTERS) {
        printf("[ERROR] write_register: R%d out of range\n", reg_num);
        return;
    }
    if (reg_num == ZERO_REGISTER) return;   /* R0 is always 0 */
    cpu->registers[reg_num] = value;
}
