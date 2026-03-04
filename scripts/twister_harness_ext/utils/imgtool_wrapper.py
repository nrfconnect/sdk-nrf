# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Wrappers and utilities for imgtool signing and key generation for MCUboot images."""

from __future__ import annotations

import logging
import shlex
from dataclasses import dataclass
from pathlib import Path
from subprocess import check_output

logger = logging.getLogger(__name__)


@dataclass
class ImgtoolParams:
    """Parameters for imgtool signing and key generation."""

    align: str
    header_size: str
    slot_size: str
    version: str = "0.0.0+0"
    tool_path: str = "imgtool"
    rom_fixed: str | None = None  # for direct-xip
    image_magic: bool = False  # used when signing app.hex to add --pad option
    key_file: Path | None = None
    encryption_key_file: Path | None = None
    sha: int | None = None
    hmac_sha: int | None = None
    pure: bool = False
    compression: str | None = None
    pad_header: bool = True
    security_counter: int | None = None

    def increase_version(self) -> None:
        """Increase the major version in the version string."""
        major, rest = self.version.split(".", 1)
        self.version = f"{int(major) + 1}.{rest}"


def imgtool_sign(
    app_to_sign: Path,
    imgtool_params: ImgtoolParams,
    output_bin: Path | None = None,
    extra_args: list[str] | None = None,
    timeout=10,
) -> Path:
    """Wrap the `imgtool sign` command."""
    command = [
        imgtool_params.tool_path,
        "sign",
        "--align",
        imgtool_params.align,
        "--header-size",
        imgtool_params.header_size,
        "--slot-size",
        imgtool_params.slot_size,
        "--version",
        imgtool_params.version,
    ]
    extend_output_name = ""
    if imgtool_params.pad_header:
        command.extend(["--pad-header"])
    if imgtool_params.rom_fixed:
        command.extend(["--rom-fixed", imgtool_params.rom_fixed])
    if imgtool_params.key_file:
        command.extend(["--key", str(imgtool_params.key_file)])
        extend_output_name += ".signed"
    if imgtool_params.encryption_key_file:
        command.extend(["--encrypt", str(imgtool_params.encryption_key_file)])
        extend_output_name += ".encrypted"
    if imgtool_params.image_magic:
        command.extend(["--pad"])
    if imgtool_params.sha:
        command.extend(["--sha", str(imgtool_params.sha)])
    if imgtool_params.pure:
        command.extend(["--pure"])
    if imgtool_params.hmac_sha:
        command.extend(["--hmac-sha", str(imgtool_params.hmac_sha)])
    if imgtool_params.compression:
        command.extend(["--compression", imgtool_params.compression])
    if imgtool_params.security_counter:
        command.extend(["--security-counter", str(imgtool_params.security_counter)])
        extend_output_name = f"_sec{imgtool_params.security_counter}" + extend_output_name
    if extra_args:
        command.extend(extra_args)

    if not output_bin:
        output_bin = app_to_sign.parent / "{}_{}{}.bin".format(
            app_to_sign.stem,
            imgtool_params.version.replace(".", "_").replace("+", "_"),
            extend_output_name,
        )

    command.extend([str(app_to_sign), str(output_bin)])

    logger.info(f"CMD: {shlex.join(command)}")
    output = check_output(command, text=True, timeout=timeout)
    logger.debug(f"OUT: {output}")
    return output_bin


def match_key_type(sig_type: str) -> str:
    """Match SB_CONFIG_SIGNATURE_TYPE to key type."""
    match sig_type:
        case "ECDSA_P256":
            return "ecdsa-p256"
        case "RSA":
            return "rsa-2048"
        case "ED25519":
            return "ed25519"
        case _:
            logger.warning(f"Unsupported signature type: {sig_type}")
            return ""


def imgtool_keygen(key_file: str | Path, key_type: str, imgtool: str | Path = "imgtool") -> Path:
    """Wrap the `imgtool keygen` command."""
    if key_type not in ["rsa-2048", "rsa-3072", "ecdsa-p256", "ecdsa-p384", "ed25519", "x25519"]:
        if matched_key_type := match_key_type(key_type):
            key_type = matched_key_type
        else:
            raise ValueError(f"Unsupported key type: {key_type}")

    command = [str(imgtool), "keygen", "--key", str(key_file), "--type", key_type]
    logger.info(f"CMD: {shlex.join(command)}")
    output = check_output(command, text=True)
    logger.debug(f"OUT: {output}")
    return Path(key_file)
