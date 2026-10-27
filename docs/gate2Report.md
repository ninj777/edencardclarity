# Gate 2: managing and saving cards

Due Wednesday, October 28, 2026. Primary investigator: Eden Yun Yu. The requirement is to complete adding, viewing, removing, saving, and reloading chosen card types in the C program. Required evidence is working source, a save-and-reload demonstration, and recorded input and file tests.

## Work divided by the proposal

Eden's area is input, the in-memory wallet, storage, and file errors. Oliver connects those functions to the main menu and retained Windows window. Joe's area keeps card IDs matched to the sample catalog. Evan's area tests all five actions and records expected and actual behavior.

The detailed file and function explanations are linked from [teamWork.md](teamWork.md). [codeChanges.md](codeChanges.md) gives the current source size and changes in each assigned area, including existing code and moved window code.

## The five completed actions

Add: the user chooses an ID from the five-card catalog. `addWalletCard` checks that the ID exists, the wallet has room, and that the type is not already selected. A successful addition appends the ID and marks the open wallet as changed.

View: `printWallet` shows only the selected card types and their example base rates. An empty wallet gets an empty-list message. The window shows selected types in My wallet and puts the chosen record's details in the right panel. Viewing does not rewrite the save file.

Remove: `removeWalletCard` searches by catalog ID. Later entries move left, the count decreases, and the unused final entry is cleared. Removing a selected card does not change any other card's ID. The menu also allows cancelling and explains when a requested card is absent.

Save: `saveWallet` checks the wallet, writes the header, count, and IDs to a new temporary file, checks writing and closing, then replaces the old save. A failed save reports an error. It does not clear the unsaved-change flag as if saving worked.

Reload: `loadWallet` checks the whole file in a temporary wallet. A successful load replaces the open list. A bad file or read failure leaves that list unchanged. The saved wallet is also loaded at startup, so the selected types return after closing and restarting the program.

The wallet holds five unique types because there are five catalog entries. This is a list of card types, not a collection of individual payment accounts. No card numbers, security codes, bank passwords, or payment processing are involved.

## Menu and window connection

Normal console launch still has three choices: View sample cards, My card wallet, and Exit. The wallet submenu supplies the five actions and Back. The earlier starter remains available with `--gate1`.

The Windows window uses the same `Wallet`, `getCardById`, and storage functions. Its Add, Remove, Save cards, and Reload saved buttons do not maintain a separate save format. The `--wallet-file` option can choose another file for either interface.

Add and remove set the unsaved-change flag. A successful save or reload clears it. Exiting with unsaved changes offers a choice about saving. The window also confirms before reloading over unsaved changes. The console's Reload option explicitly says that it undoes unsaved changes and applies it immediately.

## Saved-file rules and errors

The format begins with `cardClarityWallet1`, followed by a count and that many IDs. The allowed count is 0 through 5. IDs must be valid and unique. Full details and an example are in [savedCardsFormat.md](savedCardsFormat.md).

A missing file is allowed for a new wallet. An incorrect header, invalid count, unknown ID, duplicate ID, missing record, or extra record is rejected. Long lines and hidden null characters are rejected too. A damaged reload does not partly replace the open wallet.

A full list refuses another addition. Blank answers, text, decimals, out-of-range numbers, and long input produce a retry message instead of a crash. End of input is handled separately from a genuine reading error.

Saving uses a new `.tmp` file so opening a save attempt does not immediately erase an older save. An existing temporary file is left alone and blocks that attempt. A missing destination folder or failed replacement is reported. The group should inspect leftover files before moving or deleting them.

## Recorded evidence

The logs are genuine program and compiler output, with summaries of their observed results:

- [gate2BuildLog.txt](gate2BuildLog.txt): compiler versions, C11 commands, warnings, and exit statuses.
- [gate2TestLog.txt](gate2TestLog.txt): menu and module test output.
- [gate2DemoLog.txt](gate2DemoLog.txt): the three-process save, restart, remove, and reload demonstration.
- [testResults.md](testResults.md): expected and actual summaries, plus the unfinished demo-computer record.
- [evanWork.md](evanWork.md): the test groups, exact checks behind the five actions, and remaining Windows work.

The full recorded GCC 14.2.0, Clang 19.1.7, and checked Clang runs each passed 39 Gate 2 tests, 31 Gate 1 checks, and 40 module checks. The checked run disabled leak checking. After the later font change, the GCC build, Windows cross-build, and all 39 Gate 2 tests passed again. Documentation changes do not represent another run of those tests.

The three-process demo saved Travel Sample and Cashback Sample, restarted with both, removed Travel Sample and saved again, then restarted with only Cashback Sample. This checks persistence rather than just showing the list while the first process is still open.

The extra damaged-file test contains 22 malformed examples. A separate test changes a previously valid file while the program is open and confirms that failed Reload preserves the live wallet. These results address Gate 2's additional damaged-file criterion.

## Passing criteria and unfinished verification

The proposal gives a 3 when all five actions and required tests pass. A possible 4 also requires passing additional damaged-file tests. Gate 2 does not list a teammate review as its extra rating condition; Gate 1 does. The teacher determines the final rating, and missing source or test evidence still fails the gate.

Linux behavior and Windows compilation have been checked. Visual Studio and Windows runtime still need to be checked on the demo computer. That includes saving over an existing file, restarting, damaged-file messages, readable fonts, resizing, Tab navigation, and unsaved-change prompts. Use [demoSteps.md](demoSteps.md) and record actual results rather than signing them in advance.

The proposal's staffing exception for four students covering six investigator assignments still needs staff confirmation. No teacher approval is included here.

## Next steps from the proposal

Gate 3 is Evan's first QA review, due October 30. It needs at least 15 expected and actual results, bug notes, and retests; 20 passing cases is its extra rating target. Existing tests can support that review, but they do not replace reviewing and signing the QA report.

Gate 4 is Joe's benefits and rewards work, due November 2. It needs a checked catalog and at least six purchase comparisons. Real sources and dates, whole-cent cashback rounding, ties, and important limits still need work. The existing double-based practice formula does not complete those requirements.
