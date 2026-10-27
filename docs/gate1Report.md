# Gate 1: starter program

Due Thursday, October 22, 2026. Primary investigator: Oliver Dolfin. These requirements come from the Gate 1 section of the project proposal. This report describes the preserved Gate 1 mode in the current project; it is not a backdated submission or a teacher acceptance record.

## Required feature checklist

Gate 1 asks for agreement on these four first-version features and an owner for each planned module:

- [x] Card wallet: add, view, and remove selected card types. Eden owns `wallet.c` and `storage.c`; Oliver connects them to the menu. Persistent management is the Gate 2 milestone.
- [x] Benefits display: show rewards and important conditions. Joe owns the card records in `cards.c` and `cards.h`; Evan checks whether the wording is clear. The later researched catalog belongs to Gate 4.
- [x] Rewards calculator: estimate rewards from an amount and category. Oliver handles calculation integration, and Joe checks the expected values. The current practice formula does not complete the later calculator.
- [x] Card recommendations: compare eligible saved cards for a purchase. Oliver handles the comparison code, with Joe checking the data and rules. This is still planned work.

The checks mean the features and owners are identified. They do not mean all four final features are finished.

## Assigned starter work

Oliver handles the main menu, connecting the files, build, and submission. Eden handles the input-checking rules and saved-card format plan. Joe handles at least three sample records and their example rates. Evan handles the five required expected-versus-actual checks, extra tests, and bug notes.

The detailed sections are [Oliver](oliverWork.md), [Eden](edenWork.md), [Joe](joeWork.md), and [Evan](evanWork.md). [codeChanges.md](codeChanges.md) separates the original upload, completed starter, and later changes.

## Starter behavior and source

The required `main.c`, `cards.c`, and `cards.h` are in `CreditCardThing`. The original starter also needs its logger. The current build includes the input, wallet, storage, and window modules added for Gate 2, so use the seven-file build command in the [readMe](../readMe.md).

Run the current executable with `--gate1`. It shows exactly three numbered options: View sample cards, About CardClarity, and Exit. The normal current launch opens Gate 2 instead; it also keeps a three-option top-level menu.

View sample cards shows Travel Sample, Points Sample, Cashback Sample, Everyday Sample, and Starter Sample. Their base rates are 1.25%, 1%, 1%, 1%, and 1%. The names, rates, fees, and bonuses are labeled as practice examples rather than verified offers. The full records are explained in [joeWork.md](joeWork.md).

The input checks reject text, blank answers, negatives, zero, choices above 3, decimals, extra characters, long answers, and hidden null characters. Valid input after a bad answer still works. Viewing cards returns to the menu. Exit prints Goodbye and returns 0. A genuine input-reading error returns 1 instead of claiming a normal exit.

## The five required results

These are recorded Linux results for the current C program. They are summarized in [evanWork.md](evanWork.md), with actual suite output in [gate2TestLog.txt](gate2TestLog.txt).

1. Compilation. Expected: C11 source builds with warnings enabled and no errors. Actual: GCC 14.2.0 and Clang 19.1.7 completed with no warnings or errors. The compiler commands and versions are in [gate2BuildLog.txt](gate2BuildLog.txt). Passed.
2. Menu startup. Action: start with `--gate1`, enter `3`. Expected: exactly three numbered options. Actual: all three starter choices appeared. Test 1 passed.
3. Sample display. Input: `1`, then `3`. Expected: at least three clearly named samples with example rates. Actual: all five names, rates, and the warning appeared. Tests 2 through 4 passed.
4. Invalid input. Input: `hello`, then `3`. Expected: a retry message, a usable menu, and no crash. Actual: the program asked for 1, 2, or 3, then exited normally. Test 7 passed.
5. Exit. Input: `3`. Expected: Goodbye and exit status 0. Actual: both occurred. Test 6 passed.

Extra result: text followed by `1` and `3` was rejected, then the cards appeared and exit succeeded. Test 16 passed. The complete runner has 31 checks; those 31 are not 31 separate compile attempts.

The completed Gate 1 package and its older records remain available separately. The current logs are later regression evidence and are not presented as results recorded before the original deadline.

## Rating and remaining checks

A 3 needs all required files, the checklist, owners, compile evidence, and five passing checks. A possible 4 also needs an extra passing test and a teammate code review. Extra tests have passed. A real teammate review is still needed unless the group supplies its completed record.

The source cross-compiles for Windows, but Visual Studio and actual Windows behavior remain unverified here. The group should keep the demo computer's build output and actual observations. The proposal's request for four students to cover six investigator assignments still needs staff confirmation.

## Teammate review record

Complete this after reviewing and trying the program. No review or signature is supplied in advance.

- Reviewer and date: ____________________
- Commit or ZIP reviewed: ____________________
- Three menu choices and sample labels checked: ____________________
- Invalid-input recovery and exit checked: ____________________
- Files and declarations checked together: ____________________
- Problems found and any retest: ____________________
- Review conclusion and signature: ____________________

The teacher determines the gate rating. Missing evidence or an unresolved critical problem still prevents a passing gate.
