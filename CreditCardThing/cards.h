#ifndef CARDS_H
#define CARDS_H

#ifdef _WIN32
#include <windows.h>
#endif

#define SAMPLE_CARD_COUNT 5

enum Network {
    VISA,
    MASTERCARD,
    AMEXPRESS,
	DISCOVER
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
#ifdef _WIN32
	GUID guid;
#endif

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

/* Saved IDs stay 1 through 5 even when the program restarts. */
Card* getCardById(int cardId);
Card* testGetCard(int id);
void initTestCards(void);
void printSampleCards(void);

#ifdef _WIN32
LPCWSTR getGUIDAsLPCWSTR(GUID guid);
LPCWSTR getBaseInfo(Card card);
LPCWSTR getMultiplierInfo(Card card);
#endif

#endif
