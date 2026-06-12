#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Pack the manufacturing-app key blob from individual key files into a
# partition image (raw .bin and intelhex .hex). The struct layout MUST stay
# in sync with src/mfg_key_blob.h.

import argparse
import struct
import sys
from pathlib import Path
from typing import Optional

try:
    from cryptography.hazmat.primitives import serialization
    from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey
    from cryptography.hazmat.primitives.asymmetric.ec import EllipticCurvePublicKey
except ImportError:
    sys.exit("ERROR: 'cryptography' package required (pip install cryptography)")

try:
    from intelhex import IntelHex
except ImportError:
    sys.exit("ERROR: 'intelhex' package required (pip install intelhex)")


MFG_KEY_BLOB_MAGIC   = 0x4B47464D
MFG_KEY_BLOB_VERSION = 1

# (field name, length). Order MUST match struct mfg_key_blob.
PAYLOAD_FIELDS = [
    ("urot_pubkey_gen0",     32),
    ("urot_pubkey_gen1",     32),
    ("mfg_app_pubkey",       32),
    ("ikg_seed",             48),
    ("keyram_random0",       16),
    ("keyram_random1",       16),
    ("urot_pubkey_gen0_sig", 64),
    ("urot_pubkey_gen1_sig", 64),
    ("bl1_digest",           32),
    ("bl2_slot_a_digest",    32),
    ("bl2_slot_b_digest",    32),
    ("app_candidate_digest", 32),
]

HEADER_FORMAT = "<IHHI"
HEADER_LEN    = struct.calcsize(HEADER_FORMAT)
PAYLOAD_LEN   = sum(length for _, length in PAYLOAD_FIELDS)
BLOB_LEN      = HEADER_LEN + PAYLOAD_LEN


def pem_to_psa_bytes(pem_path: Path) -> bytes:
    """Return the PSA raw byte form of a PEM public key (Ed25519 -> 32 B,
    P-256 -> 65 B uncompressed)."""
    text = pem_path.read_text()
    if "-----BEGIN" not in text:
        raise ValueError(f"{pem_path.name}: not in PEM format")

    key = serialization.load_pem_public_key(text.encode())
    if isinstance(key, Ed25519PublicKey):
        return key.public_bytes(serialization.Encoding.Raw,
                                serialization.PublicFormat.Raw)
    if isinstance(key, EllipticCurvePublicKey):
        return key.public_bytes(serialization.Encoding.X962,
                                serialization.PublicFormat.UncompressedPoint)
    raise ValueError(f"{pem_path.name}: unsupported key type {type(key).__name__}")


def read_digest(path: Path) -> Optional[bytes]:
    """Return 32 bytes from a 64-char lowercase hex digest file, or None."""
    if not path.exists():
        return None
    txt = path.read_text().strip()
    if len(txt) != 64:
        return None
    try:
        return bytes.fromhex(txt)
    except ValueError:
        return None


def field_bytes(name: str, length: int, keys_dir: Path) -> bytes:
    """Look up the bytes for a single struct field."""
    if name in ("urot_pubkey_gen0", "urot_pubkey_gen1", "mfg_app_pubkey"):
        pem = keys_dir / f"{name}.pem"
        if pem.exists():
            data = pem_to_psa_bytes(pem)
            if len(data) != length:
                raise ValueError(
                    f"{pem.name}: PSA byte length {len(data)} != {length}")
            return data

    if name in ("ikg_seed", "keyram_random0", "keyram_random1"):
        bin_path = keys_dir / f"{name}.bin"
        if bin_path.exists():
            data = bin_path.read_bytes()
            if len(data) != length:
                raise ValueError(
                    f"{bin_path.name}: length {len(data)} != {length}")
            return data

    if name.endswith("_sig"):
        # urot_pubkey_gen0_sig -> key_verification_msgs/urot_pubkey_gen0_signed.msg
        base = name[:-len("_sig")]
        msg = keys_dir / "key_verification_msgs" / f"{base}_signed.msg"
        if msg.exists():
            data = msg.read_bytes()
            if len(data) != length:
                raise ValueError(
                    f"{msg.name}: length {len(data)} != {length}")
            return data

    if name.endswith("_digest"):
        digest = read_digest(keys_dir / f"{name}.txt")
        if digest is not None:
            return digest

    return b"\x00" * length


def build_blob(keys_dir: Path) -> bytes:
    payload = b"".join(field_bytes(n, l, keys_dir) for n, l in PAYLOAD_FIELDS)
    assert len(payload) == PAYLOAD_LEN
    header = struct.pack(HEADER_FORMAT,
                         MFG_KEY_BLOB_MAGIC,
                         MFG_KEY_BLOB_VERSION,
                         0,
                         BLOB_LEN)
    return header + payload


def main() -> None:
    p = argparse.ArgumentParser(description="Pack manufacturing key blob.")
    p.add_argument("--keys-dir", required=True, type=Path)
    p.add_argument("--addr", required=True,
                   help="Partition base address (hex or decimal)")
    p.add_argument("--size", required=True,
                   help="Partition size in bytes (hex or decimal)")
    p.add_argument("--out-bin", required=True, type=Path)
    p.add_argument("--out-hex", required=True, type=Path)
    args = p.parse_args()

    addr = int(args.addr, 0)
    size = int(args.size, 0)

    if not args.keys_dir.is_dir():
        sys.exit(f"--keys-dir not a directory: {args.keys_dir}")

    if BLOB_LEN > size:
        sys.exit(f"blob ({BLOB_LEN} B) exceeds partition size ({size} B)")

    blob = build_blob(args.keys_dir)
    padded = blob + b"\xff" * (size - len(blob))

    args.out_bin.parent.mkdir(parents=True, exist_ok=True)
    args.out_bin.write_bytes(padded)

    ih = IntelHex()
    ih.frombytes(padded, offset=addr)
    args.out_hex.parent.mkdir(parents=True, exist_ok=True)
    ih.write_hex_file(str(args.out_hex))

    print(f"mfg_keys: {BLOB_LEN} B blob, padded to {size} B, base 0x{addr:x}")


if __name__ == "__main__":
    main()
