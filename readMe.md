# CardClarity, Gate 2

CardClarity lets us choose card types, keep them in a wallet, and save the list for next time. The Windows window has a catalog, card details, and wallet buttons. All five cards and their rates are made up for practice, not real bank offers.

All application code, test runners, and the restart demonstration are C. There are no other programming-language source files in the current project. HTML and CSS are unused, so they are below the allowed 5% maximum. Visual Studio's XML project files and the Markdown instructions are build settings and documentation.

## Open and run

Extract the whole folder. Open `TestSolution.slnx` in Visual Studio, choose Debug and x64, then press Ctrl+Shift+B to build. Press Ctrl+F5 to open the window. The project sets `--gui` as the launch argument. If the solution will not open, open `CreditCardThing/CreditCardThing.vcxproj` instead.

We use C11. Visual Studio needs its native desktop compiler workload and a Windows SDK. Microsoft calls that installer workload **Desktop development with C++**; the name does not mean this project uses that language. Every application file is compiled as C. The project uses v145. If that toolset is missing, choose an installed one in the project's General properties and rebuild. Visual Studio 2022 normally uses v143.

Select a sample card to see its example rate and bonuses. Add to my wallet puts it in the lower list. Select a wallet card to remove it. Save cards keeps the list; Reload saved returns to the last save. The window asks before discarding unsaved changes when reloading or closing. Tab moves between the lists and buttons.

The default save is `savedCards.txt` next to this readMe when running from Visual Studio. It contains card IDs, not payment details. The window and console use the same file format.

Running the executable without arguments opens the three-option console menu. `--gate1` opens the original Gate 1 menu. `--wallet-file path` chooses another save for the console; `--gui --wallet-file path` does that for the window.

## Documentation

- [Gate 1 requirements and review record](docs/gate1Report.md).
- [Gate 2 requirements and evidence](docs/gate2Report.md).
- [Demonstration steps](docs/demoSteps.md).
- [How the code connects](docs/codeGuide.md).
- [Oliver: menus, window, and build](docs/oliverWork.md).
- [Eden: input, wallet, and saved files](docs/edenWork.md).
- [Joe: sample records, IDs, and practice math](docs/joeWork.md).
- [Evan: C tests, results, and demo checks](docs/evanWork.md).
- [Assigned file sizes and code changes](docs/codeChanges.md).

## Windows build and C checks

Open Developer Command Prompt for Visual Studio in this folder. These commands build the application and the separate C check programs:

```bat
if not exist build mkdir build
msbuild CreditCardThing\CreditCardThing.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /p:OutDir=..\build\windows\ /fl /flp:logfile=docs\windowsBuildLog.txt
cl /nologo /std:c11 /TC /W4 /D_CRT_SECURE_NO_WARNINGS tests\testGate1.c tests\testSupport.c /Febuild\testGate1.exe /Fobuild\
cl /nologo /std:c11 /TC /W4 /D_CRT_SECURE_NO_WARNINGS tests\testGate2.c tests\testSupport.c /Febuild\testGate2.exe /Fobuild\
cl /nologo /std:c11 /TC /W4 /D_CRT_SECURE_NO_WARNINGS tests\demoGate2.c tests\testSupport.c /Febuild\demoGate2.exe /Fobuild\
cl /nologo /std:c11 /TC /W4 /D_CRT_SECURE_NO_WARNINGS tests\testModules.c CreditCardThing\cards.c CreditCardThing\logger.c /Febuild\testModules.exe /Fobuild\ /link ole32.lib
build\testGate1.exe build\windows\CreditCardThing.exe > docs\windowsGate1Log.txt 2>&1
build\testGate2.exe build\windows\CreditCardThing.exe > docs\windowsGate2Log.txt 2>&1
build\testModules.exe > docs\windowsModuleLog.txt 2>&1
build\demoGate2.exe build\windows\CreditCardThing.exe > docs\windowsDemoLog.txt 2>&1
build\windows\CreditCardThing.exe --gui
```

Check the compiler output and each program's exit status before continuing. The tests make their own temporary folders and leave the normal saved wallet alone. Gate 1 should have 29 passes and two skips on Windows. Gate 2 should have 37 passes and two skips. The skipped checks need Linux or test the unsupported-window message on other systems. Those are expected Windows totals, not results already observed.

## Linux build and C checks

From the project root:

```sh
mkdir -p build
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror CreditCardThing/main.c CreditCardThing/cards.c CreditCardThing/logger.c CreditCardThing/input.c CreditCardThing/wallet.c CreditCardThing/storage.c CreditCardThing/window.c -o build/cardClarity
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/testGate1.c tests/testSupport.c -o build/testGate1
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/testGate2.c tests/testSupport.c -o build/testGate2
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/testModules.c CreditCardThing/cards.c CreditCardThing/logger.c -lm -o build/testModules
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror tests/demoGate2.c tests/testSupport.c -o build/demoGate2
./build/cardClarity
./build/testGate1 ./build/cardClarity
./build/testGate2 ./build/cardClarity
./build/testModules
./build/demoGate2 ./build/cardClarity
```

Use `clang` in place of `gcc` for the other compiler. To save the actual results, redirect the two menu checks and module checks to a test log, and the demonstration to a demo log. Each C runner prints the date, expected results, observed checks, and its final counts. A failed check returns a nonzero status. The support code stops a program that takes longer than five seconds.

The recorded C runs passed on GCC and Clang, including address and undefined-behavior checks. Leak checking was disabled for the checked run. The Windows application and all C check programs cross-compiled, but Visual Studio and Windows runtime still need checking on the demo computer.

The current folder contains seven application C files, their headers, the C tests, instructions, and actual build/test/demo logs. The older unused `pagereader.c` stub is preserved but not compiled. Historical versions remain in Git; the current source package contains no old scripts or C++ starter.
