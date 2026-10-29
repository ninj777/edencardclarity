/* Catalog records are shared by the menu and window. Database row positions
   are never saved as card identities. A missing value stays -1 in C. */
#ifndef CATALOG_H
#define CATALOG_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../thirdParty/sqlite3.h"

#define CARD_KEY_SIZE 64
#define CARD_NAME_SIZE 160
#define CATALOG_PAGE_SIZE 20
#define CATALOG_RULE_LIMIT 40
#define CATALOG_TEXT_SIZE 131072
#define CATALOG_SCHEMA 1

enum RewardUnit { UNIT_CASHBACK, UNIT_POINTS, UNIT_MILES, UNIT_NONE };
enum CapPeriod { PERIOD_NONE, PERIOD_MONTH, PERIOD_QUARTER, PERIOD_YEAR,
    PERIOD_ANNIVERSARY, PERIOD_STATEMENT };
enum CatalogSort { SORT_RELEVANCE, SORT_NAME, SORT_FEE };

typedef struct {
    char key[CARD_KEY_SIZE], name[CARD_NAME_SIZE], issuer[80], network[32];
    char country[3], currency[4], checkedOn[11], aliases[400];
    char summary[1200], source[600];
    int unit, firstFee, ongoingFee, aprMin, aprMax, foreignFee;
    int checked, archived;
} CatalogCard;

typedef struct {
    char key[CARD_KEY_SIZE], cardKey[CARD_KEY_SIZE], category[40], channel[40];
    char capKey[CARD_KEY_SIZE], start[11], end[11], notes[1200], source[600];
    int64_t rate, capCents, fallbackRate;
    int period, activation;
} RewardRule;

typedef struct {
    char key[CARD_KEY_SIZE], cardKey[CARD_KEY_SIZE], label[160], period[32];
    char due[11], notes[1200], source[600];
    int64_t creditCents;
    int activation;
} BenefitRule;

typedef struct {
    sqlite3* db;
    char path[1024], error[300], release[80];
    int cardCount, checkedCount;
} Catalog;

typedef struct {
    char query[160], issuers[16][80], category[40], country[3], network[32];
    int issuerCount, maxFee, unit, sort;
    bool includeArchived, checkedOnly;
} CatalogFilter;

typedef struct {
    CatalogCard cards[CATALOG_PAGE_SIZE];
    int count, total, offset;
    bool suggestions;
} CatalogPage;

void initCatalogFilter(CatalogFilter* filter);
bool openCatalog(Catalog* catalog, const char* path);
void closeCatalog(Catalog* catalog);
bool validateCatalog(Catalog* catalog);
bool findCatalogCard(Catalog* catalog, const char* key, CatalogCard* card);
bool searchCatalog(Catalog* catalog, const CatalogFilter* filter, int offset, CatalogPage* page);
int catalogIssuers(Catalog* catalog, char names[][80], int capacity);
int catalogRules(Catalog* catalog, const char* key, RewardRule* rules, int capacity);
int catalogBenefits(Catalog* catalog, const char* key, BenefitRule* rules, int capacity);
bool describeCatalogCard(Catalog* catalog, const char* key, char* text, size_t capacity);
bool buildCatalog(const char* sourceFolder, const char* destination, char* error, size_t capacity);
bool copyDatabase(const char* source, const char* destination, char* error, size_t capacity);
bool updateCatalog(Catalog* catalog, const char* candidate, int* added, int* changed,
    int* missing, char* error, size_t capacity);
const char* rewardUnitName(int unit);
const char* capPeriodName(int period);
#endif
