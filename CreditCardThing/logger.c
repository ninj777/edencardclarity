/*
Oliver Dolfin - my assigned build and logger area

I keep this small logger from the original project. I print the elapsed time,
a level, and a message when logging is enabled. I leave it off for the console
so the numbered menu is the first useful output. I turn it on in runWindow.
I keep startTime and loggerReady private to this file with static.
I use the Windows timer there and the C timespec timer on other systems.
I use these messages to help with startup errors, not as proof of a gate test.
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
