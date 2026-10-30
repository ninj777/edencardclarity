/*
Joe's catalog area. These are researched summaries of twenty common U.S.
personal products, not a ranked list or complete bank agreements. Costs come
first. I keep unknown charges visible and never add headline credit totals
as guaranteed savings. The second source pass is in docs/cardTermsFollowUp.txt;
the earlier checks are in docs/twentyCardResearch.txt.
Original real IDs 101-105 and practice IDs 1-5 keep their meanings.
C joins adjacent quoted strings below into one note. The shorter lines make
these long terms easier to read in the source.
*/
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "realCards.h"

static RealCard realCards[REAL_CARD_COUNT] = {
	{
		.id = 101, .card = {"Capital One Quicksilver Rewards", NETWORK_UNKNOWN, 0, CASHBACK, 1.5, {{0}}},
		.baseRateBp = 150, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes =
			"Annual fee $0; foreign fee $0. Published offer snapshot: purchase/transfer intro APR 0% for "
			"15 months, then 18.74%-28.74% variable. Cash advances: 28.74% variable and the larger of $5 "
			"or 5%. Transfers in the first 15 months cost 3%; other promotional transfers can cost 4%. "
			"September 30, 2026 Visa/World sample: late fee up to $40. Confirm the actual offer and card "
			"version before borrowing.",
		.earningNotes =
			"1.5% eligible base purchases. Eligible Capital One Travel hotel, rental-car and "
			"vacation-rental bookings earn 5%; Capital One Entertainment earns 5%. Flights do not "
			"receive the hotel/car bonus.",
		.benefitNotes =
			"Use the specific issuer booking service. Merchant coding and processing control "
			"eligibility. Rewards do not expire while the account is open. A price-match travel credit "
			"is not a cash refund; warranty/insurance benefits depend on the actual card version and "
			"guide.",
		.reviewNotes =
			"Named offer text was readable in issuer search results; the live reader did not show every "
			"disclosure. Network-specific protection guides are available. Match the logo and actual "
			"agreement; QuicksilverOne, student, secured and good-credit versions differ.",
		.source =
			"https://www.capitalone.com/credit-cards/quicksilver/\r\n"
			"https://www.capitalone.com/credit-cards/cash-back/?n=1\r\n"
			"https://www.capitalone.com/credit-cards/lp/credit-card-agreements/\r\n"
			"https://ecm.capitalone.com/WCM/card/credit-card-agreements-q3-2026/credit-card-agreement-for-visa-signature-and-world-mastercard-cards-in-capital-one-n.a.pdf\r\n"
			"https://www.capitalone.com/credit-cards/benefits-guide/",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 102, .card = {"Capital One Savor Rewards", NETWORK_UNKNOWN, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes =
			"Annual fee $0; foreign fee $0. Published offer snapshot: purchase/transfer intro APR 0% for "
			"12 months, then 18.74%-28.74% variable. Advances: 28.74% variable and the larger of $5 or "
			"5%. Transfers in the first 12 months cost 3%; other promotional transfers can cost 4%. "
			"September 30, 2026 Visa/World sample: late fee up to $40. Your offer and network version "
			"control.",
		.earningNotes =
			"3% eligible groceries, dining, entertainment and popular streaming; 1% other. Eligible "
			"Capital One Travel hotels, rental cars and vacation rentals earn 5%; Capital One "
			"Entertainment earns 8%.",
		.benefitNotes =
			"Superstores such as Walmart and Target are excluded from the grocery bonus. Not all "
			"streaming or entertainment merchants qualify. Payment processors can affect merchant "
			"coding. Rewards remain available while the account is open; credits, insurance and "
			"concierge services are not guaranteed cash.",
		.reviewNotes =
			"This is no-annual-fee Savor Rewards. Issuer-indexed offer versions have differing variable "
			"rates; the displayed rate is a snapshot. Current full personalized pricing and insurance "
			"depend on the actual account. Do not substitute old annual-fee Savor, SavorOne, student or "
			"good-credit terms.",
		.source =
			"https://www.capitalone.com/credit-cards/savor/\r\n"
			"https://www.capitalone.com/credit-cards/cash-back/savor/\r\n"
			"https://www.capitalone.com/credit-cards/lp/credit-card-agreements/\r\n"
			"https://ecm.capitalone.com/WCM/card/credit-card-agreements-q3-2026/credit-card-agreement-for-visa-signature-and-world-mastercard-cards-in-capital-one-n.a.pdf\r\n"
			"https://www.capitalone.com/credit-cards/benefits-guide/",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 103, .card = {"Chase Freedom Unlimited", VISA, 0, CASHBACK, 1.5, {{0}}},
		.baseRateBp = 150, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes =
			"Annual fee $0. Product snapshot: purchase/transfer intro APR 0% for 15 months, then "
			"18.49%-27.99% variable. Linked pricing dated 2025-12-16 lists foreign fee 3%; transfer fee "
			"the larger of $5 or 3% within 60 days, then $5 or 5%; advances $10 or 5%; late/returned "
			"payments up to $40. That older schedule quotes advance APR 28.49%, penalty up to 29.99%, "
			"and monthly plan fees up to 1.72% after the purchase intro. Those dated terms are not a "
			"confirmed current quote.",
		.earningNotes =
			"1.5% base, 3% total dining/drugstores, 5% eligible Chase Travel. Qualifying Lyft app "
			"purchases earn 2% total through 2027-09-30. Cash redemption is distinct from other "
			"point-redemption options.",
		.benefitNotes =
			"Chase Travel rewards exclude points-paid amounts and FROSCH. Merchant definitions apply. "
			"DashPass needs activation by 2029-12-31, provides six complimentary months, then paid "
			"automatic renewal at the current rate unless cancelled. The quarterly non-restaurant "
			"discount has order restrictions and does not cover fees/taxes/tips.",
		.reviewNotes =
			"The full linked fee schedule was read, but its date and APRs differ from the product page. "
			"Read the actual application/account terms before borrowing. Transfers and advances start "
			"interest immediately under that document; paying only the minimum does not preserve "
			"purchase grace. Protection claims require the applicable guide and covered payment.",
		.source =
			"https://creditcards.chase.com/cash-back-credit-cards/freedom/unlimited\r\n"
			"https://asset.chase.com/content/dam/card/rulesregulations/en/RPA0509_Web.pdf\r\n"
			"https://sites.chase.com/services/creatives/pricingandterms.html/content/dam/pricingandterms/LGC61498.html",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 104, .card = {"American Express Blue Cash Everyday", AMEXPRESS, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes =
			"Annual fee $0; additional cards $0. Agreement dated 2026-06-30: purchase/transfer APR Prime "
			"+ 12.74%-21.74%; advances Prime + 21.99%; penalty Prime + 26.74%, variable rates capped at "
			"29.99%. Foreign fee 2.7%; transfers the larger of $5 or 3%; advances $10 or 5%; "
			"late/returned payments up to $40. Plan fees up to 1.33% monthly; returned cashed check $38; "
			"approved account reopening $25. Penalty can last at least six months. These are dated "
			"agreement terms; a specific intro offer can differ.",
		.earningNotes =
			"3% eligible U.S. supermarkets, gas stations and online retail, each with its own $6,000 "
			"calendar-year spending cap, then 1%; 1% other. Cash back is Reward Dollars, not an "
			"automatic payment against the minimum due.",
		.benefitNotes =
			"Superstores/warehouse clubs are not supermarkets; their gas is excluded too. Online travel, "
			"restaurants and services are not online retail. Enter prior spending in the specific "
			"category/year. Disney streaming: enrollment required, qualifying direct U.S. subscriptions, "
			"up to $7/month, no minimum purchase threshold; subscriptions auto-renew.",
		.reviewNotes =
			"The June 30 agreement replaces March as the latest agreement read here. Purchases avoid "
			"interest only when the required full or plan-adjusted balance is paid; advances/transfers "
			"start interest immediately. Account amendments and application offers can differ. "
			"Purchase/rental cover has exclusions, and rental cover is secondary without liability "
			"insurance.",
		.source =
			"https://www.americanexpress.com/us/credit-cards/card/blue-cash-everyday/\r\n"
			"https://www.americanexpress.com/content/dam/amex/en-us/company/legal/cardmember-agreements/public-site-2026-q1-pdf-cmas/cps-lending/blue-cash-everyday-03-31-2026.pdf\r\n"
			"https://www.americanexpress.com/content/dam/amex/en-us/company/legal/cardmember-agreements/public-site-2026-q2-pdf-cmas/cps-lending/blue-cash-everyday-06-30-2026.pdf\r\n"
			"https://www.americanexpress.com/en-us/account/get-started/bce/uncover-your-benefits",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 105, .card = {"Wells Fargo Active Cash", VISA, 0, CASHBACK, 2.0, {{0}}},
		.baseRateBp = 200, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes =
			"Annual fee $0; foreign fee 3%. Published intro APR: purchases/qualifying transfers 0% for "
			"12 months, then 18.74%, 24.74% or 28.74% variable. Transfer fee within 120 days max($5,3%), "
			"afterward up to 5%, minimum $5. Advances max($10,5%), APR 29.74% variable; late fee up to "
			"$40; minimum interest $1.",
		.earningNotes =
			"Unlimited 2% eligible purchases. Transfers, advances, cash equivalents, fees, interest and "
			"excluded transactions do not earn rewards. Returns reduce rewards.",
		.benefitNotes =
			"Cell-phone protection: pay the wireless bill with the card in the preceding month; coverage "
			"starts next calendar month. $25 deductible, up to $600/claim, two claims/$1,200 per 12 "
			"months. Supplemental coverage; unexplained loss and cosmetic-only damage excluded. "
			"Roadside/assistance services can cost money.",
		.reviewNotes =
			"September 2026 credit terms and the applicable guide were read. Advances/transfers start "
			"interest immediately; purchase grace needs the full required balance paid. Reward credits "
			"do not replace required payments. Redeem for Purchases needs a transaction of at least $1 "
			"and enough rewards to cover it. Account amendments and claim deadlines still control.",
		.source =
			"https://creditcards.wellsfargo.com/active-cash-credit-card/\r\n"
			"https://www.wellsfargo.com/credit-cards/active-cash/terms/?FPID=012686I6F40000&cx_nm=CXNAME_CSMPD&product_code=CC&sub_channel=WEB&subproduct_code=AC&vendor_code=WF\r\n"
			"https://www.wellsfargo.com/credit-cards/active-cash/guide-to-benefits/\r\n"
			"https://www.wellsfargo.com/credit-cards/documents/active-cash-terms/?FPID=012686I6F40000&cx_nm=CXNAME_CSMPD&product_code=CC&sub_channel=WEB&subproduct_code=AC&vendor_code=WF",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 106, .card = {"Chase Freedom Flex", MASTERCARD, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes =
			"Annual fee $0; current product page states no foreign transaction fee. Published "
			"purchase/transfer intro APR 0% for 15 months, then 18.49%-27.99% variable. Exact transfer, "
			"advance, late/returned-payment and plan fees remain unconfirmed; linked pricing was "
			"unavailable.",
		.earningNotes =
			"1% base; 3% dining/drugstores; 5% eligible Chase Travel. Activated rotating categories earn "
			"5% total on $1,500 combined spending per quarter, then normal earning. The agreement "
			"separates 1% base, 2% dining bonus and 4% quarterly bonus. If both bonuses qualify, count "
			"the base once: 1 + 2 + 4 = 7%, not 5 + 3 = 8%.",
		.benefitNotes =
			"Q4 2026 categories include eligible groceries (excluding Walmart/Target), dining and "
			"American Red Cross; categories expire at quarter end. Lyft: 2% total through 2027-09-30. "
			"DashPass: activate by 2029-12-31, six months complimentary, then paid renewal. Current "
			"guide: rental coverage generally secondary in the U.S.; protection exclusions and claim "
			"deadlines apply.",
		.reviewNotes =
			"Current product page and benefit guide checked. The exact pricing link LGC65280 could not "
			"be read; an unrelated Chase sample cannot establish this offer's missing charges. The "
			"calculator also lacks activation, shared-cap and stacking inputs, so it does not rank this "
			"card as a definite winner.",
		.source =
			"https://creditcards.chase.com/cash-back-credit-cards/freedom/flex\r\n"
			"https://asset.chase.com/content/services/structured-document/document.en.pdf/card/benefits-center/product-benefits-guide-pdf/BGC11542.pdf\r\n"
			"https://sites.chase.com/services/creatives/pricingandterms.html/content/dam/pricingandterms/LGC65280.html\r\n"
			"https://www.chase.com/freedomflex/rewardsagreement",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 107, .card = {"Chase Sapphire Preferred", VISA, 95, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 95, .comparison = POINTS_REVIEW,
		.pricingNotes =
			"Annual fee $95; foreign fee $0. Product snapshot: purchase APR 19.49%-27.74% variable. "
			"Linked pricing dated 2026-05-05 quotes older 19.24%-27.49% purchase/transfer APR and 28.49% "
			"advance APR. Its fees: transfers the larger of $5 or 5%; advances $10 or 5%; late/returned "
			"payments up to $40; monthly plan fee up to 1.72%. Penalty APR up to 29.99% can remain "
			"indefinitely. Confirm the actual current offer; advances and transfers start interest "
			"immediately.",
		.earningNotes =
			"5 points/$ eligible Chase Travel; 2 other travel; 3 dining, selected streaming, online "
			"groceries excluding Target/Walmart/clubs, gas/EV charging and named vacation-home brands; 1 "
			"other. Lyft 5 points/$ through 2027-09-30. Points are not cash percentages.",
		.benefitNotes =
			"$100 Chase Travel hotel credit per anniversary year; credited amounts do not earn points. "
			"Screening credit up to $120 every four years. Apple TV: activate by 2026-12-31, one year; "
			"prior paid plans can resume. Rental coverage up to $60,000, generally primary, but New York "
			"personal-insurance exception applies; decline agency waiver, eligible vehicles/rental "
			"periods only.",
		.reviewNotes =
			"June 2026 benefit updates checked; older $50 hotel-credit and ongoing 10% anniversary-bonus "
			"summaries are outdated. Hyatt transfer ratio is 4:3 under current rules. Rewards agreement: "
			"cash redemption is 100 points for $1; other options/promotions vary. The dated pricing "
			"document and current product page are kept separate. Read the applicable protection guide.",
		.source =
			"https://creditcards.chase.com/rewards-credit-cards/sapphire/preferred\r\n"
			"https://media.chase.com/news/Meet-the-New-Chase-Sapphire-Preferred\r\n"
			"https://static.chasecdn.com/content/services/structured-document/document.en.pdf/card/benefits-center/product-benefits-guide-pdf/BGC11599.pdf\r\n"
			"https://www.chase.com/content/dam/pricingandterms/LGC65222.html\r\n"
			"https://www.chase.com/sapphirepreferred/rewardsagreement",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 108, .card = {"Chase Sapphire Reserve", VISA, 795, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 795, .comparison = POINTS_REVIEW,
		.pricingNotes =
			"Annual fee $795; each authorized user $195; foreign fee $0. Published purchase APR "
			"19.74%-28.24% variable. Other borrowing, late/returned-payment and installment charges "
			"require the unread current pricing disclosure.",
		.earningNotes =
			"8 points/$ eligible Chase Travel; 4 direct airline/hotel; 3 dining; 1 other. Purchases "
			"reimbursed by the $300 travel or The Edit credits do not earn points. Points Boost is "
			"selective, not a guaranteed two-cent cash value.",
		.benefitNotes =
			"$300 travel credit per anniversary year. The Edit: $500/year, max $250/transaction, prepaid "
			"two-night stay. Dining: $150 each half-year at eligible Exclusive Tables restaurants. "
			"Select-hotel $250 offer ends 2026-12-31 and needs two nights. Screening credit up to $120 "
			"every four years. Priority Pass needs activation; lounge space/guest rules apply. Apple "
			"subscriptions need activation by 2027-06-22; paid plans may resume. StubHub $150/half-year "
			"through 2027-12-31 requires activation; Lyft $10/month through 2027-09-30; Peloton "
			"$10/month through 2027-12-31 requires activation. DoorDash promos require eligible "
			"orders/membership and exclude fees/tax/tips.",
		.reviewNotes =
			"Guide reviewed: covered trip delay over six hours/overnight, up to $500/traveler; rental "
			"cover has payment, vehicle and liability exclusions. Rewards agreement gives 100 points for "
			"$1 cash; other options differ. Full current pricing link LGC64842 remains unreadable, so "
			"other borrowing charges remain unconfirmed. Spending-gated and partner extras are not "
			"automatic.",
		.source =
			"https://creditcards.chase.com/rewards-credit-cards/sapphire/reserve\r\n"
			"https://www.chase.com/sapphire-cards/personal/reserve\r\n"
			"https://static.chasecdn.com/content/services/structured-document/document.en.pdf/card/benefits-center/product-benefits-guide-pdf/508_BGC11559_SAPPHIRE_RESERVE-1.pdf\r\n"
			"https://sites.chase.com/services/creatives/pricingandterms.html/content/dam/pricingandterms/LGC64842.html\r\n"
			"https://www.chase.com/sapphirereserve/rewardsagreement",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 109, .card = {"American Express Gold", AMEXPRESS, 325, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 325, .comparison = POINTS_REVIEW,
		.pricingNotes =
			"Annual fee $325; first five additional Gold cards $0, sixth and later $35 each; foreign fee "
			"$0. Agreement dated 2026-06-30: Pay Over Time APR Prime + 12.74%-21.74%; advances Prime + "
			"21.99%; penalty Prime + 26.74%, variable rates capped at 29.99%. Advances cost the larger "
			"of $10 or 5%; late/returned payments up to $40; plan fees up to 1.33% monthly; returned "
			"cashed check $38; approved account reopening $25. Penalty lasts at least six months. A "
			"person's offer or later amendments can differ.",
		.earningNotes =
			"4 points/$ restaurants worldwide up to $50,000/calendar year and U.S. supermarkets up to "
			"$25,000/calendar year, then 1. 3 qualifying flights; 5 prepaid Amex Travel hotels; 2 "
			"qualifying prepaid Amex rentals/cruises; 1 other.",
		.benefitNotes =
			"Credits: dining up to $10/month at named partners, Dunkin $7/month, Resy $50 each "
			"half-year; enrollment required. Uber Cash $10/month in the U.S., linked card and qualifying "
			"Amex payment required. These are spending-dependent allowances, not $424 deposited at once. "
			"Hotel Collection benefits require a two-night eligible booking.",
		.reviewNotes =
			"Dated agreement and member guide checked. Rental cover is secondary without liability "
			"cover; country/vehicle exclusions apply. Covered round-trip delay over 12 hours: up to "
			"$300/trip, two claims/12 months, eligible payment required. Issuer example: eligible "
			"statement-charge redemption is 0.6 cents/point; 4 points/$ then equals 2.4% before fees, "
			"not automatic 4% cash back.",
		.source =
			"https://www.americanexpress.com/us/credit-cards/card/gold-card/\r\n"
			"https://www.americanexpress.com/en-us/credit-cards/credit-intel/gold-card-annual-fee/\r\n"
			"https://www.americanexpress.com/content/dam/amex/en-us/company/legal/cardmember-agreements/public-site-2026-q2-pdf-cmas/cps-charge/american-express-gold-card-06-30-2026.pdf\r\n"
			"https://www.americanexpress.com/en-us/credit-cards/credit-intel/american-express-points-value/\r\n"
			"https://www.americanexpress.com/en-us/account/get-started/gold/explore-benefits",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 110, .card = {"American Express Platinum", AMEXPRESS, 895, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 895, .comparison = POINTS_REVIEW,
		.pricingNotes =
			"Annual fee $895; additional Platinum $195 each; Companion $0 with different benefits; "
			"foreign fee $0. Agreement dated 2026-06-30: Pay Over Time APR Prime + 12.74%-21.74%; "
			"advances Prime + 21.99%; penalty Prime + 26.74%, variable rates capped at 29.99%. Advances "
			"cost the larger of $10 or 5%; late/returned payments up to $40; plan fees up to 1.33% "
			"monthly; returned cashed check $38; approved account reopening $25. Penalty can last at "
			"least six months. No preset limit is not unlimited credit.",
		.earningNotes =
			"5 points/$ direct-airline/Amex Travel flights, $500,000/calendar-year cap; 5 prepaid Amex "
			"Travel hotels; 1 other. Not 5% cash back.",
		.benefitNotes =
			"Allowances: hotels $300/half-year; Resy $100/quarter; digital entertainment $25/month; "
			"lululemon $75/quarter; Uber $15/month plus $20 December; Oura ring $200/year; "
			"selected-airline incidentals $200/year; CLEAR+ $219/year; Uber One $120/year; Walmart+ "
			"eligible monthly fee up to $12.95 plus tax. Eligibility/enrollment/renewal rules apply.",
		.reviewNotes =
			"Hotel Collection needs two nights; FHR credit also needs two nights from 2027-01-01. Delta "
			"lounge access is ten eligible visits; Priority Pass enrollment/guest charges apply. The "
			"issuer describes 0.6 cents/point for eligible statement charges; travel/transfer values "
			"differ. Credits are spending-dependent maximums. Current account amendments and the exact "
			"protection policy still need the holder's agreement.",
		.source =
			"https://www.americanexpress.com/us/credit-cards/card/platinum/\r\n"
			"https://www.americanexpress.com/en-us/credit-cards/credit-intel/amex-gold-vs-platinum/\r\n"
			"https://www.americanexpress.com/content/dam/amex/en-us/company/legal/cardmember-agreements/public-site-2026-q2-pdf-cmas/cps-charge/platinum-card-06-30-2026.pdf\r\n"
			"https://www.americanexpress.com/en-us/credit-cards/credit-intel/american-express-points-value/",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 111, .card = {"American Express Blue Cash Preferred", AMEXPRESS, 95, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes =
			"Public first-year annual fee $0, then $95; additional cards $0; foreign fee 2.7%. Agreement "
			"dated 2026-06-30: purchase/transfer APR Prime + 12.74%-21.74%; advances Prime + 21.99%; "
			"penalty Prime + 26.74%, variable rates capped at 29.99%. Transfers cost the larger of $5 or "
			"3%; advances $10 or 5%; late/returned payments up to $40; plan fees up to 1.33% monthly. "
			"Returned cashed check $38; approved account reopening $25. Specific intro offers and "
			"account amendments can differ.",
		.earningNotes =
			"6% eligible U.S. supermarkets on $6,000/calendar year, then 1%; 6% select U.S. streaming; "
			"3% eligible U.S. gas and transit; 1% other. Reward Dollars are redeemable rewards, not "
			"payment of the minimum due.",
		.benefitNotes =
			"Superstores/warehouse clubs are not supermarkets; gas and streaming definitions matter. "
			"Transit includes eligible rideshare. Disney: up to $10/month on enrolled qualifying direct "
			"U.S. subscriptions, no minimum purchase threshold, auto-renewal applies. Return protection: "
			"seller must refuse an eligible return within 90 days; up to $300/item and $1,000/calendar "
			"year, full eligible-card payment, U.S./territory purchase, shipping excluded. Rental cover "
			"is secondary without liability cover.",
		.reviewNotes =
			"June agreement and member guide checked. Streaming, gas, transit and eligible Lyft rides "
			"are modeled; groceries stay a capped range because the form does not capture Preferred's "
			"own prior use. Everyday's spending is separate. The first-year fee waiver is not the "
			"ongoing annual fee; no credits or insurance are added as guaranteed cash.",
		.source =
			"https://www.americanexpress.com/us/credit-cards/card/blue-cash-preferred/\r\n"
			"https://www.americanexpress.com/en-us/credit-cards/credit-intel/amex-gold-vs-blue-cash-preferred/\r\n"
			"https://www.americanexpress.com/content/dam/amex/en-us/company/legal/cardmember-agreements/public-site-2026-q2-pdf-cmas/cps-lending/blue-cash-preferred-06-30-2026.pdf\r\n"
			"https://www.americanexpress.com/en-us/account/get-started/bcp/uncover-your-benefits\r\n"
			"https://www.americanexpress.com/content/dam/amex/us/credit-cards/features-benefits/policies/NAC-Benefit-Guides/RP_Benefit_Guide_Rev_10-20_10.09.23.pdf",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 112, .card = {"Capital One Venture Rewards", NETWORK_UNKNOWN, 95, MILES, 2.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 95, .comparison = POINTS_REVIEW,
		.pricingNotes =
			"Annual fee $95; foreign fee $0. Issuer offer snapshot: purchase/transfer APR 19.74%-28.74% "
			"variable; advances 28.74% variable and the larger of $5 or 5%. Transfers at the standard "
			"transfer APR have no fee in that disclosure; promotional transfers can cost 4%. September "
			"30 Visa/World sample: late fee up to $40. Confirm the actual network/version and offer; "
			"these are not personalized current terms.",
		.earningNotes =
			"2 miles/$ base; 5 eligible Capital One Travel hotels, rental cars and vacation rentals, and "
			"Capital One Entertainment. Miles are not cashback percentages; cash, travel and transfer "
			"redemptions can differ.",
		.benefitNotes =
			"Screening application credit up to $120, with renewal interval and eligibility in the "
			"benefit terms. Lifestyle Collection experience credit up to $50 per eligible stay, usable "
			"for specified on-property charges. A limited-time first-year stays credit is not a "
			"recurring annual benefit. Hertz status requires eligibility/enrollment.",
		.reviewNotes =
			"Named pricing was found in issuer search results; the live reader omits some fields. "
			"Venture holders can face lounge entry charges. Insurance needs the actual network/version "
			"guide. Miles can offset eligible travel purchases within 90 days; cash, travel and partner "
			"redemptions differ. No unsupported universal cash value is assigned.",
		.source =
			"https://www.capitalone.com/credit-cards/venture/\r\n"
			"https://capitalonetravel.com/airport-lounges\r\n"
			"https://www.capitalone.com/credit-cards/\r\n"
			"https://www.capitalone.com/credit-cards/benefits-guide/\r\n"
			"https://ecm.capitalone.com/WCM/card/credit-card-agreements-q3-2026/credit-card-agreement-for-visa-signature-and-world-mastercard-cards-in-capital-one-n.a.pdf\r\n"
			"https://www.capitalone.com/learn-grow/money-management/ways-to-redeem-venture-miles/",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 113, .card = {"Capital One Venture X Rewards", VISA, 395, MILES, 2.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 395, .comparison = POINTS_REVIEW,
		.pricingNotes =
			"Annual fee $395; foreign fee $0. Issuer offer snapshot: purchase/transfer APR 19.74%-28.74% "
			"variable; advances 28.74% variable and the larger of $5 or 5%. Standard-APR transfers have "
			"no fee in that disclosure; promotional transfers can cost 4%. September agreement versions "
			"list late fees up to $40. No-preset-limit and other versions differ. Added-card lounge "
			"access costs $125/person/year separately.",
		.earningNotes =
			"2 miles/$ base; 10 eligible Capital One Travel hotels/cars; 5 portal flights/vacation "
			"rentals and Capital One Entertainment. Travel credits do not turn points or miles into cash.",
		.benefitNotes =
			"$300 annual Capital One Travel credit expires at the next anniversary; restricted to its "
			"portal. Lounge guests generally cost $45/adult or $25/child 2-17; Priority Pass guests $35. "
			"$75,000 annual spending can unlock certain Capital One guest privileges; it is not "
			"unconditional free guest access. Priority Pass enrollment and lounge capacity limits apply. "
			"10,000 anniversary miles start at the first anniversary. Premier/Lifestyle Collection "
			"credits require eligible bookings and on-property use.",
		.reviewNotes =
			"Pricing came from issuer-indexed text; account versions can differ. Issuer-linked Visa "
			"Infinite guide, effective 2021-10-01: cell cover up to $800/claim, two claims/$1,600 per 12 "
			"months, $50 deductible; pay the preceding month's wireless bill with the card. Lost phones "
			"and electronic-only failure excluded. Match the actual guide and later notices. Travel "
			"offsets need redemption within 90 days; credits do not automatically offset the fee.",
		.source =
			"https://www.capitalone.com/credit-cards/venture-x/\r\n"
			"https://capitalonetravel.com/airport-lounges/lounge-access-guide\r\n"
			"https://www.capitalone.com/learn-grow/more-than-money/all-about-venture-x/\r\n"
			"https://www.capitalone.com/credit-cards/\r\n"
			"https://www.capitalone.com/credit-cards/lp/credit-card-agreements/\r\n"
			"https://ecm.capitalone.com/WCM/card/benefits-guide/visa-benefits-guides/visa-infinite-english.pdf\r\n"
			"https://www.capitalone.com/learn-grow/money-management/ways-to-redeem-venture-miles/",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 114, .card = {"Citi Double Cash", MASTERCARD, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes =
			"Annual fee $0. Issuer table v32 (no printed effective date): foreign fee 3%; "
			"purchases/transfers 17.49%-27.49% variable; advances 29.74% variable. An indexed offer "
			"instead shows 18.49%-28.74% and 29.99%, plus a transfer-only 0% intro for 18 months if "
			"completed within four months. Versions differ; confirm the actual offer. Table fees: "
			"transfers the larger of $5 or 3% in four months, then $5 or 5%; advances $10 or 5%; "
			"late/returned payments up to $41; minimum interest $0.50; monthly plan fee up to 1.72%; "
			"penalty APR up to 29.99% can last indefinitely.",
		.earningNotes =
			"1% when buying plus 1% as qualifying purchase balances are paid, not an unconditional 2% on "
			"purchase day. Eligible Citi Travel hotel/car/attraction bookings add 3%, for 5% total only "
			"when payment conditions are met.",
		.benefitNotes =
			"Cashback is tracked as ThankYou points. Transfers do not earn rewards; the transfer intro "
			"does not cover purchase APR. Carrying a transfer can remove the purchase grace period "
			"unless the entire balance is paid. Payments need matching Purchase Tracker balances. "
			"Check-by-mail redemption has a $5 minimum; other redemptions differ.",
		.reviewNotes =
			"The v32 pricing table was read directly; indexed issuer offers differ and the live reader "
			"still shows blank fields. Their dates and versions cannot be merged into one confirmed "
			"current quote. Qualifying payment timing is also needed for comparison. Extended warranty "
			"is secondary with exclusions; no automatic insurance payout is assumed.",
		.source =
			"https://www.citi.com/credit-cards/citi-double-cash-credit-card\r\n"
			"https://www.citi.com/credit-cards/rewards-credit-cards\r\n"
			"https://www.citi.com/CRD/PDF/CMA/pricingTables/PIT_PID142-DoubleCashADA-2.pdf\r\n"
			"https://www.citi.com/credit-cards/citi-double-cash-credit-card?uuid=IPAbF8Bmuv83059X5117&yLocale=en-us&yMarket=us\r\n"
			"https://www.citi.com/credit-cards/compare-credit-cards/CMA-PIT",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 115, .card = {"Citi Strata Premier", MASTERCARD, 95, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 95, .comparison = POINTS_REVIEW,
		.pricingNotes =
			"Annual fee $95; no authorized-user or foreign fee. Indexed issuer offers disagree on "
			"variable purchase/transfer APR: 19.74%-27.74% or 20.24%-28.24%; one lists advance APR "
			"29.49%. Confirm the actual live offer. Indexed fees: transfers the larger of $5 or 5%; "
			"advances $10 or 5%; monthly plan fee up to 1.72%; minimum interest $0.50; penalty APR up to "
			"29.99% for late/returned payments. Exact current late/returned-payment dollar fees remain "
			"unconfirmed.",
		.earningNotes =
			"10 points/$ eligible Citi Travel hotels/cars/attractions; 3 air travel, other hotels, "
			"restaurants, supermarkets, gas/EV charging; 1 other. ThankYou redemption/transfer values "
			"vary.",
		.benefitNotes =
			"Once per calendar year, $100 off a single hotel stay costing at least $500 before "
			"taxes/fees, booked through Citi Travel or its specified phone line. It is not a general "
			"$100 hotel refund. Cancellation may remove the benefit; inspect booking terms.",
		.reviewNotes =
			"The live pricing fields are blank; indexed offer versions differ. A World Elite guide "
			"effective 2024-05-12 was found, but applicability to the current Strata Premier account was "
			"not established. Do not promise its claim limits. The current issuer page confirms "
			"protection categories. Redemption, offer eligibility, actual fees and account-specific "
			"coverage need the applicable terms.",
		.source =
			"https://www.citi.com/credit-cards/citi-strata-premier-credit-card\r\n"
			"https://www.citi.com/credit-cards/travel-reward-credit-cards\r\n"
			"https://www.citi.com/credit-cards/citi-strata-premier-credit-card?afc=126&category=view-all-credit-cards\r\n"
			"https://www.citi.com/credit-cards/credit-card-rewards/citi-strata-premier-travel-benefits\r\n"
			"https://snamwpm.eccmp.com/wpm/617/ContentUploads/docs/GTB_CRE131.pdf\r\n"
			"https://www.mastercard.com/us/en/personal/find-a-card/credit-card/issuers/citi.html",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 116, .card = {"Discover it Cash Back", DISCOVER, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes =
			"Annual fee $0. Product offer snapshot: purchase/transfer intro APR 0% for 15 months, then "
			"18.74%-28.74% variable; transfer fee still applies. Separate Discover-by-Capital-One family "
			"sample dated 2026-09-30: advance APR 28.49%-29.24% variable, advance fee the larger of $10 "
			"or 5%, late fee up to $41, returned payment up to $30. Its broader account APR ranges are "
			"not this card's confirmed offer. Foreign and promotional transfer charges and any "
			"first-late waiver need the actual account/offer.",
		.earningNotes =
			"1% ordinary eligible purchases; 5% on activated rotating categories up to $1,500 combined "
			"spending each quarter, then 1%. Activation and the actual quarter's eligible merchants "
			"matter.",
		.benefitNotes =
			"Issuer-indexed Q4 2026 calendar: restaurants, entertainment and utilities. Bonus begins on "
			"2026-10-01 or activation day, whichever is later, and ends 2026-12-31. Shared $1,500 "
			"spending cap; U.S. merchants and qualifying merchant coding required. Shipping/processing "
			"dates and some third-party payments can change eligibility. Cashback Match is a first-year "
			"promotion paid afterward, not permanent doubled earning.",
		.reviewNotes =
			"The issuer's indexed calendar filled the missing quarter details; the live calendar did not "
			"render them. September 2026 family pricing and legacy Discover terms must stay separate. No "
			"first-late waiver or foreign-fee value was inferred from a missing field. The calculator "
			"still needs activation and cap inputs. Insurance is not assumed.",
		.source =
			"https://www.discover.com/credit-cards/cash-back/it-card/\r\n"
			"https://www.discover.com/credit-cards/cash-back/cashback-calendar.html\r\n"
			"https://www.discover.com/credit-cards/cash-back/cashback-calendar.html?link=%2Fweb%2Frewards%2F&msockid=020eafe172196fb62678b95773226e45\r\n"
			"https://ecm.capitalone.com/WCM/card/credit-card-agreements-q3-2026/credit-card-agreement-for-discover-by-capital-one-n.a.pdf\r\n"
			"https://www.discover.com/credit-cards/cardmember-agreement/",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 117, .card = {"Bank of America Customized Cash Rewards", NETWORK_UNKNOWN, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes =
			"Annual fee $0. Public offer: purchase/eligible-transfer intro APR 0% for 15 billing cycles, "
			"then 17.74%-27.74% variable; transfers in the first 60 days cost 3%, then 5%. Sample "
			"agreement dated 2026-09-30: foreign fee 3%; cash-advance fees 5%; direct-deposit/check "
			"advance APR 20.74%-28.74%, bank advances 29.49%, variable; late fee up to $41; penalty APR "
			"up to 29.99% can last indefinitely; monthly plan fee up to 1.72%. It lists percentage fees "
			"without general dollar minimums. Actual pricing varies; an absent fee is not a guaranteed "
			"zero.",
		.earningNotes =
			"Ongoing 3% selected category, 2% groceries/wholesale clubs, 1% other. The selected category "
			"and 2% purchases share $2,500 spending per calendar quarter, then 1%. Eligible new-account "
			"first-year promotion adds 3% in the choice category; 6% is not permanent.",
		.benefitNotes =
			"Choose among gas/EV, online shopping, dining, travel, drugstores or home improvement. "
			"Changes allowed once per calendar month; merchant exclusions apply. Online-shopping "
			"definitions differ from Amex. Account/rewards-program qualification affects any extra tier "
			"bonus; no tier was assumed.",
		.reviewNotes =
			"Product offer, category exclusions and September sample agreement were checked separately. "
			"Ordinary sample fees do not replace a temporary offer. Extra rewards depend on selected "
			"category, account age, shared-cap use and program tier. Returned-payment charges not "
			"specified in the sample were not assigned zero. No promotional maximum is guaranteed cash.",
		.source =
			"https://www.bankofamerica.com/credit-cards/products/cash-back-credit-card/\r\n"
			"https://www.bankofamerica.com/credit-cards/products/cash-back-credit-card/cash-back-category-choices/\r\n"
			"https://www.bankofamerica.com/credit-cards/credit-card-agreements/\r\n"
			"https://www.bankofamerica.com/content/documents/creditcard/bank_of_america_customized_cash_rewards_english.pdf",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 118, .card = {"Amazon Prime Visa", VISA, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes =
			"Annual card fee $0; foreign fee $0. Eligible Prime membership is separate: standard "
			"$139/year or $14.99/month; other plans differ. Published purchase APR 18.99%-27.74% "
			"variable. Other borrowing/late/returned-payment and installment pricing remains unconfirmed.",
		.earningNotes =
			"5% eligible Amazon.com, Audible, U.S. Whole Foods and Chase Travel with eligible Prime; "
			"generally 3% without. 2% gas, dining, local transit/commuting; 1% other. Rates depend on "
			"membership at purchase time.",
		.benefitNotes =
			"Amazon Pay elsewhere and international Amazon sites do not get the Amazon bonus. "
			"Third-party Whole Foods transactions and some exclusions do not qualify. Selected "
			"installments/promotions can replace ordinary rewards; a promotional 10% includes the usual "
			"5%, not 15% total.",
		.reviewNotes =
			"Current full pricing link LGC59329 remains unreadable. Public earning rules and costs shown "
			"above are sourced, but missing current borrowing charges are not zero. The calculator also "
			"needs merchant, membership and financing inputs. Protection requires the account's current "
			"guide; no automatic insurance or membership-cost offset is assumed.",
		.source =
			"https://creditcards.chase.com/cash-back-credit-cards/amazon-prime-rewards\r\n"
			"https://www.aboutamazon.com/news/retail/amazon-prime-free-trial\r\n"
			"https://sites.chase.com/services/creatives/pricingandterms.html/content/dam/pricingandterms/LGC59329.html",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 119, .card = {"Costco Anywhere Visa by Citi", VISA, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes =
			"Annual card fee $0 with paid Costco membership: Gold Star $65/year or Executive $130/year "
			"plus applicable tax; foreign fee $0. Issuer table v32 (no printed effective date): "
			"purchases/transfers 18.74%-26.74% variable; advances 28.74%. Indexed offer instead shows "
			"18.99%-27.49% and advance APR 28.99%; confirm the actual version. Table fees: transfers the "
			"larger of $5 or 5%; advances $10 or 5%; late/returned payments up to $41; minimum interest "
			"$0.50; monthly plan fee up to 1.72%; penalty APR up to 29.99% can last indefinitely.",
		.earningNotes =
			"5% Costco gas, 4% other eligible gas/EV; these share a $7,000 annual spending allowance, "
			"then 1%. 3% eligible dining/travel; 2% other Costco/Costco.com; 1% other. Nonqualifying "
			"merchants earn the base rate.",
		.benefitNotes =
			"Rewards arrive as an annual certificate after the February statement, redeemable at U.S. "
			"Costco warehouses including Puerto Rico; certificate expires December 31. It is not monthly "
			"unrestricted cashback. Executive membership's separate reward is not automatically added to "
			"card rewards.",
		.reviewNotes =
			"Personal guide effective 2023-01-22: damage/theft within 120 days (90 in New York), up to "
			"$1,000/item and $50,000/calendar year; exclusions apply. Rental cover up to $50,000, max 31 "
			"days, full card payment and declined agency waiver; secondary in U.S., primary outside, no "
			"liability cover. Personal card and business guide limits differ. Membership must continue. "
			"Fees, merchant eligibility and shared gas use require the actual account rules.",
		.source =
			"https://www.citi.com/credit-cards/costco-anywhere-visa-card\r\n"
			"https://customerservice.costco.com/app/answers/answer_view/a_id/1013504/~/membership-fee-increase\r\n"
			"https://www.citi.com/CRD/PDF/CMA/pricingTables/PIT_PID520-CostcoADA-2.pdf\r\n"
			"https://www.citi.com/CRD/PDF/CMA/cardAgreement/CMA_CostcoConsADA-3.pdf\r\n"
			"https://www.cardbenefits.citi.com/-/media/CPP/Files/LegalDocs/SOAPI/CostcoCon.ashx",
		.checkedDate = "2026-10-30"
	},
	{
		.id = 120, .card = {"Wells Fargo Autograph", VISA, 0, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 0, .comparison = POINTS_REVIEW,
		.pricingNotes =
			"Annual/foreign fee $0. Purchase intro APR 0% for 12 months, then 18.74%, 24.74% or 28.74% "
			"variable. Transfers do not share the purchase intro: transfer fee first 120 days "
			"max($5,3%), afterward up to 5%, minimum $5. Advances max($10,5%), APR 29.74% variable; late "
			"fee up to $40; minimum interest $1.",
		.earningNotes =
			"3 points/$ eligible dining, travel, gas/EV, transit, popular streaming and phone plans; 1 "
			"other. Merchant/category exclusions apply. Points are not automatically a fixed cash value "
			"for every redemption.",
		.benefitNotes =
			"Cell protection requires the preceding month's wireless bill paid with this card; $25 "
			"deductible, up to $600/claim, two claims/$1,200 per 12 months. Cosmetic-only damage and "
			"unexplained loss excluded. Coverage is supplemental. Roadside and assistance referrals can "
			"cost money.",
		.reviewNotes =
			"September 2026 pricing, reward terms and insurance guide checked. The purchase intro does "
			"not apply to transfers; advances/transfers start interest immediately. Reward credits do "
			"not replace required payments. Redemption options and points rounding follow the program; "
			"this calculator does not calculate their cash value. Later amendments and claim "
			"requirements still control.",
		.source =
			"https://creditcards.wellsfargo.com/autograph-visa-credit-card/\r\n"
			"https://www.wellsfargo.com/credit-cards/autograph-visa/terms/?FPID=012988I6P10000&cx_nm=CXNAME_CSMPD&product_code=CC&sub_channel=WEB&subproduct_code=AU&vendor_code=WF\r\n"
			"https://www.wellsfargo.com/credit-cards/autograph-visa/guide-to-benefits/\r\n"
			"https://www.wellsfargo.com/credit-cards/documents/autograph-visa-terms/?FPID=012988I6P10000&cx_nm=CXNAME_CSMPD&product_code=CC&sub_channel=WEB&subproduct_code=AU&vendor_code=WF",
		.checkedDate = "2026-10-30"
	}
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
		"Eligible entertainment", "Eligible popular streaming", "Eligible Lyft ride",
		"Superstore or warehouse club", "Eligible transit"
	};
	return category >= 0 && category < PURCHASE_CATEGORY_COUNT ? names[category] : "Unknown category";
}

const char* purchaseChannelName(enum PurchaseChannel channel) {
	static const char* names[] = {"Direct / ordinary purchase", "Capital One Travel",
		"Chase Travel", "Capital One Entertainment"};
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

const char* realCardRewardUnit(const RealCard* card) {
	if (!card) return "unknown";
	if (card->card.reward == REWARD_POINTS) return "points per dollar";
	if (card->card.reward == MILES) return "miles per dollar";
	return "percent cashback";
}

const char* comparisonNote(const RealCard* card) {
	if (card->comparison == CASH_SUPPORTED) return "Cashback estimates support the modeled categories. Caps and eligibility still apply.";
	if (card->comparison == POINTS_REVIEW) return "This calculator does not estimate points/miles. Use the stated redemption method and actual benefit rules; no dollar ranking is assumed.";
	return "This form does not collect all activation, membership, payment, cap or merchant details needed for this card. Review its rules; no definite cashback ranking is assigned.";
}

/* I compare names without changing saved IDs or guessing a different product. */
bool realCardMatches(int cardId, const char* search) {
	const RealCard* card = getRealCardById(cardId);
	if (!card || !search) return false;
	size_t length = strlen(search);
	if (length == 0) return true;
	const char* name = card->card.issuer;
	for (size_t start = 0; name[start]; start++) {
		size_t i = 0;
		while (i < length && name[start + i]
			&& tolower((unsigned char)name[start + i]) == tolower((unsigned char)search[i])) i++;
		if (i == length) return true;
	}
	return false;
}

bool formatRealCard(int cardId, char* text, size_t capacity) {
	const RealCard* card = getRealCardById(cardId);
	if (!card || !text || capacity == 0) return false;
	int length = snprintf(text, capacity,
		"%s\r\n\r\nCosts and interest\r\n%s\r\n\r\nEarning and limits\r\n%s\r\n\r\nBenefit conditions\r\n%s\r\n\r\nWhat was checked and what is missing\r\n%s\r\n\r\nComparison\r\n%s\r\n\r\nChecked %s\r\nSources and follow-up links\r\n%s\r\n\r\nResearched summary, not a complete account contract. Public terms and personal offers can differ. Unknown fees are not zero; credits and insurance are not guaranteed cash. APR is a yearly borrowing rate. Prime plus a margin means add the two rates; monthly plan fees are not yearly APRs. Purchase estimates exclude interest, fees and welcome bonuses.",
		card->card.issuer, card->pricingNotes, card->earningNotes, card->benefitNotes,
		card->reviewNotes, comparisonNote(card), card->checkedDate, card->source);
	return length >= 0 && (size_t)length < capacity;
}

void printRealCardList(void) {
	puts("\n20 common U.S. cards - researched summaries checked October 30, 2026");
	puts("This is a selection, not an audited popularity ranking. Missing fees stay unconfirmed.");
	puts("No match for your exact product? It may be missing. Don't pick a similar card as a substitute.");
	for (int i = 0; i < REAL_CARD_COUNT; i++) {
		const RealCard* card = &realCards[i];
		printf("%d. %s | %.2f %s | ongoing annual fee $%d\n",
			card->id, card->card.issuer, card->card.rate, realCardRewardUnit(card), card->card.annualFee);
		if (card->firstYearFee != card->card.annualFee)
			printf("     First-year annual fee $%d under the checked offer.\n", card->firstYearFee);
	}
}
