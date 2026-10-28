/*
Evan Zhuang - my assigned C test support

I keep the process and file setup here so the menu cases can focus on answers
and expected behavior. Standard C does not provide child processes with input
pipes and time limits. I use Windows functions under _WIN32 and POSIX functions
on the other supported systems. Both branches are C, not another language.

I resolve the application's path before entering a temporary folder. I make
that folder for each case, with its own savedCards.txt, output.txt, and errors.txt.
I remove only that temporary folder after recording the result. I never need
to clean or overwrite the person's normal saved wallet for a test.

I start the compiled program directly. On Windows I use CreateProcessA and
inheritable file/pipe handles. I quote each argument, including backslashes,
so a save path with spaces stays one argument. On POSIX I use pipe, fork,
dup2, and execv. I pass the arguments as separate array entries there.

I send answers through the pipe and then close it to signal EOF. I accept an
explicit byte length so a test can send hidden nulls rather than stopping at
the first one. I wait up to five seconds. If the child takes too long, I stop
it and record a failure. I close the process and input handles after use.

I capture the output in files instead of filling an unread output pipe.
I compare text after normalizing Windows line endings, but I compare saved
files as exact bytes. I keep those two checks separate because a correct
message does not prove the save contains the right IDs.

I use expect for one yes-or-no observation, finishCase for the result of a
whole case, and finishTests for the final passed/failed/skipped totals.
I return a nonzero status for any failure so a build log cannot hide it.
I skip platform-only cases explicitly. A skip is not a successful desktop test.

For the live damaged-file test, I leave input open, wait for the initial load
message, replace the fixture, and only then send Reload. I need that order to
check that the loaded wallet survives a file becoming damaged during a session.
*/
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include "testSupport.h"

static void stopTest(const char* message) {
	fprintf(stderr, "Test setup failed: %s\n", message);
	exit(2);
}

void joinPath(char* result, size_t capacity, const char* folder, const char* name) {
	int length = snprintf(result, capacity, "%s/%s", folder, name);
	if (length < 0 || (size_t)length >= capacity) {
		stopTest("a test path is too long");
	}
}

bool pathExists(const char* path) {
#ifdef _WIN32
	return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
#else
	struct stat info;
	return lstat(path, &info) == 0;
#endif
}

void makeFolder(const char* path) {
#ifdef _WIN32
	if (!CreateDirectoryA(path, NULL)) {
#else
	if (mkdir(path, 0700) != 0) {
#endif
		stopTest("could not create a test folder");
	}
}

/* I only call this on the temporary folder made by my runner. */
static void removeFolder(const char* path) {
	char child[TEST_PATH_SIZE];
#ifdef _WIN32
	char pattern[TEST_PATH_SIZE];
	joinPath(pattern, sizeof(pattern), path, "*");
	WIN32_FIND_DATAA entry;
	HANDLE search = FindFirstFileA(pattern, &entry);
	if (search != INVALID_HANDLE_VALUE) {
		do {
			if (strcmp(entry.cFileName, ".") == 0 || strcmp(entry.cFileName, "..") == 0) {
				continue;
			}
			joinPath(child, sizeof(child), path, entry.cFileName);
			if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				removeFolder(child);
			} else if (!DeleteFileA(child)) {
				stopTest("could not remove a test file");
			}
		} while (FindNextFileA(search, &entry));
		FindClose(search);
	}
	if (!RemoveDirectoryA(path)) {
		stopTest("could not remove a test folder");
	}
#else
	DIR* folder = opendir(path);
	if (!folder) {
		stopTest("could not open a test folder for cleanup");
	}
	struct dirent* entry;
	while ((entry = readdir(folder)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		joinPath(child, sizeof(child), path, entry->d_name);
		struct stat info;
		if (lstat(child, &info) != 0) {
			stopTest("could not inspect a test file");
		}
		if (S_ISDIR(info.st_mode)) {
			removeFolder(child);
		} else if (unlink(child) != 0) {
			stopTest("could not remove a test file");
		}
	}
	closedir(folder);
	if (rmdir(path) != 0) {
		stopTest("could not remove a test folder");
	}
#endif
}

void writeBytes(const char* path, const void* data, size_t length) {
	FILE* file = fopen(path, "wb");
	if (!file) {
		stopTest("could not write a test file");
	}
	bool written = fwrite(data, 1, length, file) == length;
	if (fclose(file) != 0 || !written) {
		stopTest("a test file write did not finish");
	}
}

void writeText(const char* path, const char* text) {
	writeBytes(path, text, strlen(text));
}

bool fileEquals(const char* path, const void* data, size_t length) {
	unsigned char buffer[TEST_TEXT_SIZE];
	FILE* file = fopen(path, "rb");
	if (!file) {
		return false;
	}
	size_t read = fread(buffer, 1, sizeof(buffer), file);
	bool same = !ferror(file) && read == length && memcmp(buffer, data, length) == 0;
	if (fclose(file) != 0) {
		same = false;
	}
	return same;
}

static void readText(const char* path, char* text, size_t capacity) {
	FILE* file = fopen(path, "rb");
	if (!file) {
		text[0] = '\0';
		return;
	}
	size_t length = fread(text, 1, capacity - 1, file);
	bool failed = ferror(file) != 0 || (length == capacity - 1 && fgetc(file) != EOF);
	if (fclose(file) != 0 || failed) {
		stopTest("could not read the complete program output");
	}
	text[length] = '\0';
	/* I normalize output line endings here and check saved bytes separately. */
	size_t used = 0;
	for (size_t i = 0; i < length; i++) {
		if (text[i] != '\r' || text[i + 1] != '\n') {
			text[used++] = text[i];
		}
	}
	text[used] = '\0';
}

static void readOutput(TestContext* test) {
	char path[TEST_PATH_SIZE];
	joinPath(path, sizeof(path), test->folder, "output.txt");
	readText(path, test->output, sizeof(test->output));
	joinPath(path, sizeof(path), test->folder, "errors.txt");
	readText(path, test->errors, sizeof(test->errors));
}

int textCount(const char* text, const char* part) {
	int count = 0;
	size_t length = strlen(part);
	if (length == 0) {
		return 0;
	}
	while ((text = strstr(text, part)) != NULL) {
		count++;
		text += length;
	}
	return count;
}

void initTests(TestContext* test, const char* program, const char* suite) {
	memset(test, 0, sizeof(*test));
#ifdef _WIN32
	DWORD length = GetFullPathNameA(program, sizeof(test->program), test->program, NULL);
	if (length == 0 || length >= sizeof(test->program)) {
		stopTest("could not resolve the program path");
	}
#else
	if (!realpath(program, test->program)) {
		stopTest("could not resolve the program path");
	}
	signal(SIGPIPE, SIG_IGN);
#endif
	if (!pathExists(test->program)) {
		stopTest("the compiled program was not found");
	}
	time_t now = time(NULL);
	struct tm* stamp = gmtime(&now);
	char date[80] = "time unavailable";
	if (stamp) {
		strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S UTC", stamp);
	}
	printf("%s\nDate: %s\nProgram: %s\n", suite, date, test->program);
#ifdef _WIN32
	puts("Test platform: Windows");
#else
	puts("Test platform: POSIX");
#endif
}

void startCase(TestContext* test, int number, const char* name, const char* expected) {
	test->checks = 0;
	test->problems = 0;
	test->status = 0;
	test->output[0] = '\0';
	test->errors[0] = '\0';
#ifdef _WIN32
	char temporary[TEST_PATH_SIZE];
	DWORD length = GetTempPathA(sizeof(temporary), temporary);
	if (length == 0 || length >= sizeof(temporary)
		|| !GetTempFileNameA(temporary, "cct", 0, test->folder)
		|| !DeleteFileA(test->folder)) {
		stopTest("could not choose a temporary folder");
	}
	makeFolder(test->folder);
#else
	const char* temporary = getenv("TMPDIR");
	joinPath(test->folder, sizeof(test->folder), temporary ? temporary : "/tmp", "cardClarityXXXXXX");
	if (!mkdtemp(test->folder)) {
		stopTest("could not create a temporary folder");
	}
#endif
	joinPath(test->save, sizeof(test->save), test->folder, "savedCards.txt");
	printf("\nCase %d: %s\nExpected: %s\n", number, name, expected);
}

void expect(TestContext* test, bool condition, const char* observation) {
	test->checks++;
	printf("Observed: %s: %s\n", observation, condition ? "yes" : "no");
	if (!condition) {
		test->problems++;
	}
}

void finishCase(TestContext* test) {
	if (test->problems) {
		test->failed++;
		printf("Result: failed (%d checks did not match).\nOutput:\n%s\nErrors:\n%s\n",
			test->problems, test->output, test->errors);
	} else {
		test->passed++;
		printf("Result: passed (%d checks matched).\n", test->checks);
	}
	removeFolder(test->folder);
}

void skipCase(TestContext* test, const char* reason) {
	test->skipped++;
	printf("Result: skipped. %s\n", reason);
	removeFolder(test->folder);
}

int finishTests(TestContext* test) {
	printf("\nSummary: %d passed, %d failed, %d skipped.\n", test->passed, test->failed, test->skipped);
	return test->failed ? 1 : 0;
}

static void pauseBriefly(void) {
#ifdef _WIN32
	Sleep(20);
#else
	struct timespec delay = { 0, 20000000 };
	nanosleep(&delay, NULL);
#endif
}

#ifdef _WIN32
/* I quote each argument because Windows accepts one command string. */
static void appendArgument(char* command, size_t capacity, const char* argument) {
	size_t used = strlen(command);
	size_t length = strlen(argument);
	if (used + 2 * length + 4 >= capacity) {
		stopTest("a test command is too long");
	}
	command[used++] = ' ';
	command[used++] = '"';
	size_t slashes = 0;
	for (size_t i = 0; i <= length; i++) {
		char ch = argument[i];
		if (ch == '\\') {
			slashes++;
			continue;
		}
		size_t copies = (ch == '"' || ch == '\0') ? 2 * slashes : slashes;
		while (copies--) {
			command[used++] = '\\';
		}
		slashes = 0;
		if (ch == '\0') {
			break;
		}
		if (ch == '"') {
			command[used++] = '\\';
		}
		command[used++] = ch;
	}
	command[used++] = '"';
	command[used] = '\0';
}
#endif

void startProgram(TestContext* test, const char* const arguments[], bool badInput) {
	/* I capture this run separately from the runner's own expected/actual messages. */
	char outputPath[TEST_PATH_SIZE], errorPath[TEST_PATH_SIZE];
	joinPath(outputPath, sizeof(outputPath), test->folder, "output.txt");
	joinPath(errorPath, sizeof(errorPath), test->folder, "errors.txt");
	fflush(stdout);
#ifdef _WIN32
	(void)badInput;
	SECURITY_ATTRIBUTES security = { sizeof(security), NULL, TRUE };
	HANDLE inputRead, inputWrite;
	if (!CreatePipe(&inputRead, &inputWrite, &security, 0)
		|| !SetHandleInformation(inputWrite, HANDLE_FLAG_INHERIT, 0)) {
		stopTest("could not create the input pipe");
	}
	HANDLE output = CreateFileA(outputPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		&security, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE errors = CreateFileA(errorPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		&security, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (output == INVALID_HANDLE_VALUE || errors == INVALID_HANDLE_VALUE) {
		stopTest("could not capture program output");
	}
	char command[32768] = "";
	appendArgument(command, sizeof(command), test->program);
	for (int i = 0; arguments[i]; i++) {
		appendArgument(command, sizeof(command), arguments[i]);
	}
	STARTUPINFOA startup = { 0 };
	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESTDHANDLES;
	startup.hStdInput = inputRead;
	startup.hStdOutput = output;
	startup.hStdError = errors;
	PROCESS_INFORMATION process = { 0 };
	bool started = CreateProcessA(test->program, command + 1, NULL, NULL, TRUE,
		CREATE_NO_WINDOW, NULL, test->folder, &startup, &process) != 0;
	CloseHandle(inputRead);
	CloseHandle(output);
	CloseHandle(errors);
	if (!started) {
		CloseHandle(inputWrite);
		stopTest("could not start the compiled program");
	}
	CloseHandle(process.hThread);
	test->process = process.hProcess;
	test->input = inputWrite;
#else
	int input[2];
	if (pipe(input) != 0) {
		stopTest("could not create the input pipe");
	}
	pid_t child = fork();
	if (child < 0) {
		stopTest("could not start the compiled program");
	}
	if (child == 0) {
		/* I set up the child's input and output before replacing it with CardClarity. */
		close(input[1]);
		int in = badInput ? open(test->folder, O_RDONLY) : input[0];
		int out = open(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
		int err = open(errorPath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
		if (in < 0 || out < 0 || err < 0 || chdir(test->folder) != 0
			|| dup2(in, STDIN_FILENO) < 0 || dup2(out, STDOUT_FILENO) < 0
			|| dup2(err, STDERR_FILENO) < 0) {
			_exit(126);
		}
		close(in);
		if (badInput) {
			close(input[0]);
		}
		close(out);
		close(err);
		char* values[16];
		values[0] = test->program;
		int count = 1;
		for (int i = 0; arguments[i] && count < 15; i++) {
			values[count++] = (char*)arguments[i];
		}
		values[count] = NULL;
		execv(test->program, values);
		_exit(127);
	}
	close(input[0]);
	test->process = (int)child;
	test->input = input[1];
#endif
}

void sendAnswers(TestContext* test, const void* answers, size_t length) {
	/* I send every requested byte, including null bytes used by the input tests. */
	const char* bytes = answers;
	while (length > 0) {
#ifdef _WIN32
		DWORD sent = 0;
		bool okay = WriteFile(test->input, bytes, (DWORD)length, &sent, NULL) != 0;
		if (!okay || sent == 0) {
#else
		ssize_t sent = write(test->input, bytes, length);
		if (sent < 0 && errno == EINTR) {
			continue;
		}
		if (sent <= 0) {
#endif
			expect(test, false, "the program accepted the complete test input");
			return;
		}
		bytes += sent;
		length -= (size_t)sent;
	}
}

void closeAnswers(TestContext* test) {
#ifdef _WIN32
	CloseHandle(test->input);
	test->input = NULL;
#else
	close(test->input);
	test->input = -1;
#endif
}

void waitProgram(TestContext* test) {
	/* I stop a child that does not finish rather than letting a check hang forever. */
#ifdef _WIN32
	DWORD result = WaitForSingleObject(test->process, 5000);
	if (result != WAIT_OBJECT_0) {
		TerminateProcess(test->process, 124);
		WaitForSingleObject(test->process, INFINITE);
		expect(test, false, "the program finished within five seconds");
	}
	DWORD status = 125;
	GetExitCodeProcess(test->process, &status);
	test->status = (int)status;
	CloseHandle(test->process);
	test->process = NULL;
#else
	int status = 0;
	pid_t result = 0;
	for (int i = 0; i < 250 && result == 0; i++) {
		result = waitpid(test->process, &status, WNOHANG);
		if (result < 0 && errno == EINTR) {
			result = 0;
		}
		if (result == 0) {
			pauseBriefly();
		}
	}
	if (result <= 0) {
		kill(test->process, SIGKILL);
		while (waitpid(test->process, &status, 0) < 0 && errno == EINTR) {
		}
		expect(test, false, "the program finished within five seconds");
	}
	test->status = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
	test->process = 0;
#endif
	readOutput(test);
}

bool waitForOutput(TestContext* test, const char* text) {
	for (int i = 0; i < 250; i++) {
		readOutput(test);
		if (strstr(test->output, text)) {
			return true;
		}
		pauseBriefly();
	}
	return false;
}

void runProgram(TestContext* test, const char* const arguments[],
	const void* answers, size_t length, int expectedStatus, bool badInput) {
	startProgram(test, arguments, badInput);
	sendAnswers(test, answers, length);
	closeAnswers(test);
	waitProgram(test);
	printf("Actual exit status: %d\n", test->status);
	expect(test, test->status == expectedStatus, "exit status matches");
	if (!badInput && expectedStatus == 0) {
		expect(test, test->errors[0] == '\0', "standard error is empty");
	}
}
