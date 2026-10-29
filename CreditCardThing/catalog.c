/* Catalog access belongs to Eden's storage area; Joe supplies the records.
   I copy row text before advancing a query, because SQLite owns its returned
   pointers. Search results keep permanent keys even when their order changes. */
#include "catalog.h"
#include "catalogText.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool databaseError(Catalog* catalog) {
    snprintf(catalog->error, sizeof(catalog->error), "%s", sqlite3_errmsg(catalog->db));
    return false;
}

static int scalar(Catalog* catalog, const char* sql) {
    sqlite3_stmt* statement = NULL;
    int value = -1;
    if (sqlite3_prepare_v2(catalog->db, sql, -1, &statement, NULL) == SQLITE_OK
        && sqlite3_step(statement) == SQLITE_ROW) value = sqlite3_column_int(statement, 0);
    else databaseError(catalog);
    sqlite3_finalize(statement);
    return value;
}

static bool textColumn(sqlite3_stmt* statement, int column, char* text, size_t size) {
    const unsigned char* value = sqlite3_column_text(statement, column);
    if (value && (size_t)sqlite3_column_bytes(statement, column) != strlen((const char*)value)) return false;
    return copyText(text, size, value ? (const char*)value : "");
}

static int numberColumn(sqlite3_stmt* statement, int column) {
    return sqlite3_column_type(statement, column) == SQLITE_NULL ? -1 : sqlite3_column_int(statement, column);
}

static bool readCard(sqlite3_stmt* statement, CatalogCard* card) {
    memset(card, 0, sizeof(*card));
    if (!textColumn(statement, 0, card->key, sizeof(card->key))
        || !textColumn(statement, 1, card->issuer, sizeof(card->issuer))
        || !textColumn(statement, 2, card->name, sizeof(card->name))
        || !textColumn(statement, 3, card->network, sizeof(card->network))
        || !textColumn(statement, 4, card->country, sizeof(card->country))
        || !textColumn(statement, 5, card->currency, sizeof(card->currency))
        || !textColumn(statement, 12, card->checkedOn, sizeof(card->checkedOn))
        || !textColumn(statement, 15, card->aliases, sizeof(card->aliases))
        || !textColumn(statement, 16, card->summary, sizeof(card->summary))
        || !textColumn(statement, 17, card->source, sizeof(card->source))) return false;
    card->unit = numberColumn(statement, 6);
    card->firstFee = numberColumn(statement, 7); card->ongoingFee = numberColumn(statement, 8);
    card->aprMin = numberColumn(statement, 9); card->aprMax = numberColumn(statement, 10);
    card->foreignFee = numberColumn(statement, 11);
    card->checked = numberColumn(statement, 13); card->archived = numberColumn(statement, 14);
    return validDate(card->checkedOn);
}

void initCatalogFilter(CatalogFilter* filter) {
    memset(filter, 0, sizeof(*filter));
    filter->maxFee = -1; filter->unit = -1;
}

void closeCatalog(Catalog* catalog) {
    if (catalog->db) sqlite3_close(catalog->db);
    catalog->db = NULL;
}

bool validateCatalog(Catalog* catalog) {
    if (scalar(catalog, "PRAGMA application_id") != 1128481073
        || scalar(catalog, "PRAGMA user_version") != CATALOG_SCHEMA) {
        copyText(catalog->error, sizeof(catalog->error), "This is not a supported CardClarity catalog."); return false;
    }
    sqlite3_stmt* check = NULL;
    bool okay = sqlite3_prepare_v2(catalog->db, "PRAGMA quick_check", -1, &check, NULL) == SQLITE_OK
        && sqlite3_step(check) == SQLITE_ROW
        && !strcmp((const char*)sqlite3_column_text(check, 0), "ok");
    sqlite3_finalize(check); check = NULL;
    if (okay) okay = sqlite3_prepare_v2(catalog->db, "PRAGMA foreign_key_check", -1, &check, NULL) == SQLITE_OK
        && sqlite3_step(check) == SQLITE_DONE;
    sqlite3_finalize(check);
    if (!okay) { copyText(catalog->error, sizeof(catalog->error), "The catalog is damaged or has broken references."); return false; }
    catalog->cardCount = scalar(catalog, "SELECT count(*) FROM cards");
    catalog->checkedCount = scalar(catalog, "SELECT count(*) FROM cards WHERE checked=1");
    if (catalog->cardCount < 1 || catalog->cardCount > 100000
        || scalar(catalog, "SELECT count(*) FROM cards WHERE firstFee>1000000000 OR ongoingFee>1000000000 OR aprMin>100000 OR aprMax>100000 OR foreignFee>100000") != 0
        || scalar(catalog, "SELECT count(*) FROM cardSearch") != catalog->cardCount
        || scalar(catalog, "SELECT count(*) FROM cards c WHERE NOT EXISTS(SELECT 1 FROM cardSearch s WHERE s.key=c.key AND s.name=c.name AND s.issuer=c.issuer AND s.aliases=c.aliases)") != 0
        || scalar(catalog, "SELECT count(*) FROM rules r JOIN caps p ON p.key=r.capKey WHERE r.cardKey<>p.cardKey") != 0
        || scalar(catalog, "SELECT count(*) FROM (SELECT cardKey FROM rules GROUP BY cardKey HAVING count(*)>40)") != 0) {
        copyText(catalog->error, sizeof(catalog->error), "The catalog contains inconsistent records or too many rules."); return false;
    }
    sqlite3_stmt* rows = NULL;
    if (sqlite3_prepare_v2(catalog->db, "SELECT * FROM cards", -1, &rows, NULL) != SQLITE_OK) return databaseError(catalog);
    CatalogCard card;
    int result;
    while ((result = sqlite3_step(rows)) == SQLITE_ROW) {
        if (!readCard(rows, &card)) { okay = false; break; }
        RewardRule* rules = calloc(CATALOG_RULE_LIMIT, sizeof(*rules));
        BenefitRule* benefits = calloc(CATALOG_RULE_LIMIT, sizeof(*benefits));
        if (!rules || !benefits || catalogRules(catalog, card.key, rules, CATALOG_RULE_LIMIT) < 0
            || catalogBenefits(catalog, card.key, benefits, CATALOG_RULE_LIMIT) < 0) okay = false;
        free(rules); free(benefits);
        if (!okay) break;
    }
    if (result != SQLITE_DONE) okay = false;
    sqlite3_finalize(rows);
    if (!okay) { copyText(catalog->error, sizeof(catalog->error), "A card has invalid text or dates."); return false; }
    if (sqlite3_prepare_v2(catalog->db, "SELECT value FROM meta WHERE key='release'", -1, &rows, NULL) != SQLITE_OK) return databaseError(catalog);
    okay = sqlite3_step(rows) == SQLITE_ROW && textColumn(rows, 0, catalog->release, sizeof(catalog->release));
    sqlite3_finalize(rows);
    return okay;
}

bool openCatalog(Catalog* catalog, const char* path) {
    memset(catalog, 0, sizeof(*catalog));
    if (!copyText(catalog->path, sizeof(catalog->path), path)) {
        copyText(catalog->error, sizeof(catalog->error), "The catalog path is too long."); return false;
    }
    if (sqlite3_open_v2(path, &catalog->db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        databaseError(catalog); closeCatalog(catalog); return false;
    }
    sqlite3_db_config(catalog->db, SQLITE_DBCONFIG_DEFENSIVE, 1, NULL);
    sqlite3_db_config(catalog->db, SQLITE_DBCONFIG_TRUSTED_SCHEMA, 0, NULL);
    sqlite3_limit(catalog->db, SQLITE_LIMIT_LENGTH, 2000000);
    sqlite3_busy_timeout(catalog->db, 1000);
    if (!validateCatalog(catalog)) { closeCatalog(catalog); return false; }
    return true;
}

bool findCatalogCard(Catalog* catalog, const char* key, CatalogCard* card) {
    sqlite3_stmt* statement = NULL;
    if (sqlite3_prepare_v2(catalog->db, "SELECT * FROM cards WHERE key=?", -1, &statement, NULL) != SQLITE_OK) return databaseError(catalog);
    sqlite3_bind_text(statement, 1, key, -1, SQLITE_TRANSIENT);
    bool okay = sqlite3_step(statement) == SQLITE_ROW && readCard(statement, card);
    sqlite3_finalize(statement);
    return okay;
}

static void bindText(sqlite3_stmt* statement, const char* name, const char* text) {
    int index = sqlite3_bind_parameter_index(statement, name);
    if (index) sqlite3_bind_text(statement, index, text, -1, SQLITE_TRANSIENT);
}

static void bindInt(sqlite3_stmt* statement, const char* name, int value) {
    int index = sqlite3_bind_parameter_index(statement, name);
    if (index) sqlite3_bind_int(statement, index, value);
}

static bool searchExpression(const char* query, char* expression, size_t size) {
    expression[0] = '\0';
    while (*query) {
        while (*query && (unsigned char)*query < 128 && !isalnum((unsigned char)*query)) query++;
        if (!*query) break;
        if (expression[0] && !appendText(expression, size, " AND ")) return false;
        if (!appendText(expression, size, "\"")) return false;
        while (*query && ((unsigned char)*query >= 128 || isalnum((unsigned char)*query))) {
            if (!appendText(expression, size, "%c", *query++)) return false;
        }
        if (!appendText(expression, size, "\"*")) return false;
    }
    return true;
}

static bool whereClause(const CatalogFilter* filter, bool searching, char* where, size_t size) {
    copyText(where, size, " WHERE (:archived OR c.archived=0) AND (:checked=0 OR c.checked=1)"
        " AND (:fee<0 OR c.ongoingFee<=:fee) AND (:unit<0 OR c.unit=:unit)"
        " AND (:country='' OR c.country=:country)"
        " AND (:network='' OR c.network=:network)"
        " AND (:category='' OR EXISTS(SELECT 1 FROM rules r WHERE r.cardKey=c.key AND r.category IN(:category,'selected')))");
    if (searching && !appendText(where, size, " AND c.key IN(SELECT key FROM cardSearch WHERE cardSearch MATCH :query)")) return false;
    if (filter->issuerCount) {
        if (!appendText(where, size, " AND c.issuer IN(")) return false;
        for (int i = 0; i < filter->issuerCount; i++) if (!appendText(where, size, "%s:issuer%d", i ? "," : "", i)) return false;
        if (!appendText(where, size, ")")) return false;
    }
    return true;
}

static void bindFilters(sqlite3_stmt* statement, const CatalogFilter* filter, const char* expression) {
    bindText(statement, ":query", expression); bindText(statement, ":exact", filter->query);
    bindText(statement, ":country", filter->country); bindText(statement, ":category", filter->category);
    bindText(statement, ":network", filter->network);
    bindInt(statement, ":archived", filter->includeArchived); bindInt(statement, ":checked", filter->checkedOnly);
    bindInt(statement, ":fee", filter->maxFee); bindInt(statement, ":unit", filter->unit);
    for (int i = 0; i < filter->issuerCount; i++) {
        char name[30]; snprintf(name, sizeof(name), ":issuer%d", i);
        bindText(statement, name, filter->issuers[i]);
    }
}

/* A small bounded edit-distance pass supplies suggestions, never substitutions. */
static int nameDistance(const char* first, const char* second) {
    char a[180], b[180]; int an = 0, bn = 0;
    for (; *first && an < 179; first++) if (isalnum((unsigned char)*first)) a[an++] = (char)tolower((unsigned char)*first);
    for (; *second && bn < 179; second++) if (isalnum((unsigned char)*second)) b[bn++] = (char)tolower((unsigned char)*second);
    if (an < 4 || abs(an - bn) > 2) return 3;
    int previous[180], current[180];
    for (int j = 0; j <= bn; j++) previous[j] = j;
    for (int i = 1; i <= an; i++) {
        current[0] = i;
        for (int j = 1; j <= bn; j++) {
            int insert = current[j - 1] + 1, remove = previous[j] + 1;
            int replace = previous[j - 1] + (a[i - 1] != b[j - 1]);
            current[j] = insert < remove ? insert : remove;
            if (replace < current[j]) current[j] = replace;
        }
        memcpy(previous, current, sizeof(int) * (size_t)(bn + 1));
    }
    return previous[bn];
}

static bool suggestCards(Catalog* catalog, const CatalogFilter* filter, CatalogPage* page) {
    char where[4000], sql[5000];
    if (!whereClause(filter, false, where, sizeof(where))) return false;
    snprintf(sql, sizeof(sql), "SELECT c.* FROM cards c%s ORDER BY c.name COLLATE NOCASE LIMIT 5000", where);
    sqlite3_stmt* statement = NULL;
    if (sqlite3_prepare_v2(catalog->db, sql, -1, &statement, NULL) != SQLITE_OK) return databaseError(catalog);
    bindFilters(statement, filter, "");
    int result;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        CatalogCard card;
        if (!readCard(statement, &card)) { result = SQLITE_ERROR; break; }
        int distance = nameDistance(filter->query, card.name);
        char aliases[400]; copyText(aliases, sizeof(aliases), card.aliases);
        char* start = aliases;
        for (char* p = aliases;; p++) {
            if (*p == '|' || !*p) {
                bool last = !*p; *p = '\0';
                int other = nameDistance(filter->query, start);
                if (other < distance) distance = other;
                if (last) break;
                start = p + 1;
            }
        }
        if (distance <= 2 && page->count < CATALOG_PAGE_SIZE) page->cards[page->count++] = card;
    }
    sqlite3_finalize(statement);
    page->total = page->count; page->suggestions = page->count > 0;
    return result == SQLITE_DONE;
}

bool searchCatalog(Catalog* catalog, const CatalogFilter* filter, int offset, CatalogPage* page) {
    memset(page, 0, sizeof(*page));
    if (offset < 0 || filter->issuerCount < 0 || filter->issuerCount > 16
        || filter->unit < -1 || filter->unit > 3 || filter->maxFee < -1) return false;
    char expression[2200], where[4000], sql[6000];
    if (!searchExpression(filter->query, expression, sizeof(expression))
        || !whereClause(filter, expression[0] != '\0', where, sizeof(where))) return false;
    if (!expression[0]) {
        const char* text = filter->query;
        while (isspace((unsigned char)*text)) text++;
        if (*text) return true; /* Punctuation alone is not a request for every card. */
    }
    snprintf(sql, sizeof(sql), "SELECT count(*) FROM cards c%s", where);
    sqlite3_stmt* statement = NULL;
    if (sqlite3_prepare_v2(catalog->db, sql, -1, &statement, NULL) != SQLITE_OK) return databaseError(catalog);
    bindFilters(statement, filter, expression);
    bool okay = sqlite3_step(statement) == SQLITE_ROW;
    if (okay) page->total = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    if (!okay) return databaseError(catalog);
    if (!page->total && filter->query[0] && !offset) return suggestCards(catalog, filter, page);
    const char* order = filter->sort == SORT_FEE ? "c.ongoingFee IS NULL,c.ongoingFee,c.name COLLATE NOCASE,c.key"
        : filter->sort == SORT_NAME ? "c.name COLLATE NOCASE,c.key"
        : "CASE WHEN lower(c.name)=lower(:exact) THEN 0 ELSE 1 END,c.name COLLATE NOCASE,c.key";
    snprintf(sql, sizeof(sql), "SELECT c.* FROM cards c%s ORDER BY %s LIMIT 20 OFFSET :offset", where, order);
    if (sqlite3_prepare_v2(catalog->db, sql, -1, &statement, NULL) != SQLITE_OK) return databaseError(catalog);
    bindFilters(statement, filter, expression); bindInt(statement, ":offset", offset);
    int result;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        if (page->count == CATALOG_PAGE_SIZE || !readCard(statement, &page->cards[page->count])) { okay = false; break; }
        page->count++;
    }
    sqlite3_finalize(statement); page->offset = offset;
    return okay && result == SQLITE_DONE;
}

int catalogIssuers(Catalog* catalog, char names[][80], int capacity) {
    sqlite3_stmt* statement = NULL;
    if (sqlite3_prepare_v2(catalog->db, "SELECT DISTINCT issuer FROM cards ORDER BY issuer COLLATE NOCASE", -1, &statement, NULL) != SQLITE_OK) return -1;
    int count = 0, result;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        if (count == capacity || !textColumn(statement, 0, names[count], 80)) { count = -1; break; }
        count++;
    }
    sqlite3_finalize(statement);
    return result == SQLITE_DONE ? count : -1;
}

int catalogRules(Catalog* catalog, const char* key, RewardRule* rules, int capacity) {
    const char* sql = "SELECT r.key,r.cardKey,r.category,r.channel,r.rate,coalesce(r.capKey,''),"
        "coalesce(p.limitCents,0),coalesce(p.period,0),r.activation,r.fallbackRate,r.start,r.end,r.notes,r.source"
        " FROM rules r LEFT JOIN caps p ON r.capKey=p.key WHERE r.cardKey=? ORDER BY r.key";
    sqlite3_stmt* statement = NULL;
    if (sqlite3_prepare_v2(catalog->db, sql, -1, &statement, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(statement, 1, key, -1, SQLITE_TRANSIENT);
    int count = 0, result;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        if (count == capacity) { count = -1; break; }
        RewardRule* rule = &rules[count]; memset(rule, 0, sizeof(*rule));
        if (!textColumn(statement, 0, rule->key, sizeof(rule->key))
            || !textColumn(statement, 1, rule->cardKey, sizeof(rule->cardKey))
            || !textColumn(statement, 2, rule->category, sizeof(rule->category))
            || !textColumn(statement, 3, rule->channel, sizeof(rule->channel))
            || !textColumn(statement, 5, rule->capKey, sizeof(rule->capKey))
            || !textColumn(statement, 10, rule->start, sizeof(rule->start))
            || !textColumn(statement, 11, rule->end, sizeof(rule->end))
            || !textColumn(statement, 12, rule->notes, sizeof(rule->notes))
            || !textColumn(statement, 13, rule->source, sizeof(rule->source))) { count = -1; break; }
        rule->rate = sqlite3_column_int64(statement, 4); rule->capCents = sqlite3_column_int64(statement, 6);
        rule->period = sqlite3_column_int(statement, 7); rule->activation = sqlite3_column_int(statement, 8);
        rule->fallbackRate = sqlite3_column_int64(statement, 9);
        if ((rule->start[0] && !validDate(rule->start)) || (rule->end[0] && !validDate(rule->end))) { count = -1; break; }
        if (rule->start[0] && rule->end[0] && strcmp(rule->start, rule->end) > 0) { count = -1; break; }
        count++;
    }
    sqlite3_finalize(statement);
    return result == SQLITE_DONE ? count : -1;
}

int catalogBenefits(Catalog* catalog, const char* key, BenefitRule* rules, int capacity) {
    sqlite3_stmt* statement = NULL;
    if (sqlite3_prepare_v2(catalog->db, "SELECT * FROM benefits WHERE cardKey=? ORDER BY key", -1, &statement, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(statement, 1, key, -1, SQLITE_TRANSIENT);
    int count = 0, result;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        if (count == capacity) { count = -1; break; }
        BenefitRule* rule = &rules[count]; memset(rule, 0, sizeof(*rule));
        if (!textColumn(statement, 0, rule->key, sizeof(rule->key))
            || !textColumn(statement, 1, rule->cardKey, sizeof(rule->cardKey))
            || !textColumn(statement, 2, rule->label, sizeof(rule->label))
            || !textColumn(statement, 4, rule->period, sizeof(rule->period))
            || !textColumn(statement, 6, rule->due, sizeof(rule->due))
            || !textColumn(statement, 7, rule->notes, sizeof(rule->notes))
            || !textColumn(statement, 8, rule->source, sizeof(rule->source))) { count = -1; break; }
        rule->creditCents = sqlite3_column_type(statement, 3) == SQLITE_NULL ? -1 : sqlite3_column_int64(statement, 3);
        rule->activation = sqlite3_column_int(statement, 5);
        if (rule->due[0] && !validDate(rule->due)) { count = -1; break; }
        count++;
    }
    sqlite3_finalize(statement);
    return result == SQLITE_DONE ? count : -1;
}

const char* rewardUnitName(int unit) {
    static const char* names[] = { "cashback", "points", "miles", "no reward rate recorded" };
    return unit >= 0 && unit <= 3 ? names[unit] : "unknown";
}

const char* capPeriodName(int period) {
    static const char* names[] = { "no recorded cap", "calendar month", "calendar quarter", "calendar year", "card anniversary year", "statement cycle" };
    return period >= 0 && period <= 5 ? names[period] : "unknown period";
}

static bool describeFee(char* text, size_t size, const char* label, int cents) {
    char amount[60]; formatAmount(cents, amount, sizeof(amount));
    return appendText(text, size, "%s: %s\n", label, cents < 0 ? "not confirmed" : amount);
}

bool describeCatalogCard(Catalog* catalog, const char* key, char* text, size_t size) {
    CatalogCard card;
    if (!findCatalogCard(catalog, key, &card)) return false;
    text[0] = '\0';
    bool okay = appendText(text, size, "%s\n%s | %s | %s / %s\n%s\n\n", card.name,
        card.issuer, card.network[0] ? card.network : "network not confirmed", card.country, card.currency, card.archived ? "Archived record; legacy samples are not bank offers" : "Catalog product snapshot");
    okay = okay && appendText(text,size,"Catalog key: %s\n",card.key);
    okay = okay && describeFee(text, size, "First-year annual fee", card.firstFee)
        && describeFee(text, size, "Ongoing annual fee", card.ongoingFee);
    if (card.aprMin >= 0 && card.aprMax >= 0) okay = okay && appendText(text, size,
        "Published purchase APR range: %.2f%% to %.2f%%; individual terms may differ.\n", card.aprMin / 100.0, card.aprMax / 100.0);
    else okay = okay && appendText(text, size, "Purchase APR: not confirmed; check the pricing disclosure.\n");
    if (card.foreignFee >= 0) okay = okay && appendText(text, size, "Foreign transaction fee: %.2f%%\n", card.foreignFee / 100.0);
    else okay = okay && appendText(text, size, "Foreign transaction fee: not confirmed.\n");
    okay = okay && appendText(text, size, "\n%s\n\nReward unit: %s\n", card.summary, rewardUnitName(card.unit));
    RewardRule* rules = calloc(CATALOG_RULE_LIMIT, sizeof(*rules));
    if (!rules) return false;
    int count = catalogRules(catalog, key, rules, CATALOG_RULE_LIMIT);
    if (count < 0) okay = false;
    if (count == 0) okay = okay && appendText(text, size, "Reward rules are not yet supported for comparisons.\n");
    for (int i = 0; okay && i < count; i++) {
        RewardRule* r = &rules[i];
        okay = appendText(text, size, "\n%s / %s: %.4g %s\n%s\n", r->category, r->channel,
            r->rate / (card.unit == UNIT_CASHBACK ? 10000.0 : 1000000.0),
            card.unit == UNIT_CASHBACK ? "% cashback" : "units per dollar", r->notes);
        if (r->capKey[0]) {
            char amount[60]; formatAmount(r->capCents, amount, sizeof(amount));
            okay = okay && appendText(text, size, "Shared cap: %s eligible spending per %s. Fallback rate: %.4g.\n",
                amount, capPeriodName(r->period), r->fallbackRate / (card.unit == UNIT_CASHBACK ? 10000.0 : 1000000.0));
        }
        if (r->activation) okay = okay && appendText(text, size, "Activation or a confirmed personal condition is required.\n");
        if (r->start[0] || r->end[0]) okay = okay && appendText(text, size, "Applicable dates: %s to %s\n", r->start[0] ? r->start : "not stated", r->end[0] ? r->end : "not stated");
        okay = okay && appendText(text, size, "Source: %s\n", r->source);
    }
    free(rules);
    sqlite3_stmt* fees = NULL;
    if (sqlite3_prepare_v2(catalog->db, "SELECT kind,notes,source FROM fees WHERE cardKey=?", -1, &fees, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(fees, 1, key, -1, SQLITE_TRANSIENT);
    int result;
    while (okay && (result = sqlite3_step(fees)) == SQLITE_ROW) okay = appendText(text, size, "\n%s: %s\nSource: %s\n",
        sqlite3_column_text(fees, 0), sqlite3_column_text(fees, 1), sqlite3_column_text(fees, 2));
    if (okay && result != SQLITE_DONE) okay = false;
    sqlite3_finalize(fees);
    BenefitRule* benefits = calloc(40, sizeof(*benefits));
    if (!benefits) return false;
    count = catalogBenefits(catalog, key, benefits, 40);
    if (count < 0) okay = false;
    for (int i = 0; okay && i < count; i++) okay = appendText(text, size, "\nBenefit: %s\n%s\nSource: %s\n", benefits[i].label, benefits[i].notes, benefits[i].source);
    free(benefits);
    return okay && appendText(text, size, "\nChecked on %s. %s\nSource: %s\n"
        "Published snapshots do not confirm your personal account's terms. Unknown values are not zero.\n",
        card.checkedOn, card.checked ? "Supported key fields were checked; see conditions." : "Incomplete record; some details still need review.", card.source);
}
