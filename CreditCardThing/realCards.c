/*
Joe's catalog area. These are researched summaries of twenty common U.S.
personal products, not a ranked list or complete bank agreements. Costs come
first. I keep unknown charges visible and never add headline credit totals
as guaranteed savings. Source checks are in docs/twentyCardResearch.txt.
Original real IDs 101-105 and practice IDs 1-5 keep their meanings.
*/
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "realCards.h"

static RealCard realCards[REAL_CARD_COUNT] = {
	{
		.id = 101, .card = {"Capital One Quicksilver Rewards", NETWORK_UNKNOWN, 0, CASHBACK, 1.5, {{0}}},
		.baseRateBp = 150, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes = "Annual fee $0; foreign fee $0. Published purchase/transfer intro APR 0% for 15 months, then 18.74%-28.74% variable. Transfer fees still apply; full cash-advance, late/returned-payment, plan and transfer pricing could not be verified from the rendered disclosure.",
		.earningNotes = "1.5% eligible base purchases. Eligible Capital One Travel hotel, rental-car and vacation-rental bookings earn 5%; Capital One Entertainment earns 5%. Flights do not receive the hotel/car bonus.",
		.benefitNotes = "Use the specific issuer booking service. Merchant coding and processing control eligibility. Rewards do not expire while the account is open. A price-match travel credit is not a cash refund; warranty/insurance benefits depend on the actual card version and guide.",
		.reviewNotes = "Product/earnings checked; full pricing and account-specific insurance guide incomplete. This is Quicksilver Rewards, not QuicksilverOne or a student/secured variant.",
		.source = "https://www.capitalone.com/credit-cards/quicksilver/", .checkedDate = "2026-10-30"
	},
	{
		.id = 102, .card = {"Capital One Savor Rewards", NETWORK_UNKNOWN, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes = "Annual fee $0; foreign fee $0. Published purchase/transfer intro APR 0% for 12 months, then 18.74%-28.74% variable. Transfer, cash-advance, late/returned-payment and plan charges need the application disclosure; unverified does not mean free.",
		.earningNotes = "3% eligible groceries, dining, entertainment and popular streaming; 1% other. Eligible Capital One Travel hotels, rental cars and vacation rentals earn 5%; Capital One Entertainment earns 8%.",
		.benefitNotes = "Superstores such as Walmart and Target are excluded from the grocery bonus. Not all streaming or entertainment merchants qualify. Payment processors can affect merchant coding. Rewards remain available while the account is open; credits, insurance and concierge services are not guaranteed cash.",
		.reviewNotes = "Current no-annual-fee Savor Rewards version. Full pricing and card-specific protection limits remain incomplete; do not use old annual-fee Savor terms.",
		.source = "https://www.capitalone.com/credit-cards/savor/", .checkedDate = "2026-10-30"
	},
	{
		.id = 103, .card = {"Chase Freedom Unlimited", VISA, 0, CASHBACK, 1.5, {{0}}},
		.baseRateBp = 150, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes = "Annual fee $0. Published purchase/transfer intro APR 0% for 15 months, then 18.49%-27.99% variable. Current foreign, transfer, cash-advance, late/returned-payment and plan fees remain unconfirmed because the pricing link could not be read.",
		.earningNotes = "1.5% base, 3% total dining/drugstores, 5% eligible Chase Travel. Qualifying Lyft app purchases earn 2% total through 2027-09-30. Cash redemption is distinct from other point-redemption options.",
		.benefitNotes = "Chase Travel rewards exclude points-paid amounts and FROSCH. Merchant definitions apply. DashPass needs activation by 2029-12-31, provides six complimentary months, then paid automatic renewal at the current rate unless cancelled. The quarterly non-restaurant discount has order restrictions and does not cover fees/taxes/tips.",
		.reviewNotes = "Protection claims require the applicable guide and covered payment. Full pricing remains incomplete. No subscription, welcome bonus or insurance value is added to purchase cashback.",
		.source = "https://creditcards.chase.com/cash-back-credit-cards/freedom/unlimited\r\nhttps://asset.chase.com/content/dam/card/rulesregulations/en/RPA0509_Web.pdf", .checkedDate = "2026-10-30"
	},
	{
		.id = 104, .card = {"American Express Blue Cash Everyday", AMEXPRESS, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes = "Annual fee $0. Issuer agreement dated 2026-03-31 lists foreign fee 2.7%; transfers max($5,3%); advances max($10,5%); late/returned payments up to $40. Its variable APR/penalty and plan terms are dated agreement data, not a confirmed current application offer. Read current terms before borrowing.",
		.earningNotes = "3% eligible U.S. supermarkets, gas stations and online retail, each with its own $6,000 calendar-year spending cap, then 1%; 1% other. Cash back is Reward Dollars, not an automatic payment against the minimum due.",
		.benefitNotes = "Superstores/warehouse clubs do not count as supermarkets; their gas is excluded too. Online travel, restaurants and services are not online retail. Enter prior spending in the specific category/year. Disney streaming: enrollment required, qualifying U.S. subscriptions and monthly spending required, up to $7/month; subscriptions auto-renew.",
		.reviewNotes = "Purchase/rental protections have exclusions; rental coverage is secondary and excludes liability. Current offer APR and any amendments to the March agreement remain unverified.",
		.source = "https://www.americanexpress.com/us/credit-cards/card/blue-cash-everyday/\r\nhttps://www.americanexpress.com/content/dam/amex/en-us/company/legal/cardmember-agreements/public-site-2026-q1-pdf-cmas/cps-lending/blue-cash-everyday-03-31-2026.pdf", .checkedDate = "2026-10-30"
	},
	{
		.id = 105, .card = {"Wells Fargo Active Cash", VISA, 0, CASHBACK, 2.0, {{0}}},
		.baseRateBp = 200, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes = "Annual fee $0; foreign fee 3%. Published intro APR: purchases/qualifying transfers 0% for 12 months, then 18.74%, 24.74% or 28.74% variable. Transfer fee within 120 days max($5,3%), afterward up to 5%, minimum $5. Advances max($10,5%), APR 29.74% variable; late fee up to $40; minimum interest $1.",
		.earningNotes = "Unlimited 2% eligible purchases. Transfers, advances, cash equivalents, fees, interest and excluded transactions do not earn rewards. Returns reduce rewards.",
		.benefitNotes = "Cell-phone protection: pay the wireless bill with the card in the preceding month; coverage starts next calendar month. $25 deductible, up to $600/claim, two claims/$1,200 per 12 months. Supplemental coverage; unexplained loss and cosmetic-only damage excluded. Roadside/assistance services can cost money.",
		.reviewNotes = "Product, pricing and guide reviewed. Published terms are channel-specific; account amendments and claim deadlines still control. Insurance is not cash back.",
		.source = "https://creditcards.wellsfargo.com/active-cash-credit-card/\r\nhttps://www.wellsfargo.com/credit-cards/active-cash/terms/?FPID=012686I6F40000&cx_nm=CXNAME_CSMPD&product_code=CC&sub_channel=WEB&subproduct_code=AC&vendor_code=WF\r\nhttps://www.wellsfargo.com/credit-cards/active-cash/guide-to-benefits/", .checkedDate = "2026-10-30"
	},
	{
		.id = 106, .card = {"Chase Freedom Flex", MASTERCARD, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes = "Annual fee $0; current product page states no foreign transaction fee. Published purchase/transfer intro APR 0% for 15 months, then 18.49%-27.99% variable. Exact transfer, advance, late/returned-payment and plan fees remain unconfirmed; linked pricing was unavailable.",
		.earningNotes = "1% base; 3% dining/drugstores; 5% eligible Chase Travel. Activated rotating categories earn 5% total on $1,500 combined spending per quarter, then normal earning. Overlapping dining/quarter bonuses require the exact stacking rule; never simply add advertised percentages.",
		.benefitNotes = "Q4 2026 categories include eligible groceries (excluding Walmart/Target), dining and American Red Cross; categories expire at quarter end. Lyft: 2% total through 2027-09-30. DashPass: activate by 2029-12-31, six months complimentary, then paid renewal. Current guide: rental coverage generally secondary in the U.S.; protection exclusions and claim deadlines apply.",
		.reviewNotes = "Catalog and conditions are shown; calculator review required for activation, shared-cap usage and stacking. No definite cashback ranking is generated for this card.",
		.source = "https://creditcards.chase.com/cash-back-credit-cards/freedom/flex\r\nhttps://asset.chase.com/content/services/structured-document/document.en.pdf/card/benefits-center/product-benefits-guide-pdf/BGC11542.pdf", .checkedDate = "2026-10-30"
	},
	{
		.id = 107, .card = {"Chase Sapphire Preferred", VISA, 95, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 95, .comparison = POINTS_REVIEW,
		.pricingNotes = "Annual fee $95; foreign fee $0. Published purchase APR 19.49%-27.74% variable. Transfer, advance, late/returned-payment and installment pricing not fully verified; check the current application disclosure.",
		.earningNotes = "5 points/$ eligible Chase Travel; 2 other travel; 3 dining, selected streaming, online groceries excluding Target/Walmart/clubs, gas/EV charging and named vacation-home brands; 1 other. Lyft 5 points/$ through 2027-09-30. Points are not cash percentages.",
		.benefitNotes = "$100 Chase Travel hotel credit per anniversary year; credited amounts do not earn points. Screening credit up to $120 every four years. Apple TV: activate by 2026-12-31, one year; prior paid plans can resume. Rental coverage up to $60,000, generally primary, but New York personal-insurance exception applies; decline agency waiver, eligible vehicles/rental periods only.",
		.reviewNotes = "June 2026 updates checked: old $50 hotel-credit and ongoing 10% anniversary-bonus claims are outdated. Hyatt transfer ratio is 4:3 under current rules. Use current guide for covered events, exclusions and claim deadlines.",
		.source = "https://creditcards.chase.com/rewards-credit-cards/sapphire/preferred\r\nhttps://media.chase.com/news/Meet-the-New-Chase-Sapphire-Preferred\r\nhttps://static.chasecdn.com/content/services/structured-document/document.en.pdf/card/benefits-center/product-benefits-guide-pdf/BGC11599.pdf", .checkedDate = "2026-10-30"
	},
	{
		.id = 108, .card = {"Chase Sapphire Reserve", VISA, 795, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 795, .comparison = POINTS_REVIEW,
		.pricingNotes = "Annual fee $795; each authorized user $195; foreign fee $0. Published purchase APR 19.74%-28.24% variable. Other borrowing, late/returned-payment and installment charges require the unread current pricing disclosure.",
		.earningNotes = "8 points/$ eligible Chase Travel; 4 direct airline/hotel; 3 dining; 1 other. Purchases reimbursed by the $300 travel or The Edit credits do not earn points. Points Boost is selective, not a guaranteed two-cent cash value.",
		.benefitNotes = "$300 travel credit per anniversary year. The Edit: $500/year, max $250/transaction, prepaid two-night stay. Dining: $150 each half-year at eligible Exclusive Tables restaurants. Select-hotel $250 offer ends 2026-12-31 and needs two nights. Screening credit up to $120 every four years. Priority Pass needs activation; lounge space/guest rules apply. Apple subscriptions need activation by 2027-06-22; paid plans may resume. StubHub $150/half-year through 2027-12-31 requires activation; Lyft $10/month through 2027-09-30; Peloton $10/month through 2027-12-31 requires activation. DoorDash promos require eligible orders/membership and exclude fees/tax/tips.",
		.reviewNotes = "Guide reviewed: trip delay over six hours/overnight, up to $500/covered traveler; rental cover has vehicle, payment and liability exclusions. Spending-gated extras are not automatic. Full account-specific pricing and every partner credit condition remain outside this summary.",
		.source = "https://creditcards.chase.com/rewards-credit-cards/sapphire/reserve\r\nhttps://www.chase.com/sapphire-cards/personal/reserve\r\nhttps://static.chasecdn.com/content/services/structured-document/document.en.pdf/card/benefits-center/product-benefits-guide-pdf/508_BGC11559_SAPPHIRE_RESERVE-1.pdf", .checkedDate = "2026-10-30"
	},
	{
		.id = 109, .card = {"American Express Gold", AMEXPRESS, 325, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 325, .comparison = POINTS_REVIEW,
		.pricingNotes = "Annual fee $325; first five additional Gold cards $0, sixth and later $35 each. Foreign fee $0. Current Pay Over Time APR, advance, late/returned-payment and plan charges require application terms. No preset spending limit does not mean unlimited spending or interest-free borrowing.",
		.earningNotes = "4 points/$ restaurants worldwide up to $50,000/calendar year and U.S. supermarkets up to $25,000/calendar year, then 1. 3 qualifying flights; 5 prepaid Amex Travel hotels; 2 qualifying prepaid Amex rentals/cruises; 1 other.",
		.benefitNotes = "Credits: dining up to $10/month at named partners, Dunkin $7/month, Resy $50 each half-year; enrollment required. Uber Cash $10/month in the U.S., linked card and qualifying Amex payment required. These are spending-dependent allowances, not $424 deposited at once. Hotel Collection benefits require a two-night eligible booking.",
		.reviewNotes = "Rental insurance is secondary and excludes liability; country/vehicle exclusions apply. Trip-delay cover requires eligible fully paid round-trip and covered delay over 12 hours, up to $300/trip, two claims/12 months. Full offer pricing not verified.",
		.source = "https://www.americanexpress.com/us/credit-cards/card/gold-card/\r\nhttps://www.americanexpress.com/en-us/credit-cards/credit-intel/gold-card-annual-fee/", .checkedDate = "2026-10-30"
	},
	{
		.id = 110, .card = {"American Express Platinum", AMEXPRESS, 895, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 895, .comparison = POINTS_REVIEW,
		.pricingNotes = "Annual fee $895; additional Platinum $195 each. $0 Companion cards do not duplicate Platinum benefits. Foreign fee $0. Pay Over Time APR, advance, late/returned-payment and plan pricing need current disclosure; flexible spending is not unlimited credit.",
		.earningNotes = "5 points/$ direct-airline/Amex Travel flights, $500,000/calendar-year cap; 5 prepaid Amex Travel hotels; 1 other. Not 5% cash back.",
		.benefitNotes = "Allowances: hotels $300/half-year; Resy $100/quarter; digital entertainment $25/month; lululemon $75/quarter; Uber $15/month plus $20 December; Oura ring $200/year; selected-airline incidentals $200/year; CLEAR+ $219/year; Uber One $120/year; Walmart+ eligible monthly fee up to $12.95 plus tax. Eligibility/enrollment/renewal rules apply.",
		.reviewNotes = "Hotel Collection needs two nights; FHR credit also needs two nights from 2027-01-01. Delta lounge access is limited to ten eligible visits, not unlimited; Priority Pass enrollment and guest charges apply. These maximum allowances are not guaranteed savings or automatic fee offsets. Current pricing and complete protection terms remain unverified.",
		.source = "https://www.americanexpress.com/us/credit-cards/card/platinum/\r\nhttps://www.americanexpress.com/en-us/credit-cards/credit-intel/amex-gold-vs-platinum/", .checkedDate = "2026-10-30"
	},
	{
		.id = 111, .card = {"American Express Blue Cash Preferred", AMEXPRESS, 95, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_SUPPORTED,
		.pricingNotes = "Public offer: $0 introductory annual fee in year one, then $95/year. Foreign fee 2.7%. Current APR, transfer/advance fees, late/returned-payment and plan charges remain unconfirmed; the direct application-terms link expired.",
		.earningNotes = "6% eligible U.S. supermarkets on $6,000/calendar year, then 1%; 6% select U.S. streaming; 3% eligible U.S. gas and transit; 1% other. Reward Dollars are redeemable rewards, not payment of the minimum due.",
		.benefitNotes = "Superstores/warehouse clubs are not supermarkets; gas/category and streaming definitions matter. Disney credit up to $10/month on enrolled qualifying U.S. subscriptions; auto-renewal applies. Return protection has per-item/year limits; rental coverage is secondary and excludes liability. Read the guide before relying on insurance.",
		.reviewNotes = "App models eligible streaming, gas and transit; groceries show a capped range because this form does not capture Preferred's own prior use. It must not reuse Everyday's spending. Unavailable pricing is explicitly incomplete.",
		.source = "https://www.americanexpress.com/us/credit-cards/card/blue-cash-preferred/\r\nhttps://www.americanexpress.com/en-us/credit-cards/credit-intel/amex-gold-vs-blue-cash-preferred/", .checkedDate = "2026-10-30"
	},
	{
		.id = 112, .card = {"Capital One Venture Rewards", NETWORK_UNKNOWN, 95, MILES, 2.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 95, .comparison = POINTS_REVIEW,
		.pricingNotes = "Annual fee $95; foreign fee $0. Published purchase APR 19.74%-28.74% variable. Advance, transfer, late/returned-payment and plan fees need the full disclosure.",
		.earningNotes = "2 miles/$ base; 5 eligible Capital One Travel hotels, rental cars and vacation rentals, and Capital One Entertainment. Miles are not cashback percentages; cash, travel and transfer redemptions can differ.",
		.benefitNotes = "Screening application credit up to $120, with renewal interval and eligibility in the benefit terms. Lifestyle Collection experience credit up to $50 per eligible stay, usable for specified on-property charges. A limited-time first-year stays credit is not a recurring annual benefit. Hertz status requires eligibility/enrollment.",
		.reviewNotes = "Do not assume lounge access is included: Venture holders can face entry charges. Rental protection depends on the card version and guide. Current public page and travel rules checked; full fee/insurance disclosure incomplete.",
		.source = "https://www.capitalone.com/credit-cards/venture/\r\nhttps://capitalonetravel.com/airport-lounges", .checkedDate = "2026-10-30"
	},
	{
		.id = 113, .card = {"Capital One Venture X Rewards", VISA, 395, MILES, 2.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 395, .comparison = POINTS_REVIEW,
		.pricingNotes = "Annual fee $395; foreign fee $0. Published purchase APR 19.74%-28.74% variable. Additional-card lounge access costs $125/person/year; this is separate from the card fee. Other borrowing/penalty fees need current disclosure.",
		.earningNotes = "2 miles/$ base; 10 eligible Capital One Travel hotels/cars; 5 portal flights/vacation rentals and Capital One Entertainment. Travel credits do not turn points or miles into cash.",
		.benefitNotes = "$300 annual Capital One Travel credit expires at the next anniversary; restricted to its portal. Lounge guests generally cost $45/adult or $25/child 2-17; Priority Pass guests $35. $75,000 annual spending can unlock certain Capital One guest privileges; it is not unconditional free guest access. Priority Pass enrollment and lounge capacity limits apply. 10,000 anniversary miles start at the first anniversary. Premier/Lifestyle Collection credits require eligible bookings and on-property use.",
		.reviewNotes = "Primary issuer/travel rules checked. Account-specific fee/insurance disclosure incomplete. Do not subtract a credit or assumed lounge value from the $395 fee unless the user actually qualifies and uses it.",
		.source = "https://www.capitalone.com/credit-cards/venture-x/\r\nhttps://capitalonetravel.com/airport-lounges/lounge-access-guide\r\nhttps://www.capitalone.com/learn-grow/more-than-money/all-about-venture-x/", .checkedDate = "2026-10-30"
	},
	{
		.id = 114, .card = {"Citi Double Cash", MASTERCARD, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes = "Annual fee $0; no authorized-user fee. Current foreign fee, APR, transfer/advance fees, late/returned-payment and installment charges could not be verified from the dynamically blank pricing fields. Do not treat them as zero.",
		.earningNotes = "1% when buying plus 1% as qualifying purchase balances are paid, not an unconditional 2% on purchase day. Eligible Citi Travel hotel/car/attraction bookings add 3%, for 5% total only when payment conditions are met.",
		.benefitNotes = "Cashback is tracked as ThankYou points. Transfers do not earn rewards; the transfer intro does not cover purchase APR. Carrying a transfer can remove the purchase grace period unless the entire balance is paid. Payments need matching Purchase Tracker balances. Check-by-mail redemption has a $5 minimum; other redemptions differ.",
		.reviewNotes = "Calculator review required for payment/qualification timing. Product/rewards and fee-free annual status verified; complete current pricing is unavailable. Extended warranty is secondary coverage with exclusions, not an automatic reimbursement.",
		.source = "https://www.citi.com/credit-cards/citi-double-cash-credit-card\r\nhttps://www.citi.com/credit-cards/rewards-credit-cards", .checkedDate = "2026-10-30"
	},
	{
		.id = 115, .card = {"Citi Strata Premier", NETWORK_UNKNOWN, 95, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 95, .comparison = POINTS_REVIEW,
		.pricingNotes = "Annual fee $95; no authorized-user or foreign transaction fee. Fee confirmed by issuer-indexed card listings; some rendered pricing fields were blank. Exact APR, transfer/advance, late/returned-payment and installment charges remain unconfirmed.",
		.earningNotes = "10 points/$ eligible Citi Travel hotels/cars/attractions; 3 air travel, other hotels, restaurants, supermarkets, gas/EV charging; 1 other. ThankYou redemption/transfer values vary.",
		.benefitNotes = "Once per calendar year, $100 off a single hotel stay costing at least $500 before taxes/fees, booked through Citi Travel or its specified phone line. It is not a general $100 hotel refund. Cancellation may remove the benefit; inspect booking terms.",
		.reviewNotes = "Product lists trip cancellation/interruption, trip delay, lost/damaged luggage and rental cover, but full current claim limits/guide were not obtained. Do not assume all delays or rental liability are covered. New-account bonus eligibility has prior-card restrictions; no bonus is included in estimates.",
		.source = "https://www.citi.com/credit-cards/citi-strata-premier-credit-card\r\nhttps://www.citi.com/credit-cards/travel-reward-credit-cards", .checkedDate = "2026-10-30"
	},
	{
		.id = 116, .card = {"Discover it Cash Back", DISCOVER, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes = "Annual fee $0. Published purchase/transfer intro APR 0% for 15 months, then 18.74%-28.74% variable; transfer fee applies. Current foreign, cash-advance, late/returned-payment and detailed transfer pricing were not fully rendered.",
		.earningNotes = "1% ordinary eligible purchases; 5% on activated rotating categories up to $1,500 combined spending each quarter, then 1%. Activation and the actual quarter's eligible merchants matter.",
		.benefitNotes = "Cashback Match is a first-year promotion paid afterward, not permanent doubled earning. Exact qualification and processing rules control. The public calendar confirmed the cap but did not load the current category list; no categories were guessed.",
		.reviewNotes = "Calculator review required for the current calendar, activation and cap use. Product/standard earning checked; complete current pricing and detailed promotional terms remain incomplete. Do not assume travel or purchase insurance is included.",
		.source = "https://www.discover.com/credit-cards/cash-back/it-card/\r\nhttps://www.discover.com/credit-cards/cash-back/cashback-calendar.html", .checkedDate = "2026-10-30"
	},
	{
		.id = 117, .card = {"Bank of America Customized Cash Rewards", NETWORK_UNKNOWN, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes = "Annual fee $0. Published purchase/eligible-transfer intro APR 0% for 15 billing cycles, then 17.74%-27.74% variable. Transfer fee first 60 days 3%, then 5%; full minimum charges, foreign/advance, late/returned-payment and plan pricing remain unconfirmed.",
		.earningNotes = "Ongoing 3% selected category, 2% groceries/wholesale clubs, 1% other. The selected category and 2% purchases share $2,500 spending per calendar quarter, then 1%. Eligible new-account first-year promotion adds 3% in the choice category; 6% is not permanent.",
		.benefitNotes = "Choose among gas/EV, online shopping, dining, travel, drugstores or home improvement. Changes allowed once per calendar month; merchant exclusions apply. Online-shopping definitions differ from Amex. Account/rewards-program qualification affects any extra tier bonus; no tier was assumed.",
		.reviewNotes = "Calculator review required for chosen category, account age, shared cap and tier. Current offer and category exclusions checked. No statement-credit, bank-balance or promotional maximum is counted as guaranteed cash.",
		.source = "https://www.bankofamerica.com/credit-cards/products/cash-back-credit-card/\r\nhttps://www.bankofamerica.com/credit-cards/products/cash-back-credit-card/cash-back-category-choices/", .checkedDate = "2026-10-30"
	},
	{
		.id = 118, .card = {"Amazon Prime Visa", VISA, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes = "Annual card fee $0; foreign fee $0. Eligible Prime membership is separate: standard $139/year or $14.99/month; other plans differ. Published purchase APR 18.99%-27.74% variable. Other borrowing/late/returned-payment and installment pricing remains unconfirmed.",
		.earningNotes = "5% eligible Amazon.com, Audible, U.S. Whole Foods and Chase Travel with eligible Prime; generally 3% without. 2% gas, dining, local transit/commuting; 1% other. Rates depend on membership at purchase time.",
		.benefitNotes = "Amazon Pay elsewhere and international Amazon sites do not get the Amazon bonus. Third-party Whole Foods transactions and some exclusions do not qualify. Selected installments/promotions can replace ordinary rewards; a promotional 10% includes the usual 5%, not 15% total.",
		.reviewNotes = "Calculator review required for exact merchant, membership and financing selection. Public reward terms checked. Protection benefits require current guide; no automatic insurance or membership-cost offset is assumed.",
		.source = "https://creditcards.chase.com/cash-back-credit-cards/amazon-prime-rewards\r\nhttps://www.aboutamazon.com/news/retail/amazon-prime-free-trial", .checkedDate = "2026-10-30"
	},
	{
		.id = 119, .card = {"Costco Anywhere Visa by Citi", VISA, 0, CASHBACK, 1.0, {{0}}},
		.baseRateBp = 100, .firstYearFee = 0, .comparison = CASH_REVIEW,
		.pricingNotes = "Annual card fee $0 only with paid Costco membership: standard Gold Star $65/year or Executive $130/year, plus applicable tax. Foreign fee $0; no authorized-user fee. Current APR, transfer/advance, late/returned-payment and installment pricing remains unconfirmed.",
		.earningNotes = "5% Costco gas, 4% other eligible gas/EV; these share a $7,000 annual spending allowance, then 1%. 3% eligible dining/travel; 2% other Costco/Costco.com; 1% other. Nonqualifying merchants earn the base rate.",
		.benefitNotes = "Rewards arrive as an annual certificate after the February statement, redeemable at U.S. Costco warehouses including Puerto Rico; certificate expires December 31. It is not monthly unrestricted cashback. Executive membership's separate reward is not automatically added to card rewards.",
		.reviewNotes = "Calculator review required for merchant eligibility, shared gas use and membership. Rental cover excludes liability and has payment/waiver conditions. Emergency/roadside referral services can charge for work performed. Exact claim guide and current pricing remain incomplete.",
		.source = "https://www.citi.com/credit-cards/costco-anywhere-visa-card\r\nhttps://customerservice.costco.com/app/answers/answer_view/a_id/1013504/~/membership-fee-increase", .checkedDate = "2026-10-30"
	},
	{
		.id = 120, .card = {"Wells Fargo Autograph", VISA, 0, REWARD_POINTS, 1.0, {{0}}},
		.baseRateBp = 0, .firstYearFee = 0, .comparison = POINTS_REVIEW,
		.pricingNotes = "Annual/foreign fee $0. Purchase intro APR 0% for 12 months, then 18.74%, 24.74% or 28.74% variable. Transfers do not share the purchase intro: transfer fee first 120 days max($5,3%), afterward up to 5%, minimum $5. Advances max($10,5%), APR 29.74% variable; late fee up to $40; minimum interest $1.",
		.earningNotes = "3 points/$ eligible dining, travel, gas/EV, transit, popular streaming and phone plans; 1 other. Merchant/category exclusions apply. Points are not automatically a fixed cash value for every redemption.",
		.benefitNotes = "Cell protection requires the preceding month's wireless bill paid with this card; $25 deductible, up to $600/claim, two claims/$1,200 per 12 months. Cosmetic-only damage and unexplained loss excluded. Coverage is supplemental. Roadside and assistance referrals can cost money.",
		.reviewNotes = "Pricing, product and insurance guide checked. Account amendments, claim deadlines and applicable coverage exclusions still control. No subscription, insurance payout or welcome bonus is treated as spendable cash.",
		.source = "https://creditcards.wellsfargo.com/autograph-visa-credit-card/\r\nhttps://www.wellsfargo.com/credit-cards/autograph-visa/terms/?FPID=012988I6P10000&cx_nm=CXNAME_CSMPD&product_code=CC&sub_channel=WEB&subproduct_code=AU&vendor_code=WF\r\nhttps://www.wellsfargo.com/credit-cards/autograph-visa/guide-to-benefits/", .checkedDate = "2026-10-30"
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
		"Eligible entertainment", "Eligible popular streaming", "Qualifying Lyft app purchase",
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
	if (card->comparison == POINTS_REVIEW) return "Points/miles are not cash. This card needs a redemption choice and benefit conditions reviewed; no cash ranking is assumed.";
	return "Activation, membership, payment, cap or merchant conditions need review. Read these rules; this card is not assigned a definite cashback ranking.";
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
		"%s\r\n\r\nCosts and interest\r\n%s\r\n\r\nEarning and limits\r\n%s\r\n\r\nBenefit conditions\r\n%s\r\n\r\nWhat was checked and what is missing\r\n%s\r\n\r\nComparison\r\n%s\r\n\r\nChecked %s\r\nSources\r\n%s\r\n\r\nResearched summary, not a complete account contract. Public terms and personal offers can differ. Unknown fees are not zero; credits and insurance are not guaranteed cash. Purchase estimates exclude interest, fees and welcome bonuses.",
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
