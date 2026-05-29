#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

/* Reads assembly from filename, encodes each instruction as a 32-bit integer,
   stores them in memory[], and returns the number of instructions loaded. */
int parseAndLoadFile(int32_t *memory, int maxInstructions, const char *filename);

#endif
