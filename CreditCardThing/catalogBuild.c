/* Eden's assigned storage area. I build a separate candidate database and
   validate it before replacing a catalog. The tab-separated source records
   stay readable in Git; the application reads the resulting database. */
#include "catalog.h"
#include "catalogText.h"
#include "input.h"
#include <stdio.h>
#include <string.h>

static bool execute(sqlite3* db, const char* sql, char* error, size_t size) {
    if (sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK) return true;
    snprintf(error, size, "%s", sqlite3_errmsg(db));
    return false;
}

static const char* schema[] = {
    "CREATE TABLE meta(key TEXT PRIMARY KEY,value TEXT NOT NULL) STRICT;",
    "CREATE TABLE cards(key TEXT PRIMARY KEY CHECK(length(key) BETWEEN 1 AND 63),"
    "issuer TEXT NOT NULL CHECK(length(CAST(issuer AS BLOB))<80),"
    "name TEXT NOT NULL CHECK(length(CAST(name AS BLOB)) BETWEEN 1 AND 159),"
    "network TEXT NOT NULL CHECK(length(CAST(network AS BLOB))<32),"
    "country TEXT NOT NULL CHECK(length(country)=2),currency TEXT NOT NULL CHECK(length(currency)=3),"
    "unit INTEGER NOT NULL CHECK(unit BETWEEN 0 AND 3),"
    "firstFee INTEGER CHECK(firstFee>=0),ongoingFee INTEGER CHECK(ongoingFee>=0),"
    "aprMin INTEGER CHECK(aprMin>=0),aprMax INTEGER CHECK(aprMax>=aprMin),"
    "foreignFee INTEGER CHECK(foreignFee>=0),checkedOn TEXT NOT NULL CHECK(length(checkedOn)=10),"
    "checked INTEGER NOT NULL CHECK(checked IN(0,1)),archived INTEGER NOT NULL CHECK(archived IN(0,1)),"
    "aliases TEXT NOT NULL CHECK(length(CAST(aliases AS BLOB))<400),"
    "summary TEXT NOT NULL CHECK(length(CAST(summary AS BLOB))<1200),"
    "source TEXT NOT NULL CHECK(source LIKE 'https://%' AND length(source)<600)) STRICT;",
    "CREATE TABLE caps(key TEXT PRIMARY KEY CHECK(length(key) BETWEEN 1 AND 63),"
    "cardKey TEXT NOT NULL REFERENCES cards(key),limitCents INTEGER NOT NULL CHECK(limitCents>=0),"
    "period INTEGER NOT NULL CHECK(period BETWEEN 1 AND 5),notes TEXT NOT NULL,source TEXT NOT NULL) STRICT;",
    "CREATE TABLE rules(key TEXT PRIMARY KEY CHECK(length(key) BETWEEN 1 AND 63),"
    "cardKey TEXT NOT NULL REFERENCES cards(key),category TEXT NOT NULL CHECK(length(category)<40),"
    "channel TEXT NOT NULL CHECK(length(channel)<40),rate INTEGER NOT NULL CHECK(rate BETWEEN 0 AND 100000000),"
    "capKey TEXT REFERENCES caps(key),activation INTEGER NOT NULL CHECK(activation IN(0,1)),"
    "fallbackRate INTEGER NOT NULL CHECK(fallbackRate BETWEEN 0 AND 100000000),"
    "start TEXT NOT NULL CHECK(length(start) IN(0,10)),end TEXT NOT NULL CHECK(length(end) IN(0,10)),"
    "notes TEXT NOT NULL CHECK(length(CAST(notes AS BLOB))<1200),"
    "source TEXT NOT NULL CHECK(source LIKE 'https://%' AND length(source)<600)) STRICT;",
    "CREATE TABLE fees(cardKey TEXT NOT NULL REFERENCES cards(key),kind TEXT NOT NULL,"
    "notes TEXT NOT NULL CHECK(length(CAST(notes AS BLOB))<1600),source TEXT NOT NULL) STRICT;",
    "CREATE TABLE benefits(key TEXT PRIMARY KEY CHECK(length(key) BETWEEN 1 AND 63),"
    "cardKey TEXT NOT NULL REFERENCES cards(key),label TEXT NOT NULL CHECK(length(CAST(label AS BLOB))<160),"
    "creditCents INTEGER CHECK(creditCents>=0),period TEXT NOT NULL CHECK(length(period)<32),"
    "activation INTEGER NOT NULL CHECK(activation IN(0,1)),due TEXT NOT NULL CHECK(length(due) IN(0,10)),"
    "notes TEXT NOT NULL CHECK(length(CAST(notes AS BLOB))<1200),source TEXT NOT NULL) STRICT;",
    "CREATE INDEX ruleCard ON rules(cardKey); CREATE INDEX feeCard ON fees(cardKey);"
    "CREATE INDEX benefitCard ON benefits(cardKey); CREATE INDEX issuerIndex ON cards(issuer);",
    "CREATE VIRTUAL TABLE cardSearch USING fts5(key UNINDEXED,name,issuer,aliases,"
    "tokenize='unicode61 remove_diacritics 2',prefix='2 3 4');",
    NULL
};

/* Empty fields must survive splitting, so strtok is not suitable here. */
static int splitFields(char* text, char** fields, int capacity) {
    int count = 1;
    fields[0] = text;
    for (char* p = text; *p; p++) {
        if (*p == '\t') {
            *p = '\0';
            if (count == capacity) return -1;
            fields[count++] = p + 1;
        }
    }
    return count;
}

static bool importRows(sqlite3* db, const char* folder, const char* filename,
    const char* header, const char* insert, int columns, char* error, size_t size) {
    char path[1200];
    if (!joinCatalogPath(path, sizeof(path), folder, filename)) return false;
    FILE* file = fopen(path, "rb");
    if (!file) { snprintf(error, size, "Could not open %s.", filename); return false; }
    sqlite3_stmt* statement = NULL;
    bool okay = sqlite3_prepare_v2(db, insert, -1, &statement, NULL) == SQLITE_OK;
    char line[8192];
    int number = 0;
    while (okay) {
        enum InputResult read = readLine(file, line, sizeof(line));
        if (read == INPUT_END) break;
        number++;
        if (read != INPUT_OK) { okay = false; break; }
        if (number == 1) { if (strcmp(line, header)) okay = false; continue; }
        if (!line[0] || line[0] == '#') continue;
        char* fields[20];
        if (splitFields(line, fields, 20) != columns) { okay = false; break; }
        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
        for (int i = 0; i < columns; i++) {
            int result = !strcmp(fields[i], "-") ? sqlite3_bind_null(statement, i + 1)
                : sqlite3_bind_text(statement, i + 1, fields[i], -1, SQLITE_TRANSIENT);
            if (result != SQLITE_OK) okay = false;
        }
        if (okay && sqlite3_step(statement) != SQLITE_DONE) okay = false;
    }
    if (ferror(file) || number == 0) okay = false;
    if (fclose(file) != 0) okay = false;
    sqlite3_finalize(statement);
    if (!okay) snprintf(error, size, "%s, line %d: invalid record or %s", filename, number, sqlite3_errmsg(db));
    return okay;
}

bool buildCatalog(const char* folder, const char* destination, char* error, size_t size) {
    char temporary[1200];
    if (snprintf(temporary, sizeof(temporary), "%s.tmp", destination) >= (int)sizeof(temporary)) {
        snprintf(error, size, "The catalog path is too long."); return false;
    }
    FILE* owned = fopen(temporary, "wbx");
    if (!owned) { snprintf(error, size, "Could not create a fresh catalog temporary file."); return false; }
    if (fclose(owned) != 0) { remove(temporary); return false; }
    sqlite3* db = NULL;
    bool okay = sqlite3_open_v2(temporary, &db, SQLITE_OPEN_READWRITE, NULL) == SQLITE_OK;
    if (okay) okay = execute(db, "PRAGMA foreign_keys=ON; BEGIN IMMEDIATE;", error, size);
    for (int i = 0; okay && schema[i]; i++) okay = execute(db, schema[i], error, size);
    if (okay) okay = importRows(db, folder, "cardRecords.tsv", "cardClarityCards1",
        "INSERT INTO cards VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)", 18, error, size);
    if (okay) okay = importRows(db, folder, "capGroups.tsv", "cardClarityCaps1",
        "INSERT INTO caps VALUES(?,?,?,?,?,?)", 6, error, size);
    if (okay) okay = importRows(db, folder, "rewardRules.tsv", "cardClarityRules1",
        "INSERT INTO rules VALUES(?,?,?,?,?,?,?,?,?,?,?,?)", 12, error, size);
    if (okay) okay = importRows(db, folder, "feeNotes.tsv", "cardClarityFees1",
        "INSERT INTO fees VALUES(?,?,?,?)", 4, error, size);
    if (okay) okay = importRows(db, folder, "benefitRules.tsv", "cardClarityBenefits1",
        "INSERT INTO benefits VALUES(?,?,?,?,?,?,?,?,?)", 9, error, size);
    if (okay) okay = execute(db,
        "INSERT INTO cardSearch(key,name,issuer,aliases) SELECT key,name,issuer,aliases FROM cards;"
        "INSERT INTO meta VALUES('release','2026-10-29.2');"
        "INSERT INTO meta VALUES('coverage','U.S. public product snapshots; individual offers can differ.');"
        "PRAGMA application_id=1128481073; PRAGMA user_version=1; COMMIT;", error, size);
    if (!okay && db) sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
    if (db && sqlite3_close(db) != SQLITE_OK) okay = false;
    if (okay) {
        Catalog catalog;
        okay = openCatalog(&catalog, temporary);
        if (!okay) snprintf(error, size, "%s", catalog.error);
        closeCatalog(&catalog);
    }
    if (okay && !replaceFile(temporary, destination)) {
        snprintf(error, size, "Could not replace the catalog. The previous file is unchanged."); okay = false;
    }
    if (!okay) remove(temporary);
    return okay;
}
