# Team responsibilities for Gates 1 and 2

This division follows sections 4 and 5 of the CardClarity proposal. Gate 1 is led by Oliver Dolfin and is due October 22, 2026. Gate 2 is led by Eden Yun Yu and is due October 28, 2026. The descriptions below divide the implementation by those responsibilities.

## Oliver Dolfin

Proposal role: project lead and main programmer. Gate 1 work is the menu, connecting modules, compiling, and submission. Gate 2 work connects the wallet actions to the main menu.

Assigned files: `main.c`, `window.c`, `window.h`, `logger.c`, `logger.h`, the Visual Studio project and filters, the solution, readMe, Git settings, and source package. The original unused starter files also stay under project cleanup rather than being counted as active features.

Current C source and headers: 734 lines. Changes after Gate 1: 619 added, 395 removed, including separating the window from `main.c`.

Detailed explanation: [oliverWork.md](oliverWork.md).

## Eden Yun Yu

Proposal role: systems and security developer. Gate 1 work is the input rules and saved-file plan. Gate 2 work is input validation, wallet storage, saving, loading, and handling missing or damaged files.

Assigned files: `input.c`, `input.h`, `wallet.c`, `wallet.h`, `storage.c`, and `storage.h`. The saved-file format notes and Gate 2 report follow this area too.

Current C source and headers: 316 lines. All six files were added after Gate 1. The earlier input checking was inside the starter's `main.c`.

Detailed explanation: [edenWork.md](edenWork.md).

## Joe Kwon

Proposal role: card research and data developer. Gate 1 work is the sample records and example reward rates. Gate 2 work keeps the saved card IDs matched to the sample catalog.

Assigned files: `cards.c` and `cards.h`, plus the catalog values and expected reward examples used by the tests. Later work needs sources and dates for any real benefit data.

Current C source and headers: 257 lines. Changes after Gate 1: 17 added, 9 removed. The small difference is expected because the sample records already existed.

Detailed explanation: [joeWork.md](joeWork.md).

## Evan Zhuang

Proposal role: interface developer and tester. Gate 1 work is the five required checks, expected and actual results, and bug notes. Gate 2 work tests add, view, remove, save, and reload, including error cases.

Assigned files: `tests/testGate1.c`, `tests/testGate2.c`, `tests/testModules.c`, `tests/demoGate2.c`, `tests/testSupport.c`, and `tests/testSupport.h`. Test results, the three run logs, demonstration steps, and menu-clarity checks belong in this area. Oliver's window implementation and Evan's interface checks are related jobs, rather than two separate window implementations.

Current test source and headers: 1,285 lines, all C. Changes after Gate 1: 1,232 added, 326 removed across the tests folder. These numbers include replacing the original test tools with C runners, not new application features.

Detailed explanation: [evanWork.md](evanWork.md).

## Where the parts meet

Adding a card starts in Oliver's menu or button handler, uses Eden's `addWalletCard`, looks up Joe's ID mapping, and is checked by Evan's test cases. Saving starts in the menu or window and uses Eden's file functions. The test runner checks the actual saved IDs and launches another process to check that they survive a restart.

The full per-file counts, starting-code explanation, and comparison commands are in [codeChanges.md](codeChanges.md). Logs and documents are excluded from those source amounts.

Gate requirements and evidence are in [gate1Report.md](gate1Report.md) and [gate2Report.md](gate2Report.md). The code overview is in [codeGuide.md](codeGuide.md). Real demo-computer results belong in [testResults.md](testResults.md); the teammate review record for Gate 1 is in its report.
