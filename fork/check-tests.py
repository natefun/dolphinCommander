#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""
Compares a ctest run with the test failures that dolphinCommander expects.

Some of Dolphin's tests fail on dolphinCommander on purpose: they test what the fork changed,
such as the split view that became two sides. Those failures are listed, with the reason, in
fork/expected-test-failures.txt. This script fails only when something else fails, so a merge
of a newer Dolphin is judged by what it broke and not by what was already different.

Usage: fork/check-tests.py <ctest junit file> [--write-expected]
  The junit file comes from `ctest --output-junit <file>`.
  --write-expected rewrites the expected list from this run, keeping the reasons that are there.
"""

import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

EXPECTED_FILE = Path(__file__).resolve().parent / "expected-test-failures.txt"
FAIL_LINE = re.compile(r"^FAIL!\s+:\s+(\w+::\w+\([^)]*\))")


def read_expected():
    """Returns {entry: reason}. An entry is Class::function(data) or suite:<ctest name>."""
    expected = {}
    reason = ""
    for line in EXPECTED_FILE.read_text().splitlines():
        line = line.strip()
        if not line:
            reason = ""
        elif line.startswith("#"):
            reason = line.lstrip("# ").strip()
        else:
            expected[line] = reason
    return expected


def read_run(junit_path):
    """Returns (failed functions, suites that failed without a failed function)."""
    failures = set()
    bare_suites = set()
    root = ET.parse(junit_path).getroot()
    for case in root.iter("testcase"):
        if case.get("status") in ("run", "notrun", "disabled"):
            continue
        output = case.findtext("system-out") or ""
        found = {m.group(1) for m in map(FAIL_LINE.match, output.splitlines()) if m}
        failures |= found
        # A suite that failed with no failed function crashed, timed out or is not a Qt test.
        if not found or "Totals:" not in output:
            bare_suites.add("suite:" + case.get("name"))
    return failures, bare_suites


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    failures, bare_suites = read_run(sys.argv[1])
    actual = failures | bare_suites
    expected = read_expected()

    if "--write-expected" in sys.argv:
        lines = ["# Tests that fail on dolphinCommander on purpose. A comment line gives the reason for the",
                 "# entries below it, up to the next empty line. See fork/README.md.", ""]
        by_reason = {}
        for entry in sorted(actual):
            by_reason.setdefault(expected.get(entry, "TODO: say why this fails on the fork"), []).append(entry)
        for reason, entries in by_reason.items():
            lines += ["# " + reason] + entries + [""]
        EXPECTED_FILE.write_text("\n".join(lines))
        print(f"Wrote {len(actual)} entries to {EXPECTED_FILE}")
        return 0

    unexpected = sorted(actual - set(expected))
    now_passing = sorted(set(expected) - actual)

    print(f"{len(actual)} failures, {len(actual) - len(unexpected)} of them expected.")
    if now_passing:
        print("\nExpected to fail but passed (flaky, or fixed; can be removed from the list):")
        for entry in now_passing:
            print("  " + entry)
    if unexpected:
        print("\nNEW FAILURES:")
        for entry in unexpected:
            print("  " + entry)
        return 1
    print("\nNo new failures.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
