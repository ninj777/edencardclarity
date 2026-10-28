/*
Eden Yun Yu - my assigned input interface

I declare the reader here so the menu and storage code can use the same rules.
I use InputResult to distinguish a valid answer, invalid answer, ordinary end,
and genuine read failure. I pass the output number by pointer, and I only set
it after conversion succeeds. I need capacity to include room for the ending
null character. I explain the line and conversion checks in input.c.
*/
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

/* I finish bad lines too and reserve room for the ending \0. */
enum InputResult readLine(FILE* stream, char* line, size_t capacity);
/* I set number only after the complete answer passes. */
bool parseNumber(const char* line, int minimum, int maximum, int* number);
enum InputResult readNumber(FILE* stream, int minimum, int maximum, int* number);

#endif
