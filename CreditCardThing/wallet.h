/*
Eden Yun Yu - wallet declarations

I store catalog IDs in cardIds and the used length in count. The five-slot
limit keeps this starter wallet small. WalletResult gives the caller a reason when an
addition fails: unknown ID, full list, or duplicate. Modifying functions take
Wallet*; read-only functions take const Wallet*. The steps are in wallet.c.
*/
#ifndef WALLET_H
#define WALLET_H

#include <stdbool.h>
#include "cards.h"

/* I keep the original five-slot capacity; each selected ID must be unique. */
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
