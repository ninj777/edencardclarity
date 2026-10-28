/*
Oliver Dolfin - logger declarations

I share the message levels and two functions here. The timer and enabled flag
stay in logger.c, so including this header does not create more global copies.
The window turns logging on when it starts.
*/
#ifndef LOGGER_H
#define LOGGER_H

enum LogLevel {
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};

void logMessage(char* message, enum LogLevel level);
/* I turn logging on when the optional window starts. */
void initLogger(void);

#endif
