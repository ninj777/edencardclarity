#include <stdio.h>
#include "wallet.h"

void initWallet(Wallet* wallet) {
	wallet->count = 0;
	for (int i = 0; i < MAX_WALLET_CARDS; i++) {
		wallet->cardIds[i] = 0;
	}
}

bool walletHasCard(const Wallet* wallet, int cardId) {
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
	wallet->count++;
	return WALLET_OK;
}

bool removeWalletCard(Wallet* wallet, int cardId) {
	for (int i = 0; i < wallet->count; i++) {
		if (wallet->cardIds[i] == cardId) {
			/* Move the later cards left to fill the empty slot. */
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
