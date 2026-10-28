/*
Eden Yun Yu - checking answers

My job here is to check the complete answer before the menu uses it. Gate 1
had the input check in main.c. For Gate 2, I keep the reader here because the
saved-file code needs the same number rules. Reading a line and deciding
whether that line is a number are separate steps.

In readLine, I use fgetc until the newline or end of input. The character
variable is an int because it needs to hold either a character or EOF.
The length uses size_t, which is the library's type for buffer sizes.

I leave one space for the ending null character. If an answer is too long,
the reader marks it invalid but still finishes that line. This matters:
stopping at the buffer limit would leave the rest as input for the next
question, even though the person only typed one answer.

A hidden null also makes the line invalid. Normal string functions would
stop there and miss the following bytes, so I don't let that partly hidden
answer through. An actual read failure gets a different result from ordinary
end of input. That lets Oliver explain a problem instead of saying Goodbye
as if everything ended normally.

For Windows text files, I remove the final carriage return before the newline.
A last line without a newline can still be valid. INPUT_END means there was
no next answer, not just that the last answer was missing its newline.

I use strtol with base 10 in parseNumber. It returns a long and leaves end
pointing to the first character it couldn't convert. Clearing errno first
lets me detect ERANGE for a number too large even for long.

Spaces around a number are fine; other leftover characters are not. I accept
2 and ' 2 ', but reject blank input, 2hello, and 1.5. The caller also supplies
the allowed range, so 9 fails for a menu limited to 1 through 3. The output
number is only assigned after the complete check succeeds.

readNumber joins those steps using a 128-character buffer. The enum in
input.h names valid input, invalid input, end of input, and read failure.
I use these names so callers don't have to guess what a returned number means.

These checks are for menu choices and card IDs. A later purchase calculator
still needs its own amount rules and whole-cent rounding. This is not yet
a money parser.
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
