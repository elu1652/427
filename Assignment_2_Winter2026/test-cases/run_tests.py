#!/usr/bin/env python3
import os
import re
import subprocess
import sys
from pathlib import Path

MYSH = "../project/src/mysh"   # adjust if needed
TIMEOUT_SEC = 10
ACTUAL_DIR = Path("actual")

TEST_RE = re.compile(r"^T_.*\.txt$", re.IGNORECASE)
EXCLUDE_RE = re.compile(r".*_(result2?|actual)\.txt$", re.IGNORECASE)


def run_shell_on_test(test_path: Path, actual_path: Path):
    with test_path.open("rb") as fin:
        proc = subprocess.run(
            [MYSH],
            stdin=fin,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=TIMEOUT_SEC,
        )

    actual_path.write_bytes(proc.stdout)
    return proc.returncode, proc.stderr.decode(errors="replace")


def diff_files(a: Path, b: Path):
    proc = subprocess.run(
        ["diff", "-y", str(a), str(b)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    if proc.returncode == 0:
        return True, ""
    if proc.returncode == 1:
        return False, proc.stdout.decode(errors="replace")
    return False, proc.stderr.decode(errors="replace")


def find_tests():
    tests = []
    for p in sorted(Path(".").glob("T_*.txt")):
        if EXCLUDE_RE.match(p.name):
            continue
        tests.append(p)
    return tests


def main():
    if not Path(MYSH).exists():
        print(f"ERROR: cannot find {MYSH}")
        return 2

    # Create actual directory if it doesn't exist
    ACTUAL_DIR.mkdir(exist_ok=True)

    tests = find_tests()
    passed = 0
    failed = 0

    print(f"Found {len(tests)} tests.\n")

    for test in tests:
        base = test.stem  # e.g. T_RR4
        actual_path = ACTUAL_DIR / f"{base}_actual.txt"

        expected1 = Path(f"{base}_result.txt")
        expected2 = Path(f"{base}_result2.txt")

        if not expected1.exists() and not expected2.exists():
            print(f"[SKIP] {test.name} (no expected output)")
            continue

        rc, stderr_text = run_shell_on_test(test, actual_path)

        if rc != 0:
            print(f"[FAIL] {test.name} (exit code {rc})")
            if stderr_text.strip():
                print("  stderr:")
                print(stderr_text)
            failed += 1
            continue

        matched = False
        diff_out = ""

        if expected1.exists():
            ok, out = diff_files(actual_path, expected1)
            if ok:
                matched = True
            else:
                diff_out = out

        if not matched and expected2.exists():
            ok, out = diff_files(actual_path, expected2)
            if ok:
                matched = True
            else:
                if not diff_out:
                    diff_out = out

        if matched:
            print(f"[PASS] {test.name}")
            passed += 1
        else:
            print(f"[FAIL] {test.name}")
            print("  diff (first 20 lines):")
            for line in diff_out.splitlines()[:20]:
                print("   ", line)
            failed += 1

    print("\nSummary:")
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())