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
	/* Leave out the extra \r used in Windows text files. */
	if (length > 0 && line[length - 1] == '\r') {
		line[length - 1] = '\0';
	}
	return INPUT_OK;
}

bool parseNumber(const char* line, int minimum, int maximum, int* number) {
	char* end;
	errno = 0;
	long value = strtol(line, &end, 10);
	if (end == line || errno == ERANGE) {
		return false;
	}
	while (isspace((unsigned char)*end)) {
		end++;
	}
	if (*end != '\0' || value < minimum || value > maximum) {
		return false;
	}
	*number = (int)value;
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
