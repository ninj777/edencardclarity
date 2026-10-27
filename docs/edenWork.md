# Eden: input, wallet, and saved files

Eden Yun Yu is assigned input checking, saving, loading, and file errors in the proposal. Eden also leads Gate 2. The implementation for this area is in `input.c`, `wallet.c`, `storage.c`, and their headers. All six files together contain 316 lines, including comments and blank lines. They were added after the completed Gate 1 checkpoint.

## From the Gate 1 plan to Gate 2

Gate 1 needed checked menu answers and a plan for saving card types later. At that point the input checks were inside `main.c`. Gate 2 separates them into `input.c`, so the menu and saved-file reader can use the same number checks.

The save plan uses card IDs rather than bank details. For example, 1 means Travel Sample and 3 means Cashback Sample. An ID names a sample card type. It is not a credit card number. The program has no reason to ask for a security code, bank password, or payment account.

The earlier plan allowed 20 saved cards. The current catalog has five unique types, so the implemented wallet has five slots and rejects duplicates. That change keeps the limit consistent with the actual catalog. More types would require updating the catalog, its ID rules, and the tests together.

## Reading one complete answer

[input.c](../CreditCardThing/input.c) has three functions. `readLine` gets one complete line, `parseNumber` checks whether text is an allowed whole number, and `readNumber` combines them.

`readLine` uses `fgetc` to read one character at a time. Its character variable is an `int` because `fgetc` must be able to return either a character or the special `EOF` value. The text goes into a caller-provided character array.

The function reserves one space for `\0`, which marks the end of a C string. If there are too many characters, it marks the answer invalid but keeps reading until the line ends. This matters because leaving half an answer in the input stream could make the next menu read the leftover text as a new answer.

A hidden null character also makes the line invalid. Otherwise text after that character could be hidden from normal string functions. The reader finishes that line before returning the invalid result.

Windows text files often end a line with `\r\n`. The reader stops at `\n` and removes the final `\r`. It also accepts a final line without a newline. A stream error returns `INPUT_ERROR`; reaching the end before reading another answer returns `INPUT_END`.

## Deciding whether the answer is a number

`parseNumber` uses `strtol` with base 10. The `end` pointer shows where number conversion stopped. `errno` is cleared before conversion, and `ERANGE` catches a number too large for `long`.

After conversion, spaces are allowed, but other leftover characters are rejected. The value must also fit the minimum and maximum supplied by the caller. Only then is it copied into the output integer.

Some examples explain the difference:

- `2` is valid for the main menu.
- ` 2 ` is also valid because surrounding whitespace is allowed.
- A blank line is invalid because there is no number.
- `2hello` is invalid because letters remain after the number.
- `1.5` is invalid because the decimal point and following text remain.
- `9` is invalid for a menu whose maximum is 3.

`InputResult` is an enum. It names the four outcomes: `INPUT_OK`, `INPUT_END`, `INPUT_INVALID`, and `INPUT_ERROR`. An enum is a type with named integer values, not a method. The caller can choose a different response for a bad answer, the end of input, or a reading failure.

## Keeping the open wallet in memory

[wallet.h](../CreditCardThing/wallet.h) defines a `Wallet` struct. It contains `cardIds`, an array of five integers, and `count`, the number of used entries. The unused entries are zero.

For selected cards 1, 3, and 5, the array starts with `[1, 3, 5]` and `count` is 3. The card records themselves stay in Joe's catalog. This avoids storing another full copy of every name, rate, and bonus in the wallet.

`initWallet` sets the count and array to zero. `walletHasCard` searches the used part of the array. `addWalletCard` first checks the ID through `getCardById`, then checks space and duplicates. On success it writes the ID at `cardIds[count]` and increases the count.

`removeWalletCard` searches for the catalog ID. If ID 3 is removed from `[1, 3, 5]`, the later entry moves left and the used part becomes `[1, 5]`. The count decreases and the unused last entry is cleared. Removal does not change what ID 5 means.

`isWalletValid` checks the count and attempts to add each used ID to a separate empty wallet. Reusing `addWalletCard` means saving applies the same rules as normal additions. `printWallet` displays the selected names and example rates, or a message that the wallet is empty.

Functions that change the wallet take `Wallet*`. Functions that only need to read it take `const Wallet*`. The pointer lets the function access the existing wallet; `const` prevents changing it through that parameter.

## The saved file

This file contains two selected card types:

```text
cardClarityWallet1
2
1
3
```

The first line identifies the format. The next line says how many IDs follow. The remaining lines are Travel Sample and Cashback Sample. A zero-card file contains the header and a count of 0, with no ID lines.

The complete format rules are in [savedCardsFormat.md](savedCardsFormat.md). The file has no cardholder name, payment number, or bank login. Rates are not saved either; they come from the catalog when the program loads an ID.

## Loading without losing the current list

`loadWallet`, in [storage.c](../CreditCardThing/storage.c), opens the file in binary read mode. If opening fails because the file does not exist, it returns `STORAGE_MISSING`. Other open failures return `STORAGE_READ_ERROR`.

The file is read into a separate `Wallet loaded`, not directly into the open wallet. `readSavedWallet` checks the exact header, a count between 0 and 5, and that many IDs. Each ID goes through `addWalletCard`, so unknown IDs and duplicates are rejected. Anything after the stated records is also rejected, including an extra blank line.

The file must close successfully too. Only then does `*wallet = loaded` replace the current wallet. If the last record is damaged, the first valid records do not sneak into the open list. The original list stays available.

For example, a file declaring two cards and containing IDs 1 and 1 is invalid. A file declaring one card but containing IDs 1 and 3 is also invalid. A negative count is rejected before the code starts an ID loop.

The header has to match exactly. A byte-order mark before it is not accepted. Numeric lines can have surrounding whitespace, and either Windows or Linux line endings work.

## Saving without cutting off an older save

`saveWallet` checks the wallet before opening any file. It then builds a temporary filename by adding `.tmp` to the save path. The result has to fit in a 1,024-character buffer, including the ending null character. The operating system can impose a shorter path limit.

The temporary file opens with `wbx`. The `x` means create a new file only. If a temporary file already exists, saving fails and leaves it alone. This avoids erasing an older temporary file without checking it.

The function writes the header, count, and IDs, then checks `fclose`. A reported write or close failure prevents replacing the old save. After successful writing and closing, Linux uses `rename`; Windows uses `MoveFileExA` with replacement enabled. If replacement fails, the function tries to remove its own new temporary file and reports a write error.

This protects the old save from being truncated at the beginning of an ordinary save attempt. It is not a backup system or a guarantee against every disk or power failure. Two running copies also do not merge their edits. For this version, use one copy at a time.

## How this connects to the other parts

Oliver's menu and window call the same wallet and storage functions. Joe's `getCardById` supplies the catalog mapping. Evan's tests check the resulting messages, saved bytes, and data after restarting.

The recorded Linux tests include missing files, duplicate IDs, incorrect counts, extra data, existing temporary files, and failed replacements. They also damage a file after the program has already loaded a wallet, then confirm that a failed Reload keeps that open wallet. Those are directly relevant to the extra damaged-file requirement for Gate 2.

The Windows code cross-compiles, but replacing an existing save and handling its prompts still need a real Windows run. That result is left open in [testResults.md](testResults.md). Later gates will also need purchase-amount checks, whole-cent money calculations, rounding rules, and validation for a larger researched catalog.
