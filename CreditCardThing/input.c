/*
Eden Yun Yu - my assigned input checking

My part is making sure an answer is complete and valid before the menus use it.
I had input checking in the Gate 1 starter's main.c. For Gate 2 I keep the shared
reader here because I also need the same number rules when loading a saved file.
I separate reading a line from deciding whether that line is a valid number.

I use readLine to finish one whole answer. I read characters with fgetc until
I reach a newline or the end of the stream. I keep ch as an int because fgetc
must represent both a character and EOF. I keep the used length as size_t,
which is the size type used by arrays and the standard library.

I leave room for the ending null character. That is what lets later string
functions know where the text ends. If the answer is too long, I mark it bad
but keep reading the rest of that line. Otherwise the next question could read
leftover characters as if the person had typed another answer.

I also reject a null character hidden inside the input. A string function would
stop there even if more bytes followed it. I finish that line before returning
INPUT_INVALID. I keep an actual read failure separate from ordinary end of
input, because a broken stream should not be reported as a normal goodbye.

I remove a final carriage return for Windows files that use carriage return
plus newline. I accept a last line without a newline when it still contains a
valid answer. I return INPUT_END only when there is no next answer to read.

I use parseNumber to convert the finished text. strtol gives me a long and a
pointer to the first character it could not convert. I clear errno first, then
check for ERANGE in case the number is too large even for long.

I allow surrounding whitespace but reject any other leftover character. For
example, I accept 2 and ' 2 ', but reject a blank line, 2hello, and 1.5. I also
check the caller's minimum and maximum, so 9 is not a valid three-option choice.
I only write through the output pointer after all those checks succeed.

I combine the two steps in readNumber using a 128-character buffer. The enum
in input.h names four results: valid input, end of input, invalid input, and
read error. I use names rather than expecting each caller to remember numbers.
I let Oliver decide whether the menu should repeat or close for each result.

I keep this file about whole-number choices and IDs. For a later purchase
calculator, I still need separate amount rules and consistent whole-cent
rounding. I do not use the menu-number reader as a finished money parser.
*/
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include "input.h"

enum InputResult readLine(FILE* stream, char* line, size_t capacity) {
	if (capacity < 2) {
		return INPUT_ERROR;
	}
	size_t length = 0;
	int ch;
	bool invalid = false;

	while ((ch = fgetc(stream)) != '\n' && ch != EOF) {
		/* I keep consuming a bad line so its leftovers cannot answer the next question. */
		if (ch == '\0' || length == capacity - 1) {
			invalid = true;
		} else {
			line[length++] = (char)ch;
		}
	}
	line[length] = '\0';
	if (ferror(stream)) {
		return INPUT_ERROR;
	}
	if (ch == EOF && length == 0 && !invalid) {
		return INPUT_END;
	}
	if (invalid) {
		return INPUT_INVALID;
	}
	/* I remove the extra \r used before the newline in Windows text files. */
	if (length > 0 && line[length - 1] == '\r') {
		line[length - 1] = '\0';
	}
	return INPUT_OK;
}

bool parseNumber(const char* line, int minimum, int maximum, int* number) {
	char* end;
	errno = 0;
	long value = strtol(line, &end, 10);
	/* I reject both a missing number and a number too large for long. */
	if (end == line || errno == ERANGE) {
		return false;
	}
	while (isspace((unsigned char)*end)) {
		/* I cast to unsigned char because the character-check functions require it. */
		end++;
	}
	if (*end != '\0' || value < minimum || value > maximum) {
		return false;
	}
	*number = (int)value;
	/* I only change the caller's number once the range and trailing text pass. */
	return true;
}

enum InputResult readNumber(FILE* stream, int minimum, int maximum, int* number) {
	char line[128];
	enum InputResult result = readLine(stream, line, sizeof(line));
	if (result != INPUT_OK) {
		return result;
	}
	if (!parseNumber(line, minimum, maximum, number)) {
		return INPUT_INVALID;
	}
	return INPUT_OK;
}
