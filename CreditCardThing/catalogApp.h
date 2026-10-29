/* Oliver's assigned integration area. The window and console share this
   session and use the same catalog, wallet, and calculation functions. */
#ifndef CATALOG_APP_H
#define CATALOG_APP_H
#include "rewardMath.h"
typedef struct {
    Catalog catalog;
    CatalogWallet* wallet;
    CatalogWallet* undo;
    char walletPath[1024], today[11], status[400];
    bool undoReady, ended, readError;
} CatalogApp;
int runCatalogApp(int argc, char* argv[]);
bool describeWalletEntry(CatalogApp* app, int position, char* text, size_t capacity);
bool writeMissingRequests(CatalogApp* app, const char* path);
bool saveAppWallet(CatalogApp* app);
bool reloadAppWallet(CatalogApp* app);
bool undoWalletRemoval(CatalogApp* app);
int runCatalogWindow(CatalogApp* app);
#endif
