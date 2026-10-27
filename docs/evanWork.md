# Evan: tests and demonstration checks

Evan Zhuang is assigned the Gate 1 test cases and the five Gate 2 user-action checks. The proposal also gives Evan menu clarity, bug notes, the first QA review at Gate 3, and help with the final demo.

The current test source is `testGate1.c`, `testGate2.c`, `testModules.c`, `demoGate2.c`, `testSupport.c`, and `testSupport.h`. There are 1,285 C source and header lines, counting comments and blank lines. The earlier interpreter-based runners were replaced so the whole current code folder uses C. The exact changes are in [codeChanges.md](codeChanges.md).

## Gate 1 checks

These results are from the recorded Linux builds and tests. They are not filled-in results from a student's demo computer.

1. Compilation. Expected: C11 build with warnings enabled and no errors. Actual: GCC and Clang builds completed with no warnings or errors. The Windows cross-build also completed. Evidence: [gate2BuildLog.txt](gate2BuildLog.txt).
2. Startup. Input: `3`. Expected: the three numbered Gate 1 choices appear. Actual: View sample cards, About CardClarity, and Exit appeared. Case 1 also checks that there are exactly three choices.
3. View cards. Input: `1`, then `3`. Expected: at least three sample names and example rates, then exit. Actual: all five named samples and their rates appeared. Cases 2, 3, and 4 check the names, exact values, and example labels.
4. Invalid input. Input: `hello`, then `3`. Expected: a retry message and a usable menu. Actual: the message asked for 1, 2, or 3, and the next choice exited normally. This is test 7.
5. Exit. Input: `3`. Expected: Goodbye and exit status 0. Actual: Goodbye appeared and the process returned 0. Case 21 also confirms input after Exit is not processed.

Extra check: `not a choice`, then `1`, then `3`. Expected: reject the bad answer, show the cards, and exit. Actual: all three steps worked. This is test 16. Gate 1's rating 4 also requires a real teammate code review; a passing extra test does not replace it.

The complete Gate 1 runner has 31 checks. It launches the current program with `--gate1`. It also covers blank answers, negatives, zero, decimals, trailing text, oversized numbers, long lines, hidden null characters, whitespace, repeated choices, and end of input.

## Gate 2: the five actions

Add: test 3 adds ID 3. Expected and recorded actual: Cashback Sample appears once and saves as ID 3. Test 4 rejects a duplicate.

View: tests 1 and 3 check empty and selected wallets. Expected and recorded actual: an empty message for no cards, or the selected names and example rates.

Remove: test 5 removes ID 3 from `[1, 3, 5]`. Expected and recorded actual: the saved IDs are `[1, 5]` in that order. Other tests remove the first card, last card, absent card, and final remaining card.

Save: the tests read the actual saved bytes. Expected: the header, exact count, and selected IDs. Actual: those bytes matched. Saving zero cards and replacing an older save worked in the Linux runs.

Reload: test 9 starts another process after saving IDs 1 and 3. Expected and recorded actual: both load after restart. Test 12 checks that Reload returns to the last save after an unsaved change. Test 10 checks that a saved removal stays removed after restart.

The [three-run demo log](gate2DemoLog.txt) shows the same idea with complete program output: save IDs 1 and 3, restart, remove ID 1 and save, then restart with only ID 3. Seeing a card in the first process alone would not prove persistence.

## Bad input and file checks

The 39 Gate 2 tests also cover a full list, cancellation, unsaved exit choices, a chosen save path, and failure to save. A failed Save and exit must return to the menu rather than closing as if saving worked.

Test 25 has 22 damaged-file examples. They include wrong or missing headers, invalid counts, unknown and duplicate IDs, missing records, extra records, hidden null characters, long lines, and a byte-order mark. All 22 were rejected in the recorded runs. These are subcases of one test, not 22 extra named tests.

Test 33 starts with a valid loaded wallet, damages its file while the program is open, and chooses Reload. Expected and recorded actual: the reload fails and the open wallet is unchanged.

Tests 28 through 32 check file errors such as a directory in place of a save, a missing destination folder, an existing `.tmp` file, and a replacement that cannot complete. The checks also confirm that existing data is not silently overwritten.

## How the tests run

The C runners start the compiled application as a separate process. `testSupport.c` creates a temporary folder, opens an input pipe, and captures standard output and errors in files inside that folder. It sends the test answers, waits up to five seconds, and records the exit status. An overlong run is stopped and marked failed. Temporary folders are removed after each case, leaving the normal saved wallet alone.

`expect` checks one condition and prints the actual yes-or-no observation. `finishCase` records whether every check matched. `finishTests` reports passed, failed, and skipped cases and returns a failure status if anything failed. Hidden-null tests pass a byte count rather than using `strlen`, so the test input is not accidentally cut short.

Windows uses `CreateProcessA`, inheritable input/output handles, and `WaitForSingleObject`. POSIX systems use `pipe`, `fork`, `execv`, and `waitpid`. Both branches are C. The platform code is grouped in the support file because the menu cases should mainly describe answers and expected behavior.

Case 33 leaves the input pipe open, waits until the initial saved list has loaded, changes the file, then sends the Reload answer. It checks a live running program, rather than only testing the file-loading function in isolation.

`testModules.c` calls the card functions directly. It has 35 checks across the five records, two invalid-ID checks, and three practice reward checks, for 40 total. Some headers are deliberately included twice to check that their include guards prevent repeated definitions.

`demoGate2.c` runs the save-and-reload demonstration in three separate processes. It prints the answers, full program output, exit statuses, and checked saved bytes. Compiler commands are recorded in the build log. The menu runners and module program produce the test log. No separate build script or interpreter is required in the project.

From the project root after building:

```sh
./build/testGate1 ./build/cardClarity
./build/testGate2 ./build/cardClarity
./build/testModules
./build/demoGate2 ./build/cardClarity
```

The C compiler commands are in the [readMe](../readMe.md). Redirect output to a log to keep a run. Check its exit status too; printing a result does not make a failed check pass.

## What has actually been checked

The recorded GCC, Clang, and checked Clang builds each passed the new C runners: 31 Gate 1 cases, 39 Gate 2 cases, 40 module checks, and the three-process demonstration. Leak checking was disabled in the checked build. The Windows application, both menu runners, module checks, and demo all cross-compiled. Their Windows executables have not been run here.

Windows runtime is still unverified. On Windows, the runners skip checks that require Linux or check the unsupported-window message on other systems. Expected totals there are 29 passes and two skips for Gate 1, and 37 passes and two skips for Gate 2. Those are expected totals, not completed Windows results.

## Checks still needed on the demo computer

Build with Visual Studio and keep its real output. Run the console and window. Try all five wallet actions, close and reopen after saving, and save over an existing file. Check the resize behavior, readable labels, Tab navigation, and unsaved-change prompts.

Record what happened in [testResults.md](testResults.md). For a bug, include the input, expected result, actual result, file or function involved, and the result after a fix. Do not mark it fixed from a code edit alone; repeat the failing steps.

Gate 3 is the next QA gate. It needs at least 15 recorded results, a bug list, and retest notes. The current test count helps, but it does not complete the future QA report or its sign-off by itself.
