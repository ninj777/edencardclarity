#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

#include "logger.h"
#include "cards.h"

#ifdef _WIN32
const wchar_t* networkNames[] = {
	L"Visa",
	L"Mastercard",
	L"American Express",
	L"Discover"
};

const wchar_t* rewardTitles[] = {
	L"Cash Back",
	L"Points",
	L"Miles"
};

#endif

Card cardA = { 0 };
Card cardB = { 0 };
Card cardC = { 0 };
Card cardD = { 0 };
Card cardE = { 0 };

Card registerCard(char* issuer, enum Network network, int annualFee, enum Reward reward, double rate) {
	Card card = { 0 };

#ifdef _WIN32
	GUID guid = { 0 };
	if (!SUCCEEDED(CoCreateGuid(&guid))) {
		logMessage("Could not create a card identifier.", LOG_ERROR);
	}
	card.guid = guid;
#endif

	card.issuer = issuer;
	card.network = network;
	card.annualFee = annualFee;
	card.reward = reward;
	card.rate = rate;
	return card;
}

#ifdef _WIN32
LPCWSTR getGUIDAsLPCWSTR(GUID guid) {
	/* The window copies this text when the label is created. */
	static wchar_t buf[37];

	swprintf_s(
		buf, 
		37, 
		L"%08lx-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
		guid.Data1,
		guid.Data2,
		guid.Data3,
		guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3], guid.Data4[4],
		guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	return buf;
}

LPCWSTR getBaseInfo(Card card) {
	static wchar_t buf[256];
	
	swprintf_s(
		buf,
		256,
		L"Network: %ls\nReward type: %ls\nExample annual fee: $%d",
		networkNames[card.network],
		rewardTitles [card.reward] ,
		card.annualFee
	);

	return buf;
}

LPCWSTR getMultiplierInfo(Card card) {
	static wchar_t buf[256];

	swprintf_s(
		buf,
		256,
		L"Airlines: +%.2f%%\nStores: +%.2f%%\nServices: +%.2f%%",
		card.multipliers[AIRLINES].multiplier,
		card.multipliers[STORES].multiplier,
		card.multipliers[SERVICES].multiplier
	);

	return buf;
}

#endif

void addMultiplier(Card* card, enum MerchantCategory category, double multiplier) {
	Multiplier newMultiplier = { 0 };
	newMultiplier.category = category;
	newMultiplier.multiplier = multiplier;
	
	card->multipliers[category] = newMultiplier;
}

/* A 5 percent bonus makes the base reward 1.05 times as large. */
double calculateReward(Card card, enum MerchantCategory category, double amount) {
	double baseReward = amount * (card.rate/100);
	double multiplier = card.multipliers[category].multiplier;
	return baseReward * (1 + multiplier / 100);
}

Card* testGetCard(int id) {
	switch (id) {
	case (0):
		return &cardA;
	case (1):
		return &cardB;
	case (2):
		return &cardC;
	case (3):
		return &cardD;
	case (4):
		return &cardE;
	default:
		return &cardA;
	}
}

Card* getCardById(int cardId) {
	if (cardId < 1 || cardId > SAMPLE_CARD_COUNT) {
		return NULL;
	}
	return testGetCard(cardId - 1);
}

void initTestCards(void) {
	cardA = registerCard("Travel Sample", VISA, 95, REWARD_POINTS, 1.25);
	addMultiplier(&cardA, AIRLINES, 5.0);
	addMultiplier(&cardA, STORES, 3.0);
	addMultiplier(&cardA, SERVICES, 2.0);

	cardB = registerCard("Points Sample", AMEXPRESS, 250, REWARD_POINTS, 1.0);
	addMultiplier(&cardB, AIRLINES, 4.0);
	addMultiplier(&cardB, STORES, 2.0);
	addMultiplier(&cardB, SERVICES, 1.5);

	cardC = registerCard("Cashback Sample", MASTERCARD, 0, CASHBACK, 1.0);
	addMultiplier(&cardC, AIRLINES, 3.0);
	addMultiplier(&cardC, STORES, 2.0);
	addMultiplier(&cardC, SERVICES, 1.0);

	cardD = registerCard("Everyday Sample", AMEXPRESS, 0, CASHBACK, 1.0);
	addMultiplier(&cardD, AIRLINES, 3.0);
	addMultiplier(&cardD, STORES, 2.0);
	addMultiplier(&cardD, SERVICES, 2.0);

	cardE = registerCard("Starter Sample", DISCOVER, 0, CASHBACK, 1.0);
	addMultiplier(&cardE, AIRLINES, 4.5);
	addMultiplier(&cardE, STORES, 2.5);
	addMultiplier(&cardE, SERVICES, 1.0);

	logMessage("Sample cards are ready.", LOG_INFO);
}

void printSampleCards(void) {
	puts("\nSample cards");
	puts("These cards and rates are made up for practice, not real bank offers.");
	for (int i = 0; i < SAMPLE_CARD_COUNT; i++) {
		Card* card = testGetCard(i);
		printf("\nCard %d: %s\n", i + 1, card->issuer);
		printf("Example base rate: %.2f%% | Example annual fee: $%d\n",
			card->rate, card->annualFee);
		printf("Example bonus percentages: Airlines %.2f%% | Stores %.2f%% | Services %.2f%%\n",
			card->multipliers[AIRLINES].multiplier,
			card->multipliers[STORES].multiplier,
			card->multipliers[SERVICES].multiplier);
	}
	puts("\nA bonus percentage increases the base reward; it is not a times multiplier.");
	puts("These practice values do not compare the cash value of points or miles.");
}
