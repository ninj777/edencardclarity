/*
Oliver Dolfin - my assigned window entry point

I declare runWindow here so main.c can launch the window without knowing every
Windows control. I pass the save path in, which lets both interfaces use the
same wallet file. I return a normal program status to main.
*/
#ifndef WINDOW_H
#define WINDOW_H

/* I use the same wallet and save format as the console. */
int runWindow(const char* filePath);

#endif
