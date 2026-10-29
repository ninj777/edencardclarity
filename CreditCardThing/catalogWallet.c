/* Eden's assigned storage and input area. I keep edits in memory until Save.
   Loading validates a separate wallet; saving commits one database transaction.
   A revision check stops an older open copy overwriting a newer saved list. */
#include "catalogWallet.h"
#include "catalogText.h"
#include "input.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool columnText(sqlite3_stmt* s, int column, char* out, size_t size) {
    const unsigned char* text = sqlite3_column_text(s, column);
    if (text && (size_t)sqlite3_column_bytes(s, column) != strlen((const char*)text)) return false;
    return copyText(out, size, text ? (const char*)text : "");
}

static bool execWallet(sqlite3* db, const char* sql, char* error, size_t size) {
    if (sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK) return true;
    snprintf(error, size, "%s", sqlite3_errmsg(db)); return false;
}

static int64_t walletScalar(sqlite3* db, const char* sql) {
    sqlite3_stmt* s = NULL; int64_t value = -1;
    if (sqlite3_prepare_v2(db, sql, -1, &s, NULL) == SQLITE_OK && sqlite3_step(s) == SQLITE_ROW) value = sqlite3_column_int64(s, 0);
    sqlite3_finalize(s); return value;
}

void initCatalogWallet(CatalogWallet* wallet) {
    memset(wallet, 0, sizeof(*wallet)); wallet->nextKey = 1;
}

int findWalletEntry(const CatalogWallet* wallet, const char* key) {
    for (int i = 0; i < wallet->count; i++) if (!strcmp(wallet->entries[i].key, key)) return i;
    return -1;
}

static WalletEntry* newEntry(CatalogWallet* wallet, char* error, size_t size) {
    if (wallet->count >= WALLET_ENTRY_LIMIT || wallet->nextKey > 1000000000) {
        copyText(error, size, "Your wallet is full. Remove an entry before adding another."); return NULL;
    }
    WalletEntry* entry = &wallet->entries[wallet->count]; memset(entry, 0, sizeof(*entry));
    snprintf(entry->key, sizeof(entry->key), "entry%d", wallet->nextKey);
    entry->valuation = -1;
    return entry;
}

bool addCatalogCard(Catalog* catalog, CatalogWallet* wallet, const char* key, char* error, size_t size) {
    for (int i = 0; i < wallet->count; i++) if (!strcmp(wallet->entries[i].cardKey, key)) {
        copyText(error, size, "That card version is already in your wallet."); return false;
    }
    CatalogCard card;
    if (!findCatalogCard(catalog, key, &card)) { copyText(error, size, "That card version is not in this catalog."); return false; }
    WalletEntry* entry = newEntry(wallet, error, size); if (!entry) return false;
    copyText(entry->cardKey, sizeof(entry->cardKey), key);
    copyText(entry->name, sizeof(entry->name), card.name);
    copyText(entry->issuer, sizeof(entry->issuer), card.issuer);
    wallet->count++; wallet->nextKey++; wallet->changed = true; return true;
}

bool addUnlistedCard(CatalogWallet* wallet, const char* name, const char* issuer, char* error, size_t size) {
    if (!name[0] || !issuer[0] || strlen(name) >= CARD_NAME_SIZE || strlen(issuer) >= 80) {
        copyText(error, size, "Enter a card name and issuer that fit the fields."); return false;
    }
    WalletEntry* entry = newEntry(wallet, error, size); if (!entry) return false;
    copyText(entry->name, sizeof(entry->name), name); copyText(entry->issuer, sizeof(entry->issuer), issuer);
    wallet->count++; wallet->nextKey++; wallet->changed = true; return true;
}

bool removeCatalogCard(CatalogWallet* wallet, int position) {
    if (position < 0 || position >= wallet->count) return false;
    char key[CARD_KEY_SIZE]; copyText(key, sizeof(key), wallet->entries[position].key);
    for (int i = 0; i < wallet->reminderCount;) {
        if (!strcmp(wallet->reminders[i].entryKey, key)) removeWalletReminder(wallet, i);
        else i++;
    }
    memmove(&wallet->entries[position], &wallet->entries[position + 1],
        sizeof(WalletEntry) * (size_t)(wallet->count - position - 1));
    memset(&wallet->entries[--wallet->count], 0, sizeof(WalletEntry));
    wallet->changed = true; return true;
}

static bool validTrack(const WalletTrack* track) {
    return track->ruleKey[0] && track->periodKey[0] && track->kind >= 0 && track->kind <= 1
        && track->activated >= -1 && track->activated <= 1 && track->amount >= 0
        && track->amount <= INT64_C(100000000000);
}

bool validateCatalogWallet(const CatalogWallet* wallet) {
    if (wallet->count < 0 || wallet->count > WALLET_ENTRY_LIMIT || wallet->reminderCount < 0
        || wallet->reminderCount > WALLET_REMINDER_LIMIT || wallet->nextKey < 1 || wallet->nextKey > 1000000001
        || wallet->revision < 0 || wallet->revision == INT64_MAX) return false;
    for (int i = 0; i < wallet->count; i++) {
        const WalletEntry* e = &wallet->entries[i];
        int localId;
        if (strncmp(e->key, "entry", 5) || !parseNumber(e->key + 5, 1, 1000000000, &localId)
            || localId >= wallet->nextKey) return false;
        if (!e->key[0] || !e->name[0] || !e->issuer[0] || e->valuation < -1 || e->valuation > 100000
            || e->trackCount < 0 || e->trackCount > WALLET_TRACK_LIMIT
            || e->anniversaryMonth < 0 || e->anniversaryMonth > 12 || e->anniversaryDay < 0
            || e->anniversaryDay > daysInMonth(2000, e->anniversaryMonth)
            || (e->anniversaryMonth == 0) != (e->anniversaryDay == 0)
            || e->statementDay < 0 || e->statementDay > 31) return false;
        for (int j = 0; j < i; j++) if (!strcmp(e->key, wallet->entries[j].key)
            || (e->cardKey[0] && !strcmp(e->cardKey, wallet->entries[j].cardKey))) return false;
        for (int j = 0; j < e->trackCount; j++) {
            if (!validTrack(&e->tracks[j])) return false;
            for (int k = 0; k < j; k++) if (e->tracks[j].kind == e->tracks[k].kind
                && !strcmp(e->tracks[j].ruleKey, e->tracks[k].ruleKey)
                && !strcmp(e->tracks[j].periodKey, e->tracks[k].periodKey)) return false;
        }
    }
    for (int i = 0; i < wallet->reminderCount; i++) {
        const WalletReminder* r = &wallet->reminders[i];
        if (!r->key[0] || !r->label[0] || !validDate(r->due)
            || (r->snooze[0] && !validDate(r->snooze))
            || (r->entryKey[0] && findWalletEntry(wallet, r->entryKey) < 0)) return false;
        for (int j = 0; j < i; j++) if (!strcmp(r->key, wallet->reminders[j].key)) return false;
    }
    return true;
}

WalletTrack* findWalletTrack(WalletEntry* entry, const char* key, const char* period, int kind) {
    for (int i = 0; i < entry->trackCount; i++) if (entry->tracks[i].kind == kind
        && !strcmp(entry->tracks[i].ruleKey, key) && !strcmp(entry->tracks[i].periodKey, period)) return &entry->tracks[i];
    return NULL;
}

bool setWalletTrack(CatalogWallet* wallet, int position, const WalletTrack* track) {
    if (position < 0 || position >= wallet->count || !validTrack(track)) return false;
    WalletEntry* entry = &wallet->entries[position];
    WalletTrack* current = findWalletTrack(entry, track->ruleKey, track->periodKey, track->kind);
    if (!current) {
        if (entry->trackCount == WALLET_TRACK_LIMIT) return false;
        current = &entry->tracks[entry->trackCount++];
    }
    *current = *track; wallet->changed = true; return true;
}

bool setWalletReminder(CatalogWallet* wallet, const WalletReminder* reminder) {
    if (!reminder->key[0] || !reminder->label[0] || !validDate(reminder->due)
        || (reminder->snooze[0] && !validDate(reminder->snooze))
        || (reminder->entryKey[0] && findWalletEntry(wallet, reminder->entryKey) < 0)) return false;
    int i;
    for (i = 0; i < wallet->reminderCount; i++) if (!strcmp(wallet->reminders[i].key, reminder->key)) break;
    if (i == wallet->reminderCount && wallet->reminderCount == WALLET_REMINDER_LIMIT) return false;
    wallet->reminders[i] = *reminder;
    if (i == wallet->reminderCount) wallet->reminderCount++;
    wallet->changed = true; return true;
}

bool removeWalletReminder(CatalogWallet* wallet, int position) {
    if (position < 0 || position >= wallet->reminderCount) return false;
    memmove(&wallet->reminders[position], &wallet->reminders[position + 1],
        sizeof(WalletReminder) * (size_t)(wallet->reminderCount - position - 1));
    wallet->reminderCount--; wallet->changed = true; return true;
}

static bool validWalletDatabase(sqlite3* db) {
    return walletScalar(db, "PRAGMA application_id") == 1128481074
        && walletScalar(db, "PRAGMA user_version") == 2
        && walletScalar(db, "SELECT count(*) FROM state WHERE id=1") == 1
        && walletScalar(db, "SELECT count(*) FROM entries WHERE favorite NOT IN(0,1) OR annivMonth NOT BETWEEN 0 AND 12 OR annivDay NOT BETWEEN 0 AND 31 OR statementDay NOT BETWEEN 0 AND 31") == 0
        && walletScalar(db, "SELECT count(*) FROM tracks WHERE kind NOT IN(0,1) OR activated NOT BETWEEN -1 AND 1 OR known NOT IN(0,1)") == 0
        && walletScalar(db, "SELECT count(*) FROM reminders WHERE done NOT IN(0,1)") == 0;
}

enum SavedWalletResult loadCatalogWallet(const char* path, CatalogWallet* wallet, char* error, size_t size) {
    FILE* exists = fopen(path, "rb");
    if (!exists) { snprintf(error, size, "No saved wallet yet, or the path is not readable."); return errno == ENOENT ? SAVED_MISSING : SAVED_ERROR; }
    fclose(exists);
    sqlite3* db = NULL;
    if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        snprintf(error, size, "Could not open the saved wallet."); sqlite3_close(db); return SAVED_ERROR;
    }
    sqlite3_db_config(db, SQLITE_DBCONFIG_TRUSTED_SCHEMA, 0, NULL);
    sqlite3_db_config(db, SQLITE_DBCONFIG_DEFENSIVE, 1, NULL);
    CatalogWallet* loaded = calloc(1, sizeof(*loaded));
    bool okay = loaded && validWalletDatabase(db);
    sqlite3_stmt* s = NULL; int result = SQLITE_DONE;
    if (okay) {
        int64_t next = walletScalar(db, "SELECT nextKey FROM state WHERE id=1");
        if (next < 1 || next > 1000000001) okay = false;
        loaded->nextKey = okay ? (int)next : 0;
        loaded->revision = walletScalar(db, "SELECT revision FROM state WHERE id=1");
        okay = sqlite3_prepare_v2(db, "SELECT * FROM entries ORDER BY position", -1, &s, NULL) == SQLITE_OK;
    }
    while (okay && (result = sqlite3_step(s)) == SQLITE_ROW) {
        if (loaded->count == WALLET_ENTRY_LIMIT) { okay = false; break; }
        WalletEntry* e = &loaded->entries[loaded->count++];
        okay = columnText(s, 0, e->key, sizeof(e->key)) && columnText(s, 1, e->cardKey, sizeof(e->cardKey))
            && columnText(s, 2, e->name, sizeof(e->name)) && columnText(s, 3, e->issuer, sizeof(e->issuer))
            && columnText(s, 4, e->note, sizeof(e->note))
            && columnText(s, 10, e->chosenCategory, sizeof(e->chosenCategory));
        e->favorite = sqlite3_column_int(s, 5) != 0; e->valuation = sqlite3_column_int64(s, 6);
        if (sqlite3_column_int64(s, 5) < 0 || sqlite3_column_int64(s, 5) > 1) okay = false;
        e->anniversaryMonth = sqlite3_column_int(s, 7); e->anniversaryDay = sqlite3_column_int(s, 8);
        e->statementDay = sqlite3_column_int(s, 9);
    }
    if (result != SQLITE_DONE) okay = false;
    sqlite3_finalize(s); s = NULL;
    if (okay) okay = sqlite3_prepare_v2(db, "SELECT entryKey,ruleKey,periodKey,kind,amount,known,activated FROM tracks", -1, &s, NULL) == SQLITE_OK;
    while (okay && (result = sqlite3_step(s)) == SQLITE_ROW) {
        char key[CARD_KEY_SIZE]; WalletTrack track = { 0 };
        okay = columnText(s, 0, key, sizeof(key)) && columnText(s, 1, track.ruleKey, sizeof(track.ruleKey))
            && columnText(s, 2, track.periodKey, sizeof(track.periodKey));
        int position = okay ? findWalletEntry(loaded, key) : -1;
        track.kind = sqlite3_column_int(s, 3); track.amount = sqlite3_column_int64(s, 4);
        track.known = sqlite3_column_int(s, 5) != 0; track.activated = sqlite3_column_int(s, 6);
        if (sqlite3_column_int64(s, 5) < 0 || sqlite3_column_int64(s, 5) > 1) okay = false;
        if (position < 0 || findWalletTrack(&loaded->entries[position], track.ruleKey, track.periodKey, track.kind)
            || !setWalletTrack(loaded, position, &track)) okay = false;
    }
    if (result != SQLITE_DONE) okay = false;
    sqlite3_finalize(s); s = NULL;
    if (okay) okay = sqlite3_prepare_v2(db, "SELECT * FROM reminders ORDER BY key", -1, &s, NULL) == SQLITE_OK;
    while (okay && (result = sqlite3_step(s)) == SQLITE_ROW) {
        if (loaded->reminderCount == WALLET_REMINDER_LIMIT) { okay = false; break; }
        WalletReminder* r = &loaded->reminders[loaded->reminderCount++];
        okay = columnText(s, 0, r->key, sizeof(r->key)) && columnText(s, 1, r->entryKey, sizeof(r->entryKey))
            && columnText(s, 2, r->label, sizeof(r->label)) && columnText(s, 3, r->due, sizeof(r->due))
            && columnText(s, 4, r->snooze, sizeof(r->snooze));
        r->done = sqlite3_column_int(s, 5) != 0;
        if (sqlite3_column_int64(s, 5) < 0 || sqlite3_column_int64(s, 5) > 1) okay = false;
    }
    if (result != SQLITE_DONE) okay = false;
    sqlite3_finalize(s);
    if (okay) okay = validateCatalogWallet(loaded);
    if (sqlite3_close(db) != SQLITE_OK) okay = false;
    if (okay) { loaded->changed = false; *wallet = *loaded; }
    else snprintf(error, size, "That wallet is damaged or uses an unsupported format. Your open list has not changed.");
    free(loaded); return okay ? SAVED_OK : SAVED_INVALID;
}

static bool bindEntry(sqlite3_stmt* s, const WalletEntry* e, int position) {
    sqlite3_reset(s); sqlite3_clear_bindings(s);
    const char* strings[] = { e->key,e->cardKey,e->name,e->issuer,e->note };
    for (int i = 0; i < 5; i++) if (sqlite3_bind_text(s, i + 1, strings[i], -1, SQLITE_TRANSIENT) != SQLITE_OK) return false;
    sqlite3_bind_int(s, 6, e->favorite); sqlite3_bind_int64(s, 7, e->valuation);
    sqlite3_bind_int(s, 8, e->anniversaryMonth); sqlite3_bind_int(s, 9, e->anniversaryDay);
    sqlite3_bind_int(s, 10, e->statementDay);
    sqlite3_bind_text(s, 11, e->chosenCategory, -1, SQLITE_TRANSIENT); sqlite3_bind_int(s, 12, position);
    return sqlite3_step(s) == SQLITE_DONE;
}

enum SavedWalletResult saveCatalogWallet(const char* path, CatalogWallet* wallet, char* error, size_t size) {
    if (size) error[0] = '\0';
    if (!validateCatalogWallet(wallet)) { copyText(error, size, "The open wallet is not valid, so it was not saved."); return SAVED_INVALID; }
    bool existed = fileExists(path);
    if (!existed) {
        FILE* owned = fopen(path, "wbx");
        if (!owned) { copyText(error, size, "Could not create a new wallet file."); return SAVED_ERROR; }
        if (fclose(owned) != 0) { remove(path); return SAVED_ERROR; }
    }
    sqlite3* db = NULL;
    if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
        snprintf(error, size, "Could not open the destination for saving."); sqlite3_close(db);
        if (!existed) remove(path);
        return SAVED_ERROR;
    }
    sqlite3_busy_timeout(db, 1000);
    bool okay = execWallet(db, "PRAGMA foreign_keys=ON; BEGIN IMMEDIATE", error, size);
    enum SavedWalletResult failure = SAVED_ERROR;
    if (okay && existed && !validWalletDatabase(db)) { okay = false; failure = SAVED_INVALID; copyText(error, size, "The destination is not a supported wallet. It was not overwritten."); }
    if (okay && existed && walletScalar(db, "SELECT revision FROM state WHERE id=1") != wallet->revision) {
        okay = false; failure = SAVED_CONFLICT;
        copyText(error, size, "Another copy changed this save. Reload or back up before replacing it.");
    }
    if (okay && !existed) okay = execWallet(db,
        "CREATE TABLE state(id INTEGER PRIMARY KEY CHECK(id=1),nextKey INTEGER NOT NULL,revision INTEGER NOT NULL) STRICT;"
        "CREATE TABLE entries(entryKey TEXT PRIMARY KEY,cardKey TEXT NOT NULL,name TEXT NOT NULL,issuer TEXT NOT NULL,note TEXT NOT NULL,"
        "favorite INTEGER NOT NULL CHECK(favorite IN(0,1)),valuation INTEGER NOT NULL,annivMonth INTEGER NOT NULL,"
        "annivDay INTEGER NOT NULL,statementDay INTEGER NOT NULL,chosenCategory TEXT NOT NULL,position INTEGER NOT NULL UNIQUE) STRICT;"
        "CREATE UNIQUE INDEX oneCatalogCard ON entries(cardKey) WHERE cardKey<>'';"
        "CREATE TABLE tracks(entryKey TEXT NOT NULL REFERENCES entries(entryKey),ruleKey TEXT NOT NULL,periodKey TEXT NOT NULL,"
        "kind INTEGER NOT NULL,amount INTEGER NOT NULL,known INTEGER NOT NULL CHECK(known IN(0,1)),activated INTEGER NOT NULL,"
        "PRIMARY KEY(entryKey,ruleKey,periodKey,kind)) STRICT;"
        "CREATE TABLE reminders(key TEXT PRIMARY KEY,entryKey TEXT NOT NULL,label TEXT NOT NULL,due TEXT NOT NULL,snooze TEXT NOT NULL,"
        "done INTEGER NOT NULL CHECK(done IN(0,1))) STRICT;"
        "PRAGMA application_id=1128481074; PRAGMA user_version=2;", error, size);
    if (okay) okay = execWallet(db, "DELETE FROM tracks; DELETE FROM reminders; DELETE FROM entries; DELETE FROM state;", error, size);
    sqlite3_stmt* s = NULL;
    if (okay) okay = sqlite3_prepare_v2(db, "INSERT INTO state VALUES(1,?,?)", -1, &s, NULL) == SQLITE_OK;
    if (okay) {
        sqlite3_bind_int(s, 1, wallet->nextKey); sqlite3_bind_int64(s, 2, wallet->revision + 1);
        okay = sqlite3_step(s) == SQLITE_DONE;
    }
    sqlite3_finalize(s); s = NULL;
    if (okay) okay = sqlite3_prepare_v2(db, "INSERT INTO entries VALUES(?,?,?,?,?,?,?,?,?,?,?,?)", -1, &s, NULL) == SQLITE_OK;
    for (int i = 0; okay && i < wallet->count; i++) okay = bindEntry(s, &wallet->entries[i], i);
    sqlite3_finalize(s); s = NULL;
    if (okay) okay = sqlite3_prepare_v2(db, "INSERT INTO tracks VALUES(?,?,?,?,?,?,?)", -1, &s, NULL) == SQLITE_OK;
    for (int i = 0; okay && i < wallet->count; i++) for (int j = 0; okay && j < wallet->entries[i].trackCount; j++) {
        WalletTrack* t = &wallet->entries[i].tracks[j];
        sqlite3_reset(s); sqlite3_clear_bindings(s);
        sqlite3_bind_text(s, 1, wallet->entries[i].key, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 2, t->ruleKey, -1, SQLITE_TRANSIENT); sqlite3_bind_text(s, 3, t->periodKey, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(s, 4, t->kind); sqlite3_bind_int64(s, 5, t->amount); sqlite3_bind_int(s, 6, t->known);
        sqlite3_bind_int(s, 7, t->activated); okay = sqlite3_step(s) == SQLITE_DONE;
    }
    sqlite3_finalize(s); s = NULL;
    if (okay) okay = sqlite3_prepare_v2(db, "INSERT INTO reminders VALUES(?,?,?,?,?,?)", -1, &s, NULL) == SQLITE_OK;
    for (int i = 0; okay && i < wallet->reminderCount; i++) {
        WalletReminder* r = &wallet->reminders[i]; sqlite3_reset(s); sqlite3_clear_bindings(s);
        sqlite3_bind_text(s, 1, r->key, -1, SQLITE_TRANSIENT); sqlite3_bind_text(s, 2, r->entryKey, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 3, r->label, -1, SQLITE_TRANSIENT); sqlite3_bind_text(s, 4, r->due, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 5, r->snooze, -1, SQLITE_TRANSIENT); sqlite3_bind_int(s, 6, r->done);
        okay = sqlite3_step(s) == SQLITE_DONE;
    }
    sqlite3_finalize(s);
    if (okay) okay = execWallet(db, "COMMIT", error, size);
    else { if (!error[0]) snprintf(error, size, "%s", sqlite3_errmsg(db)); sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL); }
    if (sqlite3_close(db) != SQLITE_OK) okay = false;
    if (!okay && !existed) remove(path);
    if (okay) { wallet->revision++; wallet->changed = false; return SAVED_OK; }
    return failure;
}

bool importLegacyWallet(Catalog* catalog, const char* path, CatalogWallet* wallet, char* error, size_t size) {
    FILE* file = fopen(path, "rb"); if (!file) { copyText(error, size, "Could not open the old wallet."); return false; }
    CatalogWallet* imported = calloc(1, sizeof(*imported));
    bool okay = imported != NULL; if (imported) initCatalogWallet(imported);
    char line[128]; int count = 0;
    okay = okay && readLine(file, line, sizeof(line)) == INPUT_OK && !strcmp(line, "cardClarityWallet1")
        && readNumber(file, 0, 5, &count) == INPUT_OK;
    for (int i = 0; okay && i < count; i++) {
        int id; char key[32];
        okay = readNumber(file, 1, 5, &id) == INPUT_OK;
        if (!okay) break;
        snprintf(key, sizeof(key), "legacy%03d", id);
        okay = addCatalogCard(catalog, imported, key, error, size);
    }
    if (okay) okay = readLine(file, line, sizeof(line)) == INPUT_END;
    if (fclose(file) != 0) okay = false;
    if (okay) { imported->revision = wallet->revision; imported->changed = true; *wallet = *imported; }
    else copyText(error, size, "The old wallet is invalid or its legacy sample records are missing. Your open list is unchanged.");
    free(imported); return okay;
}

static int dateNumber(const char* date) {
    int y, m, d;
    if (!dateParts(date, &y, &m, &d)) return -1;
    int total = (y - 1) * 365 + (y - 1) / 4 - (y - 1) / 100 + (y - 1) / 400;
    for (int i = 1; i < m; i++) total += daysInMonth(y, i);
    return total + d;
}

bool reminderSummary(Catalog* catalog, const CatalogWallet* wallet, const char* today, char* text, size_t size) {
    if (!validDate(today) || !validateCatalogWallet(wallet)) return false;
    text[0] = '\0'; int shown = 0, current = dateNumber(today), y, m, d;
    dateParts(today, &y, &m, &d);
    for (int i = 0; i < wallet->reminderCount; i++) {
        const WalletReminder* r = &wallet->reminders[i];
        if (r->done || (r->snooze[0] && strcmp(r->snooze, today) > 0) || dateNumber(r->due) > current + 30) continue;
        if (!appendText(text, size, "%s: %s%s\n", r->due, r->label, strcmp(r->due, today) < 0 ? " (overdue)" : "")) return false;
        shown++;
    }
    for (int i = 0; i < wallet->count; i++) {
        const WalletEntry* e = &wallet->entries[i];
        if (e->anniversaryMonth) {
            char date[40]; int year = y;
            int day = e->anniversaryDay > daysInMonth(year, e->anniversaryMonth) ? daysInMonth(year, e->anniversaryMonth) : e->anniversaryDay;
            snprintf(date, sizeof(date), "%04d-%02d-%02d", year, e->anniversaryMonth, day);
            if (strcmp(date, today) < 0) {
                year++; day = e->anniversaryDay > daysInMonth(year, e->anniversaryMonth) ? daysInMonth(year, e->anniversaryMonth) : e->anniversaryDay;
                snprintf(date, sizeof(date), "%04d-%02d-%02d", year, e->anniversaryMonth, day);
            }
            if (validDate(date) && dateNumber(date) <= current + 30) {
                if (!appendText(text, size, "%s: Review %s before its entered anniversary; fee posting may differ.\n", date, e->name)) return false;
                shown++;
            }
        }
        BenefitRule* benefits = calloc(40, sizeof(*benefits)); if (!benefits) return false;
        int count = e->cardKey[0] ? catalogBenefits(catalog, e->cardKey, benefits, 40) : 0;
        for (int j = 0; j < count; j++) if (benefits[j].due[0] && dateNumber(benefits[j].due) <= current + 30
            && strcmp(benefits[j].due, today) >= 0) {
            if (!appendText(text, size, "%s: %s - %s\n", benefits[j].due, e->name, benefits[j].label)) { free(benefits); return false; }
            shown++;
        }
        free(benefits);
    }
    return shown ? true : appendText(text, size, "No reminders due in the next 30 days. Dates and tracking are maintained locally.\n");
}
