/*
Build and run the C checks from the project root. No extra scripting language
is needed. I stop on a failed command and keep its actual output in docs.
Windows uses the C compiler in a Visual Studio Native Tools Command Prompt.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

#define APP_SOURCES "CreditCardThing/main.c CreditCardThing/cards.c CreditCardThing/realCards.c CreditCardThing/rewards.c CreditCardThing/rewardWindow.c CreditCardThing/input.c CreditCardThing/wallet.c CreditCardThing/storage.c CreditCardThing/logger.c CreditCardThing/window.c"
#define CORE_SOURCES "CreditCardThing/cards.c CreditCardThing/realCards.c CreditCardThing/rewards.c CreditCardThing/input.c CreditCardThing/wallet.c CreditCardThing/storage.c CreditCardThing/logger.c"

static int run(const char* command, const char* logPath) {
	FILE* log = fopen(logPath, "a");
	if (!log) { perror(logPath); return 1; }
	fprintf(log, "\nCommand: %s\n", command);
	if (fclose(log) != 0) return 1;
	char redirected[6000];
	int length = snprintf(redirected, sizeof(redirected), "%s >> %s 2>&1", command, logPath);
	if (length < 0 || (size_t)length >= sizeof(redirected)) return 1;
	puts(command); fflush(stdout);
	int status = system(redirected);
	log = fopen(logPath, "a");
	if (!log) return 1;
	fprintf(log, "Command result: %s\n", status == 0 ? "passed" : "failed");
	if (fclose(log) != 0) return 1;
	return status == 0 ? 0 : 1;
}

static int startLog(const char* path, const char* title) {
	FILE* log = fopen(path, "w");
	if (!log) return 1;
	fprintf(log, "%s\n", title);
	return fclose(log) != 0;
}

int main(int argc, char* argv[]) {
	if (argc > 2 || (argc == 2 && strcmp(argv[1], "--cross") != 0)) {
		fputs("Run from the project root, optionally with --cross on Linux.\n", stderr); return 1;
	}
#ifdef _WIN32
	_mkdir("build");
	if (argc == 2) { fputs("Use the normal build on Windows.\n", stderr); return 1; }
	const char* buildLog = "docs/gateFourBuildLog.txt";
	if (startLog(buildLog, "Gate 4 native Windows C11 build")) return 1;
	const char* commands[] = {
		"cl /nologo /TC /std:c11 /W4 /D_CRT_SECURE_NO_WARNINGS /Fobuild\\ " APP_SOURCES " /Febuild/cardClarity.exe /link user32.lib gdi32.lib",
		"cl /nologo /TC /std:c11 /W4 /D_CRT_SECURE_NO_WARNINGS /Fobuild\\ tests/testModules.c CreditCardThing/cards.c CreditCardThing/realCards.c CreditCardThing/logger.c /Febuild/testModules.exe",
		"cl /nologo /TC /std:c11 /W4 /D_CRT_SECURE_NO_WARNINGS /Fobuild\\ tests/testGate1.c tests/testSupport.c /Febuild/testGate1.exe",
		"cl /nologo /TC /std:c11 /W4 /D_CRT_SECURE_NO_WARNINGS /Fobuild\\ tests/testGate2.c tests/testSupport.c /Febuild/testGate2.exe",
		"cl /nologo /TC /std:c11 /W4 /D_CRT_SECURE_NO_WARNINGS /Fobuild\\ tests/testGateFour.c tests/testSupport.c " CORE_SOURCES " /Febuild/testGateFour.exe", NULL
	};
#else
	mkdir("build", 0700);
	const char* buildLog = argc == 2 ? "docs/gateFourWindowsBuildLog.txt" : "docs/gateFourBuildLog.txt";
	if (startLog(buildLog, argc == 2 ? "Gate 4 MinGW C11 cross-build. Windows runtime remains unverified." : "Gate 4 Linux C11 build. Windows runtime remains unverified.")) return 1;
	if (argc == 2) {
		const char* cross[] = {
			"x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Wpedantic -Werror " APP_SOURCES " -o build/cardClarity.exe -luser32 -lgdi32",
			"x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Wpedantic -Werror tools/buildProject.c -o build/buildProject.exe",
			"x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/testGateFour.c tests/testSupport.c " CORE_SOURCES " -o build/testGateFour.exe", NULL
		};
		for (int i=0;cross[i];i++) if (run(cross[i], buildLog)) return 1;
		puts("Windows compilation passed. Run the window and checks on the demo computer."); return 0;
	}
	const char* commands[] = {
		"cc -std=c11 -Wall -Wextra -Wpedantic -Werror " APP_SOURCES " -o build/cardClarity -lm",
		"cc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/testModules.c CreditCardThing/cards.c CreditCardThing/realCards.c CreditCardThing/logger.c -o build/testModules -lm",
		"cc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/testGate1.c tests/testSupport.c -o build/testGate1",
		"cc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/testGate2.c tests/testSupport.c -o build/testGate2",
		"cc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/testGateFour.c tests/testSupport.c " CORE_SOURCES " -o build/testGateFour -lm", NULL
	};
#endif
	for (int i=0;commands[i];i++) if (run(commands[i], buildLog)) return 1;
	const char* testLog = "docs/gateFourTestLog.txt";
	if (startLog(testLog, "Actual module, earlier-gate and Gate 4 checks. This is not a Windows desktop review.")) return 1;
#ifdef _WIN32
	const char* checks[] = { "build\\testModules.exe", "build\\testGate1.exe build\\cardClarity.exe",
		"build\\testGate2.exe build\\cardClarity.exe", "build\\testGateFour.exe build\\cardClarity.exe", NULL };
#else
	const char* checks[] = { "build/testModules", "build/testGate1 build/cardClarity",
		"build/testGate2 build/cardClarity", "build/testGateFour build/cardClarity", NULL };
#endif
	for (int i=0;checks[i];i++) if (run(checks[i], testLog)) return 1;
	puts("Build and C checks passed. Read the actual results in docs/gateFourTestLog.txt.");
	return 0;
}
