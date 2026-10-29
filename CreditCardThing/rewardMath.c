/* Reward rules describe total earning, not an extra percentage to add blindly.
   I compare eligible scenarios, split spending at caps, and keep unknown
   conditions visible. A manual estimate is not a bank transaction guarantee. */
#include "rewardMath.h"
#include "catalogText.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* const spendingCategories[] = { "other", "groceries", "dining", "gas",
    "travel", "hotels", "airlines", "drugstores", "entertainment", "streaming",
    "onlineShopping", "transit", "utilities", "wholesale", "rent", "carRentals", "vacationRentals", "phonePlans" };
const int spendingCategoryCount = (int)(sizeof(spendingCategories) / sizeof(spendingCategories[0]));

/* Eden's input continuation: a typed label must not silently miss a bonus.
   Spaces, hyphens, underscores and letter case may vary; unknown names fail. */
static bool normalizedLabel(const char* text,char label[80]) {
    int length=0;
    for (;*text;text++) {
        unsigned char ch=(unsigned char)*text;
        if (isspace(ch)||ch=='-'||ch=='_') continue;
        if (!isalnum(ch)||length==79) return false;
        label[length++]=(char)tolower(ch);
    }
    label[length]='\0'; return length>0;
}

bool normalizeSpendingCategory(const char* text,char category[40]) {
    char typed[80],known[80];
    if (!normalizedLabel(text,typed)) return false;
    for (int i=0;i<spendingCategoryCount;i++) {
        normalizedLabel(spendingCategories[i],known);
        if (!strcmp(typed,known)) return copyText(category,40,spendingCategories[i]);
    }
    return false;
}

static bool multiply(int64_t a, int64_t b, int64_t* result) {
    if (a < 0 || b < 0 || (b && a > INT64_MAX / b)) return false;
    *result = a * b; return true;
}

static bool add(int64_t a, int64_t b, int64_t* result) {
    if (a < 0 || b < 0 || a > INT64_MAX - b) return false;
    *result = a + b; return true;
}

static int64_t rounded(int64_t numerator, int64_t denominator) {
    int64_t value = numerator / denominator;
    return value + (numerator % denominator >= denominator / 2 + denominator % 2);
}

static int64_t gcd(int64_t a, int64_t b) {
    while (b) { int64_t next = a % b; a = b; b = next; }
    return a;
}

static bool cashValue(int64_t numerator, int unit, int64_t valuation, int64_t* cents) {
    if (unit == UNIT_CASHBACK) { *cents = rounded(numerator, 1000000); return true; }
    if ((unit != UNIT_POINTS && unit != UNIT_MILES) || valuation < 0) return false;
    int64_t denominator = INT64_C(100000000000), factor = gcd(numerator, denominator);
    numerator /= factor; denominator /= factor;
    factor = gcd(valuation, denominator); valuation /= factor; denominator /= factor;
    int64_t product;
    if (!multiply(numerator, valuation, &product)) return false;
    *cents = rounded(product, denominator); return true;
}

static bool matches(const RewardRule* rule, const WalletEntry* entry, const Purchase* purchase) {
    return (!strcmp(rule->category, "other") || !strcmp(rule->category, purchase->category)
        || (!strcmp(rule->category, "selected") && (!entry->chosenCategory[0] || !strcmp(entry->chosenCategory, purchase->category))))
        && (!strcmp(rule->channel, "any") || !strcmp(rule->channel, purchase->channel))
        && (!rule->start[0] || strcmp(purchase->date, rule->start) >= 0)
        && (!rule->end[0] || strcmp(purchase->date, rule->end) <= 0);
}

static bool ruleValue(const RewardRule* rule, WalletEntry* entry, const Purchase* purchase,
    int64_t* confirmed, int64_t* possible, bool* eligible) {
    *confirmed = 0; *possible = 0; *eligible = true;
    char period[24];
    bool periodKnown = makePeriodKey(rule->period, purchase->date, entry->anniversaryMonth,
        entry->anniversaryDay, entry->statementDay, period, sizeof(period));
    const char* key = rule->capKey[0] ? rule->capKey : rule->key;
    WalletTrack* track = periodKnown ? findWalletTrack(entry, key, period, 0) : NULL;
    int activation = !rule->activation ? 1 : track ? track->activated : -1;
    if (!strcmp(rule->category, "selected") && !entry->chosenCategory[0]) activation = -1;
    if (activation == 0) { *eligible = false; return true; }
    bool capKnown = !rule->capKey[0] || (track && track->known);
    int64_t remaining = rule->capCents;
    if (rule->capKey[0] && capKnown) remaining = track->amount >= rule->capCents ? 0 : rule->capCents - track->amount;
    int64_t bonusSpend = rule->capKey[0] && remaining < purchase->amount ? remaining : purchase->amount;
    int64_t part, rest;
    if (!multiply(bonusSpend, rule->rate, &part)
        || !multiply(purchase->amount - bonusSpend, rule->fallbackRate, &rest)
        || !add(part, rest, possible)) return false;
    if (activation == 1 && capKnown && periodKnown) *confirmed = *possible;
    else if (activation == 1 && rule->capKey[0]) {
        if (!multiply(purchase->amount, rule->fallbackRate, confirmed)) return false;
    }
    return true;
}

bool estimateReward(Catalog* catalog, WalletEntry* entry, const Purchase* purchase, RewardEstimate* result) {
    memset(result, 0, sizeof(*result));
    if (purchase->amount < 0 || purchase->amount > INT64_C(100000000000) || !validDate(purchase->date)) return false;
    Purchase checked=*purchase; char channel[80];
    if (!normalizeSpendingCategory(purchase->category,checked.category)||!normalizedLabel(purchase->channel,channel)) return false;
    if (!strcmp(channel,"issuerportal")) copyText(channel,sizeof(channel),"portal");
    if (strcmp(channel,"direct")&&strcmp(channel,"portal")&&strcmp(channel,"online")) return false;
    copyText(checked.channel,sizeof(checked.channel),channel); purchase=&checked;
    CatalogCard card;
    if (!entry->cardKey[0] || !findCatalogCard(catalog, entry->cardKey, &card)) {
        copyText(result->explanation, sizeof(result->explanation), "Unlisted or unresolved card: no supported reward rules."); return true;
    }
    result->unit = card.unit;
    RewardRule* rules = calloc(CATALOG_RULE_LIMIT, sizeof(*rules)); if (!rules) return false;
    int count = catalogRules(catalog, card.key, rules, CATALOG_RULE_LIMIT);
    bool okay = count >= 0;
    char chosenNotes[1200] = "";
    for (int i = 0; okay && i < count; i++) {
        if (!matches(&rules[i], entry, purchase)) continue;
        int64_t confirmed, possible; bool eligible;
        okay = ruleValue(&rules[i], entry, purchase, &confirmed, &possible, &eligible);
        if (!okay || !eligible) continue;
        if (!result->available || confirmed > result->numerator) {
            result->numerator = confirmed;
            copyText(result->ruleKey, sizeof(result->ruleKey), rules[i].key);
            copyText(chosenNotes, sizeof(chosenNotes), rules[i].notes);
        }
        if (possible > result->upperNumerator) result->upperNumerator = possible;
        result->available = true;
    }
    free(rules);
    if (!okay) return false;
    if (!result->available) {
        copyText(result->explanation, sizeof(result->explanation), "No supported rule matches this category, channel, and date."); return true;
    }
    result->conditional = result->upperNumerator > result->numerator;
    result->cashKnown = cashValue(result->numerator, card.unit, entry->valuation, &result->cashCents)
        && cashValue(result->upperNumerator, card.unit, entry->valuation, &result->upperCashCents);
    result->rankable = card.checked && result->cashKnown && !result->conditional;
    snprintf(result->explanation, sizeof(result->explanation), "%s%s%s", chosenNotes,
        result->conditional ? " Conditions or remaining cap use are not confirmed; the range is conditional." : "",
        card.checked ? "" : " This record is incomplete and is excluded from a definite ranking.");
    return true;
}

bool formatReward(const RewardEstimate* result, char* text, size_t size) {
    text[0] = '\0';
    if (!result->available) return appendText(text, size, "%s", result->explanation);
    if (result->unit == UNIT_CASHBACK) {
        char low[60], high[60]; formatAmount(result->cashCents, low, sizeof(low));
        formatAmount(result->upperCashCents, high, sizeof(high));
        if (!appendText(text, size, "%s%s", low, result->conditional ? " to " : "")) return false;
        if (result->conditional && !appendText(text, size, "%s (conditional)", high)) return false;
    } else {
        int64_t units = rounded(result->numerator, 100000), high = rounded(result->upperNumerator, 100000);
        if (!appendText(text, size, "%" PRId64 ".%03" PRId64 " %s", units / 1000, units % 1000, rewardUnitName(result->unit))) return false;
        if (result->conditional && !appendText(text, size, " to %" PRId64 ".%03" PRId64 " (conditional)", high / 1000, high % 1000)) return false;
        if (result->cashKnown) {
            char value[60]; formatAmount(result->cashCents, value, sizeof(value));
            if (!appendText(text, size, "; %s at your entered redemption value", value)) return false;
        } else if (!appendText(text, size, "; cash value is unknown until you enter a valuation")) return false;
    }
    return appendText(text, size, "\n%s", result->explanation);
}

bool compareWallet(Catalog* catalog, CatalogWallet* wallet, const Purchase* purchase, char* text, size_t size) {
    text[0] = '\0'; int64_t best = -1; int ties = 0; bool unknown = false;
    for (int i = 0; i < wallet->count; i++) {
        RewardEstimate result; char output[2600];
        if (!estimateReward(catalog, &wallet->entries[i], purchase, &result) || !formatReward(&result, output, sizeof(output))) return false;
        if (!appendText(text, size, "%s\n%s\n\n", wallet->entries[i].name, output)) return false;
        if (result.rankable) {
            if (result.cashCents > best) { best = result.cashCents; ties = 1; }
            else if (result.cashCents == best) ties++;
        } else unknown = true;
    }
    if (best >= 0) {
        char amount[60]; formatAmount(best, amount, sizeof(amount));
        if (!appendText(text, size, "%s among confirmed supported scenarios: %s\n", ties > 1 ? "Tie" : "Highest estimate", amount)) return false;
        for (int i = 0; i < wallet->count; i++) {
            RewardEstimate result;
            if (!estimateReward(catalog, &wallet->entries[i], purchase, &result)) return false;
            if (result.rankable && result.cashCents == best && !appendText(text, size, "  %s\n", wallet->entries[i].name)) return false;
        }
    }
    if (unknown && !appendText(text, size, "Some cards cannot be ranked definitely. Their missing conditions or valuations could change the result.\n")) return false;
    return appendText(text, size, "Estimates use the selected category/channel and stored source snapshot. Actual merchant coding and account terms can differ. Ties compare estimated cash amounts rounded to cents. Annual fees are considered separately in the yearly view.\n");
}

bool estimateYear(Catalog* catalog, const WalletEntry* original, const int64_t* monthly,
    int year, bool firstYear, bool activation, int64_t credits, int64_t bonus,
    int64_t interest, YearEstimate* result) {
    memset(result, 0, sizeof(*result));
    if (year < 1900 || year > 9999 || credits < 0 || bonus < 0 || interest < 0) return false;
    CatalogCard card; if (!findCatalogCard(catalog, original->cardKey, &card)) return false;
    WalletEntry* entry = malloc(sizeof(*entry));
    RewardRule* rules = calloc(CATALOG_RULE_LIMIT, sizeof(*rules));
    if (!entry || !rules) { free(entry); free(rules); return false; }
    *entry = *original; entry->trackCount = 0;
    int count = catalogRules(catalog, card.key, rules, CATALOG_RULE_LIMIT);
    bool okay = count >= 0; result->complete = card.checked != 0;
    for (int month = 1; okay && month <= 12; month++) for (int c = 0; okay && c < spendingCategoryCount; c++) {
        if (monthly[c] < 0 || monthly[c] > INT64_C(100000000000)) { okay = false; break; }
        if (!monthly[c]) continue;
        Purchase purchase = { 0 }; purchase.amount = monthly[c];
        copyText(purchase.category, sizeof(purchase.category), spendingCategories[c]);
        copyText(purchase.channel, sizeof(purchase.channel), "direct");
        snprintf(purchase.date, sizeof(purchase.date), "%04d-%02d-15", year, month);
        /* A fresh-year model starts each cap at zero. It is not a report of
           transactions or existing bank balances. */
        for (int i = 0; okay && i < count; i++) if ((rules[i].capKey[0] || rules[i].activation) && matches(&rules[i], entry, &purchase)) {
            char period[24];
            if (!makePeriodKey(rules[i].period, purchase.date, entry->anniversaryMonth,
                entry->anniversaryDay, entry->statementDay, period, sizeof(period))) continue;
            const char* key = rules[i].capKey[0] ? rules[i].capKey : rules[i].key;
            if (!findWalletTrack(entry, key, period, 0)) {
                if (entry->trackCount == WALLET_TRACK_LIMIT) { okay = false; break; }
                WalletTrack* t = &entry->tracks[entry->trackCount++]; memset(t, 0, sizeof(*t));
                copyText(t->ruleKey, sizeof(t->ruleKey), key); copyText(t->periodKey, sizeof(t->periodKey), period);
                t->known = true; t->activated = activation ? 1 : -1;
            }
        }
        RewardEstimate estimate;
        if (okay) okay = estimateReward(catalog, entry, &purchase, &estimate);
        if (!okay) break;
        if (!estimate.available || !estimate.cashKnown || estimate.conditional) { result->complete = false; result->conditionalPurchases++; }
        if (estimate.cashKnown && !add(result->rewards, estimate.cashCents, &result->rewards)) okay = false;
        char counted[CATALOG_RULE_LIMIT][CARD_KEY_SIZE]; int countedCount = 0;
        for (int i = 0; okay && i < count; i++) if (rules[i].capKey[0] && matches(&rules[i], entry, &purchase)) {
            bool seen = false;
            for (int j = 0; j < countedCount; j++) if (!strcmp(counted[j], rules[i].capKey)) seen = true;
            if (seen) continue;
            copyText(counted[countedCount++], CARD_KEY_SIZE, rules[i].capKey);
            char period[24];
            if (!makePeriodKey(rules[i].period, purchase.date, entry->anniversaryMonth,
                entry->anniversaryDay, entry->statementDay, period, sizeof(period))) continue;
            WalletTrack* t = findWalletTrack(entry, rules[i].capKey, period, 0);
            if (t && !add(t->amount, purchase.amount, &t->amount)) okay = false;
        }
    }
    result->fee = firstYear ? card.firstFee : card.ongoingFee;
    if (result->fee < 0) result->complete = false;
    result->credits = credits; result->bonus = bonus; result->interest = interest;
    int64_t positive, costs;
    okay = okay && add(result->rewards, credits, &positive) && add(positive, bonus, &positive)
        && add(result->fee < 0 ? 0 : result->fee, interest, &costs);
    if (okay) result->net = positive - costs;
    free(entry); free(rules); return okay;
}
