#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "parser.h"

/* Write bits of 'number' (using 'bits' many bits) into the output string
   starting at position 'startIndex'. Used to build the 32-bit binary string. */
static void writeBits(uint32_t number, int bits, char *output, int startIndex) {
    for (int i = bits - 1; i >= 0; i--)
        output[startIndex++] = ((number >> i) & 1) ? '1' : '0';
}

/* Parse a register name like "R5" and return the register number (5).
   Returns -1 if the format is invalid. */
static int parseRegister(const char *name) {
    if (name[0] != 'R') {
        printf("[ERROR] Invalid register: %s\n", name);
        return -1;
    }
    int reg = atoi(name + 1);
    if (reg < 0 || reg >= 32) {
        printf("[ERROR] Register out of range: %s\n", name);
        return -1;
    }
    return reg;
}

/* Convert a 32-character binary string ("01001...") to a 32-bit integer */
static int32_t binaryStringToInt(char *bin) {
    int32_t result = 0;
    for (int i = 0; i < 32; i++)
        result = (result << 1) | (bin[i] == '1' ? 1 : 0);
    return result;
}

/* Read assembly instructions from a file, encode each one as a 32-bit integer,
   and store them in memory[]. Returns the number of instructions loaded. */
int parseAndLoadFile(int32_t *memory, int maxInstructions, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("[ERROR] Could not open %s\n", filename);
        return -1;
    }

    char line[100];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        char mnemonic[20] = "", op1[20] = "", op2[20] = "", op3[20] = "";
        sscanf(line, "%s %s %s %s", mnemonic, op1, op2, op3);

        /* Skip empty lines and comments */
        if (strlen(mnemonic) == 0) continue;
        if (mnemonic[0] == '#' || mnemonic[0] == '/') continue;

        /* Map mnemonic to opcode */
        int opcode = -1;
        if      (strcmp(mnemonic, "ADD")  == 0) opcode = 0;
        else if (strcmp(mnemonic, "SUB")  == 0) opcode = 1;
        else if (strcmp(mnemonic, "MUL")  == 0) opcode = 2;
        else if (strcmp(mnemonic, "MOVI") == 0) opcode = 3;
        else if (strcmp(mnemonic, "JEQ")  == 0) opcode = 4;
        else if (strcmp(mnemonic, "AND")  == 0) opcode = 5;
        else if (strcmp(mnemonic, "XORI") == 0) opcode = 6;
        else if (strcmp(mnemonic, "JMP")  == 0) opcode = 7;
        else if (strcmp(mnemonic, "LSL")  == 0) opcode = 8;
        else if (strcmp(mnemonic, "LSR")  == 0) opcode = 9;
        else if (strcmp(mnemonic, "MOVR") == 0) opcode = 10;
        else if (strcmp(mnemonic, "MOVM") == 0) opcode = 11;

        if (opcode == -1) {
            printf("[ERROR] Unknown instruction: %s\n", mnemonic);
            continue;
        }

        /* Build the 32-bit binary encoding as a character string */
        char bin[33];
        bin[32] = '\0';

        if (opcode == 0 || opcode == 1 || opcode == 2 || opcode == 5) {
            /* R-type: ADD, SUB, MUL, AND
               [ OPCODE(4) | RD(5) | RS(5) | RT(5) | SHAMT(13) ] */
            writeBits(opcode,             4,  bin,  0);
            writeBits(parseRegister(op1), 5,  bin,  4);
            writeBits(parseRegister(op2), 5,  bin,  9);
            writeBits(parseRegister(op3), 5,  bin, 14);
            writeBits(0,                  13, bin, 19);
        }
        else if (opcode == 8 || opcode == 9) {
            /* Shift: LSL, LSR
               [ OPCODE(4) | RD(5) | RS(5) | 00000(5) | SHAMT(13) ] */
            writeBits(opcode,             4,  bin,  0);
            writeBits(parseRegister(op1), 5,  bin,  4);
            writeBits(parseRegister(op2), 5,  bin,  9);
            writeBits(0,                  5,  bin, 14);
            writeBits(atoi(op3),          13, bin, 19);
        }
        else if (opcode == 3) {
            /* MOVI: [ OPCODE(4) | RD(5) | 00000(5) | IMMEDIATE(18) ] */
            writeBits(opcode,             4,  bin,  0);
            writeBits(parseRegister(op1), 5,  bin,  4);
            writeBits(0,                  5,  bin,  9);
            writeBits(atoi(op2) & 0x3FFFF, 18, bin, 14);
        }
        else if (opcode == 4 || opcode == 6 || opcode == 10 || opcode == 11) {
            /* I-type: JEQ, XORI, MOVR, MOVM
               [ OPCODE(4) | R1(5) | R2(5) | IMMEDIATE(18) ] */
            writeBits(opcode,              4,  bin,  0);
            writeBits(parseRegister(op1),  5,  bin,  4);
            writeBits(parseRegister(op2),  5,  bin,  9);
            writeBits(atoi(op3) & 0x3FFFF, 18, bin, 14);
        }
        else if (opcode == 7) {
            /* J-type: JMP
               [ OPCODE(4) | ADDRESS(28) ] */
            writeBits(opcode,                    4,  bin,  0);
            writeBits(atoi(op1) & 0x0FFFFFFF,   28, bin,  4);
        }

        memory[count] = binaryStringToInt(bin);

        line[strcspn(line, "\n")] = '\0';
        printf("[Parser] %2d: %-20s -> %s\n", count, line, bin);

        count++;
        if (count >= maxInstructions) {
            printf("[WARNING] Instruction memory full.\n");
            break;
        }
    }

    fclose(file);
    printf("\nTotal instructions loaded: %d\n\n", count);
    return count;
}
