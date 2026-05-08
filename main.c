#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Pull in the Processor type and functions from processor.c
#define MEMORY_SIZE      2048
#define NUM_REGISTERS    32
#define ZERO_REGISTER    0

typedef struct {
    int32_t memory[MEMORY_SIZE];
    int32_t registers[NUM_REGISTERS];
    int32_t PC;
} Processor;

// Forward declarations
void init_processor(Processor *cpu);
int  parseAndLoad(int32_t *memory, int maxInstructions);

// Converts an int32 back to a 32-char binary string for verification
void intToBinaryString(int32_t value, char *bin) {
    bin[32] = '\0';
    for (int bit = 31; bit >= 0; bit--) {
        bin[31 - bit] = ((value >> bit) & 1) ? '1' : '0';
    }
}

int main() {

    Processor cpu;
    init_processor(&cpu);

    // Parse program.txt and load instructions into processor memory
    int total = parseAndLoad(cpu.memory, 1024);
    if (total == -1) {
        printf("Failed to load program.\n");
        return 1;
    }

    // -------------------------------------------------------
    // Verification: read each slot back out of processor memory
    // and convert to binary to confirm it was stored correctly
    // -------------------------------------------------------
    printf("===== Verification: Instructions in Processor Memory =====\n");
    for (int i = 0; i < total; i++) {
        char bin[33];
        intToBinaryString(cpu.memory[i], bin);
        printf("cpu.memory[%2d] = %s  (0x%08X)\n", i, bin, (uint32_t)cpu.memory[i]);
    }

    // Confirm everything else is zero
    printf("\n===== Registers (all should be 0) =====\n");
    for (int i = 0; i < NUM_REGISTERS; i++) {
        printf("  R%-2d = %d\n", i, cpu.registers[i]);
    }
    printf("  PC  = %d\n", cpu.PC);

    return 0;
}