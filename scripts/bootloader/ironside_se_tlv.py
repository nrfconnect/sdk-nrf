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
                parser.error(f"symbol '{PERIPHCONF_START_SYMBOL}' not found in ELF: {fp.name}")
            if not end_syms:
                parser.error(f"symbol '{PERIPHCONF_END_SYMBOL}' not found in ELF: {fp.name}")

            addr_vma = start_syms[0]["st_value"]
            end = end_syms[0]["st_value"]
            size = end - addr_vma

            if size < 0:
                parser.error(
                    f"invalid iterable range in ELF {fp.name}: "
                    f"{PERIPHCONF_START_SYMBOL}=0x{addr_vma:09_x} > "
                    f"{PERIPHCONF_END_SYMBOL}=0x{end:09_x}"
                )

            if size % 8 != 0:
                parser.error(f"PERIPHCONF section size is not divisible by 8: {size}")

            # When CONFIG_XIP=n, code/data run from RAM (VMA) but are stored in the image at LMA.
            addr_lma = vma_to_lma(elf, addr_vma, size)
            if addr_lma is None:
                parser.error(
                    f"PERIPHCONF range [0x{addr_vma:x}, 0x{addr_vma + size:x}) not in any PT_LOAD "
                    f"region in ELF: {fp.name}"
                )
            offset = addr_lma - args.image_fw_base
            if offset < 0:
                parser.error(
                    f"periphconf start LMA (0x{addr_lma:x}) is before "
                    f"image FW base (0x{args.image_fw_base:x}) in ELF: {fp.name}"
                )
            count = size // 8
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


def vma_to_lma(elf: ELFFile, vma: int, size: int = 0) -> int | None:
    for segment in elf.iter_segments():
        if segment["p_type"] != "PT_LOAD":
            continue
        p_vaddr = segment["p_vaddr"]
        p_memsz = segment["p_memsz"]
        p_filesz = segment["p_filesz"]
        if not p_vaddr <= vma < p_vaddr + p_memsz:
            continue
        if p_filesz == 0 or vma >= p_vaddr + p_filesz:
            return None
        if vma + size > p_vaddr + p_filesz:
            return None
        p_paddr = segment["p_paddr"]
        return p_paddr + (vma - p_vaddr)
    return None


if __name__ == "__main__":
    main()
