#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define MEMORY_SIZE      2048
#define INSTR_MEM_START  0
#define INSTR_MEM_END    1023
#define DATA_MEM_START   1024
#define DATA_MEM_END     2047
#define NUM_REGISTERS    32
#define ZERO_REGISTER    0

typedef struct {
    int32_t memory[MEMORY_SIZE];
    int32_t registers[NUM_REGISTERS];
    int32_t PC;
} Processor;

void init_processor(Processor *cpu) {
    memset(cpu->memory,    0, sizeof(cpu->memory));
    memset(cpu->registers, 0, sizeof(cpu->registers));
    cpu->PC = 0;
}

int32_t read_memory(Processor *cpu, int32_t addr) {
    if (addr < 0 || addr >= MEMORY_SIZE) {
        printf("[ERROR] read_memory: address %d out of range\n", addr);
        return 0;
    }
    return cpu->memory[addr];
}

void write_memory(Processor *cpu, int32_t addr, int32_t value) {
    if (addr < 0 || addr >= MEMORY_SIZE) {
        printf("[ERROR] write_memory: address %d out of range\n", addr);
        return;
    }
    cpu->memory[addr] = value;
}

int32_t read_register(Processor *cpu, int reg_num) {
    if (reg_num < 0 || reg_num >= NUM_REGISTERS) {
        printf("[ERROR] read_register: R%d out of range\n", reg_num);
        return 0;
    }
    return (reg_num == ZERO_REGISTER) ? 0 : cpu->registers[reg_num];
}

void write_register(Processor *cpu, int reg_num, int32_t value) {
    if (reg_num < 0 || reg_num >= NUM_REGISTERS) {
        printf("[ERROR] write_register: R%d out of range\n", reg_num);
        return;
    }
    if (reg_num == ZERO_REGISTER) return;  // R0 is always 0
    cpu->registers[reg_num] = value;
}

void print_registers(Processor *cpu) {
    printf("\n===== REGISTERS =====\n");
    for (int i = 0; i < NUM_REGISTERS; i++) {
        int32_t val = (i == ZERO_REGISTER) ? 0 : cpu->registers[i];
        printf("  R%-2d = %d\n", i, val);
    }
    printf("  PC  = %d\n", cpu->PC);
    printf("=====================\n");
}

void print_instruction_memory(Processor *cpu) {
    printf("\n===== INSTRUCTION MEMORY (0-1023) =====\n");

    for (int i = INSTR_MEM_START; i <= INSTR_MEM_END; i++) {
        printf("  MEM[%4d] = 0x%08X\n", i, (uint32_t)cpu->memory[i]);
    }

    printf("========================================\n");
}

void print_data_memory(Processor *cpu) {
    printf("\n===== DATA MEMORY (1024-2047) =====\n");

    for (int i = DATA_MEM_START; i <= DATA_MEM_END; i++) {
        printf("  MEM[%4d] = %d\n", i, cpu->memory[i]);
    }

    printf("====================================\n");
}

void print_full_memory(Processor *cpu) {
    print_instruction_memory(cpu);
    print_data_memory(cpu);
}