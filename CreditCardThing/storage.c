/*
Eden Yun Yu - my assigned saving and loading

I save card types, not payment details. I use a short text file so the selected
IDs are easy to inspect while checking Gate 2. My default filename is
savedCards.txt, but Oliver can pass another path from either interface.

For Travel Sample and Cashback Sample, I use these four lines:
    cardClarityWallet1
    2
    1
    3
I use the first line to identify the format, the next to give the number of
records, and one following line for each catalog ID. For an empty wallet I
write the same header and a count of 0. I do not write any ID lines after it.
I keep the ID mapping in Joe's cards.c rather than saving names and rates too.

I allow zero through five unique IDs. I accept Windows or Linux line endings,
surrounding whitespace on number lines, and a last ID without a newline.
I require the header to match exactly, with no byte-order mark. I reject
unknown IDs, duplicates, missing records, and anything after the stated count,
even an extra blank line. I also reject long lines and hidden null characters
through the shared input reader. I do not silently load only the good part.

I open a save with rb and build a temporary Wallet called loaded. I check the
whole file and close it before copying it over the live wallet. For example,
a file declaring two cards but containing IDs 1 and 1 must fail. If a file is
damaged during a session, a failed Reload must leave the open list available.
I distinguish a file that does not exist from other open/read errors because
a first-time user normally has no saved file yet.

When saving, I validate the current wallet before opening any destination.
I make a temporary path by adding .tmp. The path plus suffix and ending null
must fit in the 1,024-character array; the operating system can have a shorter
limit. I check snprintf rather than assuming the path fitted.

I open the temporary file with wbx. The x means it must be a new file. If an
old temporary file is there, I leave it alone and report failure. I check each
write and the file close before replacing the older save. On Linux I use
rename, and on Windows I use MoveFileExA with replacement enabled.

If writing or replacement fails, I try to remove only the new temporary file
from this attempt. I keep the result as an error, so Oliver does not clear the
unsaved flag or close as if saving succeeded. This avoids truncating the old
save at the start of an ordinary attempt. I do not treat it as a backup system
or a guarantee against every disk or power failure. I use one program copy at
a time because this version does not merge separate people's changes.

For Gate 2, I need add, view, remove, save, and reload to work across a restart.
Evan's cases also check missing files, bad headers/counts/IDs, duplicate IDs,
existing temporary files, a missing folder, and a failed replacement. I can
find the actual output in docs/gate2TestLog.txt and the three-run example in
docs/gate2DemoLog.txt. I leave the Windows replacement and prompts for a real
Windows run; compiling that branch does not establish its runtime result.
*/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
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
#ifdef _WIN32
	/* I use this Windows function to replace an older save after writing succeeds. */
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
