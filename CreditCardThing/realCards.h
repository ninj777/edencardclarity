/* Gate 4, Joe's catalog area. I keep researched terms separate from samples. */
#ifndef REAL_CARDS_H
#define REAL_CARDS_H

#include <stdbool.h>
#include <stddef.h>
#include "cards.h"

#define REAL_CARD_COUNT 20
#define FIRST_REAL_CARD_ID 101
#define LAST_REAL_CARD_ID 120
#define REAL_CARD_TEXT_CAPACITY 10000

enum PurchaseCategory {
	PURCHASE_OTHER, PURCHASE_DINING, PURCHASE_GROCERIES, PURCHASE_GAS,
	PURCHASE_ONLINE_RETAIL, PURCHASE_DRUGSTORE, PURCHASE_HOTEL,
	PURCHASE_CAR_RENTAL, PURCHASE_VACATION_RENTAL, PURCHASE_FLIGHT,
	PURCHASE_ENTERTAINMENT, PURCHASE_STREAMING, PURCHASE_LYFT,
	PURCHASE_SUPERSTORE, PURCHASE_TRANSIT, PURCHASE_CATEGORY_COUNT
};

enum PurchaseChannel {
	CHANNEL_DIRECT, CHANNEL_CAPITAL_ONE_TRAVEL, CHANNEL_CHASE_TRAVEL,
	CHANNEL_CAPITAL_ONE_ENTERTAINMENT, PURCHASE_CHANNEL_COUNT
};

enum ComparisonSupport { CASH_SUPPORTED, CASH_REVIEW, POINTS_REVIEW };

/*
The shared Card keeps its name, ongoing fee and ordinary earning rate.
For real products, Card.rate uses the reward's unit: percent, points/$ or
miles/$. Only supported cashback rules use baseRateBp (300 means 3%).
firstYearFee keeps a temporary annual-fee waiver separate from the later fee.
Unknown charges stay in the written notes, rather than a misleading zero.
comparison tells the calculator when more information is needed.
*/
typedef struct {
	int id;
	Card card;
	int baseRateBp;
	const char* earningNotes;
	const char* benefitNotes;
	const char* pricingNotes;
	const char* source;
	const char* checkedDate;
	const char* reviewNotes;
	int firstYearFee;
	enum ComparisonSupport comparison;
} RealCard;

RealCard* getRealCardById(int cardId);
const char* realCardRewardUnit(const RealCard* card);
const char* comparisonNote(const RealCard* card);
bool realCardMatches(int cardId, const char* search);
const char* purchaseCategoryName(enum PurchaseCategory category);
const char* purchaseChannelName(enum PurchaseChannel channel);
bool isPurchaseChannelValid(enum PurchaseCategory category, enum PurchaseChannel channel);
bool formatRealCard(int cardId, char* text, size_t capacity);
void printRealCardList(void);

#endif
