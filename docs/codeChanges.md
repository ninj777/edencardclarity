# Code amounts by assigned part

The file groups follow the proposal's jobs. They show how much code is in each assigned area and how those files changed. They do not establish who personally typed each line or how long anyone worked. Comments, blank lines, and moved code count in these numbers.

The original upload already contained card records and a Windows window. The counts do not treat that starting code as newly written for Gate 2.

## Checkpoints used

- `4299578`: the uploaded final version before the Gate 1 completion changes.
- `5a22251`: the completed Gate 1 starter and evidence.
- `1832702`: Gate 2, including the smaller window fonts and updated colors.

The local Git bundle keeps these checkpoints. The new repository's initial project upload is `72a433b`, and its font follow-up is `39b06a4`. Those uploads have the same project contents as the corresponding local versions, but they are separate commits in the new repository's history.

## Oliver's assigned source

Current total: 734 lines of C source and headers.

- `main.c`: 236 lines. Gate 2 difference: 173 added, 391 removed.
- `window.c`: 432 lines. Gate 2 difference: 432 added, 0 removed.
- `window.h`: 7 lines. Gate 2 difference: 7 added, 0 removed.
- `logger.c`: 45 lines. Gate 2 difference: 6 added, 0 removed.
- `logger.h`: 14 lines. Gate 2 difference: 1 added, 4 removed.

Gate 1 difference across this group: 184 added and 46 removed. Gate 2 difference: 619 added and 395 removed.

The large removal in `main.c` includes separating the old window from the menu. The new `window.c` also changes the layout and connects wallet buttons. Adding 432 lines in a new file therefore does not mean every one of them was a new idea or a new feature.

The Visual Studio project, filters, solution, Git settings, readMe, and submission package are also assigned to Oliver. They are outside these C line counts.

## Eden's assigned source

Current total: 316 lines of C source and headers.

- `input.c`: 65 lines, all added after Gate 1.
- `input.h`: 21 lines, all added after Gate 1.
- `wallet.c`: 75 lines, all added after Gate 1.
- `wallet.h`: 31 lines, all added after Gate 1.
- `storage.c`: 104 lines, all added after Gate 1.
- `storage.h`: 20 lines, all added after Gate 1.

Gate 2 difference: 316 added and 0 removed in these six files.

These files did not exist at the Gate 1 checkpoint. That does not mean input checking was absent in Gate 1: its `readChoice` function was inside `main.c`. Input checking and the saved-file plan belong to Eden's Gate 1 assignment, while the file-level count for that combined starter is listed under Oliver's `main.c`. It is not counted a second time here.

## Joe's assigned source

Current total: 257 lines of C source and headers.

- `cards.c`: 185 lines. Gate 2 difference: 13 added, 9 removed.
- `cards.h`: 72 lines. Gate 2 difference: 4 added, 0 removed.

Gate 1 difference: 55 added and 19 removed. Gate 2 difference: 17 added and 9 removed.

Gate 2 specifically needs a reliable ID-to-record mapping. The small later difference reflects `getCardById`, its declaration, and shorter Windows detail text. Keeping and checking an existing catalog is part of this assignment even when it does not create many new lines. The practice formula and most sample-record setup predate Gate 2.

## Evan's assigned test source

Current total: 822 lines: 765 Python and 57 C.

- `testGate1.py`: 208 lines. Gate 2 difference: 2 added, 2 removed.
- `testGate2.py`: 444 lines. Gate 2 difference: 444 added, 0 removed.
- `testModules.c`: 57 lines. Gate 2 difference: 4 added, 1 removed.
- `recordGate2.py`: 113 lines. Gate 2 difference: 113 added, 0 removed.

Gate 1 difference: 379 added and 0 removed across the tests and the older recorder. Gate 2 difference: 563 added and 120 removed across the whole `tests` folder. Of those removals, 117 are the retired `recordGate1.py`, whose job is covered by the newer recorder. That file remains in Git history rather than being duplicated in the current folder.

Test logs and written results are assigned here too, but generated output and documentation are not counted as test source. The Python files run tests; the application itself is C.

## Repeating the count

From the original local history or supplied bundle, this command shows the Gate 1 changes:

```sh
git diff --numstat 4299578 5a22251 -- CreditCardThing tests
```

This command shows the Gate 2 changes through the font update:

```sh
git diff --numstat 5a22251 1832702 -- CreditCardThing tests
```

Read the first two columns as lines added and removed. The counts above group only the listed source files; project settings, unused starter files, and generated logs are excluded. Adding and removing a line during an edit can count in both columns.
