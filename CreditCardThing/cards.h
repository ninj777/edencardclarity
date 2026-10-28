/*
Joe Kwon - my assigned card types and declarations

I put the shared Card definition here so every C file uses the same fields.
I use enums for network, reward kind, and merchant category. Those are types
with named integer choices, not methods. I use Multiplier to pair a category
with its bonus, and Card to keep the name, network, fee, rate, and bonuses.

I use char* for the name's address. These names point to string literals, so I
do not edit the letters through that pointer. I use double for percentage
values like 1.25, and int for the example fee in whole dollars. The later
cashback calculation still needs the proposal's whole-cent rounding rules.
I keep the Windows GUID only in Windows builds; I do not save it as a card ID.

I declare the functions below so other files know their parameters and return
types. I use the include guard to avoid defining these types twice when two
headers include this one. I keep the public saved IDs at 1 through 5.
*/
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

/* I keep saved IDs at 1 through 5 across restarts. */
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
