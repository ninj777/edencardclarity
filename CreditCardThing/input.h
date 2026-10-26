#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum InputResult {
	INPUT_OK,
	INPUT_END,
	INPUT_INVALID,
	INPUT_ERROR
};

/* Finish bad lines too. The buffer needs room for at least one character and \0. */
enum InputResult readLine(FILE* stream, char* line, size_t capacity);
/* Only set number when the whole answer passes the checks. */
bool parseNumber(const char* line, int minimum, int maximum, int* number);
enum InputResult readNumber(FILE* stream, int minimum, int maximum, int* number);

#endif
