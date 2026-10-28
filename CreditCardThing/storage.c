/*
Eden Yun Yu - keeping the wallet between runs

I save card types, not payment details. A short text format lets me check
which IDs were actually written. The default is savedCards.txt, but main
can supply another path for either the menu or window.

For Travel Sample and Cashback Sample, the file looks like this:
    cardClarityWallet1
    2
    1
    3
The first line identifies the format. The count says how many ID lines follow.
For an empty wallet, I write the header and 0, then no IDs. Names and rates
come from Joe's catalog instead of another copy in the file.

I allow zero through five unique IDs. Number lines can have surrounding
whitespace, either line-ending style works, and the last ID doesn't need a
newline. The header must match exactly, with no byte-order mark. Unknown or
duplicate IDs, missing records, and anything after the declared count fail.
That includes an extra blank line. Long lines and hidden nulls are rejected
by the shared reader.

When loading, I open with rb and fill a separate Wallet called loaded.
Only a complete valid file, followed by a successful close, replaces the
live list. Otherwise a file with a good first ID and damaged second ID could
leave half a wallet loaded. A declared count of 2 followed by IDs 1 and 1,
for example, must fail without changing the current list.

A missing file is normal for someone who hasn't saved yet. I distinguish
that from other open or read failures so the menu can explain the difference.

Before saving, I validate the wallet and make a path ending in .tmp.
The path, suffix, and ending null must fit in the 1,024-character buffer.
snprintf tells me if they didn't fit; the file system may have a shorter
limit too.

I open the temporary file with wbx. The x prevents overwriting a temporary
file that was already there. Each write and the close must succeed before
the older save is replaced. Linux uses rename; Windows uses MoveFileExA
with replacement enabled.

If writing or replacement fails, I try to remove only the new temporary
file from that attempt and return an error. Oliver can then keep the
unsaved flag and leave the program open. The old save is not truncated
just because a new attempt started.

This is not a backup system or a promise against every disk or power
failure. I use one running copy at a time because separate edits aren't
merged. An old .tmp file should be inspected before moving or deleting it.

For Gate 2 I need all five wallet actions and a saved list that returns after
restarting. Evan's cases also cover bad headers/counts/IDs, duplicate IDs,
missing folders, existing temporary files, and failed replacements.
The actual observations are in docs/gate2TestLog.txt; the three-run example
is in docs/gate2DemoLog.txt. I still need the Windows replacement and prompts
checked on a real Windows computer.
*/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "input.h"
#include "storage.h"

static enum StorageResult readSavedWallet(FILE* file, Wallet* loaded) {
	/* I read into the temporary wallet supplied by loadWallet, not the live one. */
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
	/* I use the count to know where records end, and reject anything after them. */
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
		/* I replace the live list only after reading and closing both succeed. */
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
	/* I use x so an older temporary file is left alone instead of overwritten. */
	FILE* file = fopen(temporaryPath, "wbx");
	if (!file) {
		return STORAGE_WRITE_ERROR;
	}
	bool written = fprintf(file, "cardClarityWallet1\n%d\n", wallet->count) >= 0;
	for (int i = 0; i < wallet->count && written; i++) {
		written = fprintf(file, "%d\n", wallet->cardIds[i]) >= 0;
	}
	if (fclose(file) != 0) {
		/* I still count a close failure as a failed save. */
		written = false;
	}
	if (!written) {
		remove(temporaryPath);
		return STORAGE_WRITE_ERROR;
	}
	/* I use this Windows function to replace an older save after writing succeeds. */
	bool replaced = MoveFileExA(temporaryPath, filePath,
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
	if (!replaced) {
		remove(temporaryPath);
		return STORAGE_WRITE_ERROR;
	}
	return STORAGE_OK;
}
