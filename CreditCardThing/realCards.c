/*
Gate 4, Joe's catalog area

I use five checked U.S. cashback products for the purchase comparisons.
The source review is in docs/gateFourResearch.txt, dated October 30, 2026.
IDs 101-105 are permanent. IDs 1-5 still mean the original practice cards.
The Card inside each record supplies the existing wallet's name and base rate.
The real calculator uses total rates, never the sample bonus formula.
An unknown network or fee detail stays unknown instead of becoming a guess.
*/
#include <stdio.h>
#include "realCards.h"

static RealCard realCards[REAL_CARD_COUNT] = {
	{ 101, { "Capital One Quicksilver Rewards", NETWORK_UNKNOWN, 0, CASHBACK, 1.5, {{0}} }, 150,
		"1.5% base. 5% on eligible Capital One Travel hotels, cars and vacation rentals; 5% through Capital One Entertainment.",
		"The travel bonus requires that issuer's booking service; flights use the base rate. Eligible purchases only.",
		"Rewards do not expire while the account is open. No fixed benefit-expiration date was listed for these ordinary rates.",
		"Annual fee $0; foreign transaction fee $0. Intro purchase/transfer APR: 0% for 15 months, then 18.74%-28.74% variable. Other fees: check current account disclosure.",
		"https://www.capitalone.com/credit-cards/quicksilver/", "2026-10-30" },
	{ 102, { "Capital One Savor Rewards", NETWORK_UNKNOWN, 0, CASHBACK, 1.0, {{0}} }, 100,
		"3% eligible groceries, dining, entertainment and popular streaming; 1% other. 5% eligible Capital One Travel hotels/cars/vacation rentals; 8% Capital One Entertainment.",
		"Walmart and Target superstores are excluded from groceries. Merchant codes and payment processing affect eligibility. Not every streaming service qualifies.",
		"Rewards do not expire while the account is open. This is the no-annual-fee Savor Rewards version.",
		"Annual fee $0; foreign transaction fee $0. Intro purchase/transfer APR: 0% for 12 months, then 18.74%-28.74% variable. Full transfer pricing still needs the disclosure.",
		"https://www.capitalone.com/credit-cards/savor/", "2026-10-30" },
	{ 103, { "Chase Freedom Unlimited", VISA, 0, CASHBACK, 1.5, {{0}} }, 150,
		"1.5% base; 3% dining/drugstores; 5% eligible Chase Travel. Qualifying Lyft app purchases: 2% total through 2027-09-30.",
		"Travel paid with points and FROSCH bookings do not earn the portal bonus. Category coding matters. Lyft must meet the offer's product exclusions.",
		"DashPass: activate by 2029-12-31 for six complimentary months, then paid automatic renewal unless cancelled; eligibility applies. No subscription value is added to cashback.",
		"Annual fee $0. Intro purchase/transfer APR: 0% for 15 months, then 18.49%-27.99% variable. Current foreign and transfer fees unconfirmed: check pricing disclosure.",
		"https://creditcards.chase.com/cash-back-credit-cards/freedom/unlimited", "2026-10-30" },
	{ 104, { "American Express Blue Cash Everyday", AMEXPRESS, 0, CASHBACK, 1.0, {{0}} }, 100,
		"3% eligible U.S. supermarkets, U.S. gas stations and U.S. online retail. Each category has a separate $6,000 calendar-year spending cap, then 1%. 1% other.",
		"Superstores/warehouse clubs do not count as supermarkets. Their gas is excluded too. Online travel, dining and services are not online retail. Enter prior spending for this category/year.",
		"Disney streaming: enrollment required, up to $7 per month on qualifying U.S. subscriptions; auto-renewal applies. Not an automatic $84 payment. No credit is added to rewards.",
		"Annual fee $0. Current APR and foreign/transfer fees need current disclosure. The accessible 2024 agreement is historical, not proof of today's pricing.",
		"https://www.americanexpress.com/us/credit-cards/card/blue-cash-everyday/", "2026-10-30" },
	{ 105, { "Wells Fargo Active Cash", VISA, 0, CASHBACK, 2.0, {{0}} }, 200,
		"Unlimited 2% on eligible purchases, without a category activation.",
		"Transfers, cash advances, cash equivalents, fees and interest are not reward purchases. Returns can reduce rewards. These estimates cover eligible domestic U.S. purchases.",
		"Cell phone protection has bill-payment and coverage conditions. Insurance is not cash and is not added to this estimate. Rewards do not expire while the account is open.",
		"Annual fee $0; foreign fee 3%. Intro purchase/qualifying-transfer APR: 0% for 12 months, then 18.74%, 24.74% or 28.74% variable. Transfer fee first 120 days: max($5,3%); afterward up to 5%, minimum $5.",
		"https://creditcards.wellsfargo.com/active-cash-credit-card/", "2026-10-30" }
};

RealCard* getRealCardById(int cardId) {
	if (cardId < FIRST_REAL_CARD_ID || cardId > LAST_REAL_CARD_ID) return NULL;
	return &realCards[cardId - FIRST_REAL_CARD_ID];
}

const char* purchaseCategoryName(enum PurchaseCategory category) {
	static const char* names[] = {
		"Other eligible purchase", "Dining", "Eligible U.S. supermarket",
		"Eligible U.S. gas station", "Eligible U.S. online retail", "Drugstore",
		"Hotel", "Rental car", "Vacation rental", "Flight",
		"Eligible entertainment", "Eligible popular streaming", "Qualifying Lyft app purchase",
		"Superstore or warehouse club"
	};
	return category >= 0 && category < PURCHASE_CATEGORY_COUNT ? names[category] : "Unknown category";
}

const char* purchaseChannelName(enum PurchaseChannel channel) {
	static const char* names[] = { "Direct / ordinary purchase", "Capital One Travel",
		"Chase Travel", "Capital One Entertainment" };
	return channel >= 0 && channel < PURCHASE_CHANNEL_COUNT ? names[channel] : "Unknown booking service";
}

bool isPurchaseChannelValid(enum PurchaseCategory category, enum PurchaseChannel channel) {
	if (category < 0 || category >= PURCHASE_CATEGORY_COUNT || channel < 0
		|| channel >= PURCHASE_CHANNEL_COUNT) return false;
	if (channel == CHANNEL_DIRECT) return true;
	if (channel == CHANNEL_CAPITAL_ONE_ENTERTAINMENT) return category == PURCHASE_ENTERTAINMENT;
	return category == PURCHASE_HOTEL || category == PURCHASE_CAR_RENTAL
		|| category == PURCHASE_VACATION_RENTAL || category == PURCHASE_FLIGHT;
}

bool formatRealCard(int cardId, char* text, size_t capacity) {
	const RealCard* card = getRealCardById(cardId);
	if (!card || !text || capacity == 0) return false;
	int length = snprintf(text, capacity, "%s\r\n\r\n%s\r\n\r\n%s\r\n\r\n%s\r\n\r\n%s\r\n\r\nChecked %s\r\n%s\r\n\r\nPublic terms can change. Check your own account. Estimates exclude fees, interest and welcome bonuses.",
		card->card.issuer, card->earningNotes, card->restrictions, card->benefitNotes,
		card->pricingNotes, card->checkedDate, card->source);
	return length >= 0 && (size_t)length < capacity;
}

void printRealCards(void) {
	puts("\nReal cashback cards - issuer pages checked October 30, 2026");
	for (int i = 0; i < REAL_CARD_COUNT; i++) {
		char text[3000];
		if (formatRealCard(realCards[i].id, text, sizeof(text)))
			printf("\nCard %d\n%s\n", realCards[i].id, text);
	}
}
