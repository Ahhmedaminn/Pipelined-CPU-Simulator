#include <stdio.h>
#include <stdint.h>
#include <string.h>

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
int32_t read_memory(Processor *cpu, int32_t addr);
void write_memory(Processor *cpu, int32_t addr, int32_t value);

int32_t read_register(Processor *cpu, int reg_num);
void write_register(Processor *cpu, int reg_num, int32_t value);

/*
 * This struct stores the decoded fields of the 32-bit instruction.
 * This is created AFTER fetching the encoded instruction from memory.
 */
typedef struct {
    int32_t rawInstruction;

    int opcode;

    int r1;
    int r2;
    int r3;

    int shamt;
    int immediate;
    int address;

    int32_t r1Value;
    int32_t r2Value;
    int32_t r3Value;

    int32_t result;
    int32_t memoryAddress;
    int32_t memoryData;

    int branchTaken;
    int branchTarget;

    char mnemonic[10];
} DecodedInstruction;

int signExtend18(int value) {
    if ((value & (1 << 17)) != 0) {
        value = value | ~0x3FFFF;
    }

    return value;
}

const char *getMnemonic(int opcode) {
    switch (opcode) {
        case 0:  return "ADD";
        case 1:  return "SUB";
        case 2:  return "MUL";
        case 3:  return "MOVI";
        case 4:  return "JEQ";
        case 5:  return "AND";
        case 6:  return "XORI";
        case 7:  return "JMP";
        case 8:  return "LSL";
        case 9:  return "LSR";
        case 10: return "MOVR";
        case 11: return "MOVM";
        default: return "UNKNOWN";
    }
}

DecodedInstruction decodeInstruction(Processor *cpu, int32_t rawInstruction, int instructionPC) {
    DecodedInstruction decoded;
    uint32_t instruction = (uint32_t) rawInstruction;

    decoded.rawInstruction = rawInstruction;

    decoded.opcode = (instruction >> 28) & 0xF;

    decoded.r1 = (instruction >> 23) & 0x1F;
    decoded.r2 = (instruction >> 18) & 0x1F;
    decoded.r3 = (instruction >> 13) & 0x1F;

    decoded.shamt = instruction & 0x1FFF;

    int imm18 = instruction & 0x3FFFF;
    decoded.immediate = signExtend18(imm18);

    decoded.address = instruction & 0x0FFFFFFF;

    decoded.r1Value = read_register(cpu, decoded.r1);
    decoded.r2Value = read_register(cpu, decoded.r2);
    decoded.r3Value = read_register(cpu, decoded.r3);

    decoded.result = 0;
    decoded.memoryAddress = 0;
    decoded.memoryData = 0;

    decoded.branchTaken = 0;
    decoded.branchTarget = -1;

    strcpy(decoded.mnemonic, getMnemonic(decoded.opcode));

    printf("\n[DECODE] PC=%d Raw=0x%08X Instruction=%s\n",
           instructionPC, instruction, decoded.mnemonic);

    printf("         opcode=%d r1=%d r2=%d r3=%d shamt=%d imm=%d address=%d\n",
           decoded.opcode,
           decoded.r1,
           decoded.r2,
           decoded.r3,
           decoded.shamt,
           decoded.immediate,
           decoded.address);

    return decoded;
}

void executeInstruction(Processor *cpu, DecodedInstruction *decoded, int instructionPC) {
    printf("[EXECUTE] %s\n", decoded->mnemonic);

    switch (decoded->opcode) {
        case 0:
            decoded->result = decoded->r2Value + decoded->r3Value;
            write_register(cpu, decoded->r1, decoded->result);
            printf("          R%d = R%d + R%d = %d\n",
                   decoded->r1, decoded->r2, decoded->r3, decoded->result);
            break;

        case 1:
            decoded->result = decoded->r2Value - decoded->r3Value;
            write_register(cpu, decoded->r1, decoded->result);
            printf("          R%d = R%d - R%d = %d\n",
                   decoded->r1, decoded->r2, decoded->r3, decoded->result);
            break;

        case 2:
            decoded->result = decoded->r2Value * decoded->r3Value;
            write_register(cpu, decoded->r1, decoded->result);
            printf("          R%d = R%d * R%d = %d\n",
                   decoded->r1, decoded->r2, decoded->r3, decoded->result);
            break;

        case 3:
            decoded->result = decoded->immediate;
            write_register(cpu, decoded->r1, decoded->result);
            printf("          R%d = IMM = %d\n", decoded->r1, decoded->result);
            break;

        case 4:
            if (decoded->r1Value == decoded->r2Value) {
                decoded->branchTaken = 1;
                decoded->branchTarget = instructionPC + 1 + decoded->immediate;
                cpu->PC = decoded->branchTarget;

                printf("          JEQ taken: R%d == R%d, PC = %d\n",
                       decoded->r1, decoded->r2, cpu->PC);
            } else {
                printf("          JEQ not taken: R%d != R%d\n",
                       decoded->r1, decoded->r2);
            }
            break;

        case 5:
            decoded->result = decoded->r2Value & decoded->r3Value;
            write_register(cpu, decoded->r1, decoded->result);
            printf("          R%d = R%d & R%d = %d\n",
                   decoded->r1, decoded->r2, decoded->r3, decoded->result);
            break;

        case 6:
            decoded->result = decoded->r2Value ^ decoded->immediate;
            write_register(cpu, decoded->r1, decoded->result);
            printf("          R%d = R%d XOR IMM = %d\n",
                   decoded->r1, decoded->r2, decoded->result);
            break;

        case 7:
            decoded->branchTaken = 1;
            decoded->branchTarget = (cpu->PC & 0xF0000000) | decoded->address;
            cpu->PC = decoded->branchTarget;

            printf("          JMP: PC = %d\n", cpu->PC);
            break;

        case 8:
            decoded->result = decoded->r2Value << decoded->shamt;
            write_register(cpu, decoded->r1, decoded->result);
            printf("          R%d = R%d << %d = %d\n",
                   decoded->r1, decoded->r2, decoded->shamt, decoded->result);
            break;

        case 9:
            decoded->result = (int32_t)(((uint32_t) decoded->r2Value) >> decoded->shamt);
            write_register(cpu, decoded->r1, decoded->result);
            printf("          R%d = R%d >>> %d = %d\n",
                   decoded->r1, decoded->r2, decoded->shamt, decoded->result);
            break;

        case 10:
            decoded->memoryAddress = decoded->r2Value + decoded->immediate;
            decoded->memoryData = read_memory(cpu, decoded->memoryAddress);
            write_register(cpu, decoded->r1, decoded->memoryData);

            printf("          MOVR: R%d = MEM[%d] = %d\n",
                   decoded->r1, decoded->memoryAddress, decoded->memoryData);
            break;

        case 11:
            decoded->memoryAddress = decoded->r2Value + decoded->immediate;
            write_memory(cpu, decoded->memoryAddress, decoded->r1Value);

            printf("          MOVM: MEM[%d] = R%d = %d\n",
                   decoded->memoryAddress, decoded->r1, decoded->r1Value);
            break;

        default:
            printf("          Unknown opcode: %d\n", decoded->opcode);
            break;
    }
}

/*
 * Temporary Part C test.
 * This executes instructions normally one by one.
 * Later, Part D will replace this with the real pipeline.
 */
void runInstructionExecutionTest(Processor *cpu, int totalInstructions, int maxSteps) {
    int steps = 0;

    cpu->PC = 0;

    printf("\n===== PART C EXECUTION TEST =====\n");

    while (cpu->PC >= 0 && cpu->PC < totalInstructions && steps < maxSteps) {
        int instructionPC = cpu->PC;
        int32_t rawInstruction = read_memory(cpu, instructionPC);

        cpu->PC++;

        DecodedInstruction decoded = decodeInstruction(cpu, rawInstruction, instructionPC);
        executeInstruction(cpu, &decoded, instructionPC);

        steps++;
    }

    if (steps >= maxSteps) {
        printf("\n[WARNING] Execution stopped because maxSteps was reached.\n");
        printf("          This may happen if your program has JMP/loop instructions.\n");
    }

    printf("\n===== END OF PART C EXECUTION TEST =====\n");
}