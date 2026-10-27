/*
Evan Zhuang - my assigned Gate 1 checks

I check the compiled program, not just whether the source looks right. I start
it with --gate1, send answers, and compare the output and exit status. I use
31 cases so the normal menu and the bad-answer recovery both get checked.
I put the expected behavior in the expected array beside the case names.
I print each actual observation through expect and return failure if a check
fails. I keep the compile command and its result in docs/gate2BuildLog.txt.

For the proposal's five required checks, I use:
  Compilation: I expect C11 with warnings enabled and no build errors.
  Startup: in case 1 I expect exactly three numbered menu options.
  Samples: in cases 2-4 I expect five names, exact rates, and example labels.
  Invalid input: in case 7 I expect hello to be rejected before Exit works.
  Exit: in case 6 I expect Goodbye and status 0.
For the extra recovery check in case 16, I send a bad answer, then 1, then 3.
I expect a retry message, the sample cards, and a clean exit in that order.
I keep the actual run observations in docs/gate2TestLog.txt.

I also check zero, negatives, blank answers, decimals, trailing letters,
large numbers, long lines, hidden nulls, whitespace, repeated choices, and EOF.
I check the largest fitting line and the first overlong line separately.
I use a real Linux read error in case 27 and the unsupported-window path in
case 31. I skip those two on Windows rather than claiming the window was tested.

For Gate 1, I need every required file and five passing checks for a 3.
For a possible 4, I also need an extra passing test and a teammate code review.
I cannot replace that review with another test count. I leave this record for
an actual reviewer to fill in after reading the code and trying the program:

  Reviewer and date: _________________________________________________
  Commit or ZIP reviewed: ____________________________________________
  Three choices and sample labels: ___________________________________
  Invalid-input recovery and exit: ___________________________________
  Shared declarations and build: _____________________________________
  Problems found and retest: _________________________________________
  Review conclusion and signature: ___________________________________

I leave the actual Windows build and run for the demo computer. I record its
compiler, inputs, expected and actual behavior, and any retest in readMe.txt.
I leave the gate rating to the teacher and the staffing exception to staff.
*/
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "testSupport.h"

static TestContext test;

static void runAnswers(const char* answers) {
	/* I always choose the earlier menu here, even in a Gate 2 build. */
	const char* arguments[] = { "--gate1", NULL };
	runProgram(&test, arguments, answers, strlen(answers), 0, false);
}

static void contains(const char* text) {
	expect(&test, strstr(test.output, text) != NULL, text);
}

static void rejects(const char* answers) {
	runAnswers(answers);
	contains("Please enter 1, 2, or 3.");
}

static int menuLines(const char* text) {
	/* I count numbered choices at the start of a line, not numbers inside a rate. */
	int count = 0;
	while (*text) {
		if (*text >= '1' && *text <= '9' && text[1] == '.' && text[2] == ' ') {
			count++;
		}
		const char* next = strchr(text, '\n');
		if (!next) {
			break;
		}
		text = next + 1;
	}
	return count;
}

static bool hasCapitalWord(const char* text) {
	int capitals = 0;
	bool onlyCapitals = true;
	for (;;) {
		unsigned char ch = (unsigned char)*text;
		if (isalpha(ch)) {
			if (isupper(ch)) {
				capitals++;
			} else {
				onlyCapitals = false;
			}
		} else {
			if (onlyCapitals && capitals >= 2) {
				return true;
			}
			capitals = 0;
			onlyCapitals = true;
		}
		if (ch == '\0') {
			return false;
		}
		text++;
	}
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fputs("Run testGate1 with the compiled CardClarity path.\n", stderr);
		return 2;
	}
	initTests(&test, argv[1], "Gate 1 C menu checks");
	const char* names[] = { "Travel Sample", "Points Sample", "Cashback Sample",
		"Everyday Sample", "Starter Sample" };
	const char* cases[] = {
		"Three-option menu", "Five sample names", "Exact example values", "Example labels",
		"Four planned features", "Exit", "Text input", "Zero", "Out-of-range choice",
		"Negative choice", "Blank answer", "Decimal choice", "Trailing characters",
		"Oversized numbers", "Long text", "Recovery after bad input", "Surrounding spaces",
		"End of input", "No final newline", "Repeated choices", "Input after exit",
		"Hidden null", "Standard whitespace", "Whole overlong line", "Bad answer at end",
		"Whitespace-only answer", "Real read error", "Largest fitting line",
		"First overlong line", "Unknown argument", "Window on another system"
	};
	const char* expected[] = {
		"Exactly three numbered choices appear.", "Each of the five names appears once.",
		"All base rates, fees, and category bonuses match the sample records.",
		"Values are labeled as made-up examples and no words are all capitals.",
		"About lists the wallet, benefits, calculator, and recommendations.",
		"Goodbye appears and the program returns 0.",
		"Text is rejected and exit still works.", "Zero is rejected.", "Choice 9 is rejected.",
		"A negative choice is rejected.", "An empty answer is rejected.",
		"A decimal is rejected.", "Letters after a number are rejected.",
		"Both 30-digit and 300-digit numbers are rejected.",
		"A 600-character answer is one invalid answer.",
		"A bad answer is rejected, then cards display and exit works.",
		"Spaces and a tab around 1 are accepted.", "End of input closes normally.",
		"Exit works without a final newline.", "View and About return to the same menu.",
		"Exit does not process another choice after it.",
		"Hidden nulls are rejected, including at end of input.",
		"All ordinary surrounding whitespace is accepted.",
		"One overlong line causes one error, then the next answer works.",
		"A bad final answer is rejected before input ends.",
		"A whitespace-only line is rejected.",
		"A genuine read error returns 1 and does not say Goodbye.",
		"A 127-character answer ending with spaces is accepted.",
		"A 128-character answer is rejected.", "An unknown argument returns 1.",
		"The window option explains that Windows is needed and returns 1."
	};
	char answers[1400];
	for (int number = 1; number <= 31; number++) {
		startCase(&test, number, cases[number - 1], expected[number - 1]);
		switch (number) {
		case 1:
			runAnswers("3\n");
			contains("1. View sample cards");
			contains("2. About CardClarity");
			contains("3. Exit");
			expect(&test, menuLines(test.output) == 3, "exactly three numbered choices");
			break;
		case 2:
			runAnswers("1\n3\n");
			for (int i = 0; i < 5; i++) {
				expect(&test, textCount(test.output, names[i]) == 1, names[i]);
			}
			break;
		case 3: {
			const double rates[] = { 1.25, 1, 1, 1, 1 };
			const int fees[] = { 95, 250, 0, 0, 0 };
			const double bonuses[5][3] = { {5,3,2}, {4,2,1.5}, {3,2,1}, {3,2,2}, {4.5,2.5,1} };
			runAnswers("1\n3\n");
			for (int i = 0; i < 5; i++) {
				char record[512];
				snprintf(record, sizeof(record), "%s\nExample base rate: %.2f%% | Example annual fee: $%d\n"
					"Example bonus percentages: Airlines %.2f%% | Stores %.2f%% | Services %.2f%%",
					names[i], rates[i], fees[i], bonuses[i][0], bonuses[i][1], bonuses[i][2]);
				expect(&test, strstr(test.output, record) != NULL, names[i]);
			}
			contains("it is not a times multiplier");
			break;
		}
		case 4:
			runAnswers("1\n3\n");
			contains("made up for practice, not real bank offers");
			expect(&test, !hasCapitalWord(test.output), "no all-capital words in the display");
			break;
		case 5:
			runAnswers("2\n3\n");
			contains("Card wallet:"); contains("Benefits display:");
			contains("Rewards calculator:"); contains("Card recommendations:");
			break;
		case 6: runAnswers("3\n"); contains("Goodbye!"); break;
		case 7: rejects("hello\n3\n"); break;
		case 8: rejects("0\n3\n"); break;
		case 9: rejects("9\n3\n"); break;
		case 10: rejects("-1\n3\n"); break;
		case 11: rejects("\n3\n"); break;
		case 12: rejects("1.5\n3\n"); break;
		case 13: rejects("1xyz\n3\n"); break;
		case 14:
			for (int length = 30; length <= 300; length += 270) {
				memset(answers, '9', (size_t)length);
				strcpy(answers + length, "\n3\n");
				rejects(answers);
			}
			break;
		case 15:
			memset(answers, 'a', 600); strcpy(answers + 600, "\n3\n");
			rejects(answers);
			break;
		case 16:
			rejects("not a choice\n1\n3\n"); contains("Travel Sample"); contains("Goodbye!");
			break;
		case 17: runAnswers("  1  \t\n3\n"); contains("Travel Sample"); break;
		case 18: runAnswers(""); contains("No more input"); break;
		case 19: runAnswers("3"); contains("Goodbye!"); break;
		case 20:
			runAnswers("1\n2\n1\n3\n");
			expect(&test, textCount(test.output, "Travel Sample") == 2, "cards shown twice");
			expect(&test, textCount(test.output, "Enter 1, 2, or 3:") == 4, "four menu visits");
			expect(&test, textCount(test.output, "Goodbye!") == 1, "one exit message");
			break;
		case 21:
			runAnswers("3\n1\n");
			expect(&test, !strstr(test.output, "Travel Sample"), "no cards shown after Exit");
			expect(&test, textCount(test.output, "Enter 1, 2, or 3:") == 1, "one menu visit");
			break;
		case 22: {
			/* I use explicit lengths because strlen would hide the embedded null. */
			const char data[] = "1\0hidden\n3\n";
			const char* arguments[] = { "--gate1", NULL };
			const size_t lengths[] = { sizeof(data) - 1, 8 };
			for (int i = 0; i < 2; i++) {
				runProgram(&test, arguments, data, lengths[i], 0, false);
				contains("Please enter 1, 2, or 3."); contains("Goodbye!");
				expect(&test, !strstr(test.output, "Travel Sample"), "hidden answer did not show cards");
			}
			break;
		}
		case 23:
			runAnswers("\v\f 1\t\v\f\r\n3\n"); contains("Travel Sample");
			expect(&test, !strstr(test.output, "Please enter"), "no retry needed");
			break;
		case 24:
			answers[0] = '1'; memset(answers + 1, ' ', 1000); strcpy(answers + 1001, "2\n1\n3\n");
			runAnswers(answers);
			expect(&test, textCount(test.output, "Please enter 1, 2, or 3.") == 1, "one rejected line");
			expect(&test, textCount(test.output, "Travel Sample") == 1, "next choice worked once");
			contains("Goodbye!");
			break;
		case 25:
			rejects("wrong"); contains("No more input");
			expect(&test, textCount(test.output, "Please enter 1, 2, or 3.") == 1, "one rejected final answer");
			break;
		case 26: rejects(" \t\r\n3\n"); contains("Goodbye!"); break;
		case 27:
#ifdef __linux__
		{
			const char* arguments[] = { "--gate1", NULL };
			runProgram(&test, arguments, "", 0, 1, true);
			expect(&test, strstr(test.errors, "We couldn't read your answer") != NULL, "read error explained");
			expect(&test, !strstr(test.output, "Goodbye!"), "no normal exit claim");
		}
#else
			skipCase(&test, "This stdin read-error check needs Linux.");
			continue;
#endif
			break;
		case 28:
			/* I leave one byte for the ending null in the application's 128-byte array. */
			answers[0] = '1'; memset(answers + 1, ' ', 126); answers[127] = '\0';
			runAnswers(answers); contains("Travel Sample"); contains("No more input");
			expect(&test, !strstr(test.output, "Please enter"), "largest fitting line accepted");
			break;
		case 29:
			answers[0] = '1'; memset(answers + 1, ' ', 127); answers[128] = '\0';
			rejects(answers); contains("No more input");
			expect(&test, !strstr(test.output, "Travel Sample"), "overlong line did not show cards");
			break;
		case 30: {
			const char* arguments[] = { "--wrong", NULL };
			runProgram(&test, arguments, "", 0, 1, false);
			expect(&test, strstr(test.errors, "Run CardClarity without arguments") != NULL, "usage help printed");
			break;
		}
		case 31:
#ifdef _WIN32
			skipCase(&test, "Open the real Windows window separately.");
			continue;
#else
		{
			const char* arguments[] = { "--gui", NULL };
			runProgram(&test, arguments, "", 0, 1, false);
			expect(&test, strstr(test.errors, "window preview needs Windows") != NULL, "Windows requirement explained");
		}
#endif
			break;
		}
		finishCase(&test);
	}
	return finishTests(&test);
}
