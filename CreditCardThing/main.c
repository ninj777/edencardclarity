#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "cards.h"
#include "input.h"
#include "wallet.h"
#include "storage.h"
#include "window.h"

/* -1 means input ended; -2 means reading failed. */
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

/* Wallet questions repeat until we get an answer or input ends. */
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

/* Return 0 for Back, -1 for end of input, or -2 for a read error. */
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
