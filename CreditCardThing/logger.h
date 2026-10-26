#ifndef LOGGER_H
#define LOGGER_H

enum LogLevel {
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};

void logMessage(char* message, enum LogLevel level);
/* The optional window turns logging on when it starts. */
void initLogger(void);

#endif
