/*
Eden Yun Yu - my assigned wallet structure

I store catalog IDs in cardIds and the used length in count. I use the same
five-card limit as the catalog so every type can be selected once. I return a
named WalletResult for invalid IDs, duplicates, and a full wallet. I keep the
modifying parameters as Wallet* and read-only parameters as const Wallet*.
I explain additions and moving entries after removal in wallet.c.
*/
#ifndef WALLET_H
#define WALLET_H

#include <stdbool.h>
#include "cards.h"

/* I allow every card in the five-card catalog once. */
#define MAX_WALLET_CARDS SAMPLE_CARD_COUNT

typedef struct {
	int cardIds[MAX_WALLET_CARDS];
	int count;
} Wallet;

enum WalletResult {
	WALLET_OK,
	WALLET_INVALID_CARD,
	WALLET_DUPLICATE,
	WALLET_FULL
};

void initWallet(Wallet* wallet);
bool walletHasCard(const Wallet* wallet, int cardId);
/* I reject unknown IDs, a full list, and duplicates when adding. */
enum WalletResult addWalletCard(Wallet* wallet, int cardId);
/* I remove by catalog ID, not by the card's current position. */
bool removeWalletCard(Wallet* wallet, int cardId);
bool isWalletValid(const Wallet* wallet);
void printWallet(const Wallet* wallet);

#endif
