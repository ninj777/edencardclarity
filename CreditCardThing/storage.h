#ifndef STORAGE_H
#define STORAGE_H

#include "wallet.h"

enum StorageResult {
	STORAGE_OK,
	STORAGE_MISSING,
	STORAGE_BAD_FILE,
	STORAGE_READ_ERROR,
	STORAGE_WRITE_ERROR,
	STORAGE_INVALID_WALLET
};

/* Loading only replaces the current wallet after the whole file passes. */
enum StorageResult loadWallet(const char* filePath, Wallet* wallet);
/* Saving writes a new temporary file before replacing an older save. */
enum StorageResult saveWallet(const char* filePath, const Wallet* wallet);

#endif
