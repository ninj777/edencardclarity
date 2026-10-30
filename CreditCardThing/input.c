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

The original checks below handle menu choices and card IDs. Gate 4 adds
parseMoney and parseDate afterward. I read decimal digits into whole cents
so 12.34 becomes 1234 without a floating-point conversion. I reject a third
decimal place rather than quietly changing someone's amount. Dates are
checked against the actual month length, including leap-year rules.
*/
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include "input.h"
#include <string.h>

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

/* Gate 4, Eden's input area. I assign the output only after the whole line passes. */
bool parseMoney(const char* line, int64_t* cents) {
	if (!line || !cents) return false;
	while (isspace((unsigned char)*line)) line++;
	if (!isdigit((unsigned char)*line)) return false;
	int64_t dollars = 0;
	while (isdigit((unsigned char)*line)) {
		int digit = *line++ - '0';
		if (dollars > (1000000 - digit) / 10) return false;
		dollars = dollars * 10 + digit;
	}
	int fraction = 0;
	if (*line == '.') {
		line++;
		if (!isdigit((unsigned char)*line)) return false;
		fraction = (*line++ - '0') * 10;
		if (isdigit((unsigned char)*line)) fraction += *line++ - '0';
	}
	while (isspace((unsigned char)*line)) line++;
	if (*line || dollars > 1000000 || (dollars == 1000000 && fraction != 0)) return false;
	*cents = dollars * 100 + fraction;
	return true;
}

bool isDateNumberValid(int date) {
	int year = date / 10000, month = date / 100 % 100, day = date % 100;
	static const int monthDays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	if (year < 1900 || year > 9999 || month < 1 || month > 12 || day < 1) return false;
	int limit = monthDays[month - 1];
	if (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) limit++;
	return day <= limit;
}

bool parseDate(const char* line, int* date) {
	if (!line || !date || strlen(line) != 10 || line[4] != '-' || line[7] != '-') return false;
	int value = 0;
	for (int i = 0; i < 10; i++) {
		if (i == 4 || i == 7) continue;
		if (!isdigit((unsigned char)line[i])) return false;
		value = value * 10 + line[i] - '0';
	}
	if (!isDateNumberValid(value)) return false;
	*date = value;
	return true;
}
