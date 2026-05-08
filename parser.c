#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

void toBinary(uint32_t number, int bits, char *output, int startIndex) {
    for (int i = bits - 1; i >= 0; i--) {
        output[startIndex++] = ((number >> i) & 1) ? '1' : '0';
    }
}

int getRegisterNumber(char *registerName) {
    if (registerName[0] != 'R') {
        printf("[ERROR] Invalid register format: %s\n", registerName);
        return -1;
    }

    int reg = atoi(registerName + 1);

    if (reg < 0 || reg >= 32) {
        printf("[ERROR] Register out of range: %s\n", registerName);
        return -1;
    }

    return reg;
}

int32_t binaryStringToInt(char *bin) {
    int32_t result = 0;
    for (int i = 0; i < 32; i++) {
        result = (result << 1) | (bin[i] == '1' ? 1 : 0);
    }
    return result;
}

int parseAndLoad(int32_t *memory, int maxInstructions) {

    FILE *file = fopen("program.txt", "r");
    if (file == NULL) {
        printf("Error: could not open program.txt\n");
        return -1;
    }

    char line[100];
    int instructionCount = 0;

    while (fgets(line, sizeof(line), file)) {

        char instruction[20] = "";
        char op1[20] = "";
        char op2[20] = "";
        char op3[20] = "";

        sscanf(line, "%s %s %s %s", instruction, op1, op2, op3);

if (strlen(instruction) == 0) continue;
if (instruction[0] == '#') continue;
if (instruction[0] == '/') continue;

        int opcode = -1;
        if (strcmp(instruction, "ADD")  == 0) opcode = 0;
        if (strcmp(instruction, "SUB")  == 0) opcode = 1;
        if (strcmp(instruction, "MUL")  == 0) opcode = 2;
        if (strcmp(instruction, "MOVI") == 0) opcode = 3;
        if (strcmp(instruction, "JEQ")  == 0) opcode = 4;
        if (strcmp(instruction, "AND")  == 0) opcode = 5;
        if (strcmp(instruction, "XORI") == 0) opcode = 6;
        if (strcmp(instruction, "JMP")  == 0) opcode = 7;
        if (strcmp(instruction, "LSL")  == 0) opcode = 8;
        if (strcmp(instruction, "LSR")  == 0) opcode = 9;
        if (strcmp(instruction, "MOVR") == 0) opcode = 10;
        if (strcmp(instruction, "MOVM") == 0) opcode = 11;

        if (opcode == -1) {
            printf("Unknown instruction: %s\n", instruction);
            continue;
        }

        char bin[33];
        bin[32] = '\0';

        // R-TYPE: ADD, SUB, MUL, AND
        // [ OPCODE(4) | RD(5) | RS(5) | RT(5) | SHAMT(13) ]
        if (strcmp(instruction, "ADD") == 0 ||
            strcmp(instruction, "SUB") == 0 ||
            strcmp(instruction, "MUL") == 0 ||
            strcmp(instruction, "AND") == 0) {
            toBinary(opcode,                  4,  bin,  0);
            toBinary(getRegisterNumber(op1),  5,  bin,  4);
            toBinary(getRegisterNumber(op2),  5,  bin,  9);
            toBinary(getRegisterNumber(op3),  5,  bin, 14);
            toBinary(0,                       13, bin, 19);
        }

        // SHIFT: LSL, LSR
        // [ OPCODE(4) | RD(5) | RS(5) | 00000(5) | SHAMT(13) ]
        else if (strcmp(instruction, "LSL") == 0 ||
                 strcmp(instruction, "LSR") == 0) {
            toBinary(opcode,                  4,  bin,  0);
            toBinary(getRegisterNumber(op1),  5,  bin,  4);
            toBinary(getRegisterNumber(op2),  5,  bin,  9);
            toBinary(0,                       5,  bin, 14);
            toBinary(atoi(op3),               13, bin, 19);
        }

        // I-TYPE: MOVI
        // [ OPCODE(4) | R1(5) | 00000(5) | IMMEDIATE(18) ]
        else if (strcmp(instruction, "MOVI") == 0) {
            toBinary(opcode,                  4,  bin,  0);
            toBinary(getRegisterNumber(op1),  5,  bin,  4);
            toBinary(0,                       5,  bin,  9);
            toBinary(atoi(op2) & 0x3FFFF,     18, bin, 14);
        }

        // I-TYPE: JEQ, XORI, MOVR, MOVM
        // [ OPCODE(4) | R1(5) | R2(5) | IMMEDIATE(18) ]
        else if (strcmp(instruction, "JEQ")  == 0 ||
                 strcmp(instruction, "XORI") == 0 ||
                 strcmp(instruction, "MOVR") == 0 ||
                 strcmp(instruction, "MOVM") == 0) {
            toBinary(opcode,                  4,  bin,  0);
            toBinary(getRegisterNumber(op1),  5,  bin,  4);
            toBinary(getRegisterNumber(op2),  5,  bin,  9);
            toBinary(atoi(op3) & 0x3FFFF,     18, bin, 14);
        }

        // J-TYPE: JMP
        // [ OPCODE(4) | ADDRESS(28) ]
        else if (strcmp(instruction, "JMP") == 0) {
            toBinary(opcode,                         4,  bin,  0);
            toBinary(atoi(op1) & 0x0FFFFFFF,         28, bin,  4);;
        }

        memory[instructionCount] = binaryStringToInt(bin);

        line[strcspn(line, "\n")] = '\0';
        printf("[Parser] %2d: %-20s -> %s\n", instructionCount, line, bin);

        instructionCount++;
        if (instructionCount >= maxInstructions) {
            printf("Instruction memory is full.\n");
            break;
        }
    }

    fclose(file);
    printf("\nTotal instructions loaded: %d\n\n", instructionCount);
    return instructionCount;
}