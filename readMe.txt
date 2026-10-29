CardClarity

This version adds the real-card catalog to our earlier wallet. The application,
tests and build tool use C11. SQLite is a separate public-domain C library in
thirdParty. There is no Python, C++, HTML or CSS in this project.

Start on Windows

1. Extract the whole folder. Keep data beside CreditCardThing.
2. Open TestSolution.slnx in Visual Studio. The existing project uses the v145
   toolset. Install the Desktop development with C++ workload for its C compiler
   and Windows SDK. This does not make our source C++.
3. Choose Debug and x64, then Build Solution. The post-build step builds and
   checks data/cardCatalog.db from the five tab-separated source files.
4. Press the local Windows debugger button. The launch argument is --gui and
   the working directory is the project root.
5. Browse cards, search "quick silver", choose Quicksilver Rewards, then Add
   selected. Open My wallet to see it. Save wallet keeps it after restarting.

If Visual Studio's file sidebar is hidden, use View > Solution Explorer.
If your installed toolset is different, retarget the project to the installed
Visual Studio toolset rather than changing the source language.

Another Windows build option is the C helper from an x64 Native Tools Command
Prompt. Compile it, then run it from the project root:
  cl /nologo /TC /std:c11 /W4 /D_CRT_SECURE_NO_WARNINGS tools\buildProject.c /FebuildProject.exe
  buildProject.exe
After building:
  build\cardClarity.exe --gui
For the numbered catalog console:
  build\cardClarity.exe
For another personal save:
  build\cardClarity.exe --gui --wallet myWallet.db


The Windows executable in the download is a cross-build. Its compilation
passed, but this environment could not run a Windows desktop. Build and check
it on the demo computer before presenting it. Visual Studio itself has not
been run here. The source build is the main deliverable.

The earlier gates are still available

  build\cardClarity.exe --gate1
  build\cardClarity.exe --legacy
  build\cardClarity.exe --legacy --gui
  build\cardClarity.exe --wallet-file anotherSave.txt

Gate 1 still has exactly three numbered choices, five clearly named example
cards, input recovery and Exit. The old Gate 2 wallet still uses sample IDs
1 through 5 and savedCards.txt. Those IDs never become real bank cards.
The new catalog wallet uses walletCards.db instead. To bring an old sample
wallet across, use Tools > Import old sample wallet. It stays labeled as
practice data. The original text file is left alone.

What is in the catalog

There are 76 real product names from eight issuer groups and five archived
practice records. Nine real records have supported earning rules checked:
Quicksilver Rewards, Savor Rewards, Venture Rewards, VentureOne Rewards,
Freedom Unlimited, Venture X Rewards, Discover it Miles, Active Cash and Autograph. Freedom Flex has partial rules and stays incomplete.
Other real records have partial profiles or are names awaiting detailed review.
We did not fill their missing information with guesses.

Each record has a source link and a check date. A listing-page source supports
the product name, not every fee or benefit. Unknown values say unknown or not
confirmed. A checked flag means supported key fields were checked against the
listed source; it does not certify the whole account agreement or a personal
offer. Public APRs can vary, and introductory terms expire.

This is a starting U.S. catalog, not all cards worldwide. Reward eligibility
still depends on account terms and merchant coding. The large test catalog
contains 10000 synthetic records and is never shipped as researched bank data.

Using the catalog

Search matches card names, issuers and aliases. Results are limited to 20 per
page. A misspelling may show labeled suggestions; it never adds a suggestion
for you. Filters combine different groups together. Two issuers mean either
issuer, while an issuer plus a fee ceiling must meet both. An unknown fee does
not pass a confirmed-zero-fee filter. Clear filters starts over.

Filters include issuer, ongoing annual fee, reward type, category, country,
network, archived status and checked status. Network and country comparisons
use the recorded spelling/code. Blank means any. A category filter looks for
a recorded category or a selectable-category rule; it does not prove that an
unverified rotating offer currently includes that category.

If a search fails, clear the filters before deciding the card is missing.
An unlisted entry keeps your typed name and issuer without making up rates.
Tools can export those names and issuers to a new CSV file. Notes and tracking
are excluded, and the app sends nothing online.

The catalog is read-only during normal use. To maintain it, edit the source
files in data and build a separate candidate:
  build\cardClarity.exe --build-catalog data candidateCatalog.db
Use Tools > Install checked catalog to validate and install that file. The
previous catalog is kept as .previous. Keep or move that backup before a
second update. Personal wallet entries and notes are stored separately.
Updates are manual; there is no download service.

Wallet, comparisons and extras

Add and Remove change the open wallet. Save commits a transaction. Reload
checks the saved file in a temporary wallet before replacing the open list.
A damaged load leaves the current list alone. Closing with edits offers Save,
Discard or Cancel. An older open copy cannot overwrite a newer save revision.
Undo last removal is available until another edit replaces that undo point.
Wallet > Link an unlisted card attaches an exact confirmed catalog key to an
existing entry without losing its note or favorite. The entry details show
that key. Wallet > Remove an old tracker item frees one history slot while
keeping the other periods. Control+F focuses search, Enter searches, and
Control+S saves in the window.

Notes and settings include favorites, your point/mile valuation, an anniversary,
a statement day and one actual selected category. A selected category must
match your real account's offer. Anniversary settings do not prove the date
your bank will post a fee.

Compare > Purchase rewards estimates a purchase for the cards in your wallet.
Amounts use whole cents. Rules describe total earning, not extra multipliers.
The calculator splits a purchase at a recorded spending cap. Unknown cap use
or activation produces a conditional range. Points and miles stay units until
you enter a cents-per-unit assumption. Incomplete records and conditional
values are excluded from a definite ranking. Ties use rounded estimated cash
amounts. Portal rates apply only to the specified issuer's booking service.
Interest and foreign transaction costs are not subtracted from this purchase
reward estimate.

Compare > Yearly estimate uses the same monthly category spending for a fresh
calendar-year scenario, direct bookings, the chosen annual-fee period and your
entered credit, bonus and interest assumptions. It is not a transaction history
or an APR-based interest calculator. Unknown fees or conditions make the result
incomplete. Credits are not automatically treated as cash you will use.

Track usage records cap spending or benefit use manually. The entry details
show the saved period, amount and condition. Wallet > Reminders supports due
dates, snoozing, completion and deletion. Reminders appear while using the app;
there is no background alarm. Tools backs up and restores saved wallets using
SQLite's backup API. Choose a new backup name; existing files are not silently
overwritten. A restore preserves the previous save as .beforeRestore.

Only names, selections, notes and manually entered settings are needed.
Do not enter account numbers, security codes or bank passwords. Wallets are
local files, not encrypted vaults. Keep personal files out of GitHub.

Build and checks on Linux

Run:
  cc -std=c11 -Wall -Wextra -Wpedantic -Werror tools/buildProject.c -o buildProject
  ./buildProject

The C helper builds the portable catalog console, SQLite and three C check
programs, then runs them. There are no shell or batch helper scripts. It requires a C11 compiler and the C library
and math library. It does not build or run the original Windows-only files.

To use the built catalog console:
  build/cardClarity
To use another catalog or wallet:
  build/cardClarity --catalog data/cardCatalog.db --wallet myWallet.db

Current continuation notes and results are in docs/edenWork.txt and the
edenBuildLog.txt, edenMenuLog.txt, edenSanitizerLog.txt and edenWindowsBuildLog.txt
files. This storage/catalog continuation is filed under Eden's assigned area.
Earlier recorded commands and results are in docs/catalogBuildLog.txt,
catalogTestLog.txt, catalogMenuLog.txt, catalogScaleLog.txt and
catalogSanitizerLog.txt. docs/catalogReport.txt explains the results and
remaining checks. The older gate2 logs describe their earlier version; they
are not fresh runs of the latest Windows application.

Where to read the code

main.c keeps the old gate menus and chooses the new app when appropriate.
catalogApp.c joins the new console actions. catalogWindow.c is the new native
Windows view; window.c remains the earlier sample window.
catalog.c reads, validates and searches cards. catalogBuild.c imports records.
catalogWallet.c handles entries, saved-wallet checks, trackers and reminders.
catalogFiles.c handles backup, restore and catalog replacement.
catalogText.c checks decimal amounts, dates and bounded text.
rewardMath.c handles earning rules, caps, valuations, comparisons and yearly
scenarios. Headers share declarations; they are not separate running programs.
The C files under tests check these behaviors and record actual observations.

The proposal assigns Oliver the menu, window and integration; Eden the wallet,
input and storage; Joe the card records and benefit values; and Evan the checks
and bug records. These are responsibility areas, not a claim that file length
proves who personally wrote each line. No teammate review or teacher approval
has been recorded for this expansion.

Before the gate, follow docs/demoChecklist.txt on the actual Windows computer.
The teacher decides the gate rating. The catalog expansion does not replace
Gate 3's QA report or Gate 4's source review and required purchase examples.
