/*
Gate 4, Oliver's comparison area

I calculate the total rate, rather than adding a bank's advertised rate to
another base reward. Amounts stay in cents and rates in hundredths of a percent.
The largest permitted product fits int64_t. I split a capped purchase before
rounding once, half upward, for the displayed estimate. Actual bank posting
can differ. Unknown cap use stays a range, so it cannot name a definite winner.
*/
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "input.h"
#include "rewards.h"

static bool validPurchase(const Purchase* purchase) {
	return purchase && purchase->amountCents >= 0 && purchase->amountCents <= MAX_PURCHASE_CENTS
		&& purchase->priorCategoryCents >= -1 && purchase->priorCategoryCents <= MAX_PURCHASE_CENTS
		&& isDateNumberValid(purchase->date)
		&& isPurchaseChannelValid(purchase->category, purchase->channel);
}

static int totalRate(int cardId, const Purchase* purchase) {
	int rate = getRealCardById(cardId)->baseRateBp;
	enum PurchaseCategory category = purchase->category;
	if (cardId == 102 && (category == PURCHASE_DINING || category == PURCHASE_GROCERIES
		|| category == PURCHASE_ENTERTAINMENT || category == PURCHASE_STREAMING)) rate = 300;
	if (cardId == 103) {
		if (category == PURCHASE_DINING || category == PURCHASE_DRUGSTORE) rate = 300;
		/* I only model the dated Lyft offer that the current source supports. */
		if (category == PURCHASE_LYFT && purchase->date >= 20261030
			&& purchase->date <= 20270930) rate = 200;
		if (purchase->channel == CHANNEL_CHASE_TRAVEL) rate = 500;
	}
	if (cardId == 104 && (category == PURCHASE_GROCERIES || category == PURCHASE_GAS
		|| category == PURCHASE_ONLINE_RETAIL)) rate = 300;
	if (cardId == 111) {
		if (category == PURCHASE_GROCERIES || category == PURCHASE_STREAMING) rate = 600;
		if (category == PURCHASE_GAS || category == PURCHASE_TRANSIT) rate = 300;
	}
	if (cardId == 101 || cardId == 102) {
		if (purchase->channel == CHANNEL_CAPITAL_ONE_TRAVEL && category != PURCHASE_FLIGHT) rate = 500;
		if (purchase->channel == CHANNEL_CAPITAL_ONE_ENTERTAINMENT) rate = cardId == 102 ? 800 : 500;
	}
	return rate;
}

bool estimateCashback(int cardId, const Purchase* purchase, RewardEstimate* result) {
	const RealCard* card = getRealCardById(cardId);
	if (!card || card->comparison != CASH_SUPPORTED || !result || !validPurchase(purchase)) return false;
	RewardEstimate checked = { 0 };
	checked.rateBp = totalRate(cardId, purchase);
	checked.baseRateBp = card->baseRateBp;
	checked.bonusSpendCents = purchase->amountCents;
	bool capped = (cardId == 104 && checked.rateBp == 300)
		|| (cardId == 111 && purchase->category == PURCHASE_GROCERIES);
	/* The form asks for Everyday spending. Preferred has its own separate cap. */
	int64_t priorSpending = cardId == 111 ? -1 : purchase->priorCategoryCents;
	if (capped && priorSpending == -1 && purchase->amountCents > 0) {
		/* I do not silently assume this year's spending allowance is unused. */
		checked.conditional = true;
		checked.minimumCents = (purchase->amountCents * card->baseRateBp + 5000) / 10000;
		int64_t bestBonus = purchase->amountCents < 600000 ? purchase->amountCents : 600000;
		checked.maximumCents = (bestBonus * checked.rateBp
			+ (purchase->amountCents - bestBonus) * card->baseRateBp + 5000) / 10000;
	} else {
		if (capped && priorSpending >= 0) {
			int64_t remaining = priorSpending < 600000
				? 600000 - priorSpending : 0;
			if (checked.bonusSpendCents > remaining) checked.bonusSpendCents = remaining;
			checked.baseSpendCents = purchase->amountCents - checked.bonusSpendCents;
		}
		checked.minimumCents = (checked.bonusSpendCents * checked.rateBp
			+ checked.baseSpendCents * card->baseRateBp + 5000) / 10000;
		checked.maximumCents = checked.minimumCents;
	}
	*result = checked;
	return true;
}

static bool append(char* text, size_t capacity, size_t* used, const char* format, ...) {
	if (*used >= capacity) return false;
	va_list args;
	va_start(args, format);
	int length = vsnprintf(text + *used, capacity - *used, format, args);
	va_end(args);
	if (length < 0 || (size_t)length >= capacity - *used) return false;
	*used += (size_t)length;
	return true;
}

bool formatComparison(const Wallet* wallet, const Purchase* purchase, char* text, size_t capacity) {
	if (!wallet || !text || capacity == 0 || !isWalletValid(wallet) || !validPurchase(purchase)) return false;
	text[0] = '\0';
	size_t used = 0;
	int64_t best = -1;
	bool hasConditional = false;
	bool needsReview = false;
	int realCount = 0;
	if (!append(text, capacity, &used, "%s, $%" PRId64 ".%02" PRId64 "\r\n%s | %04d-%02d-%02d\r\n\r\n",
		purchaseCategoryName(purchase->category), purchase->amountCents / 100, purchase->amountCents % 100,
		purchaseChannelName(purchase->channel), purchase->date / 10000, purchase->date / 100 % 100, purchase->date % 100)) return false;
	for (int i = 0; i < wallet->count; i++) {
		int id = wallet->cardIds[i];
		const RealCard* card = getRealCardById(id);
		if (card) realCount++;
		RewardEstimate reward;
		if (!estimateCashback(id, purchase, &reward)) {
			if (card) {
				needsReview = true;
				if (!append(text, capacity, &used, "%s: needs review. %s\r\n",
					card->card.issuer, comparisonNote(card))) return false;
			} else if (!append(text, capacity, &used, "%s: practice card, excluded.\r\n", getCardById(id)->issuer)) return false;
			continue;
		}
		if (!append(text, capacity, &used, "%s: $%" PRId64 ".%02" PRId64,
			getCardById(id)->issuer, reward.minimumCents / 100, reward.minimumCents % 100)) return false;
		if (reward.conditional) {
			hasConditional = true;
			if (!append(text, capacity, &used, " to $%" PRId64 ".%02" PRId64 " (prior category spending unknown)",
				reward.maximumCents / 100, reward.maximumCents % 100)) return false;
		} else {
			if (reward.minimumCents > best) best = reward.minimumCents;
			if (!append(text, capacity, &used, " (%.2f%% total", reward.rateBp / 100.0)) return false;
			if (reward.baseSpendCents > 0 && !append(text, capacity, &used,
				" on $%" PRId64 ".%02" PRId64 "; %.2f%% on remaining $%" PRId64 ".%02" PRId64,
				reward.bonusSpendCents / 100, reward.bonusSpendCents % 100, reward.baseRateBp / 100.0,
				reward.baseSpendCents / 100, reward.baseSpendCents % 100)) return false;
			if (!append(text, capacity, &used, ")")) return false;
		}
		if (!append(text, capacity, &used, "\r\n")) return false;
	}
	if (realCount == 0) {
		if (!append(text, capacity, &used, "\r\nAdd a real card to your wallet to compare.\r\n")) return false;
	} else if (needsReview) {
		if (!append(text, capacity, &used, "\r\nNo overall winner: some saved cards need conditions or a redemption choice reviewed. Their benefits are available in the catalog; they have not been valued as cash.\r\n")) return false;
	} else if (hasConditional) {
		if (!append(text, capacity, &used, "\r\nNo definite winner while a card has unknown cap use. Everyday spending can be entered above; Preferred grocery use is not collected by this form.\r\n")) return false;
	} else {
		if (!append(text, capacity, &used, "\r\nBest estimated cashback (all ties shown):\r\n")) return false;
		for (int i = 0; i < wallet->count; i++) {
			RewardEstimate reward;
			if (estimateCashback(wallet->cardIds[i], purchase, &reward) && reward.minimumCents == best
				&& !append(text, capacity, &used, "%s\r\n", getCardById(wallet->cardIds[i])->issuer)) return false;
		}
	}
	return append(text, capacity, &used, "\r\nEligible domestic U.S. purchases in USD only. Category and booking restrictions apply; read the saved card's benefits. Rates checked 2026-10-30; these are snapshot rates, not a historical-offer lookup. Fees, interest, welcome bonuses and benefit credits are excluded. Rounded estimates can differ from bank posting. Unknown cap use is not zero.");
}

/* A bad line is consumed by the shared reader before this function asks again. */
static int askAmount(const char* prompt, bool allowUnknown, int64_t* amount) {
	char line[128];
	for (;;) {
		fputs(prompt, stdout);
		fflush(stdout);
		enum InputResult result = readLine(stdin, line, sizeof(line));
		if (result == INPUT_END) return -1;
		if (result == INPUT_ERROR) return -2;
		if (result == INPUT_OK) {
			if (allowUnknown && line[0] == '\0') { *amount = -1; return 0; }
			if (parseMoney(line, amount)) return 0;
		}
		puts("Enter dollars with at most two decimal places, from 0 to 1000000.");
	}
}

int compareFromConsole(const Wallet* wallet) {
	Purchase purchase = { 0 };
	purchase.priorCategoryCents = -1;
	puts("\nCompare eligible purchases. Fees, transfers and cash advances are excluded.");
	for (int i = 0; i < PURCHASE_CATEGORY_COUNT; i++) printf("%d. %s\n", i + 1, purchaseCategoryName((enum PurchaseCategory)i));
	int choice;
	for (;;) {
		fputs("Choose a category: ", stdout); fflush(stdout);
		enum InputResult result = readNumber(stdin, 1, PURCHASE_CATEGORY_COUNT, &choice);
		if (result == INPUT_END) return -1;
		if (result == INPUT_ERROR) return -2;
		if (result == INPUT_OK) break;
		puts("Choose one of the category numbers above.");
	}
	purchase.category = (enum PurchaseCategory)(choice - 1);
	int answer = askAmount("Purchase amount in dollars: ", false, &purchase.amountCents);
	if (answer < 0) return answer;
	for (int i = 0; i < PURCHASE_CHANNEL_COUNT; i++) printf("%d. %s\n", i + 1, purchaseChannelName((enum PurchaseChannel)i));
	for (;;) {
		fputs("Choose how it was booked: ", stdout); fflush(stdout);
		enum InputResult result = readNumber(stdin, 1, PURCHASE_CHANNEL_COUNT, &choice);
		if (result == INPUT_END) return -1;
		if (result == INPUT_ERROR) return -2;
		if (result == INPUT_OK && isPurchaseChannelValid(purchase.category, (enum PurchaseChannel)(choice - 1))) break;
		puts("That booking choice does not fit this category. Use direct for an ordinary purchase.");
	}
	purchase.channel = (enum PurchaseChannel)(choice - 1);
	for (;;) {
		char line[128];
		fputs("Purchase date (YYYY-MM-DD): ", stdout); fflush(stdout);
		enum InputResult result = readLine(stdin, line, sizeof(line));
		if (result == INPUT_END) return -1;
		if (result == INPUT_ERROR) return -2;
		if (result == INPUT_OK && parseDate(line, &purchase.date)) break;
		puts("Enter a real calendar date, such as 2026-11-02.");
	}
	if (walletHasCard(wallet, 104) && (purchase.category == PURCHASE_GROCERIES
		|| purchase.category == PURCHASE_GAS || purchase.category == PURCHASE_ONLINE_RETAIL)) {
		printf("Blue Cash Everyday spending in this category before this purchase, in %d.\n", purchase.date / 10000);
		answer = askAmount("Amount, or leave blank if unknown: ", true, &purchase.priorCategoryCents);
		if (answer < 0) return answer;
	}
	char text[6000];
	if (!formatComparison(wallet, &purchase, text, sizeof(text))) { puts("Couldn't compare these cards."); return 0; }
	puts(text);
	return 0;
}
