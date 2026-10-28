/*
Eden Yun Yu - save and load declarations

I return StorageResult so the caller can explain a missing file, bad format,
read failure, or write failure. The format itself is described in storage.c.
A load replaces the wallet only after every check succeeds. Saving takes a
const wallet because writing a file should not alter its selected IDs.
*/
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

/* I replace the current wallet only after the whole file passes. */
enum StorageResult loadWallet(const char* filePath, Wallet* wallet);
/* I write a new temporary file before replacing the older save. */
enum StorageResult saveWallet(const char* filePath, const Wallet* wallet);

#endif
