/*
Oliver Dolfin - my assigned menu and build work

My part is connecting the menus, the card records, and Eden's wallet functions.
I keep the main menu in this file and the Windows controls in window.c. This
lets me follow the console without scrolling through the whole window code.
I use C11 for the application and keep the tests in C too.

For Gate 1, I need a three-option starter, at least three named sample cards,
valid input, invalid input, and a working exit. Oliver Dolfin is the assigned
lead, and the proposal date is Thursday, October 22, 2026. I keep that earlier
menu available with --gate1. The current catalog has five examples, which is
more than the required three. I keep all of them rather than hiding two.

I keep this feature checklist separate from what is already finished:
  Card wallet - Eden handles the list and saving; I connect the menu.
  Benefits display - Joe handles records; Evan checks whether labels are clear.
  Rewards calculator - I connect the calculations; Joe checks example values.
  Card recommendations - I handle comparisons using Joe's checked card data.
The wallet works for Gate 2. I still need the later researched benefits,
whole-cent purchase calculations, comparisons, ties, and reward restrictions.
I do not treat the old practice formula as the finished calculator.

For Gate 2, I connect adding, viewing, removing, saving, and reloading chosen
card types. Eden Yun Yu leads that gate, due Wednesday, October 28, 2026.
I pass IDs to Eden's functions, use Joe's catalog to get the names, and leave
Evan's separate test programs to check the resulting output and saved bytes.
I keep input and file rules in Eden's modules so the window can reuse them.

I start by preparing the samples in main. Then I check the launch arguments:
  No argument: I open the three-option Gate 2 console menu.
  --gate1: I open the earlier starter menu.
  --wallet-file path: I use another save for the console.
  --gui: I open the Windows window using savedCards.txt.
  --gui --wallet-file path: I use another save for the window.
I return 1 for an unsupported or incomplete argument instead of ignoring it.
I return 0 for normal exit. The readMe.txt file has the actual build commands.

I use a loop for each menu. After showing the samples or the About text, I
reach the next loop and ask again. I do not start another copy of the menu
function. When I enter the wallet submenu, I pass its address so actions can
change the same wallet instead of a temporary copy.

I use changed to remember an unsaved addition or removal. A successful save
or reload clears it. Going Back keeps the current list. If I choose Exit
with edits, I offer save, discard, or keep editing. A failed save returns to
the menu. If input ends, I warn about unsaved edits before closing.

I label Reload as undoing unsaved changes in the console. I also ask for
confirmation in the window. Both versions use the same storage functions.
I never ask for actual payment numbers, security codes, or bank passwords.

For the build, I include main, cards, logger, input, wallet, storage, and
window as seven C files. I keep their declarations in the matching headers.
I use C11 and warnings. In Visual Studio I select CompileAsC, and --gui is
the launch argument. I leave the old pagereader.c stub out of the build.

For Gate 1 evidence, I need the feature checklist, owners, source, actual
compile output, and five expected/actual checks. Evan's cases cover startup,
sample display, invalid input, and exit; the build log covers compilation.
For a possible 4, I also need the extra recovery check and a real teammate
code review. I keep that review record at the top of tests/testGate1.c.
For Gate 2, I need all five actions, restart proof, and input/file checks.
The additional damaged-file cases are its extra rating requirement.

I use the three files in docs for actual build, C test, and restart output.
I keep the source-size snapshot before these notes at commit bc9ae72:
  My assigned menu/window/logger files: 734 lines; Gate 2 change +619/-395.
  Eden's input/wallet/storage files: 316 lines; Gate 2 change +316/-0.
  Joe's card files: 257 lines; Gate 2 change +17/-9.
  Evan's C checks and support: 1,285 lines; Gate 2 change +1,232/-326.
I include headers, comments, and blank lines in those file amounts. I do not
use them as a measure of personal authorship or time. I keep the current file
sizes in readMe.txt because moving notes into code increases the line counts.
I leave the Visual Studio and Windows runtime result unverified until the
program runs there. I also leave the proposal's four-student/six-investigator
exception for staff to confirm. I leave the final gate ratings to the teacher.
*/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "cards.h"
#include "input.h"
#include "wallet.h"
#include "storage.h"
#include "window.h"

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
static int runWalletMenu(Wallet* wallet, const char* filePath, bool* changed) {
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
			printSampleCards();
			int cardId = askNumber("Enter a card number, or 0 to cancel: ", 0, SAMPLE_CARD_COUNT);
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
			int cardId = askNumber("Enter the card number to remove, or 0 to cancel: ", 0, SAMPLE_CARD_COUNT);
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

static int runConsoleMenu(bool gate1Mode, const char* filePath) {
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
		puts("\n1. View sample cards");
		puts(gate1Mode ? "2. About CardClarity" : "2. My card wallet");
		puts("3. Exit");
		printf("Enter 1, 2, or 3: ");
		fflush(stdout);
		int choice = readChoice();
		if (choice < 0) {
			return finishInput(choice, changed);
		}
		if (choice == 1) {
			printSampleCards();
		} else if (choice == 2 && gate1Mode) {
			puts("\nWhat we're planning");
			puts("Card wallet: add, view, and remove card types.");
			puts("Benefits display: show each card's rewards and rules.");
			puts("Rewards calculator: work out estimated cashback for a purchase.");
			puts("Card recommendations: help choose a card for a purchase.");
			puts("For Gate 1, we've built the menu and five sample cards.");
		} else if (choice == 2) {
			int result = runWalletMenu(&wallet, filePath, &changed);
			if (result < 0) {
				return finishInput(result, changed);
			}
		} else if (choice == 3) {
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
	if (argc == 2 && strcmp(argv[1], "--gui") == 0) {
		return runWindow("savedCards.txt");
	}
	if (argc == 4 && strcmp(argv[1], "--gui") == 0
		&& strcmp(argv[2], "--wallet-file") == 0 && argv[3][0] != '\0') {
		return runWindow(argv[3]);
	}
	if (argc == 2 && strcmp(argv[1], "--gate1") == 0) {
		return runConsoleMenu(true, "savedCards.txt");
	}
	if (argc == 3 && strcmp(argv[1], "--wallet-file") == 0 && argv[2][0] != '\0') {
		return runConsoleMenu(false, argv[2]);
	}
	if (argc != 1) {
		fputs("Run CardClarity without arguments, with --wallet-file path, --gate1, or --gui on Windows.\n", stderr);
		return 1;
	}
	return runConsoleMenu(false, "savedCards.txt");
}
