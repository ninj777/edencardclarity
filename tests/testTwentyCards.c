/*
Evan's catalog-check area, with Joe's fee and reward examples.
These checks run the real console for every product. I also check the saved
IDs directly, because a displayed name alone would not prove a correct save.
The money examples below use cents worked out before calling the calculator.
Windows appearance and a person's account terms need separate checks.
*/
#include <stdio.h>
#include <string.h>
#include "testSupport.h"
#include "../CreditCardThing/rewards.h"
#include "../CreditCardThing/storage.h"

static TestContext test;

static void runMenu(const char* answers) {
	const char* args[] = {"--gate4", "--wallet-file", test.save, NULL};
	runProgram(&test, args, answers, strlen(answers), 0, false);
}

int main(int argc, char* argv[]) {
	if (argc != 2) { fputs("Pass the compiled application's path.\n", stderr); return 1; }
	initTestCards();
	initTests(&test, argv[1], "Twenty-card catalog checks");
	const int annualFees[] = {0,0,0,0,0,0,95,795,325,895,95,95,395,0,95,0,0,0,0,0};
	char details[REAL_CARD_TEXT_CAPACITY];
	for (int id = FIRST_REAL_CARD_ID; id <= LAST_REAL_CARD_ID; id++) {
		const RealCard* card = getRealCardById(id);
		startCase(&test, id - 100, card ? card->card.issuer : "Missing record",
			"The record has costs, conditions and dated sources; its ID can be added, saved, reopened and removed.");
		expect(&test, card && card->id == id && getCardById(id) == &card->card, "lookup returns the same catalog identity");
		if (card) {
			expect(&test, card->card.annualFee == annualFees[id - 101], "ongoing annual fee matches the checked fee list");
			bool complete = formatRealCard(id, details, sizeof(details));
			const char* costs = complete ? strstr(details, "Costs and interest") : NULL;
			const char* earning = complete ? strstr(details, "Earning and limits") : NULL;
			expect(&test, complete && costs && earning && costs < earning
				&& strstr(details, card->source) && strstr(details, "Checked 2026-10-30")
				&& card->benefitNotes[0] && card->reviewNotes[0], "complete sourced notes display, with costs first");
			for (int other = id + 1; other <= LAST_REAL_CARD_ID; other++)
				expect(&test, strcmp(card->card.issuer, getCardById(other)->issuer) != 0, "product name is unique");
			char answers[160];
			remove(test.save);
			snprintf(answers, sizeof(answers), "1\n%d\n2\n2\n%d\n4\n6\n4\n", id, id);
			runMenu(answers);
			expect(&test, strstr(test.output, card->card.issuer) && strstr(test.output, card->pricingNotes)
				&& strstr(test.output, card->earningNotes), "actual console shows this product's costs and earning rules");
			Wallet loaded = {0};
			expect(&test, loadWallet(test.save, &loaded) == STORAGE_OK && loaded.count == 1
				&& loaded.cardIds[0] == id, "saved ID is correct");
			runMenu("2\n1\n6\n4\n");
			expect(&test, strstr(test.output, card->card.issuer) && strstr(test.output, card->benefitNotes),
				"a fresh process reloads and displays the right card");
			snprintf(answers, sizeof(answers), "2\n3\n%d\n4\n6\n4\n", id);
			runMenu(answers);
			expect(&test, loadWallet(test.save, &loaded) == STORAGE_OK && loaded.count == 0,
				"removal is saved without leaving the card behind");
		}
		finishCase(&test);
	}

	startCase(&test, 21, "Case-insensitive search", "Names match regardless of letter case; missing names do not choose a substitute.");
	expect(&test, realCardMatches(111, "bLuE cAsH pReF") && realCardMatches(120, "AUTOGRAPH"), "partial name matching works");
	int matches = 0;
	for (int id = 101; id <= 120; id++) {
		if (realCardMatches(id, "")) matches++;
		expect(&test, !realCardMatches(id, "a card missing from this catalog"), "a missing product gives no match");
	}
	expect(&test, matches == 20 && !realCardMatches(121, "") && !realCardMatches(101, NULL), "clearing search restores twenty; invalid requests fail");
	finishCase(&test);

	startCase(&test, 22, "Points and miles", "Eight points/miles products keep their units and cannot produce guessed cash estimates.");
	Purchase purchase = {10000, PURCHASE_DINING, CHANNEL_DIRECT, 20261102, 0};
	RewardEstimate reward = {0};
	const int pointsIds[] = {107,108,109,110,112,113,115,120};
	for (size_t i = 0; i < sizeof(pointsIds)/sizeof(pointsIds[0]); i++) {
		const RealCard* card = getRealCardById(pointsIds[i]);
		expect(&test, card->comparison == POINTS_REVIEW && card->baseRateBp == 0
			&& strstr(realCardRewardUnit(card), "per dollar") && !estimateCashback(card->id, &purchase, &reward),
			"no cashback percentage or guessed cash value for this product");
	}
	finishCase(&test);

	startCase(&test, 23, "Complex cashback conditions", "Rotating, payment, membership and selected-category cards are explicitly held for review.");
	const int reviewIds[] = {106,114,116,117,118,119};
	for (size_t i = 0; i < sizeof(reviewIds)/sizeof(reviewIds[0]); i++)
		expect(&test, getRealCardById(reviewIds[i])->comparison == CASH_REVIEW
			&& !estimateCashback(reviewIds[i], &purchase, &reward), "missing conditions cannot become an exact estimate");
	Wallet wallet = {0};
	addWalletCard(&wallet, 105); addWalletCard(&wallet, 114); addWalletCard(&wallet, 120);
	expect(&test, formatComparison(&wallet, &purchase, details, sizeof(details))
		&& strstr(details, "Wells Fargo Active Cash: $2.00") && strstr(details, "No overall winner")
		&& strstr(details, "Citi Double Cash: needs review") && !strstr(details, "practice card")
		&& !strstr(details, "Best estimated cashback"), "mixed wallet shows supported estimates and explains incomplete comparison");
	finishCase(&test);

	startCase(&test, 24, "Preferred streaming, gas and transit", "$100 earns $6 on eligible streaming, $3 on gas/transit, and $1 at a superstore.");
	const enum PurchaseCategory categories[] = {PURCHASE_STREAMING, PURCHASE_GAS, PURCHASE_TRANSIT, PURCHASE_SUPERSTORE};
	const int expected[] = {600,300,300,100};
	for (int i = 0; i < 4; i++) {
		purchase.category = categories[i];
		expect(&test, estimateCashback(111, &purchase, &reward) && !reward.conditional
			&& reward.minimumCents == expected[i] && reward.maximumCents == expected[i], "independent expected cents match");
	}
	finishCase(&test);

	startCase(&test, 25, "Separate Amex caps", "Everyday's unused or exhausted allowance never supplies Preferred's grocery usage.");
	purchase.category = PURCHASE_GROCERIES;
	for (int i = 0; i < 2; i++) {
		purchase.priorCategoryCents = i ? 600000 : 0;
		expect(&test, estimateCashback(111, &purchase, &reward) && reward.conditional
			&& reward.minimumCents == 100 && reward.maximumCents == 600, "Preferred remains a $1-$6 range");
	}
	purchase.amountCents = 1000000;
	expect(&test, estimateCashback(111, &purchase, &reward) && reward.minimumCents == 10000
		&& reward.maximumCents == 40000, "$10,000 has maximum $400: $6,000 at 6%, $4,000 at 1%");
	purchase.amountCents = 0;
	expect(&test, estimateCashback(111, &purchase, &reward) && !reward.conditional
		&& reward.minimumCents == 0, "zero purchase stays zero");
	finishCase(&test);

	startCase(&test, 26, "Missing fees and offer periods", "Unconfirmed pricing is explicit; an introductory waiver does not hide next year's fee.");
	expect(&test, getRealCardById(111)->firstYearFee == 0 && getRealCardById(111)->card.annualFee == 95,
		"Preferred first year is zero, ongoing fee is $95");
	formatRealCard(114, details, sizeof(details));
	expect(&test, strstr(details, "Current foreign fee, APR") && strstr(details, "could not be verified")
		&& strstr(details, "Do not treat them as zero"), "unknown Citi pricing stays unknown");
	formatRealCard(118, details, sizeof(details));
	expect(&test, strstr(details, "$139/year") && strstr(details, "membership is separate"), "Prime membership cost is visible");
	formatRealCard(119, details, sizeof(details));
	expect(&test, strstr(details, "$65/year") && strstr(details, "expires December 31")
		&& strstr(details, "share a $7,000"), "Costco membership, certificate expiry and shared gas allowance are visible");
	finishCase(&test);

	startCase(&test, 27, "New last ID and damaged later ID", "ID 120 loads, but 121 later in a file leaves the open wallet unchanged.");
	wallet = (Wallet){0}; addWalletCard(&wallet, 120);
	Wallet before = wallet;
	writeText(test.save, "cardClarityWallet1\n2\n120\n121\n");
	expect(&test, loadWallet(test.save, &wallet) == STORAGE_BAD_FILE
		&& memcmp(&wallet, &before, sizeof(wallet)) == 0, "late unknown ID cannot partially replace the wallet");
	expect(&test, !getRealCardById(100) && !getRealCardById(121) && !getCardById(6), "catalog boundaries and original gap stay invalid");
	finishCase(&test);

	startCase(&test, 28, "Five-card wallet stays bounded", "Twenty catalog choices still allow five selected cards, with duplicate and full checks.");
	wallet = (Wallet){0};
	for (int id = 116; id <= 119; id++) expect(&test, addWalletCard(&wallet, id) == WALLET_OK, "new product added");
	expect(&test, addWalletCard(&wallet, 119) == WALLET_DUPLICATE, "duplicate attempt is rejected before the wallet is full");
	expect(&test, addWalletCard(&wallet, 120) == WALLET_OK && addWalletCard(&wallet, 115) == WALLET_FULL
		&& wallet.count == 5 && isWalletValid(&wallet), "full and duplicate attempts preserve a valid wallet");
	finishCase(&test);

	startCase(&test, 29, "Read a missing ID and recover", "A gap ID is rejected and the next catalog visit still displays a real product.");
	remove(test.save);
	runMenu("1\n6\n1\n120\n4\n");
	expect(&test, strstr(test.output, "That isn't a real card") && strstr(test.output, "Cell protection requires"),
		"bad ID recovery reaches Autograph's benefit notes");
	finishCase(&test);

	startCase(&test, 30, "No guessed winner through the real menu", "A saved points card is explained as needing review, not mislabeled as a practice card.");
	writeText(test.save, "cardClarityWallet1\n1\n120\n");
	runMenu("3\n2\n100\n1\n2026-11-02\n4\n");
	expect(&test, strstr(test.output, "Wells Fargo Autograph: needs review") && strstr(test.output, "No overall winner")
		&& !strstr(test.output, "practice card") && !strstr(test.output, "Add a real card"), "real console preserves points and comparison limitations");
	finishCase(&test);
	return finishTests(&test);
}
