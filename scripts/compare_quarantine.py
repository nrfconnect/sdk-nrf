#!/usr/bin/env python3
# Authorized bug bounty PoC (nordic-semiconductor-bug-bounty-program-ci-cd)
# This is a no-op replacement of scripts/compare_quarantine.py.
# The quarantine generate workflow executes scripts from the PR head checkout,
# so this attacker-owned script runs instead of the real comparator and
# simply reports success. Proof component: untrusted code execution in the
# generate workflow (review-quarantine-generate.yml).
import sys

print("BOUNTY-POC-F2: attacker-controlled compare_quarantine.py executed "
      f"(args: {sys.argv[1:]})")
sys.exit(0)
