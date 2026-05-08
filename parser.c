#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// ---------------------------------------------------------------
// Converts a binary string like "00000000100010000110000000000000"
// into a 32-bit integer so it can be stored in the processor memory.
// ---------------------------------------------------------------
int32_t binaryStringToInt(char *bin) {
    int32_t result = 0;
    for (int i = 0; i < 32; i++) {
        result = result << 1;
        if (bin[i] == '1') {
            result = result | 1;
        }
    }
    return result;
}

// ---------------------------------------------------------------
// Writes a number into a binary string at a given start position.
// Example: toBinary(5, 4, bin, 0) writes "0101" at bin[0..3]
// ---------------------------------------------------------------
void toBinary(int number, int bits, char *output, int startIndex) {
    for (int i = bits - 1; i >= 0; i--) {
        if ((number >> i) & 1)
            output[startIndex] = '1';
        else
            output[startIndex] = '0';
        startIndex++;
    }
}

// ---------------------------------------------------------------
// Reads the register number out of a string like "R5" -> 5
// ---------------------------------------------------------------
int getRegisterNumber(char *registerName) {
    return atoi(registerName + 1);
}

// ---------------------------------------------------------------
// Parses "program.txt" and loads each encoded instruction
// directly into the processor's memory array (as int32_t).
//
// Parameters:
//   memory        - the processor's memory array (int32_t[2048])
//   maxInstructions - max slots to fill (should be 1024)
//
// Returns how many instructions were loaded, or -1 on error.
// ---------------------------------------------------------------
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

        // ---------------------------------------------------
        // Map instruction name to opcode
        // ---------------------------------------------------
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

        // ---------------------------------------------------
        // Build the 32-character binary string
        // ---------------------------------------------------
        char bin[33];
        bin[32] = '\0';

        // R-TYPE: ADD, SUB, MUL, AND
        // [ OPCODE(4) | RD(5) | RS(5) | RT(5) | SHAMT(13) ]
        if (strcmp(instruction, "ADD") == 0 ||
            strcmp(instruction, "SUB") == 0 ||
            strcmp(instruction, "MUL") == 0 ||
            strcmp(instruction, "AND") == 0) {

            int rd = getRegisterNumber(op1);
            int rs = getRegisterNumber(op2);
            int rt = getRegisterNumber(op3);

            toBinary(opcode, 4,  bin,  0);
            toBinary(rd,     5,  bin,  4);
            toBinary(rs,     5,  bin,  9);
            toBinary(rt,     5,  bin, 14);
            toBinary(0,      13, bin, 19);
        }

        // SHIFT: LSL, LSR
        // [ OPCODE(4) | RD(5) | RS(5) | 00000(5) | SHAMT(13) ]
        else if (strcmp(instruction, "LSL") == 0 ||
                 strcmp(instruction, "LSR") == 0) {

            int rd    = getRegisterNumber(op1);
            int rs    = getRegisterNumber(op2);
            int shamt = atoi(op3);

            toBinary(opcode, 4,  bin,  0);
            toBinary(rd,     5,  bin,  4);
            toBinary(rs,     5,  bin,  9);
            toBinary(0,      5,  bin, 14);
            toBinary(shamt,  13, bin, 19);
        }

        // I-TYPE: MOVI
        // [ OPCODE(4) | R1(5) | 00000(5) | IMMEDIATE(18) ]
        else if (strcmp(instruction, "MOVI") == 0) {

            int r1    = getRegisterNumber(op1);
            int imm18 = atoi(op2) & 0x3FFFF;

            toBinary(opcode, 4,  bin,  0);
            toBinary(r1,     5,  bin,  4);
            toBinary(0,      5,  bin,  9);
            toBinary(imm18,  18, bin, 14);
        }

        // I-TYPE: JEQ, XORI, MOVR, MOVM
        // [ OPCODE(4) | R1(5) | R2(5) | IMMEDIATE(18) ]
        else if (strcmp(instruction, "JEQ")  == 0 ||
                 strcmp(instruction, "XORI") == 0 ||
                 strcmp(instruction, "MOVR") == 0 ||
                 strcmp(instruction, "MOVM") == 0) {

            int r1    = getRegisterNumber(op1);
            int r2    = getRegisterNumber(op2);
            int imm18 = atoi(op3) & 0x3FFFF;

            toBinary(opcode, 4,  bin,  0);
            toBinary(r1,     5,  bin,  4);
            toBinary(r2,     5,  bin,  9);
            toBinary(imm18,  18, bin, 14);
        }

        // J-TYPE: JMP
        // [ OPCODE(4) | ADDRESS(28) ]
        else if (strcmp(instruction, "JMP") == 0) {

            int address = (int)strtol(op1, NULL, 16);

            toBinary(opcode,  4,  bin,  0);
            toBinary(address, 28, bin,  4);
        }

        // ---------------------------------------------------
        // Convert the binary string to int32 and store it
        // in the processor's instruction memory
        // ---------------------------------------------------
        int32_t encoded = binaryStringToInt(bin);
        memory[instructionCount] = encoded;

        line[strcspn(line, "\n")] = '\0';
        printf("[Parser] Instruction %2d: %-20s -> %s  (0x%08X)\n",
               instructionCount, line, bin, (uint32_t)encoded);

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