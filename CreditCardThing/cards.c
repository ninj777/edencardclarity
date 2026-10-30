/*
Joe Kwon - sample catalog

My part is the card data and stable IDs. These are made-up examples, not
current offers. Gate 1 needs at least three; I keep all five:
Travel Sample, Points Sample, Cashback Sample, Everyday Sample, Starter Sample.
Travel uses a 1.25% base rate. The other four use 1%. The exact fees and
category bonuses are next to their records below.

The IDs stay 1 through 5 in that order. I don't renumber them when a wallet
card is removed. One lookup serves the menu, save file, window, and tests.
An unknown ID returns NULL. There is no need for a second zero-based lookup
or a random identifier.

I use registerCard for the main fields and addMultiplier for bonuses.
Unset bonuses stay zero. The console prints the same records used for the
shorter Windows detail labels.

For the practice formula I use amount * rate/100 * (1 + bonus/100).
A bonus of 5 means 1.05 times the base reward, not five times. An amount of
10 at 1.25% with a 2% bonus gives 0.1275 reward units. This does not value
points as dollars or subtract annual fees.

Gate 4's sourced real products are in realCards.c, with comparisons in
rewards.c. These five original records stay labeled as samples and keep
their original formula for the earlier gates.
*/
#include <stdio.h>
#include <wchar.h>

#include "logger.h"
#include "cards.h"
#include "realCards.h"

const wchar_t* networkNames[] = {
	L"Visa",
	L"Mastercard",
	L"American Express",
	L"Discover",
	L"Not confirmed"
};

const wchar_t* rewardTitles[] = {
	L"Cash Back",
	L"Points",
	L"Miles"
};

Card cardA = { 0 };
Card cardB = { 0 };
Card cardC = { 0 };
Card cardD = { 0 };
Card cardE = { 0 };

Card registerCard(char* issuer, enum Network network, int annualFee, enum Reward reward, double rate) {
	Card card = { 0 };

	card.issuer = issuer;
	card.network = network;
	card.annualFee = annualFee;
	card.reward = reward;
	card.rate = rate;
	return card;
}

const wchar_t* getBaseInfo(Card card) {
	/* I keep this buffer valid after returning; the window copies the text. */
	static wchar_t buf[256];
	
	swprintf(
		buf,
		256,
		L"Network: %ls\nReward type: %ls\nExample annual fee: $%d",
		networkNames[card.network],
		rewardTitles [card.reward] ,
		card.annualFee
	);

	return buf;
}

const wchar_t* getMultiplierInfo(Card card) {
	static wchar_t buf[256];

	swprintf(
		buf,
		256,
		L"Airlines: +%.2f%%\nStores: +%.2f%%\nServices: +%.2f%%",
		card.multipliers[AIRLINES].multiplier,
		card.multipliers[STORES].multiplier,
		card.multipliers[SERVICES].multiplier
	);

	return buf;
}

void addMultiplier(Card* card, enum MerchantCategory category, double multiplier) {
	Multiplier newMultiplier = { 0 };
	newMultiplier.category = category;
	newMultiplier.multiplier = multiplier;
	
	card->multipliers[category] = newMultiplier;
}

/* I apply a 5 percent bonus as 1.05 times the base reward. */
double calculateReward(Card card, enum MerchantCategory category, double amount) {
	double baseReward = amount * (card.rate/100);
	double multiplier = card.multipliers[category].multiplier;
	return baseReward * (1 + multiplier / 100);
}

Card* getCardById(int cardId) {
	/* I keep the public IDs the same in the menu, save file, and window. */
	switch (cardId) {
	case 1:
		return &cardA;
	case 2:
		return &cardB;
	case 3:
		return &cardC;
	case 4:
		return &cardD;
	case 5:
		return &cardE;
	default:
		/* I keep the old sample IDs and give researched products separate IDs. */
		{
			RealCard* real = getRealCardById(cardId);
			return real ? &real->card : NULL;
		}
	}
}

void initTestCards(void) {
	/* I keep these made-up values together so the displayed and saved IDs agree. */
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
		Card* card = getCardById(i + 1);
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
