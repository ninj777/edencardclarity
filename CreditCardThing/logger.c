/*
Oliver Dolfin - logger notes

I keep the original logger for startup and window errors. Once enabled, it
prints elapsed milliseconds, a level, and a message. The console leaves it off
so its menu isn't mixed with startup messages; runWindow turns it on.
The timer and enabled flag are private to this file. Windows uses its own
timer, while the other build uses the C timespec timer.
I keep actual test results in the test logs rather than treating these
startup messages as evidence that a gate passed.
*/
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "logger.h"

static double startTime;
static bool loggerReady = false;

static double getTimeMilliseconds(void) {
#ifdef _WIN32
	return (double)GetTickCount64();
#else
	struct timespec now = { 0 };
	timespec_get(&now, TIME_UTC);
	return (double)now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;
#endif
}

void logMessage(char* message, enum LogLevel level) {
	if (!loggerReady) {
		return;
	}
	double msAge = getTimeMilliseconds() - startTime;
	switch (level) {
	case LOG_INFO:
		printf("%.3f ms [Info] %s\n", msAge, message);
		break;
	case LOG_WARNING:
		printf("%.3f ms [Warning] %s\n", msAge, message);
		break;
	case LOG_ERROR:
		printf("%.3f ms [Error] %s\n", msAge, message);
		break;
	}
}

void initLogger(void) {
	loggerReady = true;
	startTime = getTimeMilliseconds();
	logMessage("Logger initialized.", LOG_INFO);
}
