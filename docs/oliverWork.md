# Oliver: menus, window, and build

Oliver Dolfin is the project lead and the Gate 1 primary investigator. The proposal assigns the main menu, connecting the C files, and preparing the build to Oliver. This section explains that part of the program through Gates 1 and 2. The exact file-change counts are in [codeChanges.md](codeChanges.md).

## Gate 1: getting the starter running

The starter needed a three-option menu, at least three named sample cards, input checks, and a working exit. The catalog contains five examples, so there was no reason to reduce it to three. The original Windows display was kept alongside the console menu.

The current program still has the Gate 1 menu. Running with `--gate1` gives these choices:

1. View sample cards
2. About CardClarity
3. Exit

The About option lists the four planned features. Gate 1 only needed the starter and the plan; it did not require the whole rewards calculator or saving system. [gate1Report.md](gate1Report.md) records the requirements and the checks.

The menu repeats inside a `for (;;)` loop. Showing the cards finishes that action and returns to the next loop. Choosing Exit returns from the function, which lets the program finish. This is easier to follow than starting another copy of the menu function each time.

## What happens when the program starts

`main`, in [main.c](../CreditCardThing/main.c), calls `initTestCards` first. That prepares Joe's card records before the menus or window try to use them.

`argc` is the number of command-line arguments. `argv` holds their text. The code uses `strcmp` to recognize a small set of launch options:

- No arguments: the Gate 2 console menu.
- `--gate1`: the earlier starter menu.
- `--wallet-file path`: the Gate 2 menu with a chosen save location.
- `--gui`: the Windows window with the default save location.
- `--gui --wallet-file path`: the window with a chosen save location.

An unknown or incomplete option prints help and returns 1. Normal exit returns 0. The program does not silently ignore an incorrect option.

## Connecting the Gate 2 menu

The main menu still has three choices. Option 2 becomes My card wallet. That opens a separate six-choice menu: view, add, remove, save, reload, and back.

`runConsoleMenu` creates a `Wallet`, empties it with `initWallet`, and tries to load the chosen save. A missing file is allowed because a new user has not saved anything yet. `showLoadResult` explains whether loading worked or why it failed.

`runWalletMenu` chooses the action. It passes the wallet to Eden's functions instead of writing a second version of adding or saving inside the menu. For example, Add asks for an ID, calls `addWalletCard`, and prints a message based on the returned result. The card name comes from Joe's `getCardById`.

`askNumber` handles repeated questions in the wallet menu. It supplies the allowed minimum and maximum to `readNumber`. Invalid input gets another question. Zero is allowed when choosing a card because it means cancel; zero is not a card ID.

`readChoice` handles the three-choice main menu. It returns the valid choice, 0 for invalid input, -1 when input ends, or -2 for a read error. Those special negative values let the menu stop rather than asking forever after input has ended.

`trySave` prints success only after `saveWallet` returns success. It returns a `bool`, so the caller can decide whether closing is safe. A failed save is explained to the user and leaves the program available for another action.

## Unsaved changes

The `changed` variable starts as false. A successful addition or removal sets it to true. Saving or successfully reloading sets it back to false. Going Back keeps the current wallet and the flag.

When Exit is chosen with unsaved changes, the console offers Save and exit, Exit without saving, or Keep editing. If saving fails, the menu stays open. If input ends instead, `finishInput` warns about unsaved changes and closes. A real read error prints to `stderr` and returns 1.

Reload in the console is labeled as undoing unsaved changes and immediately loads the saved list. The Windows version also asks for confirmation before discarding edits. That is a difference between the two interfaces, not a difference in their saved-file format.

## How the window works

[window.c](../CreditCardThing/window.c) contains the Win32 window code that used to sit inside `main.c`. [window.h](../CreditCardThing/window.h) declares `runWindow`, so `main.c` can open it. The rest of the program remains C.

`CreateWindowW` opens the main window. `makeControl` uses `CreateWindowExW` to create labels, list boxes, and buttons inside it. Each control has a handle, which is the value Windows uses to identify that control. The `WindowControls` struct keeps those handles together.

`loadWindowWidgets` creates the controls and fills the sample-card list. Each row stores its catalog ID using `LB_SETITEMDATA`. The saved wallet uses that ID, not the row's current position or the card's random Windows identifier.

`showCardDetails` puts the selected name, base rate, network, reward type, example fee, and bonuses into the right-hand panel. `fillWalletList` rebuilds the selected-card list after a change. `updateButtons` disables Add for a duplicate or full wallet and disables Remove until a wallet card is selected.

Windows sends messages when something happens. `applicationWindowProc` handles them. `WM_COMMAND` covers button clicks and list selections. `WM_SIZE` calls `onWindowResize`. `WM_PAINT` draws the background and detail panel. `WM_CLOSE` checks unsaved changes before closing.

The message loop uses `GetMessageW`, `TranslateMessage`, and `DispatchMessageW` to keep the window responding. `IsDialogMessageW` lets Tab move through its controls. Fonts and brushes are deleted when the window closes.

The latest appearance changes use Tahoma, smaller titles and rates, bold section headings, a cream background, and dark gray text. These changes are in the same window file; they do not add another interface library. The window still needs to be opened on Windows to check the actual text fit and appearance.

## Build and handoff

The build compiles seven C files: `main.c`, `cards.c`, `logger.c`, `input.c`, `wallet.c`, `storage.c`, and `window.c`. Headers provide shared declarations. The unused C++ starter is preserved but excluded from this build.

The Visual Studio project sets C11, compilation as C, and warning level 3. Its Windows libraries include `ole32`, `user32`, and `gdi32`. The launch argument is `--gui`, so Ctrl+F5 opens the window. Running the executable without that argument opens the console.

The Linux command uses `-std=c11 -Wall -Wextra -Wpedantic -Werror`. Those options select C11 and treat compiler warnings as build failures. Full commands are in the [readMe](../readMe.md). The current Windows cross-build passed, but this does not establish that Visual Studio or the window ran successfully on the demo computer.

Before presenting, build on the actual computer, save its output, and run the [demo steps](demoSteps.md). Gate 1 also needs a real teammate code review for its possible top rating. The review fields are in the Gate 1 report. The teacher decides the rating.

Next work in this area is connecting the purchase calculator and comparisons. That requires Joe's checked values and restrictions, Eden's input checks, and Evan's expected-versus-actual cases. Those later features are not complete just because the older practice formula exists.
