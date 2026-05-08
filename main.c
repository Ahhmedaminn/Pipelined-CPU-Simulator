#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define MEMORY_SIZE      2048
#define NUM_REGISTERS    32
#define ZERO_REGISTER    0

typedef struct {
    int32_t memory[MEMORY_SIZE];
    int32_t registers[NUM_REGISTERS];
    int32_t PC;
} Processor;

/*
 * Functions from processor.c
 */
void init_processor(Processor *cpu);
void print_registers(Processor *cpu);
void print_data_memory(Processor *cpu);

/*
 * Function from parser.c
 */
int parseAndLoad(int32_t *memory, int maxInstructions);

/*
 * Function from instructions.c
 */
void runInstructionExecutionTest(Processor *cpu, int totalInstructions, int maxSteps);

/*
 * Converts an int32 back to a 32-character binary string for verification.
 */
void intToBinaryString(int32_t value, char *bin) {
    bin[32] = '\0';

    for (int bit = 31; bit >= 0; bit--) {
        bin[31 - bit] = ((value >> bit) & 1) ? '1' : '0';
    }
}

int main() {
    Processor cpu;
    init_processor(&cpu);

    /*
     * Parse program.txt and load encoded instructions into processor memory.
     */
    int total = parseAndLoad(cpu.memory, 1024);

    if (total == -1) {
        printf("Failed to load program.\n");
        return 1;
    }

    /*
     * Verify that instructions were stored in memory as 32-bit encoded values.
     */
    printf("===== Verification: Instructions in Processor Memory =====\n");

    for (int i = 0; i < total; i++) {
        char bin[33];
        intToBinaryString(cpu.memory[i], bin);

        printf("cpu.memory[%2d] = %s  (0x%08X)\n",
               i, bin, (uint32_t) cpu.memory[i]);
    }

    /*
     * Before execution, all registers should still be 0.
     */
    printf("\n===== Registers before execution =====\n");

    for (int i = 0; i < NUM_REGISTERS; i++) {
        printf("  R%-2d = %d\n", i, cpu.registers[i]);
    }

    printf("  PC  = %d\n", cpu.PC);

    /*
     * Part C test: decode and execute instructions one by one.
     * This is temporary. Part D will later replace this with pipeline execution.
     */
    runInstructionExecutionTest(&cpu, total, 50);

    /*
     * Print final results after execution.
     */
    print_registers(&cpu);
    print_data_memory(&cpu);

    return 0;
}