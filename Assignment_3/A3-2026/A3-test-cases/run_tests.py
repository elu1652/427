#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

MYSH = "../project/src/mysh"
SRC_DIR = "../project/src"
TIMEOUT_SEC = 10
ACTUAL_DIR = Path("actual")

TEST_CONFIG = {
    "tc1": (18, 10),
    "tc2": (18, 10),
    "tc3": (21, 10),
    "tc4": (18, 10),
    "tc5": (6, 10),
}


def build_mysh(framesize: int, varmemsize: int):
    proc = subprocess.run(
        ["make", "clean"],
        cwd=SRC_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if proc.returncode != 0:
        return False, proc.stderr.decode(errors="replace") or proc.stdout.decode(errors="replace")

    proc = subprocess.run(
        ["make", "mysh", f"framesize={framesize}", f"varmemsize={varmemsize}"],
        cwd=SRC_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if proc.returncode != 0:
        return False, proc.stderr.decode(errors="replace") or proc.stdout.decode(errors="replace")

    return True, ""


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
    for p in sorted(Path(".").glob("tc*.txt")):
        name = p.name.lower()
        if "_result" in name or "_actual" in name:
            continue
        tests.append(p)
    return tests


def main():
    ACTUAL_DIR.mkdir(exist_ok=True)

    tests = find_tests()
    passed = 0
    failed = 0

    print(f"Found {len(tests)} tests.\n")

    for test in tests:
        base = test.stem
        actual_path = ACTUAL_DIR / f"{base}_actual.txt"
        expected1 = Path(f"{base}_result.txt")
        expected2 = Path(f"{base}_result2.txt")

        cfg = TEST_CONFIG.get(base)
        if cfg is None:
            print(f"[SKIP] {test.name} (no build config)")
            continue

        framesize, varmemsize = cfg
        ok, build_msg = build_mysh(framesize, varmemsize)
        if not ok:
            print(f"[FAIL] {test.name} (build failed)")
            print(build_msg)
            failed += 1
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
            elif not diff_out:
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