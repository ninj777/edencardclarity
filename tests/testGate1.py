"""Menu tests for Gate 1. Uses only Python's built-in libraries.

Run: python3 tests/testGate1.py ./cardClarity
"""
import os
import re
import subprocess
import sys
import unittest
from pathlib import Path


if len(sys.argv) < 2:
    print("Usage: python3 tests/testGate1.py programPath", file=sys.stderr)
    sys.exit(2)

PROGRAM = Path(sys.argv[1]).resolve()
del sys.argv[1]


class Gate1MenuTests(unittest.TestCase):
    def run_program(self, answers):
        result = subprocess.run(
            [str(PROGRAM), "--gate1"], input=answers, text=True,
            capture_output=True, timeout=5, check=False
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        return result.stdout

    def test_01_menu_has_three_options(self):
        output = self.run_program("3\n")
        self.assertIn("1. View sample cards", output)
        self.assertIn("2. About CardClarity", output)
        self.assertIn("3. Exit", output)
        self.assertEqual(re.findall(r"(?m)^[1-9][0-9]*\. .+$", output),
                         ["1. View sample cards", "2. About CardClarity", "3. Exit"])

    def test_02_view_all_five_cards(self):
        output = self.run_program("1\n3\n")
        for name in ("Travel Sample", "Points Sample", "Cashback Sample",
                     "Everyday Sample", "Starter Sample"):
            self.assertEqual(output.count(name), 1)

    def test_03_rates_are_printed(self):
        output = self.run_program("1\n3\n")
        records = (
            ("Travel Sample", "1.25", 95, "5.00", "3.00", "2.00"),
            ("Points Sample", "1.00", 250, "4.00", "2.00", "1.50"),
            ("Cashback Sample", "1.00", 0, "3.00", "2.00", "1.00"),
            ("Everyday Sample", "1.00", 0, "3.00", "2.00", "2.00"),
            ("Starter Sample", "1.00", 0, "4.50", "2.50", "1.00"),
        )
        for name, base, fee, airlines, stores, services in records:
            with self.subTest(name=name):
                self.assertIn(
                    f"{name}\nExample base rate: {base}% | Example annual fee: ${fee}\n"
                    f"Example bonus percentages: Airlines {airlines}% | "
                    f"Stores {stores}% | Services {services}%", output)
        self.assertIn("it is not a times multiplier", output)

    def test_04_rates_are_labeled_examples(self):
        output = self.run_program("1\n3\n")
        self.assertIn("made up for practice, not real bank offers", output)
        self.assertIsNone(re.search(r"\b[A-Z]{2,}\b", output))

    def test_05_about_lists_planned_features(self):
        output = self.run_program("2\n3\n")
        for feature in ("Card wallet:", "Benefits display:",
                        "Rewards calculator:", "Card recommendations:"):
            self.assertIn(feature, output)

    def test_06_exit(self):
        output = self.run_program("3\n")
        self.assertIn("Goodbye!", output)

    def test_07_text_input(self):
        self.assertIn("Please enter 1, 2, or 3.", self.run_program("hello\n3\n"))

    def test_08_zero(self):
        self.assertIn("Please enter 1, 2, or 3.", self.run_program("0\n3\n"))

    def test_09_out_of_range(self):
        self.assertIn("Please enter 1, 2, or 3.", self.run_program("9\n3\n"))

    def test_10_negative(self):
        self.assertIn("Please enter 1, 2, or 3.", self.run_program("-1\n3\n"))

    def test_11_empty_answer(self):
        self.assertIn("Please enter 1, 2, or 3.", self.run_program("\n3\n"))

    def test_12_decimal(self):
        self.assertIn("Please enter 1, 2, or 3.", self.run_program("1.5\n3\n"))

    def test_13_trailing_characters(self):
        self.assertIn("Please enter 1, 2, or 3.", self.run_program("1xyz\n3\n"))

    def test_14_very_large_number(self):
        for length in (30, 300):
            with self.subTest(length=length):
                self.assertIn("Please enter 1, 2, or 3.",
                              self.run_program("9" * length + "\n3\n"))

    def test_15_overlong_text(self):
        self.assertIn("Please enter 1, 2, or 3.", self.run_program("a" * 600 + "\n3\n"))

    def test_16_valid_after_invalid(self):
        output = self.run_program("not a choice\n1\n3\n")
        self.assertIn("Please enter 1, 2, or 3.", output)
        self.assertIn("Travel Sample", output)
        self.assertIn("Goodbye!", output)

    def test_17_whitespace_around_choice(self):
        output = self.run_program("  1  \t\n3\n")
        self.assertIn("Travel Sample", output)

    def test_18_end_of_input(self):
        output = self.run_program("")
        self.assertIn("No more input", output)

    def test_19_choice_without_final_newline(self):
        output = self.run_program("3")
        self.assertIn("Goodbye!", output)

    def test_20_repeated_choices_return_to_menu(self):
        output = self.run_program("1\n2\n1\n3\n")
        self.assertEqual(output.count("Travel Sample"), 2)
        self.assertEqual(output.count("Enter 1, 2, or 3:"), 4)
        self.assertEqual(output.count("Goodbye!"), 1)

    def test_21_exit_does_not_process_later_input(self):
        output = self.run_program("3\n1\n")
        self.assertNotIn("Travel Sample", output)
        self.assertEqual(output.count("Enter 1, 2, or 3:"), 1)

    def test_22_embedded_null_is_rejected(self):
        for answers in ("1\0hidden\n3\n", "1\0hidden"):
            with self.subTest(answers=repr(answers)):
                output = self.run_program(answers)
                self.assertIn("Please enter 1, 2, or 3.", output)
                self.assertNotIn("Travel Sample", output)
                self.assertIn("Goodbye!", output)

    def test_23_all_standard_whitespace(self):
        output = self.run_program("\v\f 1\t\v\f\r\n3\n")
        self.assertIn("Travel Sample", output)
        self.assertNotIn("Please enter 1, 2, or 3.", output)

    def test_24_long_input_is_one_rejected_answer(self):
        output = self.run_program("1" + " " * 1000 + "2\n1\n3\n")
        self.assertEqual(output.count("Please enter 1, 2, or 3."), 1)
        self.assertEqual(output.count("Travel Sample"), 1)
        self.assertIn("Goodbye!", output)

    def test_25_invalid_answer_at_eof(self):
        output = self.run_program("wrong")
        self.assertEqual(output.count("Please enter 1, 2, or 3."), 1)
        self.assertIn("No more input", output)

    def test_26_blank_whitespace(self):
        output = self.run_program(" \t\r\n3\n")
        self.assertIn("Please enter 1, 2, or 3.", output)
        self.assertIn("Goodbye!", output)

    @unittest.skipUnless(sys.platform.startswith("linux"), "Linux input-error check")
    def test_27_read_error_is_not_normal_eof(self):
        descriptor = os.open(PROGRAM.parent, os.O_RDONLY | os.O_DIRECTORY)
        try:
            result = subprocess.run([str(PROGRAM), "--gate1"], stdin=descriptor,
                                    text=True, capture_output=True, timeout=5)
        finally:
            os.close(descriptor)
        self.assertEqual(result.returncode, 1)
        self.assertIn("We couldn't read your answer", result.stderr)
        self.assertNotIn("Goodbye!", result.stdout)

    def test_28_last_fitting_answer_at_eof(self):
        output = self.run_program("1" + " " * 126)
        self.assertIn("Travel Sample", output)
        self.assertNotIn("Please enter 1, 2, or 3.", output)
        self.assertIn("No more input", output)

    def test_29_overlong_answer_at_eof(self):
        output = self.run_program("1" + " " * 127)
        self.assertEqual(output.count("Please enter 1, 2, or 3."), 1)
        self.assertNotIn("Travel Sample", output)
        self.assertIn("No more input", output)


    def test_30_unknown_argument(self):
        result = subprocess.run([str(PROGRAM), "--wrong"], capture_output=True,
                                text=True, timeout=5)
        self.assertEqual(result.returncode, 1)
        self.assertIn("Run CardClarity without arguments", result.stderr)

    @unittest.skipIf(sys.platform == "win32", "Needs a Windows desktop to check the window")
    def test_31_window_on_other_systems(self):
        result = subprocess.run([str(PROGRAM), "--gui"], capture_output=True,
                                text=True, timeout=5)
        self.assertEqual(result.returncode, 1)
        self.assertIn("window preview needs Windows", result.stderr)


if __name__ == "__main__":
    if not PROGRAM.is_file():
        print("Program not found: " + str(PROGRAM), file=sys.stderr)
        sys.exit(2)
    unittest.main(verbosity=2)
