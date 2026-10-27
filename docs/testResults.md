# Gate 2 checks

The recorded results below come from the compiled C test runners on Linux. No student's personal run or review is filled in for them.

1. Add: expected card 3 to add Cashback Sample once. Actual: it appeared in the wallet and saved as ID 3. Adding it again showed the duplicate message. Passed.

2. View: expected only the selected cards and example rates. Actual: the selected names and rates appeared. An empty list showed the empty-wallet message. Passed.

3. Remove: expected removing card 3 from [1, 3, 5] to leave [1, 5]. Actual: the saved file contained those two IDs in order, and they stayed after restarting. Passed.

4. Save: expected a header, count, and the selected IDs. Actual: the saved bytes matched. Saving an empty list and replacing an older save also worked. Passed.

5. Reload: expected the last saved list to return. Actual: a second process loaded cards 1 and 3. Reloading after an unsaved edit restored the saved list. Passed.

The separate demo in `gate2DemoLog.txt` shows all five actions across three runs. The last run has only Cashback Sample after Travel Sample was removed and the change was saved.

The other checks cover a full wallet, duplicates, cancelled actions, bad menu answers, long input, hidden null characters, end of input, and the three unsaved-exit choices. A failed save leaves the current list usable. A failed Save and exit returns to the menu.

The damaged-file check tries 22 examples: empty file, wrong header, missing count, negative count, too many cards, huge count, text count, decimal count, zero ID, unknown ID, negative ID, duplicate ID, missing ID, extra ID, extra blank line, null characters in three different places, long header, long ID, a byte-order mark, and an ID followed by letters. All were rejected without rewriting the file. Another test damaged the file after startup and confirmed the open wallet stayed unchanged after Reload.

Valid Windows line endings and a missing final newline worked. The file-error tests covered a directory used as a save file, a missing folder, an existing temporary file, and a failed replacement. They reported failure and kept existing data in place.

GCC 14.2.0, Clang 19.1.7, and Clang with address and undefined-behavior checks each passed the C runners: 39 Gate 2 cases, 31 Gate 1 cases, 40 module checks, and the three-process demonstration. The module checks include the five card mappings, their values, and the earlier practice calculations. The extra command-line check accepts a chosen file for `--gui` and reports that Windows is needed when run on Linux. Leak checking was disabled. The Windows application and all four separate C check programs cross-compiled; Windows runtime and Visual Studio are still unverified.

The exact commands and output are in `gate2BuildLog.txt` and `gate2TestLog.txt`. The tests are repeatable with the commands in the readMe.

## Demo-computer notes

Fill these in after running the checks on the actual computer. Keep its build output with this sheet.

Name and date: ____________________
Computer and compiler: ____________________
Build result and log: ____________________
Add and view result: ____________________
Remove result: ____________________
Save over an existing file result: ____________________
Restart and reload result: ____________________
Damaged-file result: ____________________
Window add/remove/save/reload result: ____________________
Resize and Tab navigation result: ____________________
Unsaved close/reload prompt result: ____________________
Any bugs and retest results: ____________________

On Windows, expect 37 passes and two skipped Gate 2 cases. One skip is the Linux read-error check, and the other checks the unsupported-window message on other systems. Those skips do not verify the Windows controls.
