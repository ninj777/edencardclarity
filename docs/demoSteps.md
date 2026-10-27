# What we show for Gate 2

Eden leads this check. We need to show add, view, remove, save, and reload, including after a restart. Visual Studio now opens the window. The console steps below are also available.

Build with the readMe steps first. From the project root, use a separate demo file:

```bat
if not exist build\demo mkdir build\demo
build\windows\CreditCardThing.exe --wallet-file build\demo\savedCards.txt
```

If that file already has a list, inspect it or choose another name. Use the same path each time you restart.

1. Say: "Our wallet saves selected card types. All the rates are practice values, and we don't store payment details."
2. Enter `2` for the wallet, then `2` to add. Enter card `1`. Add card `3` the same way.
3. Choose `1` to view both cards, then `4` to save.
4. Choose `6`, then `3` to exit. Run the same command again. Open the wallet and view the list. Both cards should still be there.
5. Choose `3` to remove, then card `1`. View the list and save it again.
6. Choose `5` to reload, then view. Only Cashback Sample should remain. Exit and restart once more to show that the removal stayed saved.
7. Try `hello` at a menu and show the retry message. Try adding the same card twice and show that it is rejected.

For a damaged-file example, create `build/demo/badCards.txt` containing:

```text
cardClarityWallet1
2
1
1
```

Start with `--wallet-file build\demo\badCards.txt`. The duplicate should be rejected. Show that the program stays open and usable. Exit without saving over the example.

Eden explains input and storage. Oliver explains how the menu connects the files and how we build it. Joe explains card IDs and the sample values. Evan explains the expected and actual checks. Each person can add their own run notes to `testResults.md` or `teamWork.md` after doing them.

Bring the source folder, report, build output, and test results. The recorded Linux runs are in the three log files. Record the Windows results on the actual demo computer. Use `--gui` to open the window or no mode argument for the console.

## Show it in the window

Run the same executable with `--gui --wallet-file build\demo\savedCards.txt`. Choose Travel Sample and click Add to my wallet. Choose Cashback Sample and add it too. Click Save cards, close, and run the same command again. Both should load. Select Travel Sample in My wallet, remove it, and save. Reload saved should keep only Cashback Sample.

Resize the window and check the labels and footer stay readable. Use Tab and Space to move to a button and activate it. Make an unsaved edit and check that Reload and closing ask first. Record the actual Windows results in `testResults.md`.

Oliver's area covers the window and menu connection. Eden's storage functions are shared by both views. Joe's area covers the card names, rates, and detail labels. Evan's area covers the behavior checks and recording the Windows run. No Windows runtime result is filled in before someone actually runs it.
