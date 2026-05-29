#ifndef PRINTER_H
#define PRINTER_H

#include "processor.h"
#include "instructions.h"

/* ---- Cycle / pipeline stage prints ---- */
void print_cycle_start(int cycle);
void print_pipeline_done(int cycle, int totalInstructions);

void print_stage_IF(const char *mnemonic, int instrPC, int cycleInStage);
void print_stage_ID(const char *mnemonic, int instrPC, int cycleInStage, int r1, int r2, int r3, int imm);
void print_stage_EX(const char *mnemonic, int cycleInStage);
void print_stage_MEM(const char *mnemonic);
void print_stage_WB(const char *mnemonic);

void print_flushed(const char *stage, const char *mnemonic);
void print_no_more_instructions(void);

/* ---- EX result prints ---- */
void print_ex_alu(const char *op, int rd, int32_t a, int32_t b, int32_t result);
void print_ex_movi(int rd, int32_t imm);
void print_ex_shift(const char *op, int rd, int32_t val, int shamt, int32_t result);
void print_ex_mem_addr(const char *mnemonic, int r2, int32_t imm, int32_t addr);
void print_ex_jeq_taken(int instrPC, int newPC);
void print_ex_jeq_not_taken(void);
void print_ex_jmp(int newPC);

/* ---- MEM result prints ---- */
void print_mem_load(int addr, int32_t value, int rd);
void print_mem_store(int r1, int32_t value, int addr);
void print_mem_no_op(void);

/* ---- Update prints ---- */
void print_register_update(int reg, int32_t value);
void print_memory_update(int addr, int32_t value);
void print_flush(const char *stageName, const char *mnemonic);

/* ---- Pipeline diagram ---- */
void print_pipeline_diagram(char diagram[][200][4], char labels[][32], int instrCount, int cycleCount);

/* ---- Final state prints ---- */
void print_all_registers(Processor *cpu);
void print_instruction_memory(Processor *cpu, int total);
void print_data_memory_full(Processor *cpu);
void print_data_memory_nonzero(Processor *cpu);

#endif