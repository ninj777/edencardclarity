/*
Eden Yun Yu - the selected-card list

I keep selected IDs in a Wallet instead of copying full Card records. It has
five slots and a count of the used entries. For cards 1, 3, and 5, the used
part is [1, 3, 5] and count is 3. Names, rates, and fees stay in Joe's catalog,
so the wallet cannot keep a separate set of values that gradually disagrees.

I start the count and slots at zero and only search the used entries.
Adding checks the ID, space, and duplicates before writing at cardIds[count].
The count then increases. Gate 2 had five unique samples. Gate 4 adds five
real products and keeps this wallet's five-slot limit. I can mix valid IDs,
but real-card comparisons explicitly exclude any selected practice cards.

For removal, I find the ID, move later entries left, and clear the unused last
slot. Removing 3 from [1, 3, 5] leaves [1, 5]; ID 5 still means the same card.
If the ID isn't selected, the list stays as it was.

Before saving, I check the count and try the used IDs in a separate empty
wallet. Reusing the Add rules keeps file validation consistent with normal
menu additions, especially for unknown or duplicate IDs.

I pass Wallet* when a function changes the list and const Wallet* when it only
reads. Both point to the existing wallet, but const prevents changes through
that parameter. printWallet shows the selected names and example rates, or
an empty message. Viewing the list does not rewrite its save.
*/
#include <stdio.h>
#include "wallet.h"
#include "realCards.h"

void initWallet(Wallet* wallet) {
	wallet->count = 0;
	for (int i = 0; i < MAX_WALLET_CARDS; i++) {
		wallet->cardIds[i] = 0;
	}
}

bool walletHasCard(const Wallet* wallet, int cardId) {
	/* I search the used part of the array, not the unused zero-filled slots. */
	for (int i = 0; i < wallet->count; i++) {
		if (wallet->cardIds[i] == cardId) {
			return true;
		}
	}
	return false;
}

enum WalletResult addWalletCard(Wallet* wallet, int cardId) {
	if (!getCardById(cardId)) {
		return WALLET_INVALID_CARD;
	}
	if (wallet->count >= MAX_WALLET_CARDS) {
		return WALLET_FULL;
	}
	if (walletHasCard(wallet, cardId)) {
		return WALLET_DUPLICATE;
	}
	wallet->cardIds[wallet->count] = cardId;
	/* I fill the next free slot before increasing the number of used entries. */
	wallet->count++;
	return WALLET_OK;
}

bool removeWalletCard(Wallet* wallet, int cardId) {
	for (int i = 0; i < wallet->count; i++) {
		if (wallet->cardIds[i] == cardId) {
			/* I move later cards left to fill the removed slot. */
			for (int j = i; j < wallet->count - 1; j++) {
				wallet->cardIds[j] = wallet->cardIds[j + 1];
			}
			wallet->count--;
			wallet->cardIds[wallet->count] = 0;
			return true;
		}
	}
	return false;
}

bool isWalletValid(const Wallet* wallet) {
	if (!wallet || wallet->count < 0 || wallet->count > MAX_WALLET_CARDS) {
		return false;
	}
	Wallet checked = { 0 };
	/* I reuse the Add rules so a duplicate or unknown ID cannot be saved. */
	for (int i = 0; i < wallet->count; i++) {
		if (addWalletCard(&checked, wallet->cardIds[i]) != WALLET_OK) {
			return false;
		}
	}
	return true;
}

void printWallet(const Wallet* wallet) {
	printf("\nMy card wallet (%d of %d cards)\n", wallet->count, MAX_WALLET_CARDS);
	if (wallet->count == 0) {
		puts("Your wallet is empty. Choose Add a card to get started.");
		return;
	}
	bool hasRealCard = false;
	for (int i = 0; i < wallet->count; i++) if (getRealCardById(wallet->cardIds[i])) hasRealCard = true;
	puts(hasRealCard ? "Sample IDs 1-5 are practice cards. Real IDs 101-105 have dated issuer sources."
		: "These cards and rates are made up for practice, not real bank offers.");
	for (int i = 0; i < wallet->count; i++) {
		int cardId = wallet->cardIds[i];
		const Card* card = getCardById(cardId);
		if (getRealCardById(cardId)) {
			char details[3000];
			if (formatRealCard(cardId, details, sizeof(details))) printf("Card %d: %s\n", cardId, details);
		} else {
			printf("Card %d: %s | Example base rate: %.2f%%\n", cardId, card->issuer, card->rate);
		}
	}
}
