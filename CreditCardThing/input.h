/*
Eden Yun Yu - shared input functions

I declare the reader here for the menu and saved-file code. InputResult keeps
a valid answer, bad answer, ordinary end, and real read failure separate.
The output pointer is only assigned when conversion succeeds. A buffer's
capacity must include room for its ending null. The steps are in input.c.
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
