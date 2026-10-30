/* Gate 4, Oliver's comparisons and Joe's expected-value area. */
#ifndef REWARDS_H
#define REWARDS_H

#include <stdint.h>
#include "realCards.h"
#include "wallet.h"

/* I cap one estimate at $1,000,000 to keep arithmetic and input sensible. */
#define MAX_PURCHASE_CENTS INT64_C(100000000)

typedef struct {
	int64_t amountCents;
	enum PurchaseCategory category;
	enum PurchaseChannel channel;
	int date; /* YYYYMMDD, validated before use. */
	int64_t priorCategoryCents; /* Blue Cash Everyday only; -1 means unknown. */
} Purchase;

typedef struct {
	int64_t minimumCents, maximumCents;
	int rateBp, baseRateBp;
	int64_t bonusSpendCents, baseSpendCents;
	bool conditional;
} RewardEstimate;

bool estimateCashback(int cardId, const Purchase* purchase, RewardEstimate* result);
bool formatComparison(const Wallet* wallet, const Purchase* purchase, char* text, size_t capacity);
int compareFromConsole(const Wallet* wallet);

#endif
