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
		Card* card = testGetCard(i);
		errors += check(getCardById(i + 1) == card, "Saved card ID maps to its record");
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
	/* Worked by hand: $10 at 1.25%, with a 2% bonus, is 0.1275 units. */
	errors += check(fabs(calculateReward(*testGetCard(0), SERVICES, 10) - 0.1275) < 0.000001,
		"Service reward with a bonus");
	errors += check(calculateReward(*testGetCard(0), SERVICES, 0) == 0,
		"Zero purchase");
	errors += check(fabs(calculateReward(*testGetCard(0), HOTEL, 100) - 1.25) < 0.000001,
		"Category without a bonus");
	if (errors) {
		return 1;
	}
	puts("All 40 module checks passed, including the practice reward examples.");
	return 0;
}
