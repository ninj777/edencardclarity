/* Eden's assigned wallet area. Catalog size and personal wallet capacity are
   separate. Saved identity labels keep a missing catalog entry recognizable. */
#ifndef CATALOG_WALLET_H
#define CATALOG_WALLET_H
#include "catalog.h"
#define WALLET_ENTRY_LIMIT 100
#define WALLET_TRACK_LIMIT 64
#define WALLET_REMINDER_LIMIT 128

typedef struct {
    char ruleKey[CARD_KEY_SIZE], periodKey[24];
    int kind, activated;
    int64_t amount;
    bool known;
} WalletTrack;

typedef struct {
    char key[CARD_KEY_SIZE], cardKey[CARD_KEY_SIZE], name[CARD_NAME_SIZE], issuer[80], note[800];
    char chosenCategory[40];
    bool favorite;
    int64_t valuation; /* Thousandths of one cent per point/mile; -1 is unknown. */
    int anniversaryMonth, anniversaryDay, statementDay;
    WalletTrack tracks[WALLET_TRACK_LIMIT];
    int trackCount;
} WalletEntry;

typedef struct {
    char key[CARD_KEY_SIZE], entryKey[CARD_KEY_SIZE], label[160], due[11], snooze[11];
    bool done;
} WalletReminder;

typedef struct {
    WalletEntry entries[WALLET_ENTRY_LIMIT];
    WalletReminder reminders[WALLET_REMINDER_LIMIT];
    int count, reminderCount, nextKey;
    int64_t revision;
    bool changed;
} CatalogWallet;

enum SavedWalletResult { SAVED_OK, SAVED_MISSING, SAVED_INVALID, SAVED_ERROR, SAVED_CONFLICT };
void initCatalogWallet(CatalogWallet* wallet);
int findWalletEntry(const CatalogWallet* wallet, const char* entryKey);
bool addCatalogCard(Catalog* catalog, CatalogWallet* wallet, const char* cardKey, char* error, size_t capacity);
bool addUnlistedCard(CatalogWallet* wallet, const char* name, const char* issuer, char* error, size_t capacity);
bool removeCatalogCard(CatalogWallet* wallet, int position);
bool validateCatalogWallet(const CatalogWallet* wallet);
enum SavedWalletResult loadCatalogWallet(const char* path, CatalogWallet* wallet, char* error, size_t capacity);
enum SavedWalletResult saveCatalogWallet(const char* path, CatalogWallet* wallet, char* error, size_t capacity);
bool importLegacyWallet(Catalog* catalog, const char* path, CatalogWallet* wallet, char* error, size_t capacity);
WalletTrack* findWalletTrack(WalletEntry* entry, const char* key, const char* period, int kind);
bool setWalletTrack(CatalogWallet* wallet, int position, const WalletTrack* track);
bool setWalletReminder(CatalogWallet* wallet, const WalletReminder* reminder);
bool removeWalletReminder(CatalogWallet* wallet, int position);
bool reminderSummary(Catalog* catalog, const CatalogWallet* wallet, const char* today, char* text, size_t capacity);
bool backupCatalogWallet(const char* path, const char* destination, char* error, size_t capacity);
bool restoreCatalogWallet(const char* backup, const char* path, CatalogWallet* wallet, char* error, size_t capacity);
#endif
