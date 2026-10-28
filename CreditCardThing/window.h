/*
Oliver Dolfin - opening the window

I keep just the entry point here. main.c passes the save path and gets a
program status back, without needing to know about every Windows control.
The wallet file format is the same as the console's.
*/
#ifndef WINDOW_H
#define WINDOW_H

/* I use the same wallet and save format as the console. */
int runWindow(const char* filePath);

#endif
