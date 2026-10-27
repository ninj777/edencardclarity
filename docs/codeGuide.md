# How our code fits together

The program is split into seven C files. `main.c` connects the menu to the card, wallet, input, and storage functions. `window.c` handles the Windows layout and buttons. `logger.c` prints its startup and error messages.

This page is the short overview. The detailed explanations are divided by the proposal: [Oliver's menus and build](oliverWork.md), [Eden's input and storage](edenWork.md), [Joe's catalog](joeWork.md), and [Evan's tests](evanWork.md). [codeChanges.md](codeChanges.md) gives the source amounts without counting documentation or generated logs as application code.

## Cards and wallet

`cards.h` defines `Card`, which groups a name, network, fee, reward type, base rate, and category bonuses. `cards.c` sets up the five practice records. The existing reward formula is unchanged: a 5% bonus makes the base reward 1.05 times as large. The menu is not a full purchase calculator yet.

`getCardById` checks that a saved number is between 1 and 5, then finds the matching record. Bad numbers return `NULL`. `printSampleCards` shows those numbers so the user can choose them.

`wallet.h` defines `Wallet`. It has an array called `cardIds` and an integer called `count`. For cards 1 and 3, the used entries are `[1, 3]` and the count is 2. The array has five slots.

The wallet functions in `wallet.c` are:

- `initWallet`: set the count and all slots to zero.
- `walletHasCard`: check whether an ID is already selected.
- `addWalletCard`: check the ID, available space, and duplicates, then append it.
- `removeWalletCard`: find the ID and move later entries left to fill the gap.
- `isWalletValid`: check the count and IDs before saving.
- `printWallet`: show only selected cards, or explain that the list is empty.

We pass `Wallet*` when a function changes the wallet. `const Wallet*` lets a function read it without changing it through that pointer.

## Reading answers

`readLine`, in `input.c`, reads characters with `fgetc`. It leaves room for the ending null character. If the line is too long or contains a hidden null, it finishes reading the line and rejects it. This stops the next answer from picking up leftover characters.

`parseNumber` uses `strtol` to turn the text into a number. It rejects missing numbers, overflow, extra text, and values outside the allowed range. Spaces around a valid number are allowed. `readNumber` combines the line reader and number check.

The `InputResult` enum names four outcomes: valid answer, end of input, invalid answer, and read error. A real read error is handled separately from normal end of input.

## Saving and loading

`loadWallet`, in `storage.c`, opens the file and makes a temporary empty wallet. The private `readSavedWallet` function checks the header, count, IDs, and end of file. Only after all checks and the file close succeed does the temporary wallet replace the current one. A bad file cannot leave half a list loaded.

`saveWallet` checks the wallet, adds `.tmp` to the filename, and checks the path length. It opens a new file using `wbx`; the `x` means an existing temporary file cannot be overwritten. It checks each write and the file close before replacing the old save. A failed write or replacement reports failure.

Linux uses `rename` for the replacement. Windows uses `MoveFileExA` because its usual `rename` does not replace an existing file. The exact file layout is in `savedCardsFormat.md`.

## The menus

`main` creates the sample records and selects a mode. Normal launch opens Gate 2. `--wallet-file path` changes the save location, `--gate1` opens the earlier menu, and `--gui` opens the window, which switches on its logger. `--gui --wallet-file path` chooses another saved list. The console has no startup timing messages.

`runConsoleMenu` creates the wallet, tries loading the save, and shows the three choices. `readChoice` checks the main-menu answer. It returns 0 for a bad choice, -1 for end of input, and -2 for a read error.

`runWalletMenu` handles viewing, adding, removing, saving, and reloading. `askNumber` repeats a wallet question until it gets a valid answer or input ends. `showLoadResult` explains load failures, and `trySave` prints success only after saving succeeds.

The `changed` flag tracks unsaved edits. Add and remove set it; save and successful reload clear it. Back keeps the open list. Exit offers save, discard, or keep editing. A failed save returns to the menu. `finishInput` closes on end of input and warns if changes were not saved.

## The Windows window

The Win32 code moved from `main.c` into `window.c`. It still uses the same window creation and message-loop approach. `WindowControls` groups the handles for the labels, lists, and buttons.

`loadWindowWidgets` creates the controls and fills the catalog. It stores the real card ID with each list entry, so selecting a row does not change which card will be saved. `showCardDetails` fills the right panel with the selected name, rate, network, reward type, fee, and bonuses. Joe's existing text-formatting functions now return the shorter detail text.

`fillWalletList` reads the current wallet into the lower list. `updateButtons` shows the count and disables adding a card that is already saved. Remove stays disabled until a wallet card is selected.

`handleWindowAction` receives button clicks and list selections through `WM_COMMAND`. It calls the same `addWalletCard`, `removeWalletCard`, `saveWallet`, and `loadWallet` functions as the console. The window does not have a separate saving system.

`reloadFromWindow` asks before discarding unsaved edits. `saveFromWindow` clears the changed flag only after saving succeeds. Closing with edits asks whether to save; a failed save keeps the window open.

`onWindowResize` moves the controls when the window changes size. A minimum size keeps the panels and footer from overlapping. The message loop uses `IsDialogMessageW` for moving between controls with Tab. Fonts and brushes are released after the window closes.

For Oliver's window section, the font is Tahoma. The title and rate are smaller now, and section headings use bold text. The background is light cream, with dark gray writing and a white card panel. The sentence at the top explains how to choose a card. These are appearance changes; the wallet buttons still call the same functions.

The new layout and button behavior still need a real Windows desktop check. The Linux tests check the shared wallet code and the command arguments; the cross-build checks that the Windows code compiles and links.

## Checking it

The Visual Studio project compiles all seven application files as C11. The separate check programs are C too. `testGate1.c` and `testGate2.c` run the application with temporary save files and compare the output and saved bytes with expected results. `testModules.c` calls the card functions directly. `demoGate2.c` records the three-run restart demonstration.

`testSupport.c` handles temporary files, input pipes, captured output, and the five-second limit. Windows uses `CreateProcessA`; POSIX systems use `fork`, `execv`, and `waitpid`. Those are operating-system functions called from C. They are needed because standard C does not supply a process API with redirected input and a timeout.

The three logs keep actual build, C test, and demonstration results. The C runners print expected results and observations themselves. No script interpreter is needed. Windows runtime is still unverified. The Linux sanitizer run had leak checking disabled.
