#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Generate custom TLV data containing parameters to pass to IronSide SE
to initialize peripherals.
"""

import argparse

from elftools.elf.elffile import ELFFile

# PERIPHCONF data consists of 'struct periphconf_entry' stored in an iterable section.
# The iterable sections have two symbols _<struct name>_list_start and _<struct name>_list_end
# which mark the start and end of the list of structures.
PERIPHCONF_START_SYMBOL = "_periphconf_entry_list_start"
PERIPHCONF_END_SYMBOL = "_periphconf_entry_list_end"

# Size of each entry (4B register pointer, 4B register value)
PERIPHCONF_ENTRY_SIZE = 8


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument(
        "--image-fw-base",
        required=True,
        type=lambda x: int(x, 0),
        help=(
            "Absolute address of the start of the firmware blob (slot_start + header_size). "
            "Offsets in the generated TLV entries are relative to this address."
        ),
    )
    parser.add_argument(
        "--elf",
        required=True,
        dest="elfs",
        action="append",
        type=argparse.FileType("rb"),
        help="Path to an ELF file containing constant data to generate TLVs for",
    )
    parser.add_argument(
        "--periphconf-tlv-out",
        default=None,
        type=argparse.FileType("wb"),
        help="Binary file to write: for each ELF, 4-byte offset + 4-byte size (little-endian).",
    )
    args = parser.parse_args()

    periphconf_tlv_params = []

    for fp in args.elfs:
        elf = ELFFile(fp)
        symtab = elf.get_section_by_name(".symtab")
        if symtab is None:
            parser.error(f"ELF has no .symtab: {fp.name}")

        if args.periphconf_tlv_out:
            start_syms = symtab.get_symbol_by_name(PERIPHCONF_START_SYMBOL)
            end_syms = symtab.get_symbol_by_name(PERIPHCONF_END_SYMBOL)
            if not start_syms:
                parser.error(
                    f"failed to locate PERIPHCONF section in ELF {fp.name}: "
                    f"missing symbol '{PERIPHCONF_START_SYMBOL}'"
                )
            if not end_syms:
                parser.error(
                    f"failed to locate PERIPHCONF section in ELF {fp.name}: "
                    f"missing symbol '{PERIPHCONF_END_SYMBOL}'"
                )

            addr_vma = start_syms[0]["st_value"]
            end = end_syms[0]["st_value"]
            size = end - addr_vma

            if size < 0:
                parser.error(
                    f"invalid iterable range in ELF {fp.name}: "
                    f"{PERIPHCONF_START_SYMBOL}=0x{addr_vma:09_x} > "
                    f"{PERIPHCONF_END_SYMBOL}=0x{end:09_x}"
                )

            if size % PERIPHCONF_ENTRY_SIZE != 0:
                parser.error(
                    f"PERIPHCONF section size is not divisible by {PERIPHCONF_ENTRY_SIZE}: {size}"
                )

            # When CONFIG_XIP=n, code/data run from RAM (VMA) but are stored in the image at LMA.
            try:
                addr_lma = vma_to_lma(elf, addr_vma, size)
            except LmaNotFoundError:
                parser.error(
                    f"failed to map PERIPHCONF range [0x{addr_vma:x}, 0x{addr_vma + size:x}) in "
                    f"ELF {fp.name} to its nonvolatile location: VMA address did not match any "
                    f"PT_LOAD segment in ELF {fp.name}."
                )
            except LmaOutOfBoundsError:
                parser.error(
                    f"PERIPHCONF range [0x{addr_vma:x}, 0x{addr_vma + size:x}) is placed in "
                    f"uninitialized memory range in ELF: {fp.name}: the p_filesz of the matching "
                    f"PT_LOAD segment in ELF {fp.name} is less than the size of the VMA range."
                )
            offset = addr_lma - args.image_fw_base
            if offset < 0:
                parser.error(
                    f"failed to map PERIPHCONF nonvolatile start address (0x{addr_lma:x}) in "
                    f"ELF {fp.name} to image firmware region offset. Address is less than the "
                    f"the image firmware base address (0x{args.image_fw_base:x}). "
                )
            count = size // PERIPHCONF_ENTRY_SIZE
            periphconf_tlv_params.append((offset, count, fp.name))

    # Sort by ascending offset for more predictable output
    periphconf_tlv_params.sort(key=lambda entry: entry[0])

    if args.periphconf_tlv_out:
        print(f"PERIPHCONF TLV contents (img base 0x{args.image_fw_base:09_x}):")
        for i, (offset, count, filename) in enumerate(periphconf_tlv_params):
            print(
                f"  [{i}] offset 0x{offset:_x} (abs. 0x{args.image_fw_base + offset:09_x}), "
                f"{count} entries (from {filename})"
            )
            args.periphconf_tlv_out.write(offset.to_bytes(4, byteorder="little"))
            args.periphconf_tlv_out.write(count.to_bytes(4, byteorder="little"))


class LmaNotFoundError(RuntimeError):
    """No PT_LOAD segment contained the VMA."""


class LmaOutOfBoundsError(RuntimeError):
    """The VMA range size exceeds the p_filesz of its matching PT_LOAD segment."""


def vma_to_lma(elf: ELFFile, vma: int, size: int = 0) -> int:
    for segment in elf.iter_segments():
        if segment["p_type"] != "PT_LOAD":
            continue

        p_vaddr = segment["p_vaddr"]
        p_memsz = segment["p_memsz"]
        p_filesz = segment["p_filesz"]
        if not p_vaddr <= vma <= vma + size < p_vaddr + p_memsz:
            continue
        if not p_vaddr <= vma <= vma + size < p_vaddr + p_filesz:
            raise LmaOutOfBoundsError()
        p_paddr = segment["p_paddr"]
        return p_paddr + (vma - p_vaddr)

    raise LmaNotFoundError()


if __name__ == "__main__":
    main()
