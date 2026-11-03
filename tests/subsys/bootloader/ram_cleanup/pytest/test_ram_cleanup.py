# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import time
from pathlib import Path

from elftools.elf.elffile import ELFFile
from intelhex import IntelHex
from twister_harness import DeviceAdapter
from twister_harness_ext.adapters.cmd import run_command


def dump_memory_to_file(
    address: int, size: int, filename: Path, serial_number: str | None = None
) -> None:
    """Dump memory to a file."""
    cmd = [
        "nrfutil",
        "device",
        "read",
        "--direct",
        "--address",
        hex(address),
        "--bytes",
        hex(size),
        "--width",
        "32",
        "--to-file",
        str(filename),
    ]
    if serial_number:
        cmd.extend(["--serial-number", serial_number])

    run_command(cmd)


def get_ramfunc_region(elf_path: Path) -> tuple[int, int]:
    """Return (start, end) of the .ramfunc region, which MCUboot does not clean up."""
    with open(elf_path, "rb") as f:
        symtab = ELFFile(f).get_section_by_name(".symtab")
        start = symtab.get_symbol_by_name("__ramfunc_region_start")[0]["st_value"]
        end = symtab.get_symbol_by_name("__ramfunc_end")[0]["st_value"]
    return start, end


def test_ram_cleanup(dut: DeviceAdapter, config_reader):
    """Verify that the RAM cleanup works properly and RAM contains only 0x00 values."""
    time.sleep(2)  # give some times for ram cleanup after flashing
    reader = config_reader(dut.device_config.build_dir / "mcuboot/zephyr/.config")
    ram_address = reader.read_int("CONFIG_SRAM_BASE_ADDRESS")
    ram_size = reader.read_int("CONFIG_SRAM_SIZE") * 1024
    dump_file = dut.device_config.build_dir / "ram_dump.hex"

    dump_memory_to_file(
        address=ram_address, size=ram_size, filename=dump_file, serial_number=dut.device_config.id
    )

    intel_hex = IntelHex()
    intel_hex.loadfile(dump_file, "hex")
    data = intel_hex.gets(ram_address, ram_size)

    gap_start, gap_end = get_ramfunc_region(
        dut.device_config.build_dir / "mcuboot/zephyr/zephyr.elf"
    )
    for idx, value in enumerate(data):
        address = ram_address + idx
        if gap_start <= address < gap_end:
            continue
        assert value == 0x00, f"Data 0x{value:02x} not matching 0x00 at address 0x{address:08x}"
