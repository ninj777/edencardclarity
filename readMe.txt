CardClarity - Gate 4

Start here

This version starts from GitHub main's Gate 2 code at b907959. It keeps the
original C files, sample cards, wallet format and Windows window. Gate 4 adds
five researched cashback cards and purchase comparisons. The larger catalog
experiment is not included.

Open TestSolution.slnx in Visual Studio. Choose x64 and build the solution.
The project uses the v145 toolset; retarget it if your installed version uses
another toolset. Install the Desktop development with C++ workload to get
Microsoft's C compiler and Windows SDK. Our source still compiles as C11.
Press the local debugger button. Its arguments are now --gate4 --gui.

If the files sidebar is missing, choose View > Solution Explorer.

The download's build/cardClarity.exe is a Windows cross-build. Compilation
passed here, but its window has not been run here. Check the actual Windows
computer before the presentation. Visual Studio itself remains unverified.

Commands

  build\cardClarity.exe --gate4 --gui
  build\cardClarity.exe --gate4
  build\cardClarity.exe --gate4 --wallet-file myDemoCards.txt
  build\cardClarity.exe --gate4 --gui --wallet-file myDemoCards.txt

Gate 4 normally uses realSavedCards.txt. The old commands still work:
  build\cardClarity.exe --gate1
  build\cardClarity.exe
  build\cardClarity.exe --gui
  build\cardClarity.exe --wallet-file anotherSampleSave.txt

Those earlier modes normally use savedCards.txt. Sample IDs 1-5 keep their
original meaning. Real IDs 101-105 are separate. Do not replace an old sample
name with a bank name or reuse its ID. Both save formats store up to five
selected card types; they never need a payment number or bank password.

How to demonstrate Gate 4

Follow docs/gateFourDemo.txt. Add all five real cards, save, close and reopen.
Select a saved card to read its benefits and conditions. Open Compare a
purchase, choose a category, amount, booking service and purchase date.
For Blue Cash Everyday grocery/gas/online-retail purchases, enter the spending
already used in that same category and calendar year before this purchase.
Leave it blank if unknown. The app does not track purchases automatically.

The six required examples and extra tie/cap/date checks are in the demo sheet.
The calculator compares only the real cards in the wallet. Practice cards are
clearly excluded. It shows all rounded ties and refuses to call a conditional
range a definite winner. Its figures are cashback estimates, not net profit.

What is researched

101: Capital One Quicksilver Rewards
102: Capital One Savor Rewards
103: Chase Freedom Unlimited
104: American Express Blue Cash Everyday
105: Wells Fargo Active Cash

Issuer sources were checked October 30, 2026, before coding the new feature.
docs/gateFourResearch.txt explains the sources, conditions and decisions.
Each in-app record also shows its source and check date. Some full pricing
disclosures were unavailable. Those details say unconfirmed; they are not zero.

This is a small checked U.S. catalog, not every available card or an account
agreement. Estimates assume eligible domestic purchases in USD, accurate
merchant categories and the exact named booking service. Ordinary snapshot
rates are applied to the entered date; this is not a historical-offer lookup.
The dated Lyft promotion is checked against its modeled expiration.

Category rates are total rates. A stated 3% is not added to a second base
rate. Capped purchases split between bonus and base rates. Money stays in
whole cents; the displayed estimate rounds once to the nearest cent, with a
half cent rounded upward. A bank's actual posting can differ.

The program displays benefit activation, renewal and expiration notes. It
does not enroll anyone or automatically add credits, insurance, introductory
bonuses, interest or transaction fees to a purchase comparison. This version
does not link bank accounts or update terms online.

Build and tests

On Windows, open an x64 Native Tools Command Prompt in this folder:
  cl /nologo /TC /std:c11 /W4 /D_CRT_SECURE_NO_WARNINGS tools\buildProject.c /FebuildProject.exe
  buildProject.exe

On Linux:
  cc -std=c11 -Wall -Wextra -Wpedantic -Werror tools/buildProject.c -o buildProject
  ./buildProject

The C helper builds the application and four C check programs, then runs
them. It stops if a command fails. It writes real build output to
docs/gateFourBuildLog.txt and expected/actual results to
docs/gateFourTestLog.txt. Copy those files first if you want to keep an older
run before testing again.

With MinGW installed, the Linux helper can also compile the Windows program:
  ./buildProject --cross
That writes docs/gateFourWindowsBuildLog.txt. It does not run Windows.

Both wallet windows and the purchase form now move their controls together
when resizing, then repaint the old and new areas. The purchase form can
also be resized and maximized. The minimum size leaves space for its fields.
On Windows, the C helper also builds an optional desktop resize check:
  build\testWindowResize.exe build\cardClarity.exe
This opens temporary test copies of both modes and the purchase form. It
checks bounds, overlaps and responsiveness through repeated resizing,
minimize/restore and maximize/restore. Watch for visual trails and flickering
too; those require a person's observation. This desktop check was compiled
here but has not been run here. See docs/windowResizeFix.txt.

Current recorded results: 40 original module checks, 31 Gate 1 menu cases,
39 Gate 2 cases and 34 Gate 4 cases pass on Linux. The MinGW C11 build passes
with warnings treated as errors. Native Visual Studio execution and desktop
appearance, keyboard use, scrolling, dialogs and Windows replacement still
need checks on the demo computer. See docs/gateFourStatus.txt for the limits
of the recorded results and the sanitizer check.

The old docs/gate2 logs describe an earlier version. They are kept as earlier
evidence, not claimed as current runs. No teammate review or teacher approval
has been filled in automatically. The teacher decides the gate rating.

Where the code is

main.c joins the console actions and chooses a gate mode.
window.c is the original Windows wallet view, extended for real records.
rewardWindow.c contains the small purchase form.
windowSizing.h batches control moves and calculates the minimum window frame.
cards.c/cards.h keep the original sample records and shared Card type.
realCards.c/realCards.h hold the five sourced products and benefit notes.
rewards.c/rewards.h calculate and format purchase comparisons.
input.c checks whole numbers, money and dates.
wallet.c handles selected IDs, duplicates, count and removal.
storage.c validates a complete temporary wallet before replacing the open one.
logger.c keeps the original optional startup logger.
tests/testGateFour.c records the six purchase examples and extra checks.

The proposal assigns Joe card data and expected values, Oliver menu/build and
comparison integration, Eden input and storage checks, and Evan test evidence
and readable results. Comments identify those areas. File sizes do not prove
who personally wrote a line or performed a review.

All application, test and build-helper source is C. Visual Studio's XML files
are project settings. Documentation is plain text; there are no Markdown,
Python, C++, JavaScript, HTML/CSS or shell/batch helper files.
