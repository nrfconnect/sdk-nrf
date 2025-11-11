"""
Generate register configuration hex file from compiled device tree.

Extracts register configurations from a devicetree node and generates
an Intel HEX file for programming. Works with any node containing child
registers with 'reg' (offset) and 'value' properties.

Copyright (c) 2025 Nordic Semiconductor ASA
SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import argparse
import os
import sys
import traceback
from pathlib import Path

from intelhex import IntelHex

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    sys.exit("$ZEPHYR_BASE environment variable undefined")

sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "dts", "python-devicetree", "src"))
from devicetree import dtlib  # noqa: E402, I001


def find_node(dt, label_or_path):
    """Find a devicetree node by its label or path.

    Args:
        dt: The devicetree
        label_or_path: Node label (e.g., 'ipc0') or path (e.g., '/ipc/ipc0')

    Returns:
        The Node if found, None otherwise
    """

    # Try as label first
    if label_or_path in dt.label2node:
        return dt.label2node[label_or_path]

    # Try as path
    try:
        return dt.get_node(label_or_path)
    except Exception:
        pass

    # Fallback: search by node name (for nodes without labels)
    for node in dt.node_iter():
        if node.name == label_or_path:
            return node

    return None


def extract_registers(dt, node_label):
    """Extract register configuration from devicetree node.

    Args:
        dt: Devicetree instance
        node_label: Label or path of the node to extract from

    Returns:
        tuple: (node, base_addr, list of (offset, value, name) tuples)
                or (None, None, None) on error
    """

    node = find_node(dt, node_label)

    if not node:
        raise ValueError(f"Could not find node '{node_label}' in devicetree.")

    if "reg" not in node.props:
        raise ValueError(f"Node '{node_label}' has no 'reg' property.")

    base_addr = node.props["reg"].to_nums()[0]
    registers = []

    for group in node.nodes.values():
        for register in group.nodes.values():
            if "reg" in register.props and "value" in register.props:
                reg_nums = register.props["reg"].to_nums()
                offset = reg_nums[0]
                value = register.props["value"].to_num()
                name = f"{group.name}/{register.name}"
                registers.append((offset, value, name))

    registers.sort(key=lambda x: x[0])
    return node, base_addr, registers


def generate_hex(node, base_addr, registers, output_path, verbose=False):
    """Generate Intel HEX file from register configuration.

    Args:
        node: Devicetree node being processed
        base_addr: Base address for the register block
        registers: List of (offset, value, name) tuples
        output_path: Path to output hex file
        verbose: Enable verbose output
    """

    if not registers:
        raise ValueError("No registers found to generate")

    ih = IntelHex()

    if verbose:
        print(f"Generating hex from node '{node.name}' at base address 0x{base_addr:08x}")
        print(f"Found {len(registers)} register(s) from {node.path}:\n")

    for offset, value, name in registers:
        addr = base_addr + offset
        ih[addr + 0] = (value >> 0) & 0xFF
        ih[addr + 1] = (value >> 8) & 0xFF
        ih[addr + 2] = (value >> 16) & 0xFF
        ih[addr + 3] = (value >> 24) & 0xFF

        if verbose:
            print(f"  0x{addr:08x} (offset 0x{offset:03x}): 0x{value:08x}  // {name}")

    ih.write_hex_file(output_path)
    print(f"Generated {os.path.basename(output_path)}: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        allow_abbrev=False,
        description="Generate register configuration hex file from device tree",
    )
    parser.add_argument(
        "-d",
        "--dts",
        type=str,
        required=True,
        help="Path to compiled devicetree file (zephyr.dts)",
    )
    parser.add_argument(
        "-n",
        "--node",
        type=str,
        required=True,
        help="Node label or path to extract registers from",
    )
    parser.add_argument("-o", "--output", type=str, required=True, help="Output hex file path")
    parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose output")
    args = parser.parse_args()

    if not Path(args.dts).exists():
        raise FileNotFoundError(f"DTS file not found: {args.dts}")

    dt = dtlib.DT(args.dts, include_path=[])
    node, base_addr, registers = extract_registers(dt, args.node)

    if not registers:
        raise ValueError("No registers found in devicetree")

    try:
        generate_hex(node, base_addr, registers, args.output, args.verbose)
        return 0
    except Exception as e:
        print(f"Error during hex generation: {e}", file=sys.stderr)
        if args.verbose:
            traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
