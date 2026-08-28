#!/usr/bin/env python3
"""
Convert nRF71 RX ADC capture (J-Link mem32 text or raw binary) to I;Q lines.

Sample layout (Nordic radio_test rx_cap_iq_sample):
  3 bytes per sample, little-endian 24-bit word: Q[11:0], I[23:12] (signed 12-bit).

Usage:
  python3 decode_rx_cap.py capture.txt iq.txt
  python3 decode_rx_cap.py capture.bin iq.txt --binary
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

STRIDE = 3


def sign12(v: int) -> int:
    v &= 0xFFF
    return v - 0x1000 if v & 0x800 else v


def decode_triplet(b0: int, b1: int, b2: int) -> tuple[int, int]:
    raw = (b2 << 16) | (b1 << 8) | b0
    return sign12(raw >> 12), sign12(raw & 0xFFF)


def parse_mem32_text(text: str) -> bytes:
    """Parse J-Link 'mem32' console output into raw bytes (little-endian words)."""
    words: list[int] = []
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("J-Link"):
            continue
        m = re.match(r"^[0-9A-Fa-f]+\s*=\s*(.+)$", line)
        if not m:
            continue
        for tok in m.group(1).split():
            words.append(int(tok, 16))

    out = bytearray()
    for w in words:
        out.extend(w.to_bytes(4, "little"))
    return bytes(out)


def convert(data: bytes) -> list[tuple[int, int]]:
    n = len(data) // STRIDE
    samples: list[tuple[int, int]] = []
    for idx in range(n):
        base = idx * STRIDE
        b0, b1, b2 = data[base], data[base + 1], data[base + 2]
        samples.append(decode_triplet(b0, b1, b2))
    return samples


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input", type=Path, help="J-Link mem32 text or raw binary")
    parser.add_argument("output", type=Path, help="I;Q output file (one sample per line)")
    parser.add_argument("--binary", action="store_true",
                        help="Input is raw binary (not J-Link mem32 text)")
    args = parser.parse_args()

    data = args.input.read_bytes() if args.binary else parse_mem32_text(args.input.read_text())
    if not data:
        print("No data parsed.", file=sys.stderr)
        return 1

    samples = convert(data)
    if not samples:
        print("No samples decoded.", file=sys.stderr)
        return 1

    with args.output.open("w", encoding="utf-8") as fh:
        for i_val, q_val in samples:
            fh.write(f"{i_val};{q_val}\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
