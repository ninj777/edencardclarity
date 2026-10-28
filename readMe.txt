CardClarity, Gates 1 and 2

All application code, C checks, and the restart demonstration are C11.
No HTML or CSS is used. That is within the proposal's 5% maximum.
The five card names and rates are invented practice data, not bank offers.
The save contains card IDs, not payment details.

The explanations are in the code

Oliver's assigned menu, feature checklist, gate requirements, and build notes:
CreditCardThing/main.c, window.c, window.h, logger.c, and logger.h.

Eden's assigned input, wallet, and saved-file explanations:
CreditCardThing/input.c, input.h, wallet.c, wallet.h, storage.c, and storage.h.
The example file format and loading/saving rules are at the top of storage.c.

Joe's assigned card records, types, stable IDs, and practice formula:
CreditCardThing/cards.c and cards.h. The exact sample values are in initTestCards.

Evan's assigned expected/actual cases and test support:
tests/testGate1.c, testGate2.c, testModules.c, testSupport.c, testSupport.h,
and demoGate2.c. The real teammate-review record is at the top of testGate1.c.

Open and run in Visual Studio

Extract the whole folder. Open TestSolution.slnx, choose Debug and x64, then
press Ctrl+Shift+B to build. Press Ctrl+F5 to open the window. If the solution
will not open, open CreditCardThing/CreditCardThing.vcxproj instead.

Visual Studio needs a Windows SDK and its native desktop compiler workload.
Microsoft calls that installer item Desktop development with C++; this project
still compiles every application file as C. The project uses toolset v145.
If it is missing, select an installed toolset in the project's General settings.
Visual Studio 2022 normally uses v143. Rebuild after changing the toolset.

In the window, select a sample on the left to see its example details. Add to
my wallet puts it in the lower list. Select a wallet card before removing it.
Save cards keeps the list; Reload saved restores the last save. The window
asks before discarding unsaved changes. Tab moves between controls.

The default save is savedCards.txt in the project root when launched from
Visual Studio. That is the folder containing this readMe.txt.

Running the executable without arguments opens the console menu.
--gate1 selects the earlier three-option starter.
--wallet-file path selects another save for the console.
--gui opens the window with the default save.
--gui --wallet-file path selects another save for the window.

Windows build and C checks

Open Developer Command Prompt for Visual Studio in the project root.
Run the following commands one at a time. Check each build and exit status.

if not exist build mkdir build
msbuild CreditCardThing\CreditCardThing.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /p:OutDir=..\build\windows\ /fl /flp:logfile=docs\windowsBuildLog.txt
cl /nologo /std:c11 /TC /W4 /D_CRT_SECURE_NO_WARNINGS tests\testGate1.c tests\testSupport.c /Febuild\testGate1.exe /Fobuild\
cl /nologo /std:c11 /TC /W4 /D_CRT_SECURE_NO_WARNINGS tests\testGate2.c tests\testSupport.c /Febuild\testGate2.exe /Fobuild\
cl /nologo /std:c11 /TC /W4 /D_CRT_SECURE_NO_WARNINGS tests\demoGate2.c tests\testSupport.c /Febuild\demoGate2.exe /Fobuild\
cl /nologo /std:c11 /TC /W4 /D_CRT_SECURE_NO_WARNINGS tests\testModules.c CreditCardThing\cards.c CreditCardThing\logger.c /Febuild\testModules.exe /Fobuild\
build\testGate1.exe build\windows\CreditCardThing.exe > docs\windowsGate1Log.txt 2>&1
build\testGate2.exe build\windows\CreditCardThing.exe > docs\windowsGate2Log.txt 2>&1
build\testModules.exe > docs\windowsModuleLog.txt 2>&1
build\demoGate2.exe build\windows\CreditCardThing.exe > docs\windowsDemoLog.txt 2>&1
build\windows\CreditCardThing.exe --gui

The tests use temporary folders rather than the normal saved wallet. On Windows,
expect 29 passes and two skips for Gate 1, and 37 passes and two skips for Gate 2.
Those are expected totals, not a completed Windows run. The skipped cases need
Linux or check the unsupported-window message on another system.

Linux build and C checks

Run these from the project root. Use clang instead of gcc for another compiler.

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

Each test runner prints its date, expected results, actual observations, and
passed/failed/skipped counts. A failed check returns a nonzero exit status.
The process support stops a program that takes longer than five seconds.
Redirect the output to a file to keep another run. Keep the compiler output too.

What to show for Gate 1

Run build\windows\CreditCardThing.exe --gate1 on Windows, or
./build/cardClarity --gate1 on Linux. Show the three numbered choices.
Enter 1 to display all five samples. Enter hello to show a rejected answer,
then 1 to show that valid input still works. Enter 2 to show the feature plan.
Enter 3 to show normal exit. Keep the build output and expected/actual results.
The feature checklist and assigned modules are at the top of main.c.
A real teammate needs to complete the review record in tests/testGate1.c
for a possible Gate 1 rating of 4. The teacher determines the actual rating.

What to show for Gate 2

Use a separate demo file and the same path on every restart. On Windows:

if not exist build\demo mkdir build\demo
build\windows\CreditCardThing.exe --wallet-file build\demo\demoCards.txt

If that file already exists, inspect it or choose a new demo name. Open the
wallet with 2. Add with 2, choose card 1, then add card 3 the same way. View
with 1 and save with 4. Go Back with 6, then Exit with 3.

Run the same command again. View both cards, remove card 1 with option 3,
and save again. Reload with 5 and view the list. Only Cashback Sample should
remain. Exit and restart again to show that the removal stayed saved.

Try a bad menu answer and a duplicate card. For a damaged-file example, use
a different file containing these four lines:

cardClarityWallet1
2
1
1

That duplicate-ID file should be rejected without crashing. Do not save over
the fixture during the demonstration. A full list, missing file, bad count,
and failed save are also covered by the C cases.

To show the window with the same demo save:

build\windows\CreditCardThing.exe --gui --wallet-file build\demo\demoCards.txt

Check Add, Remove, Save cards, Reload saved, restarting, resizing, Tab/Space,
and the unsaved close/reload prompts. The window uses the same storage code.

Evidence and checks still needed

docs/gate2BuildLog.txt contains actual commands, compiler versions, and results.
docs/gate2TestLog.txt contains actual C runner observations and module results.
docs/gate2DemoLog.txt contains the three-process save/remove/restart output.
The recorded native builds passed 31 Gate 1 cases, 39 Gate 2 cases, 40 module
checks, and the restart demo on GCC, Clang, and the checked build. Leak checking
was disabled for that checked build. The Windows application and C check
programs cross-compiled; Visual Studio and Windows runtime remain unverified.

Record the demo computer's actual results after running it:
Name and date: _________________________________________________________
Computer and compiler: _________________________________________________
Build result and saved log: ____________________________________________
Gate 1 startup/samples/invalid input/exit: ______________________________
Gate 2 add/view/remove: ________________________________________________
Save over an existing file: ____________________________________________
Restart and reload: ___________________________________________________
Damaged file: _________________________________________________________
Window buttons, text, resize, and keyboard: ____________________________
Unsaved-close and reload prompts: ______________________________________
Any bug, change, and actual retest: _____________________________________

The proposal's four-student/six-investigator exception still needs staff
confirmation. A passing test count does not supply that approval or a review.
The later Gate 3 QA report and Gate 4 benefits/comparison work are still separate
milestones. The current program does not yet complete the full purchase tool.

Current assigned source sizes, including these comments

Oliver: 887 lines across the assigned source and headers.
  CreditCardThing/main.c: 318
  CreditCardThing/window.c: 478
  CreditCardThing/window.h: 14
  CreditCardThing/logger.c: 56
  CreditCardThing/logger.h: 21
Eden: 479 lines across the assigned source and headers.
  CreditCardThing/input.c: 113
  CreditCardThing/input.h: 29
  CreditCardThing/wallet.c: 105
  CreditCardThing/wallet.h: 39
  CreditCardThing/storage.c: 165
  CreditCardThing/storage.h: 28
Joe: 262 lines across the assigned source and headers.
  CreditCardThing/cards.c: 181
  CreditCardThing/cards.h: 81
Evan: 1,437 lines across the assigned source and headers.
  tests/testGate1.c: 299
  tests/testGate2.c: 437
  tests/testModules.c: 69
  tests/testSupport.c: 520
  tests/testSupport.h: 56
  tests/demoGate2.c: 56

These are file sizes, including comments and blank lines, not personal
authorship or time measurements. The pre-annotation snapshot is in main.c.
