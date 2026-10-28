/*
Evan Zhuang - Gate 2 wallet checks

I check add, view, remove, save, and reload in 39 cases. Each gets a temporary
folder so the normal save is left alone. Messages, exit status, and exact
saved bytes all have expected results in the arrays before the switch.

Case 3 adds and views card 3, which should be Cashback Sample. Case 4 rejects
a duplicate; cases 1 and 7 cover an empty wallet. I check a full five-card
list, space after removal, and removing a middle, first, last, absent,
or final remaining card.

In case 9 I start another process to load saved cards 1 and 3. Case 10 checks
that a saved removal stays removed after restarting. Case 12 reloads over an
unsaved edit and expects the saved list and a cleared unsaved flag.
Seeing a card before closing would not prove that it survived a restart.

I also check bad menu answers and IDs, long and hidden-null input, cancelling,
input ending, and all three unsaved-exit choices. Paths include the default,
one with spaces, and one too long. Failed Save and exit should keep the menu open.

Case 25 has 22 damaged-file examples. I use bad headers/counts, unknown and
duplicate IDs, missing or extra records, long lines, hidden nulls, and a
byte-order mark. All should fail without rewriting the file. They are examples
inside one case, not 22 extra cases. Valid Windows line endings and an ID
without a last newline are checked separately.

For case 33 I wait for a valid wallet to load, damage the file, then send
Reload through the open input pipe. Card 1 should stay in memory and the
damaged file should stay unchanged. Other cases cover a directory at the
save path, a missing folder, an old temporary file, and replacement failure.

I keep expected and actual observations in docs/gate2TestLog.txt and the
three-process example in docs/gate2DemoLog.txt. No payment details are needed.
The Linux read-error case and unsupported-window message are skipped on Windows.
Buttons, resizing, and close prompts still need a desktop check.

Gate 2 needs all five actions and required cases for a 3. Extra damaged-file
checks are its possible 4 requirement. Gate 3 still needs its QA checklist,
bug list, actual retests, and review; this test count doesn't sign that report.
*/
#include <stdio.h>
#include <string.h>
#include "testSupport.h"

static TestContext test;

static void contains(const char* text) {
	expect(&test, strstr(test.output, text) != NULL, text);
}

static void missing(const char* text) {
	expect(&test, strstr(test.output, text) == NULL, "unwanted text is absent");
}

static void runBytes(const void* answers, size_t length) {
	const char* arguments[] = { "--wallet-file", test.save, NULL };
	runProgram(&test, arguments, answers, length, 0, false);
}

static void runMenu(const char* answers) {
	runBytes(answers, strlen(answers));
}

static void saved(const char* text) {
	expect(&test, fileEquals(test.save, text, strlen(text)), "saved bytes match the expected IDs");
}

static void seed(const char* text) {
	writeText(test.save, text);
}

static void damagedFiles(void) {
	char hugeCount[400], longHeader[1100], longId[1100];
	strcpy(hugeCount, "cardClarityWallet1\n");
	size_t used = strlen(hugeCount);
	memset(hugeCount + used, '9', 300); strcpy(hugeCount + used + 300, "\n");
	memset(longHeader, 'x', 1000); strcpy(longHeader + 1000, "\n0\n");
	strcpy(longId, "cardClarityWallet1\n1\n"); used = strlen(longId);
	memset(longId + used, '1', 1000); strcpy(longId + used + 1000, "\n");
	/* I use sizeof to keep hidden null bytes in these fixtures. */
#define FILE_CASE(label, content) { label, content, sizeof(content) - 1 }
	const struct {
		const char* name;
		const char* bytes;
		size_t length;
	} files[] = {
		FILE_CASE("empty", ""),
		FILE_CASE("wrong header", "otherWallet\n0\n"),
		FILE_CASE("header only", "cardClarityWallet1\n"),
		FILE_CASE("negative count", "cardClarityWallet1\n-1\n"),
		FILE_CASE("too many cards", "cardClarityWallet1\n6\n1\n2\n3\n4\n5\n1\n"),
		{ "huge count", hugeCount, strlen(hugeCount) },
		FILE_CASE("text count", "cardClarityWallet1\ntwo\n"),
		FILE_CASE("decimal count", "cardClarityWallet1\n1.5\n"),
		FILE_CASE("zero ID", "cardClarityWallet1\n1\n0\n"),
		FILE_CASE("unknown ID", "cardClarityWallet1\n1\n6\n"),
		FILE_CASE("negative ID", "cardClarityWallet1\n1\n-1\n"),
		FILE_CASE("duplicate", "cardClarityWallet1\n2\n1\n1\n"),
		FILE_CASE("missing ID", "cardClarityWallet1\n2\n1\n"),
		FILE_CASE("extra ID", "cardClarityWallet1\n1\n1\n3\n"),
		FILE_CASE("extra blank line", "cardClarityWallet1\n0\n\n"),
		FILE_CASE("hidden null in header", "cardClarityWallet1\0hidden\n0\n"),
		FILE_CASE("hidden null in count", "cardClarityWallet1\n0\0hidden\n"),
		FILE_CASE("hidden null in ID", "cardClarityWallet1\n1\n1\0hidden\n"),
		{ "long header", longHeader, strlen(longHeader) },
		{ "long ID", longId, strlen(longId) },
		FILE_CASE("byte-order mark", "\xef\xbb\xbf" "cardClarityWallet1\n0\n"),
		FILE_CASE("ID followed by letters", "cardClarityWallet1\n1\n1abc\n")
	};
#undef FILE_CASE
	for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
		printf("Damaged-file example: %s\n", files[i].name);
		writeBytes(test.save, files[i].bytes, files[i].length);
		runMenu("2\n1\n6\n3\n");
		contains("looks damaged or uses a different format");
		contains("Your wallet is empty");
		expect(&test, fileEquals(test.save, files[i].bytes, files[i].length), "damaged file is unchanged");
	}
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fputs("Run testGate2 with the compiled CardClarity path.\n", stderr);
		return 2;
	}
	initTests(&test, argv[1], "Gate 2 C wallet checks");
	const char* names[] = {
		"Start empty", "Stable card IDs", "Add and view", "Duplicate card", "Remove middle",
		"Remove absent card", "Remove from empty", "Full wallet", "Save and restart",
		"Removal after restart", "Save empty wallet", "Reload saved list", "Missing reload",
		"Space after removal", "Bad menu choices", "Bad card numbers", "Long and hidden input",
		"Cancel actions", "End of input", "Save on exit", "Discard changes", "Keep editing",
		"Default save path", "Path with spaces", "Damaged files", "Windows line endings",
		"No final newline", "Directory as save", "Missing folder", "Existing temporary file",
		"Failed replacement", "Failed save on exit", "Damage after startup", "Numeric whitespace",
		"Remove first and last", "Real read error", "Bad arguments", "Long save path",
		"Chosen window path on another system"
	};
	const char* expected[] = {
		"A missing file starts an empty wallet without writing a save.",
		"The five names keep IDs 1 through 5 and example labels.",
		"Adding card 3 displays Cashback Sample and saves ID 3.",
		"Adding card 1 twice is rejected and saves it once.",
		"Removing 3 from [1,3,5] saves [1,5].",
		"Removing an absent ID keeps the current list.",
		"An empty list explains that there is nothing to remove.",
		"A full five-card list refuses another addition.",
		"A new process reloads the saved cards 1 and 3.",
		"A removed and saved card stays removed after restarting.",
		"Removing the last card saves a valid empty wallet.",
		"Reload restores [1] after an unsaved addition of card 3.",
		"A missing reload keeps an unsaved card available for saving.",
		"Removal frees a slot and adding appends the card.",
		"Bad answers at both menus allow recovery.",
		"Six invalid card answers are rejected before a valid answer.",
		"A long answer and hidden null are rejected before adding card 3.",
		"Zero cancels add and remove without changing [1].",
		"Input ending at nested questions exits without saving.",
		"Save and exit writes the unsaved card first.",
		"Exit without saving preserves the older saved list.",
		"Keep editing returns to the menu and later saving works.",
		"Normal launch saves and reloads savedCards.txt in its current folder.",
		"A selected filename containing spaces works across restarts.",
		"All 22 malformed files are rejected without rewriting them.",
		"Windows line endings load cards 1 and 3.",
		"The final ID is accepted without a newline.",
		"A directory used as a save reports an error and keeps its contents.",
		"A missing destination folder causes a save error without losing the live wallet.",
		"An existing temporary file blocks saving and stays unchanged.",
		"Failed replacement keeps the directory and removes the newly written temporary file.",
		"Failed Save and exit keeps the menu usable.",
		"Damage after startup causes Reload to keep the live card 1.",
		"Surrounding whitespace is allowed in numeric lines.",
		"Removing first and last leaves the middle ID 3.",
		"A genuine stdin read error returns 1.",
		"Unknown, incomplete, and empty arguments return 1 without saving.",
		"An overlong save path reports a save error without overflowing.",
		"The chosen window path is parsed before explaining that Windows is required."
	};
	char answers[1600], other[TEST_PATH_SIZE], marker[TEST_PATH_SIZE];
	for (int number = 1; number <= 39; number++) {
		startCase(&test, number, names[number - 1], expected[number - 1]);
		switch (number) {
		case 1:
			runMenu("2\n1\n6\n3\n"); contains("no saved file yet"); contains("Your wallet is empty");
			expect(&test, !pathExists(test.save), "viewing created no save");
			break;
		case 2: {
			const char* cards[] = { "Travel", "Points", "Cashback", "Everyday", "Starter" };
			runMenu("1\n3\n");
			for (int i = 0; i < 5; i++) {
				char label[80]; snprintf(label, sizeof(label), "Card %d: %s Sample", i + 1, cards[i]);
				contains(label);
			}
			contains("not real bank offers");
			break;
		}
		case 3:
			runMenu("2\n2\n3\n1\n4\n6\n3\n"); contains("Added Cashback Sample.");
			contains("Card 3: Cashback Sample | Example base rate: 1.00%");
			saved("cardClarityWallet1\n1\n3\n");
			break;
		case 4:
			runMenu("2\n2\n1\n2\n1\n4\n6\n3\n"); contains("already in your wallet");
			saved("cardClarityWallet1\n1\n1\n");
			break;
		case 5:
			seed("cardClarityWallet1\n3\n1\n3\n5\n"); runMenu("2\n3\n3\n4\n6\n3\n");
			contains("Removed Cashback Sample."); saved("cardClarityWallet1\n2\n1\n5\n");
			break;
		case 6:
			seed("cardClarityWallet1\n1\n1\n"); runMenu("2\n3\n2\n4\n6\n3\n");
			contains("isn't in your wallet"); saved("cardClarityWallet1\n1\n1\n");
			break;
		case 7:
			runMenu("2\n3\n6\n3\n"); contains("Your wallet is empty"); missing("Enter the card number to remove");
			break;
		case 8:
			seed("cardClarityWallet1\n5\n1\n2\n3\n4\n5\n"); runMenu("2\n2\n1\n4\n6\n3\n");
			contains("Your wallet is full"); contains("My card wallet (5 of 5 cards)");
			saved("cardClarityWallet1\n5\n1\n2\n3\n4\n5\n");
			break;
		case 9:
			/* I launch another process to check the saved file, not just the live list. */
			runMenu("2\n2\n1\n2\n3\n4\n6\n3\n"); saved("cardClarityWallet1\n2\n1\n3\n");
			runMenu("2\n1\n6\n3\n"); contains("Loaded your saved card list");
			contains("My card wallet (2 of 5 cards)"); contains("Card 1: Travel Sample |");
			contains("Card 3: Cashback Sample |"); missing("Card 2: Points Sample |");
			break;
		case 10:
			seed("cardClarityWallet1\n3\n1\n3\n5\n"); runMenu("2\n3\n3\n4\n6\n3\n");
			runMenu("2\n1\n6\n3\n"); saved("cardClarityWallet1\n2\n1\n5\n");
			contains("My card wallet (2 of 5 cards)"); missing("Card 3: Cashback Sample |");
			break;
		case 11:
			seed("cardClarityWallet1\n1\n1\n"); runMenu("2\n3\n1\n4\n6\n3\n");
			saved("cardClarityWallet1\n0\n"); runMenu("2\n1\n6\n3\n"); contains("Your wallet is empty");
			break;
		case 12: {
			seed("cardClarityWallet1\n1\n1\n"); runMenu("2\n2\n3\n5\n1\n6\n3\n");
			const char* view = strstr(test.output, "Loaded your saved card list.");
			if (view) {
				view = strstr(view + 1, "Loaded your saved card list.");
			}
			expect(&test, view != NULL, "Reload reported a second successful load");
			if (view) {
				expect(&test, strstr(view, "My card wallet (1 of 5 cards)") != NULL, "one saved card restored");
				expect(&test, !strstr(view, "Card 3: Cashback Sample |"), "unsaved card removed by Reload");
				expect(&test, !strstr(view, "You haven't saved your changes yet"), "unsaved flag cleared");
			}
			saved("cardClarityWallet1\n1\n1\n");
			break;
		}
		case 13:
			runMenu("2\n2\n1\n5\n1\n4\n6\n3\n"); contains("Your current list hasn't changed");
			contains("Card 1: Travel Sample |"); saved("cardClarityWallet1\n1\n1\n");
			break;
		case 14:
			seed("cardClarityWallet1\n5\n1\n2\n3\n4\n5\n"); runMenu("2\n3\n3\n2\n3\n4\n6\n3\n");
			saved("cardClarityWallet1\n5\n1\n2\n4\n5\n3\n");
			break;
		case 15: {
			char huge[301]; memset(huge, '9', 300); huge[300] = '\0';
			const char* bad[] = { "", "hello", "1.5", "-1", "0", "9", "1x", huge };
			for (int i = 0; i < 8; i++) {
				snprintf(answers, sizeof(answers), "%s\n2\n%s\n1\n6\n3\n", bad[i], bad[i]);
				runMenu(answers); contains("Please enter 1, 2, or 3.");
				contains("Please enter a whole number from 1 to 6."); contains("Your wallet is empty");
			}
			break;
		}
		case 16:
			runMenu("2\n2\n\nwrong\n-1\n6\n1.5\n1x\n3\n4\n6\n3\n");
			expect(&test, textCount(test.output, "Please enter a whole number from 0 to 5.") == 6, "six bad answers rejected");
			saved("cardClarityWallet1\n1\n3\n");
			break;
		case 17: {
			const char ending[] = "\n1\0hidden\n3\n4\n6\n3\n";
			memcpy(answers, "2\n2\n", 4); memset(answers + 4, '1', 1000);
			memcpy(answers + 1004, ending, sizeof(ending) - 1);
			runBytes(answers, 1004 + sizeof(ending) - 1);
			expect(&test, textCount(test.output, "Please enter a whole number from 0 to 5.") == 2, "both invalid lines rejected");
			saved("cardClarityWallet1\n1\n3\n");
			break;
		}
		case 18:
			seed("cardClarityWallet1\n1\n1\n"); runMenu("2\n2\n0\n3\n0\n6\n3\n");
			contains("Add cancelled"); contains("Remove cancelled"); missing("You haven't saved your changes yet");
			saved("cardClarityWallet1\n1\n1\n");
			break;
		case 19: {
			const char* endings[] = { "", "2\n", "2\n2\n", "2\n2\n1\n" };
			for (int i = 0; i < 4; i++) {
				runMenu(endings[i]); contains("No more input. Goodbye!");
				expect(&test, !pathExists(test.save), "end of input did not create a save");
			}
			break;
		}
		case 20:
			runMenu("2\n2\n1\n6\n3\n1\n"); contains("Save and exit"); saved("cardClarityWallet1\n1\n1\n");
			break;
		case 21:
			seed("cardClarityWallet1\n1\n1\n"); runMenu("2\n2\n3\n6\n3\n2\n");
			saved("cardClarityWallet1\n1\n1\n");
			break;
		case 22:
			runMenu("2\n2\n1\n6\n3\n3\n2\n4\n6\n3\n");
			expect(&test, textCount(test.output, "Goodbye!") == 1, "only the final Exit closed the program");
			saved("cardClarityWallet1\n1\n1\n");
			break;
		case 23: {
			const char* arguments[] = { NULL };
			const char* first = "2\n2\n5\n4\n6\n3\n";
			const char* second = "2\n1\n6\n3\n";
			runProgram(&test, arguments, first, strlen(first), 0, false);
			saved("cardClarityWallet1\n1\n5\n");
			runProgram(&test, arguments, second, strlen(second), 0, false); contains("Card 5: Starter Sample |");
			break;
		}
		case 24:
			joinPath(test.save, sizeof(test.save), test.folder, "my saved cards.txt");
			runMenu("2\n2\n2\n4\n6\n3\n"); saved("cardClarityWallet1\n1\n2\n");
			runMenu("2\n1\n6\n3\n"); contains("Card 2: Points Sample |");
			break;
		case 25: damagedFiles(); break;
		case 26:
			seed("cardClarityWallet1\r\n2\r\n1\r\n3\r\n"); runMenu("2\n1\n6\n3\n");
			contains("Card 1: Travel Sample |"); contains("Card 3: Cashback Sample |");
			break;
		case 27:
			seed("cardClarityWallet1\n1\n5"); runMenu("2\n1\n6\n3\n"); contains("Card 5: Starter Sample |");
			break;
		case 28:
			makeFolder(test.save); joinPath(marker, sizeof(marker), test.save, "keep.txt"); writeText(marker, "keep this");
			runMenu("2\n1\n6\n3\n"); contains("Couldn't read your saved cards");
			expect(&test, fileEquals(marker, "keep this", 9), "directory contents preserved");
			break;
		case 29:
			joinPath(test.save, sizeof(test.save), test.folder, "missing/savedCards.txt");
			runMenu("2\n2\n1\n4\n1\n6\n3\n2\n"); contains("Couldn't save your cards");
			contains("Card 1: Travel Sample |"); expect(&test, !pathExists(test.save), "no save created in missing folder");
			break;
		case 30:
			seed("cardClarityWallet1\n1\n1\n");
			joinPath(other, sizeof(other), test.folder, "savedCards.txt.tmp"); writeText(other, "keep temporary data");
			runMenu("2\n2\n3\n4\n1\n6\n3\n2\n"); contains("Couldn't save your cards"); contains("Card 3: Cashback Sample |");
			saved("cardClarityWallet1\n1\n1\n"); expect(&test, fileEquals(other, "keep temporary data", 19), "old temporary file preserved");
			break;
		case 31:
			makeFolder(test.save); joinPath(marker, sizeof(marker), test.save, "keep.txt"); writeText(marker, "keep");
			runMenu("2\n2\n1\n4\n6\n3\n2\n"); contains("Couldn't save your cards");
			joinPath(other, sizeof(other), test.folder, "savedCards.txt.tmp");
			expect(&test, !pathExists(other), "new temporary file removed");
			expect(&test, fileEquals(marker, "keep", 4), "directory contents preserved");
			break;
		case 32:
			joinPath(test.save, sizeof(test.save), test.folder, "missing/savedCards.txt");
			runMenu("2\n2\n1\n6\n3\n1\n2\n1\n6\n3\n2\n"); contains("Couldn't save your cards"); contains("Card 1: Travel Sample |");
			expect(&test, textCount(test.output, "Goodbye!") == 1, "failure did not close the program early");
			break;
		case 33: {
			/* I damage the file only after the first valid wallet has loaded. */
			seed("cardClarityWallet1\n1\n1\n");
			const char* arguments[] = { "--wallet-file", test.save, NULL };
			startProgram(&test, arguments, false);
			expect(&test, waitForOutput(&test, "Loaded your saved card list."), "initial save loaded before damage");
			seed("cardClarityWallet1\n2\n3\n3\n");
			const char* input = "2\n5\n1\n6\n3\n";
			sendAnswers(&test, input, strlen(input)); closeAnswers(&test); waitProgram(&test);
			expect(&test, test.status == 0 && test.errors[0] == '\0', "program stayed usable after failed Reload");
			contains("looks damaged or uses a different format"); contains("Card 1: Travel Sample |");
			missing("Card 3: Cashback Sample |"); saved("cardClarityWallet1\n2\n3\n3\n");
			break;
		}
		case 34:
			seed("cardClarityWallet1\n 1 \t\n 3 \t\n"); runMenu(" 2 \n 1 \n 6 \n 3 \n"); contains("Card 3: Cashback Sample |");
			break;
		case 35:
			seed("cardClarityWallet1\n3\n1\n3\n5\n"); runMenu("2\n3\n1\n3\n5\n4\n6\n3\n"); saved("cardClarityWallet1\n1\n3\n");
			break;
		case 36:
#ifdef __linux__
		{
			const char* arguments[] = { "--wallet-file", test.save, NULL };
			runProgram(&test, arguments, "", 0, 1, true);
			expect(&test, strstr(test.errors, "We couldn't read your answer") != NULL, "real read error explained");
			missing("Goodbye!");
		}
#else
			skipCase(&test, "This stdin read-error check needs Linux.");
			continue;
#endif
			break;
		case 37: {
			const char* invalid[][5] = {
				{ "--wrong", NULL }, { "--wallet-file", NULL }, { "--wallet-file", "", NULL },
				{ "--wallet-file", test.save, "extra", NULL }, { "--gui", "--wallet-file", NULL },
				{ "--gui", "--wallet-file", "", NULL }, { "--gate1", "extra", NULL },
				{ "--gui", "--wrong", "cards.txt", NULL }
			};
			for (int i = 0; i < 8; i++) {
				runProgram(&test, invalid[i], "", 0, 1, false);
				expect(&test, strstr(test.errors, "Run CardClarity without arguments") != NULL, "bad arguments explained");
				expect(&test, !pathExists(test.save), "bad arguments created no save");
			}
			break;
		}
		case 38: {
			char path[5001]; memset(path, 'a', 5000); path[5000] = '\0';
			const char* arguments[] = { "--wallet-file", path, NULL };
			const char* input = "2\n2\n1\n4\n6\n3\n2\n";
			runProgram(&test, arguments, input, strlen(input), 0, false); contains("Couldn't save your cards");
			expect(&test, !pathExists(path), "overlong save path created no file");
			char temporary[5010]; snprintf(temporary, sizeof(temporary), "%s.tmp", path);
			expect(&test, !pathExists(temporary), "overlong save path created no temporary file");
			break;
		}
		case 39:
#ifdef _WIN32
			skipCase(&test, "Open the window on the desktop to check this mode.");
			continue;
#else
		{
			const char* arguments[] = { "--gui", "--wallet-file", test.save, NULL };
			runProgram(&test, arguments, "", 0, 1, false);
			expect(&test, strstr(test.errors, "window preview needs Windows") != NULL, "chosen window path accepted");
			expect(&test, !strstr(test.errors, "Run CardClarity without arguments"), "valid window arguments did not print usage help");
			expect(&test, !pathExists(test.save), "unsupported window created no save");
		}
#endif
			break;
		}
		finishCase(&test);
	}
	return finishTests(&test);
}
