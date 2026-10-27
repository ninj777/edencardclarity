# Gate 2

Due: Wednesday, October 28, 2026. Lead: Eden Yun Yu.

Our program now adds, views, removes, saves, and reloads card types. A saved list comes back when we restart. We kept the five sample records and updated the existing Windows display with wallet controls.

The window has a catalog on the left, a details panel on the right, and the selected cards below the catalog. Add, remove, save, and reload buttons call the same wallet and storage functions as the console. The three-option console menu also remains available. The wallet holds up to five unique card types. Bad answers get a retry message. A missing save starts a new list. A damaged file is rejected, and a failed reload keeps the cards already open.

The save file contains a format name, a count, and card IDs. We write a new temporary file before replacing an old save. The full format is explained in `savedCardsFormat.md`.

Our assigned jobs are in `teamWork.md`: Eden handles input and storage, Oliver connects the menu and build, Joe checks card numbers, and Evan handles the tests. The file counts show the size of each assigned part. Personal review notes are left for each person to fill in.

The test scripts passed 39 Gate 2 cases, 31 Gate 1 checks, and 40 module checks on GCC, Clang, and the sanitizer build. The damaged-file case includes 22 examples. A separate three-run demo saved two cards, restarted, removed one, saved, and restarted again with the correct remaining card.

The evidence we need is kept in three files:

- `gate2BuildLog.txt`: compiler versions, commands, and build results.
- `gate2TestLog.txt`: the actual test results.
- `gate2DemoLog.txt`: the actual save, restart, remove, and reload runs.

`testResults.md` explains the expected and actual results. `demoSteps.md` is our guide for showing them in person.

The C11 builds had no warnings or errors. The Windows cross-build passed too. A real Visual Studio build and Windows run are still needed, including saving over an existing file resizing the window, and checking its buttons, keyboard navigation, and close prompt. The window has cross-compiled successfully but has not been run on a Windows desktop here. Leak checking was disabled for the sanitizer run.

The extra damaged-file tests meet the extra test part of the Gate 2 rubric. The teacher decides the rating. Staff still need to confirm the four-student staffing exception in our proposal.
