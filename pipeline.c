#include <stdio.h>
#include <string.h>
#include "processor.h"
#include "instructions.h"
#include "pipeline.h"
#include "printer.h"

/* ============================================================
 * WHAT IS A HAZARD?
 *
 * A hazard is a situation where the pipeline cannot proceed
 * correctly because of a dependency between instructions.
 * There are two types relevant to this project:
 *
 * 1. DATA HAZARD
 *    Occurs when an instruction needs a register value that a
 *    previous instruction has not yet written back.
 *
 *    Example:
 *      MOVI R1 10      <- writes R1 in WB (cycle 8)
 *      ADD  R3 R1 R2   <- reads R1 in ID (cycle 4) -- TOO EARLY
 *
 *    ADD reads R1 during its ID stage, but MOVI has not finished
 *    WB yet, so ADD reads the OLD value of R1 (which is 0).
 *    The result will be wrong. This is a data hazard.
 *
 *    Solutions exist (stalling, forwarding) but the spec says
 *    we do NOT need to handle data hazards in this project.
 *    The programmer is responsible for spacing instructions out.
 *
 * 2. CONTROL HAZARD (we DO handle this)
 *    Occurs when a branch or jump changes the PC, but the pipeline
 *    has already started fetching the wrong instructions.
 *
 *    Example:
 *      JEQ R0 R0 5     <- branch taken, jumps to PC=7
 *      ADD R1 R2 R3    <- fetched by mistake (in ID when branch resolves)
 *      SUB R4 R5 R6    <- fetched by mistake (in IF when branch resolves)
 *
 *    We handle this by FLUSHING (cancelling) the instructions in
 *    IF and ID when the branch finishes EX. They are marked with
 *    flushed=1 and pass through the remaining stages doing nothing.
 * ============================================================ */

/* ============================================================
 * Stage index constants and their durations in clock cycles
 * ============================================================ */
#define STAGE_IF   0
#define STAGE_ID   1
#define STAGE_EX   2
#define STAGE_MEM  3
#define STAGE_WB   4
#define NUM_STAGES 5

static const int  STAGE_DURATION[NUM_STAGES] = {2, 2, 2, 1, 1};
static const char *STAGE_NAME[NUM_STAGES]    = {"IF", "ID", "EX", "MEM", "WB"};

/* ============================================================
 * Pipeline slot: one instruction travelling through the stages
 * ============================================================ */
typedef struct {
    int valid;          /* 1 = there is an instruction in this slot */
    int flushed;        /* 1 = cancelled by a branch/jump, passes through doing nothing */
    int stage;          /* current stage (STAGE_IF to STAGE_WB) */
    int cycleInStage;   /* how many cycles spent in the current stage so far (0-based) */
    DecodedInstruction d;
} PipelineSlot;

/* Up to 5 instructions can be in the pipeline at once */
#define MAX_SLOTS 5
static PipelineSlot pipeline[MAX_SLOTS];

/* ============================================================
 * Pipeline diagram tracker
 *
 * We record what stage each instruction was in for every cycle,
 * then print a visual table at the end. This makes it easy to
 * see the pipeline flow at a glance.
 *
 * diagram[instrIndex][cycle] holds a short label like "IF","ID","EX",
 * "ME","WB","--" (cancelled), or "  " (not yet / already done).
 * ============================================================ */
#define MAX_DIAG_INSTRS 64
#define MAX_DIAG_CYCLES 200

static char diagram[MAX_DIAG_INSTRS][MAX_DIAG_CYCLES][4];
static char diagLabel[MAX_DIAG_INSTRS][32];  /* mnemonic + PC for each instruction */
static int  diagInstrCount = 0;
static int  diagCycleCount = 0;

/* Called once per cycle to snapshot which instruction is in which stage */
static void record_diagram(int cycle) {
    if (cycle > MAX_DIAG_CYCLES) return;
    if (diagCycleCount < cycle) diagCycleCount = cycle;

    /* Nothing to pre-fill -- cells start as "" (memset to 0).
     * The printer treats "" as idle and prints ".". */

    /* Now fill in the actual stage for each active slot */
    for (int s = 0; s < MAX_SLOTS; s++) {
        if (!pipeline[s].valid) continue;
        int idx = pipeline[s].d.instructionPC;
        if (idx < 0 || idx >= MAX_DIAG_INSTRS) continue;
        if (cycle - 1 >= MAX_DIAG_CYCLES) continue;

        /* Register the instruction label the first time we see it */
        if (strlen(diagLabel[idx]) == 0) {
            snprintf(diagLabel[idx], sizeof(diagLabel[idx]),
                     "%s", pipeline[s].d.mnemonic);
            if (idx + 1 > diagInstrCount) diagInstrCount = idx + 1;
        }

        const char *label;
        if (pipeline[s].flushed) {
            label = "--";   /* cancelled instruction */
        } else {
            switch (pipeline[s].stage) {
                case STAGE_IF:  label = "IF"; break;
                case STAGE_ID:  label = "ID"; break;
                case STAGE_EX:  label = "EX"; break;
                case STAGE_MEM: label = "ME"; break;
                case STAGE_WB:  label = "WB"; break;
                default:        label = "??"; break;
            }
        }
        strcpy(diagram[idx][cycle - 1], label);
    }
}

/* ============================================================
 * Helper functions
 * ============================================================ */

static int findFreeSlot(void) {
    for (int i = 0; i < MAX_SLOTS; i++)
        if (!pipeline[i].valid) return i;
    return -1;
}

static PipelineSlot *slotInStage(int stage) {
    for (int i = 0; i < MAX_SLOTS; i++)
        if (pipeline[i].valid && pipeline[i].stage == stage)
            return &pipeline[i];
    return NULL;
}

/* ============================================================
 * Stage work functions
 * ============================================================ */

static void doIF(Processor *cpu, PipelineSlot *slot, int totalInstructions, int *fetchDone) {
    if (slot->cycleInStage == 0) {
        /* First cycle of IF: fetch the instruction from memory */
        if (cpu->PC < 0 || cpu->PC >= totalInstructions) {
            print_no_more_instructions();
            slot->valid = 0;
            *fetchDone  = 1;
            return;
        }
        int instrPC = cpu->PC;
        int32_t raw = read_memory(cpu, instrPC);
        cpu->PC++;
        slot->d = decodeInstruction(cpu, raw, instrPC);
        print_stage_IF(slot->d.mnemonic, instrPC, 0);
    } else {
        /* Second cycle of IF: wait while memory access completes */
        print_stage_IF(slot->d.mnemonic, slot->d.instructionPC, 1);
    }
}

static void doID(Processor *cpu, PipelineSlot *slot) {
    if (slot->flushed) {
        print_flushed("ID", slot->d.mnemonic);
        return;
    }
    /* On the second cycle of ID, read the actual register values.
     * Note: this is where DATA HAZARDS occur. If a previous instruction
     * has not yet reached WB, we read the OLD (stale) value from the
     * register file. The spec does not require us to handle this -- it
     * is expected behaviour for this pipeline design. */
    if (slot->cycleInStage == 1) {
        slot->d.r1Value = read_register(cpu, slot->d.r1);
        slot->d.r2Value = read_register(cpu, slot->d.r2);
        slot->d.r3Value = read_register(cpu, slot->d.r3);
    }
    print_stage_ID(slot->d.mnemonic, slot->d.instructionPC, slot->cycleInStage,
                   slot->d.r1, slot->d.r2, slot->d.r3, slot->d.immediate);
}

static void doEX(Processor *cpu, PipelineSlot *slot) {
    if (slot->flushed) {
        print_flushed("EX", slot->d.mnemonic);
        return;
    }
    print_stage_EX(slot->d.mnemonic, slot->cycleInStage);

    /* Actual computation only on the second EX cycle */
    if (slot->cycleInStage != 1) return;

    DecodedInstruction *d = &slot->d;
    switch (d->opcode) {
        case 0:  d->result = d->r2Value + d->r3Value;
                 print_ex_alu("+", d->r1, d->r2Value, d->r3Value, d->result); break;
        case 1:  d->result = d->r2Value - d->r3Value;
                 print_ex_alu("-", d->r1, d->r2Value, d->r3Value, d->result); break;
        case 2:  d->result = d->r2Value * d->r3Value;
                 print_ex_alu("*", d->r1, d->r2Value, d->r3Value, d->result); break;
        case 3:  d->result = d->immediate;
                 print_ex_movi(d->r1, d->result); break;
        case 4:  /* JEQ -- compare and branch if equal */
            if (d->r1Value == d->r2Value) {
                d->branchTaken  = 1;
                /* instructionPC+1 is the next sequential address (PC was already
                 * incremented during fetch), then we add the offset IMM */
                d->branchTarget = d->instructionPC + 1 + d->immediate;
                cpu->PC = d->branchTarget;
                print_ex_jeq_taken(d->instructionPC, cpu->PC);
            } else {
                print_ex_jeq_not_taken();
            }
            break;
        case 5:  d->result = d->r2Value & d->r3Value;
                 print_ex_alu("&", d->r1, d->r2Value, d->r3Value, d->result); break;
        case 6:  d->result = d->r2Value ^ d->immediate;
                 print_ex_alu("^", d->r1, d->r2Value, d->immediate, d->result); break;
        case 7:  /* JMP -- unconditional jump */
            d->branchTaken  = 1;
            /* Upper 4 bits come from current PC, lower 28 from the instruction */
            d->branchTarget = (cpu->PC & 0xF0000000) | d->address;
            cpu->PC = d->branchTarget;
            print_ex_jmp(cpu->PC); break;
        case 8:  d->result = d->r2Value << d->shamt;
                 print_ex_shift("<<", d->r1, d->r2Value, d->shamt, d->result); break;
        case 9:  d->result = (int32_t)((uint32_t)d->r2Value >> d->shamt);
                 print_ex_shift(">>", d->r1, d->r2Value, d->shamt, d->result); break;
        case 10: d->memoryAddress = d->r2Value + d->immediate;
                 print_ex_mem_addr("MOVR", d->r2, d->immediate, d->memoryAddress); break;
        case 11: d->memoryAddress = d->r2Value + d->immediate;
                 print_ex_mem_addr("MOVM", d->r2, d->immediate, d->memoryAddress); break;
    }
}

static void doMEM(Processor *cpu, PipelineSlot *slot) {
    if (slot->flushed) {
        print_flushed("MEM", slot->d.mnemonic);
        return;
    }
    print_stage_MEM(slot->d.mnemonic);
    DecodedInstruction *d = &slot->d;
    if (d->opcode == 10) {
        /* MOVR: load a value from data memory into a register */
        d->result = read_memory(cpu, d->memoryAddress);
        print_mem_load(d->memoryAddress, d->result, d->r1);
    } else if (d->opcode == 11) {
        /* MOVM: store a register value into data memory */
        write_memory(cpu, d->memoryAddress, d->r1Value);
        print_mem_store(d->r1, d->r1Value, d->memoryAddress);
        print_memory_update(d->memoryAddress, d->r1Value);
    } else {
        print_mem_no_op();
    }
}

static void doWB(Processor *cpu, PipelineSlot *slot) {
    if (slot->flushed) {
        print_flushed("WB", slot->d.mnemonic);
        return;
    }
    print_stage_WB(slot->d.mnemonic);
    DecodedInstruction *d = &slot->d;

    /* These opcodes produce a result that gets written back to a register */
    int writesRegister = (d->opcode == 0  || d->opcode == 1  || d->opcode == 2  ||
                          d->opcode == 3  || d->opcode == 5  || d->opcode == 6  ||
                          d->opcode == 8  || d->opcode == 9  || d->opcode == 10);
    if (writesRegister) {
        if (d->r1 == 0) {
            /* R0 is hardwired to 0 -- write is blocked */
            printf("  [R0 PROTECTED] write blocked (value stays 0)\n");
        } else {
            write_register(cpu, d->r1, d->result);
            print_register_update(d->r1, d->result);
        }
    }
}

/* ============================================================
 * Flush: cancel any instructions in IF or ID when a branch/jump
 * is taken. Called after the 2nd EX cycle of the branch/jump.
 * We only flush IF and ID -- EX is the branch itself and must
 * finish. MEM and WB are earlier instructions that should complete.
 * ============================================================ */
static void flushPipeline(void) {
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (!pipeline[i].valid) continue;
        if (pipeline[i].stage == STAGE_IF || pipeline[i].stage == STAGE_ID) {
            print_flush(STAGE_NAME[pipeline[i].stage], pipeline[i].d.mnemonic);
            pipeline[i].flushed = 1;
        }
    }
}

/* ============================================================
 * Advance all slots by one cycle at end of each cycle.
 * If a slot finished its stage, move it to the next.
 * If it finished WB, remove it from the pipeline.
 * ============================================================ */
static void advanceSlots(void) {
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (!pipeline[i].valid) continue;
        pipeline[i].cycleInStage++;
        if (pipeline[i].cycleInStage >= STAGE_DURATION[pipeline[i].stage]) {
            if (pipeline[i].stage == STAGE_WB) {
                pipeline[i].valid = 0;   /* instruction completed */
            } else {
                pipeline[i].stage++;
                pipeline[i].cycleInStage = 0;
            }
        }
    }
}

/* ============================================================
 * Main pipeline loop
 * ============================================================ */
void runPipeline(Processor *cpu, int totalInstructions) {
    memset(pipeline, 0, sizeof(pipeline));
    memset(diagram,  0, sizeof(diagram));
    memset(diagLabel, 0, sizeof(diagLabel));
    diagInstrCount = 0;
    diagCycleCount = 0;

    int cycle     = 0;
    int fetchDone = 0;

    printf("\n===== PIPELINE EXECUTION =====\n");

    while (1) {
        cycle++;
        print_cycle_start(cycle);

        /* Find which slot is in each stage this cycle */
        PipelineSlot *wb     = slotInStage(STAGE_WB);
        PipelineSlot *mem    = slotInStage(STAGE_MEM);
        PipelineSlot *ex     = slotInStage(STAGE_EX);
        PipelineSlot *id     = slotInStage(STAGE_ID);
        PipelineSlot *ifSlot = slotInStage(STAGE_IF);

        /* Execute each stage (WB down to IF, so writes happen before reads) */
        if (wb)     doWB(cpu, wb);
        if (mem)    doMEM(cpu, mem);
        if (ex)     doEX(cpu, ex);
        if (id)     doID(cpu, id);

        /* IF fetches every 2 cycles regardless of MEM.
         * The spec table shows IF and MEM coexisting on the same cycle --
         * the hardware interleaves them on the shared memory bus. */
        if (ifSlot) {
            doIF(cpu, ifSlot, totalInstructions, &fetchDone);
        } else if (!fetchDone) {
            int slot = findFreeSlot();
            if (slot >= 0) {
                pipeline[slot].valid        = 1;
                pipeline[slot].flushed      = 0;
                pipeline[slot].stage        = STAGE_IF;
                pipeline[slot].cycleInStage = 0;
                doIF(cpu, &pipeline[slot], totalInstructions, &fetchDone);
                if (!pipeline[slot].valid) fetchDone = 1;
            }
        }

        /* If a branch/jump just finished its 2nd EX cycle and was taken,
         * cancel the instructions that entered the pipeline after it. */
        if (ex && ex->cycleInStage == 1 && !ex->flushed && ex->d.branchTaken) {
            flushPipeline();
            fetchDone = 0;   /* start fetching from the new PC */
        }

        /* Record the current pipeline state for the diagram */
        record_diagram(cycle);

        /* Check if the last instruction just finished WB this cycle */
        int lastWBdone = 0;
        if (fetchDone) {
            lastWBdone = 1;
            for (int i = 0; i < MAX_SLOTS; i++) {
                if (!pipeline[i].valid) continue;
                if (pipeline[i].stage != STAGE_WB ||
                    pipeline[i].cycleInStage < STAGE_DURATION[STAGE_WB] - 1) {
                    lastWBdone = 0;
                    break;
                }
            }
        }

        /* Advance every slot by one cycle */
        advanceSlots();

        /* Stop once the last instruction finishes WB */
        if (lastWBdone) {
            print_pipeline_done(cycle, totalInstructions);
            break;
        }

        if (cycle > 2000) {
            printf("\n[WARNING] Exceeded 2000 cycles -- stopping.\n");
            break;
        }
    }

    /* Print the pipeline diagram */
    print_pipeline_diagram(diagram, diagLabel, diagInstrCount, diagCycleCount);
}