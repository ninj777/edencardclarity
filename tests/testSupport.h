/*
Evan Zhuang - shared test declarations

I group the paths, captured output, counts, and process handles in TestContext.
Both menu suites and the demo share this setup. Windows handles and POSIX
process numbers need separate declarations, but both versions are C.
*/
#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <stdbool.h>
#include <stddef.h>

#define TEST_PATH_SIZE 4096
#define TEST_TEXT_SIZE 32768

typedef struct {
	char program[TEST_PATH_SIZE];
	char folder[TEST_PATH_SIZE];
	char save[TEST_PATH_SIZE];
	char output[TEST_TEXT_SIZE];
	char errors[8192];
	int passed, failed, skipped, checks, problems, status;
#ifdef _WIN32
	void* process;
	void* input;
#else
	int process;
	int input;
#endif
} TestContext;

void initTests(TestContext* test, const char* program, const char* suite);
void startCase(TestContext* test, int number, const char* name, const char* expected);
void expect(TestContext* test, bool condition, const char* observation);
void finishCase(TestContext* test);
void skipCase(TestContext* test, const char* reason);
int finishTests(TestContext* test);
void joinPath(char* result, size_t capacity, const char* folder, const char* name);
void writeBytes(const char* path, const void* data, size_t length);
void writeText(const char* path, const char* text);
bool fileEquals(const char* path, const void* data, size_t length);
bool pathExists(const char* path);
void makeFolder(const char* path);
int textCount(const char* text, const char* part);

/* I can keep input open while changing a fixture before choosing Reload. */
void startProgram(TestContext* test, const char* const arguments[], bool badInput);
void sendAnswers(TestContext* test, const void* answers, size_t length);
void closeAnswers(TestContext* test);
void waitProgram(TestContext* test);
bool waitForOutput(TestContext* test, const char* text);
void runProgram(TestContext* test, const char* const arguments[],
	const void* answers, size_t length, int expectedStatus, bool badInput);

#endif
