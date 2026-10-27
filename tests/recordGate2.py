"""Build Gate 2, run its tests, and keep the actual commands and output."""
import argparse
from datetime import datetime
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import sys
import tempfile
from zoneinfo import ZoneInfo

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ["CreditCardThing/" + name + ".c" for name in
           ("main", "cards", "logger", "input", "wallet", "storage", "window")]
FLAGS = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang", default="clang")
    parser.add_argument("--mingw", default="x86_64-w64-mingw32-gcc")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--no-leak-check", action="store_true")
    args = parser.parse_args()
    os.chdir(ROOT)
    (ROOT / "build").mkdir(exist_ok=True)
    docs = ROOT / "docs"
    docs.mkdir(exist_ok=True)
    date = datetime.now(ZoneInfo("America/New_York")).isoformat()
    header = "Date: " + date + "\nEnvironment: " + platform.platform()
    builds = ["Gate 2 build log", header]
    tests = ["Gate 2 test log", header]
    demo = ["Gate 2 save-and-reload demonstration", header]
    env = os.environ.copy()
    if args.no_leak_check:
        env["ASAN_OPTIONS"] = "detect_leaks=0"
        tests.append("ASAN_OPTIONS=detect_leaks=0. Leak checking was not run.")
    summary = []

    def run(command, log, *, answers=None, cwd=ROOT):
        result = subprocess.run(command, input=answers, capture_output=True,
                                text=True, timeout=60, cwd=cwd, env=env)
        log.extend(["\nCommand: " + shlex.join(command), "Working folder: " + str(cwd),
                    "Input: " + repr(answers), "Standard output:\n" + result.stdout,
                    "Standard error:\n" + result.stderr, "Exit status: " + str(result.returncode)])
        if result.returncode != 0:
            raise RuntimeError("Failed: " + shlex.join(command))
        return result

    def check_build(compiler, name, extra=()):
        run([compiler, "--version"], builds)
        binary = ROOT / "build" / name
        result = run([compiler, *FLAGS, *extra, *SOURCES, "-o", str(binary)], builds)
        if result.stdout or result.stderr:
            raise RuntimeError("The build produced output; check it before recording a clean build.")
        command = [sys.executable, "tests/testGate2.py", str(binary)]
        run(command, tests)
        run([sys.executable, "tests/testGate1.py", str(binary)], tests)
        module_binary = str(binary) + "Modules"
        run([compiler, *FLAGS, *extra, "tests/testModules.c", *SOURCES[1:], "-o", module_binary], builds)
        run([module_binary], tests)
        summary.append(name + ": clean C11 build; 39 Gate 2 tests, 31 Gate 1 checks, and 40 module checks passed.")

    try:
        check_build("gcc", "cardClarity")
        if shutil.which(args.clang):
            check_build(args.clang, "cardClarityClang")
        else:
            summary.append("Clang was not found; that build was not run.")
        if args.sanitize:
            if not shutil.which(args.clang):
                raise RuntimeError("The sanitizer check needs Clang.")
            check_build(args.clang, "cardClarityChecked",
                        ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"])
        if shutil.which(args.mingw):
            run([args.mingw, "--version"], builds)
            run([args.mingw, *FLAGS, "-DUNICODE", "-D_UNICODE", *SOURCES,
                 "-o", "build/cardClarity.exe", "-lole32", "-luser32", "-lgdi32"], builds)
            summary.append("The Windows cross-build passed. Windows runtime and MSVC were not tested.")
        else:
            summary.append("The Windows cross-compiler was not found; that build was not run.")
        # Restart three times with the same file to check that saving really works.
        with tempfile.TemporaryDirectory(prefix="cardClarityDemo") as folder:
            binary = str(ROOT / "build/cardClarity")
            path = Path(folder) / "savedCards.txt"
            demo.extend(["\nExpected first run: add cards 1 and 3, view them, and save their IDs."])
            first = run([binary], demo, answers="2\n2\n1\n2\n3\n1\n4\n6\n3\n", cwd=folder)
            assert "Card 1: Travel Sample |" in first.stdout and "Card 3: Cashback Sample |" in first.stdout
            assert path.read_bytes() == b"cardClarityWallet1\n2\n1\n3\n"
            demo.append("Actual saved file after first run: " + repr(path.read_bytes()))
            demo.append("\nExpected second run: automatically load both cards, remove card 1, save, and reload card 3.")
            second = run([binary], demo, answers="2\n1\n3\n1\n4\n5\n1\n6\n3\n", cwd=folder)
            assert "Loaded your saved card list." in second.stdout and "Removed Travel Sample." in second.stdout
            assert path.read_bytes() == b"cardClarityWallet1\n1\n3\n"
            demo.append("Actual saved file after second run: " + repr(path.read_bytes()))
            demo.append("\nExpected third run: after another restart, only Cashback Sample is saved.")
            third = run([binary], demo, answers="2\n1\n6\n3\n", cwd=folder)
            assert "Card 3: Cashback Sample |" in third.stdout and "Card 1: Travel Sample |" not in third.stdout
            demo.append("Result: passed. All five actions and persistence across separate processes were observed.")
        summary.append("The three-process save, remove, and reload demonstration passed.")
    finally:
        builds.append("\nVisual Studio and the Windows window have not been run here.")
        tests.extend(["\nRecorded summary:", *summary])
        for filename, lines in (("gate2BuildLog.txt", builds), ("gate2TestLog.txt", tests),
                                ("gate2DemoLog.txt", demo)):
            (docs / filename).write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(summary))


if __name__ == "__main__":
    main()
