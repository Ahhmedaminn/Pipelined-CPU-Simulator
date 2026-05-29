#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdint.h>

#define MEMORY_SIZE   2048
#define NUM_REGISTERS 32
#define ZERO_REGISTER 0

/* The main processor state: memory, registers, and program counter */
typedef struct {
    int32_t memory[MEMORY_SIZE];
    int32_t registers[NUM_REGISTERS];
    int32_t PC;
} Processor;

void    init_processor(Processor *cpu);

int32_t read_memory(Processor *cpu, int32_t addr);
void    write_memory(Processor *cpu, int32_t addr, int32_t value);

int32_t read_register(Processor *cpu, int reg_num);
void    write_register(Processor *cpu, int reg_num, int32_t value);

#endif
