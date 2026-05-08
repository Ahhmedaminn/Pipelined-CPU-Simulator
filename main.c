#include <stdio.h>
#include <string.h>
#include <stdint.h>

// ---------------------------------------------------------------
// Same memory layout as processor.c
// ---------------------------------------------------------------
#define MEMORY_SIZE      2048
#define NUM_REGISTERS    32

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

// Forward declaration of parseAndLoad from parser.c
int parseAndLoad(int32_t *memory, int maxInstructions);

// ---------------------------------------------------------------
// Converts an int32 back to a 32-character binary string
// so we can verify what is actually stored in memory
// ---------------------------------------------------------------
void intToBinaryString(int32_t value, char *bin) {
    bin[32] = '\0';
    for (int bit = 31; bit >= 0; bit--) {
        bin[31 - bit] = ((value >> bit) & 1) ? '1' : '0';
    }
}

int main() {

    // Create and initialize the processor
    Processor cpu;
    init_processor(&cpu);

    // Load instructions from program.txt into processor memory
    int total = parseAndLoad(cpu.memory, 1024);

    if (total == -1) {
        printf("Failed to load program.\n");
        return 1;
    }

    // ---------------------------------------------------------------
    // Verify: read each instruction back out of processor memory
    // and print it as binary so we can confirm it was stored correctly
    // ---------------------------------------------------------------
    printf("===== Verification: Instructions in Processor Memory =====\n");
    for (int i = 0; i < total; i++) {
        char bin[33];
        intToBinaryString(cpu.memory[i], bin);
        printf("cpu.memory[%2d] = %s  (0x%08X)\n", i, bin, (uint32_t)cpu.memory[i]);
    }

    // Confirm registers are all zero after loading
    printf("\n===== Registers (should all be 0) =====\n");
    for (int i = 0; i < NUM_REGISTERS; i++) {
        printf("R%-2d = %d\n", i, cpu.registers[i]);
    }
    printf("PC  = %d\n", cpu.PC);

    return 0;
}