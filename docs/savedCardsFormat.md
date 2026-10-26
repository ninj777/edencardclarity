# Our saved-card file

The default file is `savedCards.txt`. It saves card types, not payment information. This example saves Travel Sample and Cashback Sample:

```text
cardClarityWallet1
2
1
3
```

The first line names the format. The second says two cards are saved. The last lines are their IDs:

- 1: Travel Sample, example base rate 1.25%, example fee $95.
- 2: Points Sample, example base rate 1.00%, example fee $250.
- 3: Cashback Sample, example base rate 1.00%, example fee $0.
- 4: Everyday Sample, example base rate 1.00%, example fee $0.
- 5: Starter Sample, example base rate 1.00%, example fee $0.

Joe's catalog check is to keep those IDs matched to the same names. The values are made up. The program checks all five mappings in the module tests.

A wallet can have 0 to 5 cards, with no duplicates. The earlier plan mentioned 20, but we chose five because this catalog only has five unique types. The random identifier used by the Windows display is not saved.

Loading checks the whole file in a temporary wallet. Only a complete, valid file replaces the current list. Bad headers, counts, IDs, missing lines, extra lines, and hidden null characters are rejected. Windows and Linux line endings both work. The header must match exactly, with no byte-order mark. Numeric lines can have surrounding whitespace, and the last line does not need a newline.

Saving creates a new `.tmp` file beside the destination. Once writing and closing succeed, it replaces the old save. If a temporary file is already there, it is left alone and saving fails. After closing the program, inspect the save and temporary file before moving either one.

The path plus `.tmp` must fit in the 1,024-character buffer, and the file system may have a shorter limit. We use `MoveFileExA` on Windows to replace an existing save. That Windows behavior still needs a real runtime check.

Reload undoes unsaved edits. Saving an empty wallet is allowed. Viewing a wallet does not rewrite its file. Run one copy at a time; this version does not merge edits from separate copies.
