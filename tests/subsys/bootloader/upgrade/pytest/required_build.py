# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Utilities for managing required builds and images for MCUboot test scenarios."""

from __future__ import annotations

import logging
import shlex
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

from filelock import FileLock, Timeout
from helpers import run_command, timer
from twister_harness import DeviceAdapter
from twister_harness.helpers.domains_helper import get_default_domain_name
from twister_harness.helpers.utils import find_in_config

logger = logging.getLogger(__name__)


class RequiredBuildException(Exception):
    """Exception raised for errors in required build operations."""


@dataclass
class RequiredBuild:
    """Class for managing required build directories and operations."""

    source_dir: Path
    build_dir: Path
    board: str
    testsuite: str = ""
    suffix: str = ""
    extra_args: str = ""
    timeout: int = 300
    build_info: BuildInfo | None = None

    @classmethod
    def create_from_dut(
        cls,
        dut: DeviceAdapter,
        source_dir: Path | str | None = None,
        build_dir: Path | str | None = None,
        testsuite: str | None = None,
        suffix: str | None = None,
        board: str | None = None,
        extra_args: str = "",
        timeout: int = 300,
    ):
        """Create a RequiredBuild instance from a DeviceAdapter and build parameters."""
        if not source_dir:
            source_dir = find_in_config(
                dut.device_config.build_dir / "CMakeCache.txt", "APP_DIR:PATH"
            ) or find_in_config(
                dut.device_config.build_dir / "CMakeCache.txt", "APPLICATION_SOURCE_DIR:PATH"
            )
            testsuite = testsuite or dut.device_config.build_dir.name
        if not source_dir:
            raise RequiredBuildException("Not found source dir")
        source_dir = Path(source_dir)
        if not source_dir.exists():
            raise RequiredBuildException(f"Not found source dir: {source_dir}")
        if not build_dir:
            test_dir = testsuite or "required_build"
            build_dir = build_dir or dut.device_config.build_dir.parent / test_dir
        if suffix:
            build_dir = str(build_dir) + suffix
        build_dir = Path(build_dir)  # type: ignore
        if build_dir == dut.device_config.build_dir:
            raise RequiredBuildException(
                "Build dir is the same as the current build dir, use suffix"
            )
        board = (
            board
            or dut.device_config.platform
            or find_in_config(dut.device_config.build_dir / "CMakeCache.txt", "BOARD:STRING")
        )
        if not board:
            raise RequiredBuildException("Not found board")

        return cls(
            source_dir=source_dir,
            build_dir=build_dir,
            testsuite=testsuite,  # type: ignore
            suffix=suffix,  # type: ignore
            board=board,
            extra_args=extra_args,
            timeout=timeout,
        )

    @timer
    def west_build(self):
        """Run west build for the required build configuration."""
        command = [
            "west",
            "build",
            "-p",
            "-b",
            self.board,
            str(self.source_dir),
            "-d",
            str(self.build_dir),
        ]
        if self.testsuite:
            command.extend(["-T", self.testsuite])
        if self.extra_args:
            command.extend(shlex.split(self.extra_args))

        try:
            run_command(command, timeout=self.timeout)
        except subprocess.CalledProcessError:
            # create empty file to indicate a build error, will be used
            # to avoid unnecessary builds in other tests
            Path(self.build_dir / "build.error").touch()
            raise RequiredBuildException("Failed to build required app") from None
        except subprocess.TimeoutExpired:
            logger.error("Timeout building required app")
            shutil.rmtree(self.build_dir)
            raise RequiredBuildException("Timeout building required app") from None

    def get_ready_build(self):
        """Get or create a ready build directory, using file locking to avoid conflicts."""
        lockfile: Path = Path(str(self.build_dir) + ".lock")
        wait_for_build = False
        try:
            with FileLock(str(lockfile), timeout=0):
                if self.build_dir.exists():
                    if (self.build_dir / "build.error").exists():
                        raise RequiredBuildException("Previous build failed, do not rebuild")
                    if self.is_build_ready():
                        logger.info("Reuse existing build")
                        return self.build_dir
                logger.info("Building required app")
                self.west_build()
        except Timeout:
            logger.info("Another instance is already running. Wait for it to finish")
            wait_for_build = True

        if wait_for_build:
            try:
                logging.getLogger("filelock").setLevel(logging.WARNING)
                with FileLock(str(lockfile), timeout=self.timeout):
                    pass
            except Timeout:
                raise RequiredBuildException(f"Timeout waiting for {self.build_dir}") from None

        if not self.is_build_ready():
            raise RequiredBuildException(f"Build is not ready: {self.build_dir}")
        return self.build_dir

    def is_build_ready(self) -> bool:
        """Check if the build directory contains a ready build."""
        try:
            self.build_info = BuildInfo.create_from_req_build(self)
        except Exception as e:
            logger.debug(f"Build not ready: {e}")
            return False
        return self.build_info.is_ready


@dataclass
class BuildInfo:
    """Class for storing information about a build directory."""

    build_dir: Path
    is_ready: bool = False
    pm: bool = False
    sysbuild: bool = False
    default_domain: str = ""
    network_core: str = ""

    @classmethod
    def create_from_req_build(cls, req_build: RequiredBuild):
        """Create BuildInfo from a RequiredBuild instance."""
        return cls(build_dir=req_build.build_dir)

    def __post_init__(self):
        """Post-initialization to set build info flags and check readiness."""
        if (self.build_dir / "pm.config").exists():
            self.pm = True
        if (self.build_dir / "domains.yaml").exists():
            self.sysbuild = True
            self.default_domain = get_default_domain_name(self.build_dir / "domains.yaml")  # type: ignore
        self.is_ready = self.check_build_ready()

    def check_build_ready(self) -> bool:
        """Check if the build is ready based on build info flags and files."""
        if self.pm:
            if self.sysbuild:
                return (self.build_dir / "merged.hex").exists()
            else:
                return (self.build_dir / "zephyr" / "merged.hex").exists()
        if self.sysbuild:
            return (self.build_dir / self.default_domain / "zephyr" / "zephyr.hex").exists()
        return (self.build_dir / "zephyr" / "zephyr.hex").exists()


def get_required_build(dut: DeviceAdapter, **kwargs) -> Path:
    """Get a ready build directory for the given DeviceAdapter and parameters."""
    req_build = RequiredBuild.create_from_dut(dut, **kwargs)
    return req_build.get_ready_build()


def get_required_images_to_update(
    dut: DeviceAdapter, sign_version: str, firmware_version: int, netcore_name: str | None = None
) -> tuple[Path, Path, Path]:
    """Get required images to update for a given DeviceAdapter and build parameters."""
    extra_args = " --"
    extra_args += f' -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\\"{sign_version}\\"'
    extra_args += f" -Dmcuboot_CONFIG_FW_INFO_FIRMWARE_VERSION={firmware_version}"
    if netcore_name:
        extra_args += f" -D{netcore_name}_CONFIG_FW_INFO_FIRMWARE_VERSION={firmware_version}"
        extra_args += f" -Db0n_CONFIG_FW_INFO_FIRMWARE_VERSION={firmware_version}"

    req_build = RequiredBuild.create_from_dut(
        dut=dut,
        suffix=f"_v{firmware_version}_sign" + sign_version.replace(".", "_").replace("+", "_"),
        extra_args=extra_args,
    )
    req_build_dir = req_build.get_ready_build()

    updated_app = (
        req_build_dir / req_build.build_info.default_domain / "zephyr" / "zephyr.signed.bin"
    )  # type: ignore
    updated_netcore = req_build_dir / f"signed_by_mcuboot_and_b0_{netcore_name}.bin"
    s1_image = req_build_dir / "signed_by_mcuboot_and_b0_s1_image.bin"
    return updated_app, updated_netcore, s1_image
