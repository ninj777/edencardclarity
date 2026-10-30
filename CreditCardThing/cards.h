/*
Joe Kwon - shared card types

I keep Card here so every file sees the same fields. The enums name network,
reward kind, and category choices. They are types, not methods. Multiplier
pairs a category with its bonus; Card groups the name, fee, rate, and bonuses.

The name pointer refers to a string literal, so I don't change its letters.
double holds percentage values like 1.25. The example fee is an int in whole
dollars; whole-cent cashback rounding is still later work. Both views use
the catalog ID instead of a random identifier.

I declare the shared functions below. The include guard stops repeated
includes from defining these types twice. Sample IDs stay 1 through 5.
Gate 4 adds researched IDs 101-120 in realCards.c without changing the samples.
*/
#ifndef CARDS_H
#define CARDS_H

#include <wchar.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define SAMPLE_CARD_COUNT 5

enum Network {
    VISA,
    MASTERCARD,
    AMEXPRESS,
	DISCOVER,
	NETWORK_UNKNOWN
};

enum Reward {
	CASHBACK,
	REWARD_POINTS,
	MILES
};

enum MerchantCategory {
	AGRICULTURE,
	CONSTRUCTION,
	AIRLINES,
	CAR_RENTAL,
	HOTEL,
	UTILITIES,
	STORES,
	SERVICES,
	ORGANIZATIONS,
	GOVERNMENT
};

typedef struct {
	enum MerchantCategory category;
	double multiplier;
} Multiplier;

typedef struct {
    char* issuer;
	enum Network network;
	int annualFee;

	enum Reward reward;
	double rate;

	Multiplier multipliers[10];
} Card;

Card registerCard(char* issuer, enum Network network, int annualFee, enum Reward reward, double rate);
void addMultiplier(Card* card, enum MerchantCategory category, double multiplier);
double calculateReward(Card card, enum MerchantCategory category, double amount);

/* I keep saved IDs at 1 through 5 across restarts. */
Card* getCardById(int cardId);
void initTestCards(void);
void printSampleCards(void);

const wchar_t* getBaseInfo(Card card);
const wchar_t* getMultiplierInfo(Card card);

#endif
