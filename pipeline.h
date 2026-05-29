#ifndef PIPELINE_H
#define PIPELINE_H

#include "processor.h"

/* Runs the 5-stage pipeline (IF, ID, EX, MEM, WB) on the instructions
   already loaded into cpu->memory[0..totalInstructions-1]. */
void runPipeline(Processor *cpu, int totalInstructions);

#endif
