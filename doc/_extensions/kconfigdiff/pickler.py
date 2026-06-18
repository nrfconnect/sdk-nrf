#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Kconfig objects from kconfiglib are deeply nested which would result in
recursion errors if trying to pickle. Therefore this module introduces
utilities for flattening the kconfig object so that it can be pickled.
"""

import pickle
import sys
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import IO, Any
from zipfile import ZIP_LZMA, ZipFile

sys.path.insert(0, str(Path(__file__).parents[4] / "zephyr/scripts/kconfig"))

import kconfiglib

KCONFIG_TYPES = (
    kconfiglib.Symbol,
    kconfiglib.Choice,
    kconfiglib.MenuNode,
    kconfiglib.Variable,
    kconfiglib.Kconfig,
)

CONTAINER_TYPES = (tuple, list, set, frozenset, dict)

SUPPORTED_TYPES = (str, bytes, bool, int, float, type(None), Path)


class Ref:
    """A placeholder standing in for a reference to another kconfig object.
    Python's `id()` function is used for checking assuring unique identifiers
    for each object.
    """

    __slots__ = ("id", "type")

    def __init__(self, value: Any):
        self.id = id(value)
        self.type = type(value)


def _all_slots(cls) -> list[str]:
    """Collect ``__slots__`` across the full MRO"""
    slots: list[str] = []
    for klass in cls.__mro__:
        slots.extend(getattr(klass, "__slots__", ()))
    return slots


def flatten_kconfig(kconfig: kconfiglib.Kconfig) -> dict:
    """
    Flattens a Kconfig object graph.
    Returns a flattened object which can be pickled. Use
    :func:`unflatten_kconfig` to retrieve the original object.
    """
    objects: dict[int, Any] = {}
    queue: list = [kconfig]
    seen: set[int] = {id(kconfig)}

    def encode(value):
        """Return a picklable stand-in for ``value`` and if value hasn't been
        flattened yet appends it to queue.

        Kconfig objects and containers become a :class:`Ref` (and are queued
        for processing); plain data types from `SUPPORTED_TYPES` remain as is,
        because the don't cause deep recursion; Any other types are dropped.
        """
        if isinstance(value, Ref):
            return value
        if type(value) in KCONFIG_TYPES or isinstance(value, CONTAINER_TYPES):
            if id(value) not in seen:
                seen.add(id(value))
                queue.append(value)
            return Ref(value)
        if isinstance(value, SUPPORTED_TYPES):
            return value
        return None

    while queue:
        obj = queue.pop()
        if type(obj) in KCONFIG_TYPES:
            for slot in _all_slots(type(obj)):
                try:
                    value = getattr(obj, slot)
                except AttributeError:
                    # Slot was never assigned on this instance; leave it out.
                    continue
                setattr(obj, slot, encode(value))
            objects[id(obj)] = obj
        elif isinstance(obj, dict):
            # Keys in kconfiglib containers are plain scalars (symbol names,
            # ints); keep them as-is and only rewrite the values.
            objects[id(obj)] = {k: encode(v) for k, v in obj.items()}
        else:  # list, tuple, set or frozenset
            objects[id(obj)] = type(obj)(encode(v) for v in obj)

    return objects


def resolve(value, immutable, flat, rebuilt):
    if not isinstance(value, Ref):
        return value
    if value.id in immutable:
        return rebuilt[value.id]
    return flat[value.id]


def rebuild_immutable(flat):
    immutable = {oid for oid, obj in flat.items() if isinstance(obj, (tuple, set, frozenset))}

    rebuilt: dict[int, Any] = {}
    for start in immutable:
        if start in rebuilt:
            continue
        stack = [start]
        while stack:
            oid = stack[-1]
            if oid in rebuilt:
                stack.pop()
                continue
            sub = flat[oid]
            pending = [
                v.id
                for v in sub
                if isinstance(v, Ref) and v.id in immutable and v.id not in rebuilt
            ]
            if pending:
                stack.extend(pending)
                continue
            rebuilt[oid] = type(sub)(resolve(v, immutable, flat, rebuilt) for v in sub)
            stack.pop()
    return immutable, rebuilt


def unflatten_kconfig(flat: dict) -> kconfiglib.Kconfig:
    """Rebuild a Kconfig object graph produced by :func:`flatten_kconfig`.

    Returns the reconstructed root :class:`kconfiglib.Kconfig`.
    """
    # Immutable containers cannot be patched in place, so they are rebuilt;
    # everything else (kconfig objects, dicts, lists) keeps its identity and is
    # patched in place. ``rebuilt`` maps the original id to its rebuilt value.

    immutable, rebuilt = rebuild_immutable(flat)

    for oid, obj in flat.items():
        if oid in immutable:
            continue
        if isinstance(obj, dict):
            for k in list(obj):
                obj[k] = resolve(obj[k], immutable, flat, rebuilt)
        elif isinstance(obj, list):
            for i, value in enumerate(obj):
                obj[i] = resolve(value, immutable, flat, rebuilt)
        else:  # kconfig object
            for slot in _all_slots(type(obj)):
                try:
                    value = getattr(obj, slot)
                except AttributeError:
                    continue
                setattr(obj, slot, resolve(value, immutable, flat, rebuilt))

    return next(iter(flat.values()))


class _RefUnpickler(pickle.Unpickler):
    """Unpickler that resolves :class:`Ref` regardless of the module it was
    pickled from.

    Snapshots may have been generated while this file was importable as a
    top-level ``pickler`` module (or executed as ``__main__``). When the
    extension is later imported as the ``kconfigdiff`` package (e.g. during a
    Sphinx build) those modules no longer define ``Ref``; redirect it here.
    """

    def __init__(self, *k, kconfiglib_=None, **kw):
        super().__init__(*k, **kw)

        if kconfiglib_ is None:
            self.kconfiglib = kconfiglib
            return

        self.kconfiglib = kconfiglib_

    def find_class(self, module: str, name: str):
        if module == "kconfiglib":
            return getattr(self.kconfiglib, name)
        if name == "Ref":
            return Ref
        return super().find_class(module, name)


def load_file(path: Path | str | IO[bytes]):
    with (
        ZipFile(path, "r", compression=ZIP_LZMA) as zip_file,
        TemporaryDirectory() as td,
    ):
        kconfiglib_path = Path(td) / "old_kconfiglib.py"
        with open(kconfiglib_path, "wb") as f:
            f.write(zip_file.read("kconfiglib.py"))

        sys.path.insert(0, str(td))
        import old_kconfiglib

        with zip_file.open("kconfig.pickle", "r") as f:
            kconfig, sys_kconfig = _RefUnpickler(f, kconfiglib_=old_kconfiglib).load()
        return unflatten_kconfig(kconfig), unflatten_kconfig(sys_kconfig), old_kconfiglib


def save_kconfig(
    path: Path | str, kconfig: kconfiglib.Kconfig, sysbuild_kconfig: kconfiglib.Kconfig
):
    with ZipFile(path, "w", compression=ZIP_LZMA) as zip_file:
        with zip_file.open("kconfig.pickle", "w") as f:
            pickle.dump((flatten_kconfig(kconfig), flatten_kconfig(sysbuild_kconfig)), f)
        # store the kconfiglib.py file so that the pickle can be opened using
        # the same version of kconfiglib
        zip_file.write(kconfiglib.__file__, arcname="kconfiglib.py")
