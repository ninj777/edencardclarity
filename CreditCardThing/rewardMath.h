/* Oliver connects comparisons; Joe checks the expected values. All purchase
   money is in cents. Points/miles need an explicit redemption assumption. */
#ifndef REWARD_MATH_H
#define REWARD_MATH_H
#include "catalogWallet.h"
typedef struct {
    int64_t amount;
    char category[40], channel[40], date[11];
} Purchase;
typedef struct {
    bool available, conditional, cashKnown, rankable;
    int unit;
    int64_t numerator, upperNumerator, cashCents, upperCashCents;
    char ruleKey[CARD_KEY_SIZE], explanation[1600];
} RewardEstimate;
typedef struct {
    bool complete;
    int64_t rewards, fee, credits, bonus, interest, net;
    int conditionalPurchases;
} YearEstimate;
bool estimateReward(Catalog* catalog, WalletEntry* entry, const Purchase* purchase, RewardEstimate* result);
bool formatReward(const RewardEstimate* result, char* text, size_t capacity);
bool compareWallet(Catalog* catalog, CatalogWallet* wallet, const Purchase* purchase, char* text, size_t capacity);
bool estimateYear(Catalog* catalog, const WalletEntry* entry, const int64_t* monthly,
    int year, bool firstYear, bool assumeActivation, int64_t credits, int64_t bonus,
    int64_t interest, YearEstimate* result);
extern const char* const spendingCategories[];
extern const int spendingCategoryCount;
#endif
