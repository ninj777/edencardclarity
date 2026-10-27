/*
Eden Yun Yu - my assigned file interface

I return StorageResult so Oliver can explain success, a missing file, a bad
format, or a read/write failure. I keep the two public functions here and the
file-format details in storage.c. I only replace the live wallet after a full
successful load. I use a const wallet when saving because writing the file
does not need to change the selected IDs in memory.
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
