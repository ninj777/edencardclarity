/*
Evan Zhuang - my assigned restart demonstration

I run CardClarity three separate times with the same temporary save path.
First I add cards 1 and 3, view, and save. Next I restart, remove card 1, save,
and reload. Last I restart again and expect only card 3. I print each set of
answers, full output, exit status, and checked saved bytes for the demo log.
I use expectedFiles to check the file itself after each process finishes.
I keep the demonstration separate from the 39 menu cases so its actual output
is easy to show during the gate check. I leave the usual saved wallet alone.
*/
#include <stdio.h>
#include <string.h>
#include "testSupport.h"

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fputs("Run demoGate2 with the compiled CardClarity path.\n", stderr);
		return 2;
	}
	TestContext test;
	initTests(&test, argv[1], "Gate 2 C save-and-reload demonstration");
	startCase(&test, 1, "Three separate program runs", "A saved removal survives two restarts.");
	const char* arguments[] = { "--wallet-file", test.save, NULL };
	const char* inputs[] = {
		"2\n2\n1\n2\n3\n1\n4\n6\n3\n",
		"2\n1\n3\n1\n4\n5\n1\n6\n3\n",
		"2\n1\n6\n3\n"
	};
	const char* expectedFiles[] = {
		"cardClarityWallet1\n2\n1\n3\n",
		"cardClarityWallet1\n1\n3\n",
		"cardClarityWallet1\n1\n3\n"
	};
	for (int i = 0; i < 3; i++) {
		/* I keep the same file but start a fresh program for each step. */
		printf("\nRun %d\nAnswers, one per line:\n%s\n", i + 1, inputs[i]);
		runProgram(&test, arguments, inputs[i], strlen(inputs[i]), 0, false);
		printf("Actual standard output:\n%s\nActual standard error:\n%s\n", test.output, test.errors);
		expect(&test, strstr(test.output, "Card 3: Cashback Sample |") != NULL, "Cashback Sample is visible");
		if (i == 0) {
			expect(&test, strstr(test.output, "Card 1: Travel Sample |") != NULL, "Travel Sample is visible before removal");
		} else {
			expect(&test, strstr(test.output, "Loaded your saved card list.") != NULL, "new process loaded the save");
		}
		if (i == 1) {
			expect(&test, strstr(test.output, "Removed Travel Sample.") != NULL, "Travel Sample was removed");
		}
		if (i == 2) {
			expect(&test, !strstr(test.output, "Card 1: Travel Sample |"), "removed card stayed absent after restart");
		}
		expect(&test, fileEquals(test.save, expectedFiles[i], strlen(expectedFiles[i])), "saved bytes match");
		printf("Checked saved bytes:\n%s", expectedFiles[i]);
	}
	finishCase(&test);
	return finishTests(&test);
}
