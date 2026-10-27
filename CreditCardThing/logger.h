/*
Oliver Dolfin - my assigned logger interface

I share the message levels and two logger functions here. I keep the timer and
enabled flag in logger.c, so including this header does not define extra global
copies. I turn logging on explicitly for the window.
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
