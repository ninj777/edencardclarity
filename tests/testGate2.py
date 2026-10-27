"""Gate 2 wallet checks. Each run uses a temporary folder, not anyone's real save."""
import argparse
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import threading
import unittest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("program")
parser.add_argument("--record", help="Save expected results and actual program output")
args = parser.parse_args()
PROGRAM = str(Path(args.program).resolve())


class Gate2Tests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory(prefix="cardClarityTest")
        self.root = Path(self.folder.name)
        self.save = self.root / "savedCards.txt"
        self.trace = ["\nTest: " + self.id(), "Expected: " + self.shortDescription()]

    def tearDown(self):
        self.folder.cleanup()

    def write_save(self, ids):
        self.save.write_text("cardClarityWallet1\n" + str(len(ids)) + "\n"
                             + "".join(str(card) + "\n" for card in ids))

    def run_menu(self, answers, *, path=None, default=False):
        command = [PROGRAM] if default else [PROGRAM, "--wallet-file", str(path or self.save)]
        result = subprocess.run(command, input=answers, text=True, capture_output=True,
                                cwd=self.root, timeout=5)
        self.record_run(command, answers, result.stdout, result.stderr, result.returncode)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        return result.stdout

    def record_run(self, command, answers, output, errors, status):
        self.trace.extend(["Command: " + repr(command), "Input: " + repr(answers),
                           "Actual standard output:\n" + output,
                           "Actual standard error:\n" + errors, "Exit status: " + str(status)])
        if self.save.is_file():
            self.trace.append("Actual saved file: " + repr(self.save.read_bytes()))

    def assert_save(self, ids):
        expected = ("cardClarityWallet1\n" + str(len(ids)) + "\n"
                    + "".join(str(card) + "\n" for card in ids)).encode()
        self.assertEqual(self.save.read_bytes(), expected)

    def test_01_start_empty(self):
        """A missing file starts an empty wallet and creates no file just by viewing."""
        out = self.run_menu("2\n1\n6\n3\n")
        self.assertIn("no saved file yet", out)
        self.assertIn("Your wallet is empty", out)
        self.assertFalse(self.save.exists())

    def test_02_catalog_has_stable_numbers(self):
        """The five sample names have card numbers 1 to 5 and example labels."""
        out = self.run_menu("1\n3\n")
        for number, name in enumerate(("Travel", "Points", "Cashback", "Everyday", "Starter"), 1):
            self.assertIn(f"Card {number}: {name} Sample", out)
        self.assertIn("not real bank offers", out)

    def test_03_add_and_view(self):
        """Adding card 3 shows only Cashback Sample in the wallet, then saves ID 3."""
        out = self.run_menu("2\n2\n3\n1\n4\n6\n3\n")
        self.assertIn("Added Cashback Sample.", out)
        self.assertIn("Card 3: Cashback Sample | Example base rate: 1.00%", out)
        self.assert_save([3])

    def test_04_duplicate(self):
        """A duplicate card is rejected and the saved list contains it only once."""
        out = self.run_menu("2\n2\n1\n2\n1\n4\n6\n3\n")
        self.assertIn("already in your wallet", out)
        self.assert_save([1])

    def test_05_remove_middle_card(self):
        """Removing card 3 from [1, 3, 5] saves [1, 5] in order."""
        self.write_save([1, 3, 5])
        out = self.run_menu("2\n3\n3\n4\n6\n3\n")
        self.assertIn("Removed Cashback Sample.", out)
        self.assert_save([1, 5])

    def test_06_remove_absent_card(self):
        """Removing a valid catalog ID not in the wallet leaves the saved list alone."""
        self.write_save([1])
        out = self.run_menu("2\n3\n2\n4\n6\n3\n")
        self.assertIn("isn't in your wallet", out)
        self.assert_save([1])

    def test_07_remove_from_empty(self):
        """Removing from an empty wallet explains the empty list and does not ask for an ID."""
        out = self.run_menu("2\n3\n6\n3\n")
        self.assertIn("Your wallet is empty", out)
        self.assertNotIn("Enter the card number to remove", out)

    def test_08_full_wallet(self):
        """A five-card wallet refuses another addition and saves all five IDs unchanged."""
        self.write_save([1, 2, 3, 4, 5])
        out = self.run_menu("2\n2\n1\n4\n6\n3\n")
        self.assertIn("Your wallet is full", out)
        self.assertIn("My card wallet (5 of 5 cards)", out)
        self.assert_save([1, 2, 3, 4, 5])

    def test_09_save_and_restart(self):
        """After adding and saving cards 1 and 3, a second process loads those two cards."""
        self.run_menu("2\n2\n1\n2\n3\n4\n6\n3\n")
        self.assert_save([1, 3])
        out = self.run_menu("2\n1\n6\n3\n")
        self.assertIn("Loaded your saved card list", out)
        self.assertIn("My card wallet (2 of 5 cards)", out)
        self.assertIn("Card 1: Travel Sample |", out)
        self.assertIn("Card 3: Cashback Sample |", out)
        self.assertNotIn("Card 2: Points Sample |", out)

    def test_10_removal_survives_restart(self):
        """A removed and saved card stays removed after restarting."""
        self.write_save([1, 3, 5])
        self.run_menu("2\n3\n3\n4\n6\n3\n")
        out = self.run_menu("2\n1\n6\n3\n")
        self.assert_save([1, 5])
        self.assertIn("My card wallet (2 of 5 cards)", out)
        self.assertNotIn("Card 3: Cashback Sample |", out)

    def test_11_empty_save(self):
        """Removing the final card writes a valid zero-count save that reloads empty."""
        self.write_save([1])
        self.run_menu("2\n3\n1\n4\n6\n3\n")
        self.assert_save([])
        self.assertIn("Your wallet is empty", self.run_menu("2\n1\n6\n3\n"))

    def test_12_reload_replaces_unsaved_changes(self):
        """Reloading restores the saved [1] list after adding unsaved card 3."""
        self.write_save([1])
        out = self.run_menu("2\n2\n3\n5\n1\n6\n3\n")
        view = out.rsplit("Loaded your saved card list.", 1)[1]
        self.assertIn("My card wallet (1 of 5 cards)", view)
        self.assertNotIn("Card 3: Cashback Sample |", view)
        self.assertNotIn("You haven't saved your changes yet", view)
        self.assert_save([1])

    def test_13_missing_reload_keeps_current_cards(self):
        """Reloading a missing file leaves an unsaved card in memory so it can still be saved."""
        out = self.run_menu("2\n2\n1\n5\n1\n4\n6\n3\n")
        self.assertIn("Your current list hasn't changed", out)
        self.assertIn("Card 1: Travel Sample |", out)
        self.assert_save([1])

    def test_14_room_after_removal(self):
        """Removing from a full wallet frees a slot and adding again appends the card."""
        self.write_save([1, 2, 3, 4, 5])
        self.run_menu("2\n3\n3\n2\n3\n4\n6\n3\n")
        self.assert_save([1, 2, 4, 5, 3])

    def test_15_bad_main_and_wallet_choices(self):
        """Blank, text, decimal, negative, zero, and out-of-range choices allow recovery."""
        for bad in ("", "hello", "1.5", "-1", "0", "9", "1x", "9" * 300):
            with self.subTest(answer=bad):
                out = self.run_menu(bad + "\n2\n" + bad + "\n1\n6\n3\n")
                self.assertIn("Please enter 1, 2, or 3.", out)
                self.assertIn("Please enter a whole number from 1 to 6.", out)
                self.assertIn("Your wallet is empty", out)

    def test_16_bad_card_numbers(self):
        """Invalid card answers are rejected before a valid card 3 is added."""
        out = self.run_menu("2\n2\n\nwrong\n-1\n6\n1.5\n1x\n3\n4\n6\n3\n")
        self.assertEqual(out.count("Please enter a whole number from 0 to 5."), 6)
        self.assert_save([3])

    def test_17_overlong_and_hidden_null_input(self):
        """Overlong answers and hidden null bytes do not add a card or spoil the next answer."""
        out = self.run_menu("2\n2\n" + "1" * 1000 + "\n1\0hidden\n3\n4\n6\n3\n")
        self.assertEqual(out.count("Please enter a whole number from 0 to 5."), 2)
        self.assert_save([3])

    def test_18_cancel_add_and_remove(self):
        """Zero cancels an add or removal and leaves [1] unchanged."""
        self.write_save([1])
        out = self.run_menu("2\n2\n0\n3\n0\n6\n3\n")
        self.assertIn("Add cancelled", out)
        self.assertIn("Remove cancelled", out)
        self.assertNotIn("You haven't saved your changes yet", out)
        self.assert_save([1])

    def test_19_end_of_input(self):
        """End of input at nested prompts exits without a crash or an unexpected save."""
        for answers in ("", "2\n", "2\n2\n", "2\n2\n1\n"):
            with self.subTest(answers=answers):
                out = self.run_menu(answers)
                self.assertIn("No more input. Goodbye!", out)
                self.assertFalse(self.save.exists())

    def test_20_save_on_exit(self):
        """Save and exit writes unsaved card 1 before closing."""
        out = self.run_menu("2\n2\n1\n6\n3\n1\n")
        self.assertIn("Save and exit", out)
        self.assert_save([1])

    def test_21_exit_without_saving(self):
        """Exit without saving preserves the older save [1] instead of writing [1, 3]."""
        self.write_save([1])
        self.run_menu("2\n2\n3\n6\n3\n2\n")
        self.assert_save([1])

    def test_22_keep_editing(self):
        """Cancelling exit returns to the menu and a later save keeps the edits."""
        out = self.run_menu("2\n2\n1\n6\n3\n3\n2\n4\n6\n3\n")
        self.assertEqual(out.count("Goodbye!"), 1)
        self.assert_save([1])

    def test_23_default_path(self):
        """Normal launch saves and reloads savedCards.txt in the current folder."""
        self.run_menu("2\n2\n5\n4\n6\n3\n", default=True)
        self.assert_save([5])
        out = self.run_menu("2\n1\n6\n3\n", default=True)
        self.assertIn("Card 5: Starter Sample |", out)

    def test_24_path_with_spaces(self):
        """A chosen save filename containing spaces works across two launches."""
        self.save = self.root / "my saved cards.txt"
        self.run_menu("2\n2\n2\n4\n6\n3\n")
        self.assert_save([2])
        self.assertIn("Card 2: Points Sample |", self.run_menu("2\n1\n6\n3\n"))

    def test_25_damaged_files(self):
        """Malformed headers, counts, IDs, duplicates, missing data, and extra data are rejected unchanged."""
        fixtures = {
            "empty": b"", "wrong header": b"otherWallet\n0\n",
            "header only": b"cardClarityWallet1\n",
            "negative count": b"cardClarityWallet1\n-1\n",
            "too many cards": b"cardClarityWallet1\n6\n1\n2\n3\n4\n5\n1\n",
            "huge count": b"cardClarityWallet1\n" + b"9" * 300 + b"\n",
            "text count": b"cardClarityWallet1\ntwo\n",
            "decimal count": b"cardClarityWallet1\n1.5\n",
            "zero ID": b"cardClarityWallet1\n1\n0\n",
            "unknown ID": b"cardClarityWallet1\n1\n6\n",
            "negative ID": b"cardClarityWallet1\n1\n-1\n",
            "duplicate": b"cardClarityWallet1\n2\n1\n1\n",
            "missing ID": b"cardClarityWallet1\n2\n1\n",
            "extra ID": b"cardClarityWallet1\n1\n1\n3\n",
            "extra blank line": b"cardClarityWallet1\n0\n\n",
            "hidden null in header": b"cardClarityWallet1\0hidden\n0\n",
            "hidden null in count": b"cardClarityWallet1\n0\0hidden\n",
            "hidden null in ID": b"cardClarityWallet1\n1\n1\0hidden\n",
            "long header": b"x" * 1000 + b"\n0\n",
            "long ID": b"cardClarityWallet1\n1\n" + b"1" * 1000 + b"\n",
            "BOM header": b"\xef\xbb\xbfcardClarityWallet1\n0\n",
            "ID with extra text": b"cardClarityWallet1\n1\n1abc\n",
        }
        for name, data in fixtures.items():
            with self.subTest(file=name):
                self.trace.append("Damaged-file variant: " + name)
                self.save.write_bytes(data)
                out = self.run_menu("2\n1\n6\n3\n")
                self.assertIn("looks damaged or uses a different format", out)
                self.assertIn("Your wallet is empty", out)
                self.assertEqual(self.save.read_bytes(), data)

    def test_26_windows_line_endings(self):
        """A valid CRLF file loads the same IDs as an LF file."""
        self.save.write_bytes(b"cardClarityWallet1\r\n2\r\n1\r\n3\r\n")
        out = self.run_menu("2\n1\n6\n3\n")
        self.assertIn("Card 1: Travel Sample |", out)
        self.assertIn("Card 3: Cashback Sample |", out)

    def test_27_no_final_newline(self):
        """A valid file without a newline after its last ID still loads."""
        self.save.write_bytes(b"cardClarityWallet1\n1\n5")
        self.assertIn("Card 5: Starter Sample |", self.run_menu("2\n1\n6\n3\n"))

    def test_28_directory_is_not_a_save(self):
        """A directory at the save path reports a read error and leaves its contents alone."""
        self.save.mkdir()
        marker = self.save / "keep.txt"
        marker.write_text("keep this")
        out = self.run_menu("2\n1\n6\n3\n")
        self.assertIn("Couldn't read your saved cards", out)
        self.assertEqual(marker.read_text(), "keep this")

    def test_29_save_missing_folder(self):
        """Saving into a missing folder fails clearly while the current wallet remains usable."""
        path = self.root / "missing" / "savedCards.txt"
        out = self.run_menu("2\n2\n1\n4\n1\n6\n3\n2\n", path=path)
        self.assertIn("Couldn't save your cards", out)
        self.assertIn("Card 1: Travel Sample |", out)
        self.assertFalse(path.exists())

    def test_30_stale_temporary_file(self):
        """An existing temporary file blocks saving without changing it or the previous save."""
        self.write_save([1])
        temp = Path(str(self.save) + ".tmp")
        temp.write_bytes(b"keep temporary data")
        out = self.run_menu("2\n2\n3\n4\n1\n6\n3\n2\n")
        self.assertIn("Couldn't save your cards", out)
        self.assertIn("Card 3: Cashback Sample |", out)
        self.assert_save([1])
        self.assertEqual(temp.read_bytes(), b"keep temporary data")

    def test_31_failed_replacement_cleans_own_temp(self):
        """A failed replacement of a directory removes the new temp and preserves the directory."""
        self.save.mkdir()
        marker = self.save / "keep.txt"
        marker.write_text("keep")
        out = self.run_menu("2\n2\n1\n4\n6\n3\n2\n")
        self.assertIn("Couldn't save your cards", out)
        self.assertFalse(Path(str(self.save) + ".tmp").exists())
        self.assertEqual(marker.read_text(), "keep")

    def test_32_failed_save_on_exit_returns_to_menu(self):
        """A failed Save and exit leaves the program open for editing or explicit discard."""
        path = self.root / "missing" / "savedCards.txt"
        out = self.run_menu("2\n2\n1\n6\n3\n1\n2\n1\n6\n3\n2\n", path=path)
        self.assertIn("Couldn't save your cards", out)
        self.assertIn("Card 1: Travel Sample |", out)
        self.assertEqual(out.count("Goodbye!"), 1)

    def test_33_reload_damaged_file_keeps_current_wallet(self):
        """If the file is damaged after startup, Reload leaves the already-loaded card 1 intact."""
        self.write_save([1])
        process = subprocess.Popen([PROGRAM, "--wallet-file", str(self.save)], cwd=self.root,
                                   stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, text=True)
        timer = threading.Timer(5, process.kill)
        timer.start()
        prefix = ""
        try:
            while "Loaded your saved card list." not in prefix:
                line = process.stdout.readline()
                self.assertTrue(line, "Program stopped before loading the initial file")
                prefix += line
            damaged = b"cardClarityWallet1\n2\n3\n3\n"
            self.save.write_bytes(damaged)
            answers = "2\n5\n1\n6\n3\n"
            out, err = process.communicate(answers, timeout=5)
            out = prefix + out
            self.record_run(process.args, answers, out, err, process.returncode)
            self.assertEqual(process.returncode, 0, err)
            self.assertEqual(err, "")
            self.assertIn("looks damaged or uses a different format", out)
            self.assertIn("Card 1: Travel Sample |", out)
            self.assertNotIn("Card 3: Cashback Sample |", out)
            self.assertEqual(self.save.read_bytes(), damaged)
        finally:
            timer.cancel()
            if process.poll() is None:
                process.kill()
                process.communicate()
            process.stdout.close()
            process.stderr.close()

    def test_34_whitespace_in_numeric_lines(self):
        """Whitespace around a count or ID is accepted and the correct card loads."""
        self.save.write_bytes(b"cardClarityWallet1\n 1 \t\n 3 \t\n")
        out = self.run_menu(" 2 \n 1 \n 6 \n 3 \n")
        self.assertIn("Card 3: Cashback Sample |", out)

    def test_35_remove_first_and_last(self):
        """Removing the first and last IDs leaves the middle card correctly saved."""
        self.write_save([1, 3, 5])
        self.run_menu("2\n3\n1\n3\n5\n4\n6\n3\n")
        self.assert_save([3])

    @unittest.skipUnless(sys.platform.startswith("linux"), "Linux input-error check")
    def test_36_real_input_error(self):
        """A real stdin read error returns status 1 rather than claiming a normal exit."""
        descriptor = os.open(self.root, os.O_RDONLY | os.O_DIRECTORY)
        try:
            result = subprocess.run([PROGRAM, "--wallet-file", str(self.save)], stdin=descriptor,
                                    text=True, capture_output=True, cwd=self.root, timeout=5)
        finally:
            os.close(descriptor)
        self.record_run(result.args, "directory used as stdin", result.stdout, result.stderr, result.returncode)
        self.assertEqual(result.returncode, 1)
        self.assertIn("couldn't read your answer", result.stderr)
        self.assertNotIn("Goodbye!", result.stdout)

    def test_37_bad_arguments(self):
        """Unknown, incomplete, or empty command arguments return status 1 and do not save."""
        for arguments in (("--wrong",), ("--wallet-file",), ("--wallet-file", ""),
                          ("--gui", "--wallet-file"), ("--gui", "--wallet-file", ""),
                          ("--gui", "--wrong", "cards.txt")):
            with self.subTest(arguments=arguments):
                result = subprocess.run([PROGRAM, *arguments], cwd=self.root,
                                        capture_output=True, text=True, timeout=5)
                self.record_run(result.args, "no input", result.stdout, result.stderr, result.returncode)
                self.assertEqual(result.returncode, 1)
                self.assertIn("Run CardClarity without arguments", result.stderr)
                self.assertFalse(self.save.exists())

    def test_38_too_long_save_path(self):
        """An overlong save path is rejected without overflowing the temporary-path buffer."""
        path = self.root / ("x" * 1100)
        out = self.run_menu("2\n2\n1\n4\n6\n3\n2\n", path=path)
        self.assertIn("Couldn't save your cards", out)
        self.assertEqual(list(self.root.iterdir()), [])

    @unittest.skipIf(sys.platform == "win32", "The window needs a real desktop check on Windows")
    def test_39_window_path_on_other_systems(self):
        """The window accepts a chosen save path, then explains that it needs Windows without writing."""
        result = subprocess.run([PROGRAM, "--gui", "--wallet-file", str(self.save)],
                                cwd=self.root, capture_output=True, text=True, timeout=5)
        self.record_run(result.args, "no input", result.stdout, result.stderr, result.returncode)
        self.assertEqual(result.returncode, 1)
        self.assertIn("window preview needs Windows", result.stderr)
        self.assertNotIn("Run CardClarity without arguments", result.stderr)
        self.assertFalse(self.save.exists())


class RecordedResults(unittest.TextTestResult):
    def __init__(self, *values, **options):
        super().__init__(*values, **options)
        self.records = []

    def addSuccess(self, test):
        super().addSuccess(test)
        self.records.extend(test.trace + ["Result: passed. All expected checks matched."])

    def addFailure(self, test, error):
        super().addFailure(test, error)
        self.records.extend(test.trace + ["Result: failed. See the test report."])

    def addError(self, test, error):
        super().addError(test, error)
        self.records.extend(getattr(test, "trace", []) + ["Result: error. See the test report."])

    def addSubTest(self, test, subtest, error):
        super().addSubTest(test, subtest, error)
        if error:
            self.records.extend(test.trace + ["Result: failed subtest " + str(subtest)])

    def addSkip(self, test, reason):
        super().addSkip(test, reason)
        self.records.append("Skipped " + test.id() + ": " + reason)


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(Gate2Tests)
    result = unittest.TextTestRunner(verbosity=2, resultclass=RecordedResults).run(suite)
    if args.record:
        Path(args.record).write_text("Gate 2 expected and actual results\n" + "\n".join(result.records) + "\n")
    sys.exit(0 if result.wasSuccessful() else 1)
