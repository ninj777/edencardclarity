/*
Evan Zhuang - direct card checks

I compare Joe's names, rates, fees, and bonuses without going through a menu.
There are seven checks per card, two invalid-ID checks, and three calculation
checks: 35 + 2 + 3 = 40. Including the headers twice checks their guards.
For nonzero double results, I allow a small tolerance. Any mismatch returns
1; the success message only appears when the error count stays zero.
*/
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../CreditCardThing/cards.h"
#include "../CreditCardThing/cards.h"
#include "../CreditCardThing/logger.h"
#include "../CreditCardThing/logger.h"

static int check(int condition, const char* message) {
	if (!condition) {
		fprintf(stderr, "Failed: %s\n", message);
		return 1;
	}
	return 0;
}

int main(void) {
	int errors = 0;
	const char* names[] = {
		"Travel Sample", "Points Sample", "Cashback Sample",
		"Everyday Sample", "Starter Sample"
	};
	const double rates[] = { 1.25, 1.0, 1.0, 1.0, 1.0 };
	const int fees[] = { 95, 250, 0, 0, 0 };
	const double bonuses[5][3] = {
		{ 5.0, 3.0, 2.0 }, { 4.0, 2.0, 1.5 }, { 3.0, 2.0, 1.0 },
		{ 3.0, 2.0, 2.0 }, { 4.5, 2.5, 1.0 }
	};
	const enum MerchantCategory categories[] = { AIRLINES, STORES, SERVICES };
	initLogger();
	initTestCards();
	for (int i = 0; i < 5; i++) {
		Card* card = getCardById(i + 1);
		errors += check(card != NULL, "Saved card ID has a record");
		if (!card) {
			continue;
		}
		errors += check(strcmp(card->issuer, names[i]) == 0, "Sample name");
		errors += check(card->rate == rates[i], "Base rate");
		errors += check(card->annualFee == fees[i], "Example fee");
		for (int j = 0; j < 3; j++) {
			errors += check(card->multipliers[categories[j]].multiplier == bonuses[i][j],
				"Category bonus");
		}
	}
	errors += check(getCardById(0) == NULL, "Zero is not a saved card ID");
	errors += check(getCardById(SAMPLE_CARD_COUNT + 1) == NULL, "Unknown card ID is rejected");
	/* I expect 10 * 0.0125 * 1.02 = 0.1275 reward units for this example. */
	errors += check(fabs(calculateReward(*getCardById(1), SERVICES, 10) - 0.1275) < 0.000001,
		"Service reward with a bonus");
	errors += check(calculateReward(*getCardById(1), SERVICES, 0) == 0,
		"Zero purchase");
	errors += check(fabs(calculateReward(*getCardById(1), HOTEL, 100) - 1.25) < 0.000001,
		"Category without a bonus");
	if (errors) {
		return 1;
	}
	puts("All 40 module checks passed, including the practice reward examples.");
	return 0;
}
