# CardClarity, Gate 2

CardClarity lets us choose card types, keep them in a wallet, and save the list for next time. The Windows window now has a catalog, a card details panel, and a wallet with working buttons. The five sample cards and their rates are made up for practice, not real bank offers.

## Open and run

Extract the whole folder. Open `TestSolution.slnx` in Visual Studio, choose Debug and x64, then press Ctrl+Shift+B to build. Press Ctrl+F5 to open the window. The project sets `--gui` as the debugger argument. If the solution will not open, open `CreditCardThing/CreditCardThing.vcxproj` instead.

We use C11. Visual Studio needs the Desktop development with C++ workload and a Windows SDK. The project uses v145. If that toolset is missing, choose an installed one in the project's General properties and rebuild. Visual Studio 2022 normally uses v143.

In the window, choose a sample card on the left to see its example rate and bonuses on the right. Add to my wallet adds it to the lower list. Select a wallet card to remove it. Save cards keeps the list; Reload saved returns to the last save. Reload asks before discarding unsaved changes, and closing with edits asks whether to save. The window can be resized, and Tab moves between its lists and buttons.

The default save is `savedCards.txt` next to this readMe when running from Visual Studio. It contains card IDs, not payment details. The window and console use the same file format.

Running the executable without arguments opens the three-option console menu. `--gate1` opens the original Gate 1 menu.

`docs/demoSteps.md` has the steps for the gate check. `docs/codeGuide.md` explains the code, and `docs/teamWork.md` lists our assigned parts and the size of the changes.

## Keep a Windows build log

From this folder in Developer Command Prompt for Visual Studio:

```bat
if not exist build mkdir build
msbuild CreditCardThing\CreditCardThing.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /p:OutDir=..\build\windows\ /fl /flp:logfile=docs\windowsBuildLog.txt
build\windows\CreditCardThing.exe --gui
py -3 tests\testGate2.py build\windows\CreditCardThing.exe > docs\windowsTestLog.txt 2>&1
```

The tests use temporary folders and leave the normal save alone. A successful Windows run should have 37 passes and two skipped checks for other systems.

## Build on Linux

```sh
mkdir -p build
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror CreditCardThing/main.c CreditCardThing/cards.c CreditCardThing/logger.c CreditCardThing/input.c CreditCardThing/wallet.c CreditCardThing/storage.c CreditCardThing/window.c -o build/cardClarity
./build/cardClarity
python3 tests/testGate2.py build/cardClarity
python3 tests/testGate1.py build/cardClarity
```

Use `clang` instead of `gcc` for the other compiler. `python3 tests/recordGate2.py` repeats the builds and saves the build, test, and restart-demo logs. It also tries Clang and MinGW-w64 if installed. `--sanitize` adds address and undefined-behavior checks. `--no-leak-check` disables leak checking when the environment cannot run it.

The saved results are Linux runs. The Windows cross-build passed, but Visual Studio and Windows runtime still need to be checked on the demo computer. Leak checking was disabled for the sanitizer run.

`--wallet-file path` chooses another save for the console. Use `--gui --wallet-file path` to choose one for the window. The window keeps its startup logger, while the console starts straight at the menu.

Earlier Gate 1 records remain in the original Gate 1 ZIP and Git history. The unused C++ starter and `pagereader.c` are still present from the uploaded project; the build uses the seven C files in the command above.
