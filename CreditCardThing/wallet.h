#ifndef WALLET_H
#define WALLET_H

#include <stdbool.h>
#include "cards.h"

/* We can save every card in the current five-card catalog once. */
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
/* Adding rejects unknown IDs, a full list, and duplicate card types. */
enum WalletResult addWalletCard(Wallet* wallet, int cardId);
/* Removal uses the catalog ID, not the card's current position in the list. */
bool removeWalletCard(Wallet* wallet, int cardId);
bool isWalletValid(const Wallet* wallet);
void printWallet(const Wallet* wallet);

#endif
