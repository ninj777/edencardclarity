/*
Gate 4, Evan's checks and Joe's hand-calculation area

I write the expected cents before running the functions. Cases 1-6 are the
six required purchases; the rest check ties, limits, dates, files and inputs.
These runs are software evidence, not a teammate signature or a Windows demo.
*/
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "testSupport.h"
#include "../CreditCardThing/rewards.h"
#include "../CreditCardThing/storage.h"
#include "../CreditCardThing/input.h"

static TestContext test;
static const char* savedRealCards = "cardClarityWallet1\n5\n101\n102\n103\n104\n105\n";

static Wallet allRealCards(void) {
	Wallet wallet;
	initWallet(&wallet);
	for (int id = 101; id <= 105; id++) addWalletCard(&wallet, id);
	return wallet;
}

static void centsEqual(int id, const Purchase* purchase, int64_t expected) {
	RewardEstimate reward;
	bool valid = estimateCashback(id, purchase, &reward);
	printf("Card %d: expected %" PRId64 " cents; actual ", id, expected);
	if (valid) printf("%" PRId64 " to %" PRId64 " cents\n", reward.minimumCents, reward.maximumCents);
	else puts("calculation rejected");
	if (valid) printf("Actual total rate: %.2f%%; base rate: %.2f%%\n", reward.rateBp / 100.0, reward.baseRateBp / 100.0);
	expect(&test, valid && !reward.conditional && reward.minimumCents == expected
		&& reward.maximumCents == expected, "expected cashback matches actual cashback");
}

static void runMenu(const char* answers) {
	const char* args[] = { "--gate4", "--wallet-file", test.save, NULL };
	runProgram(&test, args, answers, strlen(answers), 0, false);
}

int main(int argc, char* argv[]) {
	if (argc != 2) { fputs("Pass the compiled application's path.\n", stderr); return 1; }
	initTestCards();
	initTests(&test, argv[1], "Gate 4 real-card checks");
	Wallet wallet = allRealCards();
	const enum PurchaseCategory categories[] = { PURCHASE_GROCERIES, PURCHASE_DINING,
		PURCHASE_GAS, PURCHASE_DRUGSTORE, PURCHASE_OTHER, PURCHASE_HOTEL };
	const int amounts[] = { 10000, 5000, 6000, 4000, 8000, 20000 };
	/* Columns: Quicksilver, Savor, Freedom Unlimited, Blue Cash Everyday, Active Cash. */
	const int expected[6][5] = {
		{150,300,150,300,200}, {75,150,150,50,100}, {90,60,90,180,120},
		{60,40,120,40,80}, {120,80,120,80,160}, {1000,1000,300,200,400}
	};
	for (int row = 0; row < 6; row++) {
		Purchase purchase = { amounts[row], categories[row], row == 5 ? CHANNEL_CAPITAL_ONE_TRAVEL : CHANNEL_DIRECT, 20261102, 0 };
		startCase(&test, row + 1, purchaseCategoryName(categories[row]), "All five real cards match independently calculated cents.");
		printf("Purchase: $%.2f | %s | prior category spending $0 | 2026-11-02\n", amounts[row] / 100.0, purchaseChannelName(purchase.channel));
		for (int i = 0; i < 5; i++) centsEqual(101 + i, &purchase, expected[row][i]);
		finishCase(&test);
	}
	Purchase purchase = { 20000, PURCHASE_HOTEL, CHANNEL_CHASE_TRAVEL, 20261102, 0 };
	char text[6000];
	startCase(&test, 7, "Issuer booking services stay separate", "A $200 Chase Travel hotel gives Chase $10, Quicksilver $3 and Savor $2.");
	centsEqual(103, &purchase, 1000); centsEqual(101, &purchase, 300); centsEqual(102, &purchase, 200);
	purchase.channel = CHANNEL_DIRECT; centsEqual(103, &purchase, 300);
	finishCase(&test);
	startCase(&test, 8, "Entertainment service", "$100 through Capital One Entertainment gives Quicksilver $5 and Savor $8.");
	purchase = (Purchase){10000, PURCHASE_ENTERTAINMENT, CHANNEL_CAPITAL_ONE_ENTERTAINMENT, 20261102, 0};
	centsEqual(101, &purchase, 500); centsEqual(102, &purchase, 800);
	finishCase(&test);
	startCase(&test, 9, "Cross the grocery cap", "$10 at 3% plus $20 at 1% = $0.50, with $5,990 already spent.");
	purchase = (Purchase){3000, PURCHASE_GROCERIES, CHANNEL_DIRECT, 20261102, 599000};
	centsEqual(104, &purchase, 50);
	RewardEstimate reward;
	expect(&test, estimateCashback(104, &purchase, &reward) && reward.bonusSpendCents == 1000
		&& reward.baseSpendCents == 2000, "purchase split is $10 plus $20"); finishCase(&test);
	startCase(&test, 10, "Exhausted cap and separate category", "$30 groceries after $6,000 gives $0.30; unused gas gives $0.90.");
	purchase.priorCategoryCents = 600000; centsEqual(104, &purchase, 30);
	purchase.category = PURCHASE_GAS; purchase.priorCategoryCents = 0; centsEqual(104, &purchase, 90);
	finishCase(&test);
	startCase(&test, 11, "Unknown cap use", "$30 has a $0.30-$0.90 range and no definite winner.");
	purchase.priorCategoryCents = -1;
	expect(&test, estimateCashback(104, &purchase, &reward) && reward.conditional
		&& reward.minimumCents == 30 && reward.maximumCents == 90, "actual range is 30-90 cents");
	expect(&test, formatComparison(&wallet, &purchase, text, sizeof(text))
		&& strstr(text, "No definite winner") != NULL, "unknown spending prevents a definite ranking"); finishCase(&test);
	startCase(&test, 12, "Unknown usage on a purchase larger than the cap", "$10,000 gives a maximum $220, not $300.");
	purchase.amountCents = 1000000;
	expect(&test, estimateCashback(104, &purchase, &reward) && reward.minimumCents == 10000
		&& reward.maximumCents == 22000, "range uses at most $6,000 at the bonus rate"); finishCase(&test);
	startCase(&test, 13, "Promotion expiry boundary", "Lyft $100 earns $2 on 2027-09-30 and $1.50 on 2027-10-01.");
	purchase = (Purchase){10000, PURCHASE_LYFT, CHANNEL_DIRECT, 20270930, 0};
	centsEqual(103, &purchase, 200); purchase.date = 20271001; centsEqual(103, &purchase, 150); finishCase(&test);
	startCase(&test, 14, "Flights do not get the Capital One hotel bonus", "$100 portal flight gives Quicksilver $1.50 and Savor $1.");
	purchase = (Purchase){10000, PURCHASE_FLIGHT, CHANNEL_CAPITAL_ONE_TRAVEL, 20261102, 0};
	centsEqual(101, &purchase, 150); centsEqual(102, &purchase, 100); finishCase(&test);
	startCase(&test, 15, "Show both dining winners", "Savor and Freedom Unlimited both appear after the winners heading.");
	purchase = (Purchase){5000, PURCHASE_DINING, CHANNEL_DIRECT, 20261102, 0};
	bool formatted = formatComparison(&wallet, &purchase, text, sizeof(text));
	const char* winners = formatted ? strstr(text, "Best estimated cashback") : NULL;
	expect(&test, winners && strstr(winners, "Savor Rewards") && strstr(winners, "Chase Freedom Unlimited"), "both $1.50 winners are displayed");
	expect(&test, winners && !strstr(winners, "Wells Fargo"), "a lower result is not listed as a winner"); finishCase(&test);
	startCase(&test, 16, "Save and reload real IDs", "The existing format preserves all five real IDs in order.");
	expect(&test, saveWallet(test.save, &wallet) == STORAGE_OK, "real-card wallet saved");
	Wallet loaded = {0};
	expect(&test, loadWallet(test.save, &loaded) == STORAGE_OK && memcmp(&wallet, &loaded, sizeof(wallet)) == 0,
		"actual reloaded IDs match the original wallet"); finishCase(&test);
	startCase(&test, 17, "Late damaged record", "A valid first real ID followed by unknown ID 6 leaves the current wallet alone.");
	writeText(test.save, "cardClarityWallet1\n2\n101\n6\n");
	expect(&test, loadWallet(test.save, &loaded) == STORAGE_BAD_FILE && memcmp(&wallet, &loaded, sizeof(wallet)) == 0,
		"partial load did not replace the current wallet"); finishCase(&test);
	startCase(&test, 18, "Duplicate real IDs", "A duplicate 101 fails loading without changing the wallet.");
	writeText(test.save, "cardClarityWallet1\n2\n101\n101\n");
	expect(&test, loadWallet(test.save, &loaded) == STORAGE_BAD_FILE && memcmp(&wallet, &loaded, sizeof(wallet)) == 0,
		"duplicate rejected and wallet preserved"); finishCase(&test);
	startCase(&test, 19, "Old sample save", "IDs 1 and 3 still identify Travel Sample and Cashback Sample.");
	writeText(test.save, "cardClarityWallet1\n2\n1\n3\n");
	expect(&test, loadWallet(test.save, &loaded) == STORAGE_OK && loaded.cardIds[0] == 1
		&& strcmp(getCardById(1)->issuer, "Travel Sample") == 0 && strcmp(getCardById(3)->issuer, "Cashback Sample") == 0,
		"sample identities were not changed into bank products");
	expect(&test, formatComparison(&loaded, &purchase, text, sizeof(text)) && strstr(text, "practice card, excluded")
		&& strstr(text, "Add a real card"), "samples cannot become real-card recommendations"); finishCase(&test);
	startCase(&test, 20, "Source and benefit notes", "Every real card has a link, date, restrictions and pricing notes.");
	for (int id = 101; id <= 105; id++) {
		const RealCard* card = getRealCardById(id);
		expect(&test, card && formatRealCard(id, text, sizeof(text)) && strstr(text, card->source)
			&& strstr(text, "2026-10-30") && card->benefitNotes[0] && card->pricingNotes[0], "source, date and conditions display");
	}
	formatRealCard(103, text, sizeof(text)); expect(&test, strstr(text, "2029-12-31") && strstr(text, "renewal"), "activation deadline and paid renewal are visible");
	formatRealCard(104, text, sizeof(text)); expect(&test, strstr(text, "enrollment required") != NULL, "enrollment is visible"); finishCase(&test);
	startCase(&test, 21, "Money input", "Whole cents parse exactly; bad text, fractions, signs and excessive amounts are rejected.");
	const char* valid[] = {"0", "12", "12.3", " 12.34 ", "1000000.00"};
	const int64_t values[] = {0,1200,1230,1234,100000000};
	for (int i = 0; i < 5; i++) { int64_t value = -7; expect(&test, parseMoney(valid[i], &value) && value == values[i], "accepted amount has exact expected cents"); }
	const char* bad[] = {"", " ", "-1", "+1", ".50", "1.", "1.001", "1e3", "nan", "inf", "$5", "1,000", "12x", "1000000.01", "999999999999999999999999"};
	for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); i++) { int64_t value = -7; expect(&test, !parseMoney(bad[i], &value) && value == -7, "bad money leaves the output unchanged"); }
	finishCase(&test);
	startCase(&test, 22, "Calendar dates", "Leap years work; impossible, incomplete and malformed dates fail.");
	int date;
	expect(&test, parseDate("2028-02-29", &date) && date == 20280229, "leap day accepted");
	const char* badDates[] = {"2026-02-29","2100-02-29","2026-04-31","2026-00-10","2026-12-00","2026-1-01","2026-10-30x","0000-01-01"};
	for (size_t i=0;i<sizeof(badDates)/sizeof(badDates[0]);i++) { date=17; expect(&test, !parseDate(badDates[i], &date) && date==17, "bad date leaves output unchanged"); }
	finishCase(&test);
	startCase(&test, 23, "Invalid calculation inputs", "Negative, excessive, invalid-category, invalid-channel and invalid-date requests fail.");
	purchase = (Purchase){-1, PURCHASE_OTHER, CHANNEL_DIRECT, 20261102, 0};
	expect(&test, !estimateCashback(101, &purchase, &reward), "negative amount rejected");
	purchase.amountCents=MAX_PURCHASE_CENTS+1; expect(&test,!estimateCashback(101,&purchase,&reward),"overlarge amount rejected");
	purchase.amountCents=100; purchase.category=PURCHASE_CATEGORY_COUNT; expect(&test,!estimateCashback(101,&purchase,&reward),"unknown category rejected");
	purchase.category=PURCHASE_DINING; purchase.channel=CHANNEL_CHASE_TRAVEL; expect(&test,!estimateCashback(103,&purchase,&reward),"impossible channel rejected");
	purchase.channel=CHANNEL_DIRECT; purchase.date=20260229; expect(&test,!estimateCashback(103,&purchase,&reward),"impossible date rejected");
	purchase.date=20261102; purchase.priorCategoryCents=-2; expect(&test,!estimateCashback(104,&purchase,&reward),"unknown marker must be exactly -1"); finishCase(&test);
	startCase(&test, 24, "Zero purchase", "All real cards return zero, including a capped category with unknown usage.");
	purchase=(Purchase){0,PURCHASE_GROCERIES,CHANNEL_DIRECT,20261102,-1};
	for(int id=101;id<=105;id++) centsEqual(id,&purchase,0);
	finishCase(&test);
	startCase(&test, 25, "Round only after adding split portions", "$0.50 at 3% plus $0.50 at 1% rounds to $0.02.");
	purchase=(Purchase){100,PURCHASE_GROCERIES,CHANNEL_DIRECT,20261102,599950}; centsEqual(104,&purchase,2);
	purchase=(Purchase){50,PURCHASE_OTHER,CHANNEL_DIRECT,20261102,0}; centsEqual(104,&purchase,1); finishCase(&test);
	startCase(&test, 26, "Largest supported purchase", "$1,000,000 at 2% gives $20,000 without overflow.");
	purchase=(Purchase){MAX_PURCHASE_CENTS,PURCHASE_OTHER,CHANNEL_DIRECT,20261102,0}; centsEqual(105,&purchase,2000000); finishCase(&test);
	startCase(&test, 27, "Small display buffer", "Formatting reports failure instead of claiming a truncated result is complete.");
	char small[8]; expect(&test,!formatComparison(&wallet,&purchase,small,sizeof(small)),"comparison reports insufficient space");
	expect(&test,!formatRealCard(101,small,sizeof(small)),"card notes report insufficient space"); finishCase(&test);
	startCase(&test, 28, "Invalid wallet", "A negative count cannot be used for comparison or saving.");
	Wallet invalid=wallet; invalid.count=-1;
	expect(&test,!formatComparison(&invalid,&purchase,text,sizeof(text)),"invalid wallet rejected");
	expect(&test,saveWallet(test.save,&invalid)==STORAGE_INVALID_WALLET,"invalid wallet cannot be saved"); finishCase(&test);
	startCase(&test, 29, "Superstore exclusion", "$100 at a superstore uses Savor and Amex base rates, both $1.");
	purchase=(Purchase){10000,PURCHASE_SUPERSTORE,CHANNEL_DIRECT,20261102,0}; centsEqual(102,&purchase,100); centsEqual(104,&purchase,100); finishCase(&test);
	startCase(&test, 30, "Console: save, restart and compare", "A fresh process loads five real IDs and reports both $1.50 dining winners.");
	remove(test.save);
	runMenu("2\n2\n101\n2\n102\n2\n103\n2\n104\n2\n105\n4\n6\n4\n");
	expect(&test,fileEquals(test.save,savedRealCards,strlen(savedRealCards)),"saved file contains the expected real IDs");
	runMenu("3\n2\n50\n1\n2026-11-02\n4\n");
	expect(&test,strstr(test.output,"Capital One Savor Rewards: $1.50") && strstr(test.output,"Chase Freedom Unlimited: $1.50"),"restarted comparison shows correct rewards"); finishCase(&test);
	startCase(&test, 31, "Console: bad amount recovery", "Invalid text, negative and fractional-cent amounts are rejected before a correct comparison.");
	writeText(test.save,savedRealCards);
	runMenu("3\n5\nhello\n-2\n1.001\n80\n1\n2026-02-29\n2026-11-02\n4\n");
	expect(&test,textCount(test.output,"Enter dollars with at most two decimal places") == 3,"three bad amounts were rejected");
	expect(&test,strstr(test.output,"Enter a real calendar date") && strstr(test.output,"Wells Fargo Active Cash: $1.60"),"date recovery and correct comparison"); finishCase(&test);
	startCase(&test, 32, "Console: cap crossing", "Entered prior grocery spending gives the expected $0.50 split.");
	writeText(test.save,savedRealCards);
	runMenu("3\n3\n30\n1\n2026-11-02\n5990\n4\n");
	expect(&test,strstr(test.output,"Blue Cash Everyday: $0.50") && strstr(test.output,"on $10.00; 1.00% on remaining $20.00"),"cap split appears in the real menu"); finishCase(&test);
	startCase(&test, 33, "Console: unknown spending", "Blank prior spending shows a range and no definite winner.");
	writeText(test.save,savedRealCards);
	runMenu("3\n3\n30\n1\n2026-11-02\n\n4\n");
	expect(&test,strstr(test.output,"$0.30 to $0.90") && strstr(test.output,"No definite winner"),"menu keeps unknown use visible"); finishCase(&test);
	startCase(&test, 34, "Real removal survives restart", "Removing real ID 103 and saving keeps the other four IDs after restart.");
	writeText(test.save,savedRealCards);
	runMenu("2\n3\n103\n4\n6\n4\n");
	loaded=(Wallet){0}; expect(&test,loadWallet(test.save,&loaded)==STORAGE_OK && loaded.count==4 && !walletHasCard(&loaded,103),"saved removal is correct");
	runMenu("2\n1\n6\n4\n"); expect(&test,strstr(test.output,"Chase Freedom Unlimited")==NULL,"removed card is absent after restart"); finishCase(&test);
	return finishTests(&test);
}
