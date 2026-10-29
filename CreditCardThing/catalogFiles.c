/* Eden's assigned file handling. I use SQLite's backup API for live databases.
   Candidates are checked before replacement, and personal saves are separate
   from the public catalog. A plain file copy is not used as a live DB backup. */
#include "catalogWallet.h"
#include "catalogText.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool copyDatabase(const char* source, const char* destination, char* error, size_t size) {
    sqlite3* from = NULL; sqlite3* to = NULL;
    if (sqlite3_open_v2(source, &from, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        copyText(error, size, "Could not open the source database."); sqlite3_close(from); return false;
    }
    FILE* owned = fopen(destination, "wbx");
    if (!owned) { copyText(error, size, "The destination exists or cannot be created. Choose a new name."); sqlite3_close(from); return false; }
    bool okay = fclose(owned) == 0;
    if (okay) okay = sqlite3_open_v2(destination, &to, SQLITE_OPEN_READWRITE, NULL) == SQLITE_OK;
    if (okay) {
        sqlite3_busy_timeout(from, 1000); sqlite3_busy_timeout(to, 1000);
        sqlite3_backup* backup = sqlite3_backup_init(to, "main", from, "main");
        if (!backup) okay = false;
        else {
            int stepped = sqlite3_backup_step(backup, -1);
            int finished = sqlite3_backup_finish(backup);
            okay = stepped == SQLITE_DONE && finished == SQLITE_OK;
        }
    }
    if (to && sqlite3_close(to) != SQLITE_OK) okay = false;
    if (sqlite3_close(from) != SQLITE_OK) okay = false;
    if (!okay) { remove(destination); copyText(error, size, "The database copy failed. The source was not replaced."); }
    return okay;
}

bool backupCatalogWallet(const char* path, const char* destination, char* error, size_t size) {
    CatalogWallet* checked = malloc(sizeof(*checked));
    if (!checked) return false;
    initCatalogWallet(checked);
    bool okay = loadCatalogWallet(path, checked, error, size) == SAVED_OK;
    free(checked);
    return okay && copyDatabase(path, destination, error, size);
}

bool restoreCatalogWallet(const char* backup, const char* path, CatalogWallet* wallet, char* error, size_t size) {
    CatalogWallet* checked = malloc(sizeof(*checked));
    if (!checked) return false;
    initCatalogWallet(checked);
    bool okay = loadCatalogWallet(backup, checked, error, size) == SAVED_OK;
    char temporary[1200], previous[1200];
    if (snprintf(temporary, sizeof(temporary), "%s.restore", path) >= (int)sizeof(temporary)
        || snprintf(previous, sizeof(previous), "%s.beforeRestore", path) >= (int)sizeof(previous)) okay = false;
    bool staged = false;
    if (okay) { okay = copyDatabase(backup, temporary, error, size); staged = okay; }
    /* A restore is a new saved revision. Reusing an older revision would let
       another open copy mistake a restored wallet for the old saved state. */
    int64_t nextRevision = checked->revision;
    if (okay && fileExists(path)) {
        CatalogWallet* current = malloc(sizeof(*current));
        if (!current) okay = false;
        else {
            initCatalogWallet(current);
            if (loadCatalogWallet(path, current, error, size) != SAVED_OK) okay = false;
            else if (current->revision > nextRevision) nextRevision = current->revision;
            free(current);
        }
    }
    if (okay) {
        if (nextRevision >= INT64_MAX - 1) okay = false;
        else {
            sqlite3* db = NULL; sqlite3_stmt* revision = NULL;
            okay = sqlite3_open_v2(temporary, &db, SQLITE_OPEN_READWRITE, NULL) == SQLITE_OK
                && sqlite3_prepare_v2(db, "UPDATE state SET revision=? WHERE id=1", -1, &revision, NULL) == SQLITE_OK;
            if (okay) { sqlite3_bind_int64(revision, 1, nextRevision + 1); okay = sqlite3_step(revision) == SQLITE_DONE; }
            sqlite3_finalize(revision);
            if (db && sqlite3_close(db) != SQLITE_OK) okay = false;
            if (okay) checked->revision = nextRevision + 1;
        }
    }
    if (okay && fileExists(path)) {
        /* A valid current save is backed up. A damaged current file is left for
           inspection rather than silently deleted by a restore. */
        if (!copyDatabase(path, previous, error, size)) {
            copyText(error, size, "Could not preserve the current file. Move it aside or choose another restore path."); okay = false;
        }
    }
    if (okay) okay = replaceFile(temporary, path);
    if (okay) { checked->changed = false; *wallet = *checked; }
    else if (staged) remove(temporary);
    free(checked); return okay;
}

bool updateCatalog(Catalog* catalog, const char* candidatePath, int* added, int* changed,
    int* missing, char* error, size_t size) {
    *added = 0; *changed = 0; *missing = 0;
    Catalog candidate;
    if (!openCatalog(&candidate, candidatePath)) { copyText(error, size, candidate.error); return false; }
    char* before = malloc(CATALOG_TEXT_SIZE); char* after = malloc(CATALOG_TEXT_SIZE);
    sqlite3_stmt* rows = NULL;
    bool okay = before && after && sqlite3_prepare_v2(candidate.db, "SELECT key FROM cards", -1, &rows, NULL) == SQLITE_OK;
    int result = SQLITE_DONE;
    while (okay && (result = sqlite3_step(rows)) == SQLITE_ROW) {
        const char* key = (const char*)sqlite3_column_text(rows, 0);
        CatalogCard old, fresh;
        if (!findCatalogCard(catalog, key, &old)) (*added)++;
        else {
            okay = describeCatalogCard(catalog, key, before, CATALOG_TEXT_SIZE)
                && describeCatalogCard(&candidate, key, after, CATALOG_TEXT_SIZE)
                && findCatalogCard(&candidate, key, &fresh);
            if (okay && (strcmp(before, after) || memcmp(&old, &fresh, sizeof(old)))) (*changed)++;
        }
    }
    if (result != SQLITE_DONE) okay = false;
    sqlite3_finalize(rows); rows = NULL;
    if (okay) okay = sqlite3_prepare_v2(catalog->db, "SELECT key FROM cards", -1, &rows, NULL) == SQLITE_OK;
    while (okay && (result = sqlite3_step(rows)) == SQLITE_ROW) {
        CatalogCard card;
        if (!findCatalogCard(&candidate, (const char*)sqlite3_column_text(rows, 0), &card)) (*missing)++;
    }
    if (result != SQLITE_DONE) okay = false;
    sqlite3_finalize(rows); free(before); free(after); closeCatalog(&candidate);
    char current[1024], temporary[1200], previous[1200];
    copyText(current, sizeof(current), catalog->path);
    if (snprintf(temporary, sizeof(temporary), "%s.update", current) >= (int)sizeof(temporary)
        || snprintf(previous, sizeof(previous), "%s.previous", current) >= (int)sizeof(previous)) okay = false;
    if (!okay) { copyText(error, size, "Could not compare or stage the catalog update."); return false; }
    if (fileExists(previous)) { copyText(error, size, "An earlier .previous catalog exists. Keep or move that backup before updating again."); return false; }
    if (!copyDatabase(candidatePath, temporary, error, size)) return false;
    if (!copyDatabase(current, previous, error, size)) { remove(temporary); return false; }
    closeCatalog(catalog);
    if (!replaceFile(temporary, current)) {
        remove(temporary); openCatalog(catalog, current);
        copyText(error, size, "Could not replace the catalog. The earlier catalog and wallet remain available."); return false;
    }
    if (!openCatalog(catalog, current)) {
        replaceFile(previous, current); openCatalog(catalog, current);
        copyText(error, size, "The new catalog could not be opened; the previous version was restored."); return false;
    }
    return true;
}
