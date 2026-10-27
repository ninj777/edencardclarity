#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "input.h"
#include "storage.h"

static enum StorageResult readSavedWallet(FILE* file, Wallet* loaded) {
	char line[128];
	enum InputResult result = readLine(file, line, sizeof(line));
	if (result == INPUT_ERROR) {
		return STORAGE_READ_ERROR;
	}
	if (result != INPUT_OK || strcmp(line, "cardClarityWallet1") != 0) {
		return STORAGE_BAD_FILE;
	}
	int count;
	result = readNumber(file, 0, MAX_WALLET_CARDS, &count);
	if (result != INPUT_OK) {
		if (result == INPUT_ERROR) {
			return STORAGE_READ_ERROR;
		}
		return STORAGE_BAD_FILE;
	}
	for (int i = 0; i < count; i++) {
		int cardId;
		result = readNumber(file, 1, SAMPLE_CARD_COUNT, &cardId);
		if (result != INPUT_OK) {
			if (result == INPUT_ERROR) {
				return STORAGE_READ_ERROR;
			}
			return STORAGE_BAD_FILE;
		}
		if (addWalletCard(loaded, cardId) != WALLET_OK) {
			return STORAGE_BAD_FILE;
		}
	}
	/* The count tells us how many cards to read. Anything after that is extra. */
	result = readLine(file, line, sizeof(line));
	if (result == INPUT_ERROR) {
		return STORAGE_READ_ERROR;
	}
	if (result != INPUT_END) {
		return STORAGE_BAD_FILE;
	}
	return STORAGE_OK;
}

enum StorageResult loadWallet(const char* filePath, Wallet* wallet) {
	FILE* file = fopen(filePath, "rb");
	if (!file) {
		return errno == ENOENT ? STORAGE_MISSING : STORAGE_READ_ERROR;
	}
	Wallet loaded = { 0 };
	enum StorageResult result = readSavedWallet(file, &loaded);
	if (fclose(file) != 0) {
		result = STORAGE_READ_ERROR;
	}
	if (result == STORAGE_OK) {
		*wallet = loaded;
	}
	return result;
}

enum StorageResult saveWallet(const char* filePath, const Wallet* wallet) {
	if (!isWalletValid(wallet)) {
		return STORAGE_INVALID_WALLET;
	}
	char temporaryPath[1024];
	int length = snprintf(temporaryPath, sizeof(temporaryPath), "%s.tmp", filePath);
	if (length < 0 || (size_t)length >= sizeof(temporaryPath)) {
		return STORAGE_WRITE_ERROR;
	}
	/* x means we can only create a new file, not overwrite one. */
	FILE* file = fopen(temporaryPath, "wbx");
	if (!file) {
		return STORAGE_WRITE_ERROR;
	}
	bool written = fprintf(file, "cardClarityWallet1\n%d\n", wallet->count) >= 0;
	for (int i = 0; i < wallet->count && written; i++) {
		written = fprintf(file, "%d\n", wallet->cardIds[i]) >= 0;
	}
	if (fclose(file) != 0) {
		written = false;
	}
	if (!written) {
		remove(temporaryPath);
		return STORAGE_WRITE_ERROR;
	}
#ifdef _WIN32
	/* On Windows, use this to replace an older save. */
	bool replaced = MoveFileExA(temporaryPath, filePath,
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
	bool replaced = rename(temporaryPath, filePath) == 0;
#endif
	if (!replaced) {
		remove(temporaryPath);
		return STORAGE_WRITE_ERROR;
	}
	return STORAGE_OK;
}
