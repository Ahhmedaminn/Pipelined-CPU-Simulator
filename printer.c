#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "printer.h"

/* ============================================================
 * Cycle / pipeline stage prints
 * ============================================================ */

void print_cycle_start(int cycle) {
    printf("\n--- Cycle %d ---\n", cycle);
}

void print_pipeline_done(int cycle, int totalInstructions) {
    printf("\n--- Pipeline empty. Done. ---\n");
    printf("\nTotal clock cycles: %d\n", cycle);
    printf("Expected (formula): %d\n", 8 + (totalInstructions - 1) * 2);
    printf("===== END PIPELINE =====\n");
}

void print_stage_IF(const char *mnemonic, int instrPC, int cycleInStage) {
    if (cycleInStage == 0)
        printf("  [IF ] cycle 1/2 -- fetched %s (PC=%d)\n", mnemonic, instrPC);
    else
        printf("  [IF ] cycle 2/2 -- %s (PC=%d)\n", mnemonic, instrPC);
}

void print_stage_ID(const char *mnemonic, int instrPC, int cycleInStage,
                    int r1, int r2, int r3, int imm) {
    printf("  [ID ] cycle %d/2 -- %s (PC=%d)  r1=%d r2=%d r3=%d imm=%d\n",
           cycleInStage + 1, mnemonic, instrPC, r1, r2, r3, imm);
}

void print_stage_EX(const char *mnemonic, int cycleInStage) {
    printf("  [EX ] cycle %d/2 -- %s\n", cycleInStage + 1, mnemonic);
}

void print_stage_MEM(const char *mnemonic) {
    printf("  [MEM] %s\n", mnemonic);
}

void print_stage_WB(const char *mnemonic) {
    printf("  [WB ] %s\n", mnemonic);
}

void print_flushed(const char *stage, const char *mnemonic) {
    printf("  [%s] (cancelled) -- %s\n", stage, mnemonic);
}

void print_no_more_instructions(void) {
    printf("  [IF ] no more instructions\n");
}

/* ============================================================
 * EX result prints
 * ============================================================ */

void print_ex_alu(const char *op, int rd, int32_t a, int32_t b, int32_t result) {
    printf("         R%d = %d %s %d = %d\n", rd, a, op, b, result);
}

void print_ex_movi(int rd, int32_t imm) {
    printf("         R%d = IMM = %d\n", rd, imm);
}

void print_ex_shift(const char *op, int rd, int32_t val, int shamt, int32_t result) {
    printf("         R%d = %d %s %d = %d\n", rd, val, op, shamt, result);
}

void print_ex_mem_addr(const char *mnemonic, int r2, int32_t imm, int32_t addr) {
    printf("         %s: addr = R%d + %d = %d\n", mnemonic, r2, imm, addr);
}

void print_ex_jeq_taken(int instrPC, int newPC) {
    /* Show the full calculation so the examiner can verify:
     * target = instrPC + 1 + IMM
     * instrPC+1 is the next sequential address (PC was already incremented
     * during fetch, so we use the stored instructionPC here) */
    printf("         JEQ taken -- PC was %d, jumping to %d\n", instrPC + 1, newPC);
}

void print_ex_jeq_not_taken(void) {
    printf("         JEQ not taken -- continuing sequentially\n");
}

void print_ex_jmp(int newPC) {
    printf("         JMP -- jumping to %d\n", newPC);
}

/* ============================================================
 * MEM result prints
 * ============================================================ */

void print_mem_load(int addr, int32_t value, int rd) {
    printf("         load MEM[%d] = %d -> R%d\n", addr, value, rd);
}

void print_mem_store(int r1, int32_t value, int addr) {
    printf("         store R%d = %d -> MEM[%d]\n", r1, value, addr);
}

void print_mem_no_op(void) {
    printf("         (no memory operation)\n");
}

/* ============================================================
 * Update prints
 * ============================================================ */

void print_register_update(int reg, int32_t value) {
    printf("  [REGISTER UPDATE] R%d = %d\n", reg, value);
}

void print_memory_update(int addr, int32_t value) {
    printf("  [MEMORY UPDATE] MEM[%d] = %d\n", addr, value);
}

void print_flush(const char *stageName, const char *mnemonic) {
    printf("  [CANCELLED] cancelling %s in %s\n", mnemonic, stageName);
}

/* ============================================================
 * Pipeline diagram
 *
 * Prints a table showing which stage each instruction was in
 * for every clock cycle. Example:
 *
 *   Instr         |  1  2  3  4  5  6  7  8
 *   PC=0 MOVI R1  | IF IF ID ID EX EX ME WB
 *   PC=1 MOVI R2  |       IF IF ID ID EX EX ...
 *
 * This is the same format as the spec's own pipeline table (page 4).
 * "--" means the instruction was cancelled (branch/jump flush).
 * ============================================================ */
void print_pipeline_diagram(char diagram[][200][4], char labels[][32],
                             int instrCount, int cycleCount) {
    if (instrCount == 0 || cycleCount == 0) return;

    /* Use single-character stage codes so the whole table fits on one line.
     * F=IF  D=ID  E=EX  M=MEM  W=WB  x=cancelled  .=empty */
    printf("\n===== PIPELINE DIAGRAM =====\n");

    /* Header: cycle numbers (units digit only to save space) */
    printf("  %-12s |", "Instruction");
    for (int c = 0; c < cycleCount; c++)
        printf("%d", (c + 1) % 10);
    printf("\n");

    /* Tens row (only printed if cycles go past 9) */
    if (cycleCount > 9) {
        printf("  %-12s |", "");
        for (int c = 0; c < cycleCount; c++)
            printf("%d", (c + 1) / 10);
        printf("\n");
    }

    /* Separator */
    printf("  ");
    for (int i = 0; i < 14 + cycleCount; i++) printf("-");
    printf("\n");

    /* One row per instruction */
    for (int i = 0; i < instrCount; i++) {
        if (strlen(labels[i]) == 0) continue;
        printf("  PC=%-2d %-6s |", i, labels[i]);
        for (int c = 0; c < cycleCount; c++) {
            const char *cell = diagram[i][c];
            if      (strlen(cell) == 0)        printf(".");
            else if (strcmp(cell, "IF") == 0)  printf("F");
            else if (strcmp(cell, "ID") == 0)  printf("D");
            else if (strcmp(cell, "EX") == 0)  printf("E");
            else if (strcmp(cell, "ME") == 0)  printf("M");
            else if (strcmp(cell, "WB") == 0)  printf("W");
            else if (strcmp(cell, "--") == 0)  printf("x");
            else                               printf(".");
        }
        printf("\n");
    }

    printf("\n  F=Fetch D=Decode E=Execute M=Memory W=WriteBack x=Cancelled .=Idle\n");
    printf("===========================\n");
}

/* ============================================================
 * Final state prints
 * ============================================================ */

void print_all_registers(Processor *cpu) {
    printf("\n===== FINAL REGISTERS =====\n");
    for (int i = 0; i < NUM_REGISTERS; i++)
        printf("  R%-2d = %d\n", i, cpu->registers[i]);
    printf("  PC  = %d\n", cpu->PC);
}

void print_instruction_memory(Processor *cpu, int total) {
    const char *names[] = {"ADD","SUB","MUL","MOVI","JEQ","AND","XORI","JMP",
                           "LSL","LSR","MOVR","MOVM"};
    printf("\n===== INSTRUCTION MEMORY (0 to %d) =====\n", total - 1);
    for (int i = 0; i < total; i++) {
        int op = ((uint32_t)cpu->memory[i] >> 28) & 0xF;
        printf("  MEM[%4d] = 0x%08X  (%s)\n", i, (uint32_t)cpu->memory[i],
               op < 12 ? names[op] : "???");
    }
}

void print_data_memory_full(Processor *cpu) {
    printf("\n===== DATA MEMORY (1024-2047) =====\n");
    for (int i = 1024; i <= 2047; i++)
        printf("  MEM[%4d] = %d\n", i, cpu->memory[i]);
}

void print_data_memory_nonzero(Processor *cpu) {
    printf("\n===== DATA MEMORY (non-zero only) =====\n");
    int found = 0;
    for (int i = 1024; i <= 2047; i++) {
        if (cpu->memory[i] != 0) {
            printf("  MEM[%4d] = %d\n", i, cpu->memory[i]);
            found = 1;
        }
    }
    if (!found) printf("  (all zero)\n");
}