/*
Eden Yun Yu - my assigned wallet list

I keep selected card types in a Wallet rather than copying every full Card.
The struct has five ID slots and a count of how many are used. If I select
cards 1, 3, and 5, the used part is [1, 3, 5] and count is 3. The actual names,
rates, and fees stay in Joe's catalog, so I do not save conflicting copies.

I initialize the count and slots to zero. I search only the used entries when
checking duplicates. Before adding, I check the ID, available room, and whether
the card is already there. I then write at cardIds[count] and increase count.
I use five slots because the catalog has five unique types. The earlier plan
mentioned twenty, but I cannot fill twenty unique slots from five records.

When I remove an ID, I move later entries left and clear the unused last slot.
Removing 3 from [1, 3, 5] leaves [1, 5]; it does not change what ID 5 means.
I return false if I cannot find that ID, leaving the list alone.

Before saving, I use isWalletValid to check the count and add the used IDs to
a separate empty wallet. That reuses my normal ID and duplicate rules instead
of letting the saved-file checks gradually disagree with menu additions.

I pass Wallet* to functions that change the list. I use const Wallet* when a
function only needs to read it. The pointer accesses the existing wallet, and
const prevents that function from changing it through the parameter.
I use printWallet for the selected names and example rates, including an empty
message when no card is selected. I do not write a save just for viewing.
*/
#include <stdio.h>
#include "wallet.h"

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
	if (wallet->count < 0 || wallet->count > MAX_WALLET_CARDS) {
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
	puts("These cards and rates are made up for practice, not real bank offers.");
	for (int i = 0; i < wallet->count; i++) {
		int cardId = wallet->cardIds[i];
		const Card* card = getCardById(cardId);
		printf("Card %d: %s | Example base rate: %.2f%%\n", cardId, card->issuer, card->rate);
	}
}
