/*
Oliver Dolfin - menu and build notes

My part is joining the menus, Joe's card records, and Eden's wallet functions.
The console stays here, while the Windows controls are in window.c. That keeps
the two views easier to follow. The application and its check programs use C11.

For Gate 1, I need three menu choices, at least three named sample cards, valid
and invalid input, and a clean exit. I'm the assigned lead for that gate, due
Thursday, October 22, 2026. The earlier menu is still available with --gate1.
There are five samples, so the starter has more than the required three.

This is the feature checklist from our proposal, with the work divided up:
  Card wallet: Eden handles the list and saving; I connect the menus.
  Benefits display: Joe handles the records; Evan checks the wording.
  Rewards calculator: I connect the calculations; Joe checks the examples.
  Card recommendations: I compare cards using Joe's checked data.
The wallet works for Gate 2. Gate 4 adds researched benefits, whole-cent
purchase amounts, comparisons, ties and limits through realCards and rewards.
The practice reward formula remains separate from those real-card estimates.

Eden leads Gate 2, due Wednesday, October 28, 2026. I connect its five actions:
add, view, remove, save, and reload. The menu passes IDs to Eden's functions
and gets the names from Joe's catalog. Evan's C cases check the results.
Keeping the rules in those modules also lets the window call the same code.

At startup, I prepare the samples and check the command-line arguments:
  No argument opens the Gate 2 console menu.
  --gate1 opens the earlier starter.
  --wallet-file path uses another save for the console.
  --gui opens the Windows window with savedCards.txt.
  --gui --wallet-file path uses another save for the window.
  --gate4 uses real products and a separate realSavedCards.txt wallet.
  --gate4 --gui opens that mode in the existing Windows window.
An incomplete or unknown argument returns 1. A normal exit returns 0.
The actual compiler commands are in readMe.txt.

I use a loop for each menu. Viewing samples or reading About finishes that
action, then the loop asks again. It does not call another copy of the menu.
The wallet goes to the submenu by address so both functions use the same list.

The changed flag is how I track unsaved additions and removals. A successful
save or reload clears it, but Back keeps it. If there are edits at Exit, I
offer save, discard, or keep editing. A failed save also keeps the menu open.
When input ends, the program warns about unsaved edits before closing.

I label console Reload as undoing unsaved changes. The window asks first too.
Both views use the same storage format. Neither needs actual payment numbers,
security codes, or bank passwords.

For the build I include main, cards, logger, input, wallet, storage, and window.
Their headers supply the shared declarations. Visual Studio uses C11 and
CompileAsC, and its launch argument is --gui. The old page-reader placeholder
had no working code and never supplied bank data or a benefits feature.

For Gate 1, I need the checklist, owners, source, compile output, and five
expected/actual checks. The build log covers compilation; Evan's cases cover
startup, samples, invalid input, and exit. A possible 4 also needs the extra
recovery case and a real teammate code review. That review record is at the
top of tests/testGate1.c. For Gate 2, the extra rating requirement is passing
additional damaged-file cases along with the five working actions.

I keep the actual build, C test, and restart output in the three docs files.
The source-size snapshot before these annotations is commit bc9ae72:
  My menu/window/logger area: 734 lines; Gate 2 change +619/-395.
  Eden's input/wallet/storage area: 316 lines; change +316/-0.
  Joe's card area: 257 lines; change +17/-9.
  Evan's C checks and support: 1,285 lines; change +1,232/-326.
These file amounts include headers, comments, and blank lines. They do not
measure personal authorship or time. Current sizes are in readMe.txt.

A cross-build does not show that the window ran. I leave the Visual Studio
and Windows runtime result open until it is checked on the demo computer.
Staff also need to confirm the four-student/six-investigator exception.
The teacher decides the gate ratings.
*/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "cards.h"
#include "input.h"
#include "wallet.h"
#include "storage.h"
#include "window.h"
#include "rewards.h"

/* I use -1 for input ending and -2 for an actual read failure. */
static int readChoice(void) {
	int choice;
	enum InputResult result = readNumber(stdin, 1, 3, &choice);
	if (result == INPUT_END) {
		return -1;
	}
	if (result == INPUT_ERROR) {
		return -2;
	}
	if (result != INPUT_OK) {
		return 0;
	}
	return choice;
}

/* I repeat this question for bad answers, but stop when input ends or fails. */
static int askNumber(const char* prompt, int minimum, int maximum) {
	for (;;) {
		printf("%s", prompt);
		fflush(stdout);
		int answer;
		enum InputResult result = readNumber(stdin, minimum, maximum, &answer);
		if (result == INPUT_OK) {
			return answer;
		}
		if (result == INPUT_END) {
			return -1;
		}
		if (result == INPUT_ERROR) {
			return -2;
		}
		printf("Please enter a whole number from %d to %d.\n", minimum, maximum);
	}
}

static void showLoadResult(enum StorageResult result) {
	/* I keep the status messages here and the file checks in Eden's module. */
	if (result == STORAGE_OK) {
		puts("Loaded your saved card list.");
	} else if (result == STORAGE_MISSING) {
		puts("There's no saved file yet. Your current list hasn't changed.");
	} else if (result == STORAGE_BAD_FILE) {
		puts("That file looks damaged or uses a different format. Your cards haven't changed.");
	} else {
		puts("Couldn't read your saved cards. Your list hasn't changed.");
	}
}

static bool trySave(const char* filePath, const Wallet* wallet) {
	/* I only tell the caller it saved after the storage function succeeds. */
	enum StorageResult result = saveWallet(filePath, wallet);
	if (result == STORAGE_OK) {
		printf("Saved your card list to %s.\n", filePath);
		return true;
	}
	if (result == STORAGE_INVALID_WALLET) {
		puts("Your card list isn't valid, so it wasn't saved.");
	} else {
		puts("Couldn't save your cards. Check that the folder allows saving and no old .tmp file is blocking it.");
	}
	return false;
}

/* I return 0 for Back, -1 for input ending, and -2 for a read error. */
static int runWalletMenu(Wallet* wallet, const char* filePath, bool* changed, bool gateFour) {
	for (;;) {
		printf("\nMy card wallet (%d of %d cards)\n", wallet->count, MAX_WALLET_CARDS);
		if (*changed) {
			puts("You haven't saved your changes yet.");
		}
		puts("1. View my cards");
		puts("2. Add a card");
		puts("3. Remove a card");
		puts("4. Save my cards");
		puts("5. Reload my last save (undoes unsaved changes)");
		puts("6. Back");
		int choice = askNumber("Choose 1 to 6: ", 1, 6);
		if (choice < 0) {
			return choice;
		}
		if (choice == 1) {
			printWallet(wallet);
		} else if (choice == 2) {
			if (wallet->count == MAX_WALLET_CARDS) {
				puts("Your wallet is full. Remove a card before adding another.");
				continue;
			}
			if (gateFour) printRealCards(); else printSampleCards();
			int cardId = askNumber("Enter a card number, or 0 to cancel: ", 0,
				gateFour ? LAST_REAL_CARD_ID : SAMPLE_CARD_COUNT);
			if (cardId < 0) {
				return cardId;
			}
			if (cardId == 0) {
				puts("Add cancelled.");
				continue;
			}
			enum WalletResult result = addWalletCard(wallet, cardId);
			if (result == WALLET_OK) {
				printf("Added %s.\n", getCardById(cardId)->issuer);
				/* I mark an edit here, not when someone just opens the Add question. */
				*changed = true;
			} else if (result == WALLET_DUPLICATE) {
				puts("That card is already in your wallet.");
			} else {
				puts("That card couldn't be added.");
			}
		} else if (choice == 3) {
			printWallet(wallet);
			if (wallet->count == 0) {
				continue;
			}
			int cardId = askNumber("Enter the card number to remove, or 0 to cancel: ", 0,
				gateFour ? LAST_REAL_CARD_ID : SAMPLE_CARD_COUNT);
			if (cardId < 0) {
				return cardId;
			}
			if (cardId == 0) {
				puts("Remove cancelled.");
			} else if (removeWalletCard(wallet, cardId)) {
				printf("Removed %s.\n", getCardById(cardId)->issuer);
				*changed = true;
			} else {
				puts("That card isn't in your wallet.");
			}
		} else if (choice == 4) {
			if (trySave(filePath, wallet)) {
				*changed = false;
			}
		} else if (choice == 5) {
			/* I reload the saved version; the option label explains that it undoes edits. */
			enum StorageResult result = loadWallet(filePath, wallet);
			showLoadResult(result);
			if (result == STORAGE_OK) {
				*changed = false;
			}
		} else {
			return 0;
		}
	}
}

static int finishInput(int result, bool changed) {
	/* I report unsaved edits even if the person closes input without choosing Exit. */
	if (changed) {
		puts("Your last changes weren't saved.");
	}
	if (result == -2) {
		fputs("We couldn't read your answer. Please restart CardClarity.\n", stderr);
		return 1;
	}
	puts("\nNo more input. Goodbye!");
	return 0;
}

static int runConsoleMenu(bool gate1Mode, const char* filePath, bool gateFour) {
	/* I use one wallet for this session and share it with the submenu by address. */
	Wallet wallet;
	initWallet(&wallet);
	bool changed = false;
	puts("CardClarity");
	puts("You don't need to enter card numbers or bank passwords.");
	if (!gate1Mode) {
		printf("Saved-card file: %s\n", filePath);
		showLoadResult(loadWallet(filePath, &wallet));
	}
	for (;;) {
		puts(gateFour ? "\n1. View real card benefits" : "\n1. View sample cards");
		puts(gate1Mode ? "2. About CardClarity" : "2. My card wallet");
		puts(gateFour ? "3. Compare a purchase\n4. Exit" : "3. Exit");
		int choice;
		if (gateFour) choice = askNumber("Choose 1 to 4: ", 1, 4);
		else {
			printf("Enter 1, 2, or 3: "); fflush(stdout);
			choice = readChoice();
		}
		if (choice < 0) {
			return finishInput(choice, changed);
		}
		if (choice == 1) {
			if (gateFour) printRealCards(); else printSampleCards();
		} else if (choice == 2 && gate1Mode) {
			puts("\nWhat we're planning");
			puts("Card wallet: add, view, and remove card types.");
			puts("Benefits display: show each card's rewards and rules.");
			puts("Rewards calculator: work out estimated cashback for a purchase.");
			puts("Card recommendations: help choose a card for a purchase.");
			puts("For Gate 1, we've built the menu and five sample cards.");
		} else if (choice == 2) {
			int result = runWalletMenu(&wallet, filePath, &changed, gateFour);
			if (result < 0) {
				return finishInput(result, changed);
			}
		} else if (gateFour && choice == 3) {
			int result = compareFromConsole(&wallet);
			if (result < 0) return finishInput(result, changed);
		} else if (choice == (gateFour ? 4 : 3)) {
			if (changed) {
				/* I keep editing when asked, or when saving before Exit fails. */
				puts("\nYou haven't saved your changes yet.");
				puts("1. Save and exit");
				puts("2. Exit without saving");
				puts("3. Keep editing");
				int answer = askNumber("Enter 1, 2, or 3: ", 1, 3);
				if (answer < 0) {
					return finishInput(answer, changed);
				}
				if (answer == 3 || (answer == 1 && !trySave(filePath, &wallet))) {
					continue;
				}
			}
			puts("Goodbye!");
			return 0;
		} else {
			puts("Please enter 1, 2, or 3.");
		}
	}
}

int main(int argc, char* argv[]) {
	/* I prepare the catalog before any menu, saved ID, or window tries to read it. */
	initTestCards();
	/* Gate 4 is opt-in; the Gate 2 commands and sample saves keep their meaning. */
	if (argc == 2 && strcmp(argv[1], "--gate4") == 0)
		return runConsoleMenu(false, "realSavedCards.txt", true);
	if (argc == 3 && strcmp(argv[1], "--gate4") == 0 && strcmp(argv[2], "--gui") == 0)
		return runGateFourWindow("realSavedCards.txt");
	if (argc == 4 && strcmp(argv[1], "--gate4") == 0 && strcmp(argv[2], "--wallet-file") == 0 && argv[3][0])
		return runConsoleMenu(false, argv[3], true);
	if (argc == 5 && strcmp(argv[1], "--gate4") == 0 && strcmp(argv[2], "--gui") == 0
		&& strcmp(argv[3], "--wallet-file") == 0 && argv[4][0]) return runGateFourWindow(argv[4]);
	if (argc == 2 && strcmp(argv[1], "--gui") == 0) {
		return runWindow("savedCards.txt");
	}
	if (argc == 4 && strcmp(argv[1], "--gui") == 0
		&& strcmp(argv[2], "--wallet-file") == 0 && argv[3][0] != '\0') {
		return runWindow(argv[3]);
	}
	if (argc == 2 && strcmp(argv[1], "--gate1") == 0) {
		return runConsoleMenu(true, "savedCards.txt", false);
	}
	if (argc == 3 && strcmp(argv[1], "--wallet-file") == 0 && argv[2][0] != '\0') {
		return runConsoleMenu(false, argv[2], false);
	}
	if (argc != 1) {
		fputs("Run CardClarity without arguments, with --wallet-file path, --gate1, --gate4, or --gui on Windows.\n", stderr);
		return 1;
	}
	return runConsoleMenu(false, "savedCards.txt", false);
}
