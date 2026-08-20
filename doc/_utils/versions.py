"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import json
import re
from collections.abc import Callable
from pathlib import Path
from typing import Any, TypeVar, overload

from sphinx.application import Sphinx

VERSIONS_FILE = (Path(__file__).parents[1] / "versions.json").as_posix()

T = TypeVar("T")
VersionFmt = Callable[[str], T]


class Versions:
    """
    An object intended to use as a singleton for accessing version information.
    Intended usage is through get_versions method for example:

        get_versions(app).normalized().minor().all() # Get all minor versions as a list

        get_versions(app).major().latest() # Get latest major version

        get_versions(app).major().latest(lambda v: f"version {v}")
        # Latest major version with custom format
    """

    _major_version_re = re.compile(r"^\d+\.0.0$")
    _minor_version_re = re.compile(r"^\d+\.\d+\.0$")
    _patchlevel_version_re = re.compile(r"^\d+\.\d+\.\d+$")

    def __init__(self, versions: list[str]) -> None:
        self._versions = versions

    @staticmethod
    def from_topdir() -> "Versions":
        """
        Creates the Versions object from doc/versions.json file
        @return New Versions object
        """
        with open(VERSIONS_FILE, "rb") as f:
            return Versions(json.load(f))

    @staticmethod
    def is_major(v: str) -> bool:
        """
        Checks if version is major.

        @param v Version to check
        @return True if version is major
        """
        return bool(Versions._major_version_re.match(v))

    @staticmethod
    def is_minor(v: str) -> bool:
        """
        Checks if version is minor.

        A major version is considered minor but not the other way around.
        @param v Version to check
        @return True if version is minor
        """
        return bool(Versions._minor_version_re.match(v))

    @staticmethod
    def is_patchlevel(v: str) -> bool:
        """
        Checks if version is patchlevel.

        A patchlevel version set here is considered to contain minor and major versions
        @param v Version to check
        @return True if version is minor
        """
        return bool(Versions._patchlevel_version_re.match(v))

    @staticmethod
    def is_placeholder(v: str) -> bool:
        """
        Checks if version is a placeholder for latest with the .99 suffix.

        @param v Version to check
        @return True if version is the placeholder.
        """
        return v.endswith(".99")

    @overload
    def latest(self) -> str | None: ...
    @overload
    def latest(self, fmt: VersionFmt[T]) -> T | None: ...
    def latest(self, fmt: VersionFmt[Any] = lambda v: v) -> Any:
        """
        @return Latest version available, passed through fmt or None if Versions object is empty
        """
        if self._versions:
            return fmt(self._versions[0])
        return None

    def vlatest(self) -> str | None:
        """
        @return Latest version formated with a ``v`` as in: ``v3.4.0``
            or None if Versions object is empty
        """
        return self.latest(Versions._to_vformat)

    @overload
    def all(self) -> list[str]: ...
    @overload
    def all(self, fmt: VersionFmt[T]) -> list[T]: ...
    def all(self, fmt: VersionFmt[Any] = lambda v: v) -> list[Any]:
        """
        @return List of all versions, passed through fmt if given
        """
        return [fmt(v) for v in self._versions]

    def vall(self) -> list[str]:
        """
        @return List of all versions formatted as in ``vlatest``
        """
        return self.all(Versions._to_vformat)

    def normalized(self) -> "Versions":
        """
        Filters versions removing the placeholder version if it's present.
        """
        if not self._versions:
            return self

        if Versions.is_placeholder(self._versions[0]):
            return Versions(self._versions[1:])

        return self

    def matching(self, regex: re.Pattern[str]) -> "Versions":
        """
        Filters versions by regex.
        """
        return Versions([v for v in self._versions if regex.match(v)])

    def major(self) -> "Versions":
        """
        Filters versions to only major versions.
        """
        return self.matching(Versions._major_version_re)

    def minor(self) -> "Versions":
        """
        Filters versions to only minor version (filters out any -addition versions).
        """
        return self.matching(Versions._minor_version_re)

    def patchlevel(self) -> "Versions":
        """
        Filters versions to only patchlevel version (filters out any -addition versions).
        """
        return self.matching(Versions._patchlevel_version_re)

    @staticmethod
    def _to_vformat(v: str) -> str:
        return f"v{v}"

    def __len__(self) -> int:
        return len(self._versions)


def get_versions(app: Sphinx) -> Versions:
    if hasattr(app.env, "ncs_versions"):
        return app.env.ncs_versions
    app.env.ncs_versions = Versions.from_topdir()
    return app.env.ncs_versions
