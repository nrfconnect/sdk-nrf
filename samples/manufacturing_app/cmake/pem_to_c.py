#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# pem_to_c.py
#
# Reads every *.pem file in --keys-dir, every *.bin file in --keys-dir, and
# every *.msg file in --msgs-dir (optional), then emits a C header (--output)
# containing:
#
#   - A byte array for each public key (PSA raw bytes, from .pem).
#   - A byte array for each raw binary key file (.bin).
#   - A byte array for each verification message / signature file (.msg).
#   - A table of mfg_key_info_t entries describing all PEM-derived keys.
#
# .bin files are emitted as named arrays whose identifier matches the
# filename stem (e.g. ikg_seed.bin → ikg_seed[], keyram_random0.bin →
# keyram_random0[]).
#
# Usage:
#   python3 pem_to_c.py --keys-dir <dir> [--msgs-dir <dir>] --output <file.h>
#
# To list SHA-256 fingerprints of all keys in keys-dir (for populating
# KNOWN_PLACEHOLDER_FINGERPRINTS below):
#   python3 pem_to_c.py --keys-dir keys/ --list-fingerprints
#

import argparse
import hashlib
import io
import os
import re
import sys
import tempfile
from pathlib import Path
from typing import Optional

try:
    from cryptography.hazmat.primitives import serialization
    from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey
    from cryptography.hazmat.primitives.asymmetric.ec import EllipticCurvePublicKey
except ImportError:
    print(
        "ERROR: 'cryptography' Python package is required but not installed.\n"
        "       Run: pip install cryptography",
        file=sys.stderr,
    )
    sys.exit(1)

# SHA-256 fingerprints (of the PSA key bytes) of placeholder/example
# keys shipped with this sample. Any key whose fingerprint appears here emits a
# compile-time warning reminding the developer to replace it with a production
# key.
#
# Update this set whenever the placeholder keys change by running:
#   python3 cmake/pem_to_c.py --keys-dir keys/ --list-fingerprints
KNOWN_PLACEHOLDER_FINGERPRINTS: set = {
    # urot_pubkey_gen0.pem — example UROT gen-0 key (Ed25519 seed = 0x01 * 32)
    "34750f98bd59fcfc946da45aaabe933be154a4b5094e1c4abf42866505f3c97e",
    # urot_pubkey_gen1.pem — example UROT gen-1 key (Ed25519 seed = 0x02 * 32)
    "6a3803d5f059902a1c6dafbc9ba4729212f7caac08634cc3ae76b27529f03827",
    # mfg_app_pubkey.pem — example mfg-app authentication key (Ed25519 seed = 0x03 * 32)
    "b62e867fa2f33afe62d5d6b1642e1621d543307846b2a57b897e710919b76709",
}

_C_ARRAY_COLUMNS = 12  # hex bytes per line in generated C arrays


class PemConversionError(Exception):
    pass


def pem_to_psa_bytes(pem_text: str, filename: str) -> bytes:
    """Convert a PEM public key to the PSA byte representation.

    PSA formats:
      Ed25519  → 32 raw bytes (y-coordinate with sign bit)
      P-256    → 65 bytes (04 || x || y, uncompressed point)
      P-384    → 97 bytes (04 || x || y, uncompressed point)

    The CRACEN KMU driver's cracen_kmu_provision() rejects SubjectPublicKeyInfo
    DER (44 bytes for Ed25519, 91 bytes for P-256) with PSA_ERROR_INVALID_ARGUMENT
    because the byte counts do not match any of its accepted key sizes.  We must
    therefore strip the ASN.1 wrapper and return only the raw key material.
    """
    try:
        key = serialization.load_pem_public_key(pem_text.encode("utf-8"))
    except Exception as exc:
        raise PemConversionError(f"Failed to load {filename}: {exc}") from exc

    if isinstance(key, Ed25519PublicKey):
        return key.public_bytes(serialization.Encoding.Raw,
                                serialization.PublicFormat.Raw)
    if isinstance(key, EllipticCurvePublicKey):
        return key.public_bytes(serialization.Encoding.X962,
                                serialization.PublicFormat.UncompressedPoint)
    raise PemConversionError(
        f"{filename}: unsupported key type {type(key).__name__}"
    )


def key_fingerprint(psa_bytes: bytes) -> str:
    """Return the hex SHA-256 fingerprint of PSA key bytes."""
    return hashlib.sha256(psa_bytes).hexdigest()


def is_pem_format(text: str) -> bool:
    """Return True if the text contains a PEM header (-----BEGIN ...)."""
    return "-----BEGIN" in text


def bytes_to_c_array(name: str, data: bytes, comment: str = "") -> str:
    """Format a byte sequence as a C static const uint8_t array."""
    lines = []
    if comment:
        lines.append(f"/* {comment} */")
    lines.append(f"static const uint8_t {name}[] = {{")
    raw = [f"0x{byte_val:02x}" for byte_val in data]
    for i in range(0, len(raw), _C_ARRAY_COLUMNS):
        lines.append("    " + ", ".join(raw[i:i + _C_ARRAY_COLUMNS]) + ",")
    lines.append("};")
    lines.append(f"static const size_t {name}_len = sizeof({name});")
    return "\n".join(lines)


def c_identifier(path: Path) -> str:
    """Convert a Path stem to a valid C identifier."""
    ident = re.sub(r"[^0-9a-zA-Z]", "_", path.stem)
    if ident and ident[0].isdigit():
        ident = "_" + ident
    return ident


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate a C header from PEM keys, .bin, and .msg files."
    )
    parser.add_argument("--keys-dir", required=True,
                        help="Directory containing .pem and .bin files")
    parser.add_argument("--msgs-dir", default=None,
                        help="Directory containing .msg files (optional)")
    parser.add_argument("--output", default=None,
                        help="Path to write the generated header "
                             "(required unless --list-fingerprints is used)")
    parser.add_argument("--list-fingerprints", action="store_true",
                        help="Print SHA-256 fingerprints of all PEM keys and exit")
    parser.add_argument("--quiet", action="store_true",
                        help="Suppress progress output")
    args = parser.parse_args()

    if not args.list_fingerprints and args.output is None:
        parser.error("--output is required unless --list-fingerprints is used")

    keys_dir = Path(args.keys_dir)
    if not keys_dir.is_dir():
        parser.error(
            f"--keys-dir does not exist or is not a directory: {keys_dir}"
        )

    msgs_dir: Optional[Path] = Path(args.msgs_dir) if args.msgs_dir else None
    if msgs_dir is not None and not msgs_dir.is_dir():
        parser.error(
            f"--msgs-dir does not exist or is not a directory: {msgs_dir}"
        )

    pem_files = sorted(keys_dir.glob("*.pem"))
    bin_files = sorted(keys_dir.glob("*.bin"))
    msg_files = sorted(msgs_dir.glob("*.msg")) if msgs_dir else []

    # --list-fingerprints: print fingerprints and exit (used to populate
    # KNOWN_PLACEHOLDER_FINGERPRINTS above)
    if args.list_fingerprints:
        for pem_path in pem_files:
            pem_text = pem_path.read_text()
            if not is_pem_format(pem_text):
                print(f"(placeholder, no PEM data)  {pem_path.name}")
                continue
            try:
                psa_bytes = pem_to_psa_bytes(pem_text, pem_path.name)
                print(f"{key_fingerprint(psa_bytes)}  {pem_path.name}")
            except PemConversionError as exc:
                print(f"ERROR: {exc}", file=sys.stderr)
        return

    buf = io.StringIO()

    def emit(line: str = "") -> None:
        buf.write(line + "\n")

    emit("/*")
    emit(f" * Auto-generated by {Path(__file__).name} — DO NOT EDIT.")
    emit(" *")
    emit(" * Copyright (c) 2026 Nordic Semiconductor ASA")
    emit(" * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause")
    emit(" */")
    emit("#ifndef PROVISIONED_KEYS_H_")
    emit("#define PROVISIONED_KEYS_H_")
    emit()
    emit("#include <stdint.h>")
    emit("#include <stddef.h>")
    emit("#include <stdbool.h>")
    emit()

    key_names = []

    # ----- PEM public-key files -----
    for pem_path in pem_files:
        pem_text = pem_path.read_text()
        name = c_identifier(pem_path)

        # Stage 1: files without a PEM header are plain-text placeholder
        # instructions (not yet replaced with a real key).
        if not is_pem_format(pem_text):
            emit(f"/* WARNING: {pem_path.name} is a placeholder — replace with a real key. */")
            emit(f"static const uint8_t {name}[] = {{0}};")
            emit(f"static const size_t {name}_len = 0;")
            emit(f"static const bool {name}_is_example = true;")
            emit()
            key_names.append((name, pem_path.name, True))
            continue

        # Stage 2: valid PEM file — parse and check against known example
        # key fingerprints.
        try:
            psa_bytes = pem_to_psa_bytes(pem_text, pem_path.name)
        except PemConversionError as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            sys.exit(1)

        is_placeholder = key_fingerprint(psa_bytes) in KNOWN_PLACEHOLDER_FINGERPRINTS

        if is_placeholder:
            emit(f"/* WARNING: {pem_path.name} is a known example key — replace with a production key. */")
            emit(bytes_to_c_array(name, psa_bytes, comment=f"Source: {pem_path.name} (EXAMPLE KEY)"))
            emit(f"static const bool {name}_is_example = true;")
        else:
            emit(bytes_to_c_array(name, psa_bytes, comment=f"Source: {pem_path.name}"))
            emit(f"static const bool {name}_is_example = false;")
        emit()
        key_names.append((name, pem_path.name, is_placeholder))

    # ----- Raw binary key files -----
    if bin_files:
        emit("/* Raw binary key files — replace placeholders with per-device data. */")
    for bin_path in bin_files:
        name = c_identifier(bin_path)
        emit(bytes_to_c_array(
            name, bin_path.read_bytes(),
            comment=f"Generated from keys/{bin_path.name} — replace with per-device data"
        ))
        emit()

    # ----- Verification messages -----
    if msg_files:
        emit("/* Verification messages (pre-signed with the private key counterpart) */")
    for msg_path in msg_files:
        name = c_identifier(msg_path)
        emit(bytes_to_c_array(name, msg_path.read_bytes(),
                              comment=f"Source: {msg_path.name}"))
        # Sentinel macro so C code can use #if defined(PROVISIONED_MSG_HAS_<name>)
        # to detect whether a given message was embedded at build time.
        emit(f"#define PROVISIONED_MSG_HAS_{name} 1")
        emit()

    # ----- Summary table for PEM keys -----
    emit(f"#define MFG_NUM_UROT_KEYS {len(key_names)}U")
    emit()
    emit("typedef struct {")
    emit("    const char     *filename;")
    emit("    const uint8_t  *data;")
    emit("    size_t          len;")
    emit("    bool            is_example;")
    emit("} mfg_key_info_t;")
    emit()
    if key_names:
        emit("static const mfg_key_info_t mfg_urot_keys[] = {")
        for (name, fname, is_ex) in key_names:
            emit(f'    {{ "{fname}", {name}, {name}_len, {str(is_ex).lower()} }},')
        emit("};")
    else:
        emit("/* No PEM keys found — mfg_urot_keys[] table intentionally omitted. */")
    emit()
    emit("#endif /* PROVISIONED_KEYS_H_ */")

    # Atomic write: write to a temp file then rename, so a crash mid-write
    # never leaves a truncated but present output that the build system would
    # not re-run.
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        "w", dir=output_path.parent, delete=False, suffix=".tmp"
    ) as tmp:
        tmp.write(buf.getvalue())
        tmp_path = tmp.name
    os.replace(tmp_path, output_path)

    if not args.quiet:
        print(
            f"Generated: {args.output}  "
            f"({len(key_names)} PEM keys, {len(bin_files)} binary keys, "
            f"{len(msg_files)} messages)"
        )


if __name__ == "__main__":
    main()
