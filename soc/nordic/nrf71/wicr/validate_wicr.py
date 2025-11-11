"""
Validate WICR and IPC memory consistency using Zephyr's dtlib.

This script reads the compiled device tree and validates that IPC shared memory
regions match the WICR register configuration. This prevents runtime issues where
the Wi-Fi firmware looks for IPC buffers at different addresses than the
application expects.

Copyright (c) 2025 Nordic Semiconductor ASA
SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import argparse
import os
import sys
import traceback
from pathlib import Path

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    sys.exit("$ZEPHYR_BASE environment variable undefined")

sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "dts", "python-devicetree", "src"))
from devicetree import dtlib  # noqa: E402, I001

# Validation mapping: defines which IPC regions should match which WICR registers
# defined in the WICR snippet
VALIDATION_MAPPING = {
    "ipc0/tx-region": {
        "wicr_group": "ipcconfig",
        "addr_register": "commandmboxaddr",
        "size_register": "commandmboxsize",
    },
    "ipc0/rx-region": {
        "wicr_group": "ipcconfig",
        "addr_register": "eventmboxaddr",
        "size_register": "eventmboxsize",
    },
}


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


def get_memory_region_info(node):
    """Get address and size from a memory region node

    Args:
        node: The memory region node to extract information from

    Returns:
        tuple: (absolute address, size) or (None, None) on error
    """

    if not node or "reg" not in node.props:
        return None, None

    reg_nums = node.props["reg"].to_nums()
    parent = node.parent

    # Check if parent has ranges property (needs address translation)
    if parent and "ranges" in parent.props and "reg" in parent.props:
        # Translate child offset to absolute address
        parent_base = parent.props["reg"].to_nums()[0]
        child_offset = reg_nums[0]
        child_size = reg_nums[1]
        absolute_addr = parent_base + child_offset
        return absolute_addr, child_size

    # Direct address (no translation needed)
    return reg_nums[0], reg_nums[1] if len(reg_nums) > 1 else None


def get_wicr_register_value(wicr_node, group_name, register_name):
    """Get the value of a WICR register from a specific group.

    Args:
        wicr_node: The WICR device tree node
        group_name: Name of the WICR child group (e.g., "ipcconfig", "firmware")
        register_name: Name of the register within the group (e.g., "commandmboxaddr")

    Returns:
        tuple: (register path as string, The register value as an integer)
    """

    for child in wicr_node.nodes.values():
        if child.name == group_name:
            for subchild in child.nodes.values():
                if subchild.name.startswith(register_name) and "value" in subchild.props:
                    return (subchild.path, subchild.props["value"].to_num())

    raise ValueError(f"Could not find WICR register '{register_name}' in group '{group_name}'")


def validate_consistency(dts_file, verbose=False):
    """Validate WICR and IPC memory consistency using dtlib.

    Args:
        dts_file: Path to the compiled device tree file
        verbose: Enable verbose output

    Returns:
        bool: True if validation passed, False otherwise
    """

    dt = dtlib.DT(dts_file, include_path=[])

    # Find WICR node
    wicr_node = find_node(dt, "wicr")
    if not wicr_node:
        raise ValueError("Could not find WICR node in device tree")

    if verbose:
        print(f"Found WICR node: {wicr_node.path}")

    # Process each mapping entry
    errors = []
    validations = []

    for mapping_key, mapping_config in VALIDATION_MAPPING.items():
        parts = Path(mapping_key)
        ipc_node_name = str(parts.parent)
        ipc_property = str(parts.name)

        # Find the IPC node
        ipc_node = find_node(dt, ipc_node_name)
        if not ipc_node:
            raise ValueError(f"Could not find IPC node '{ipc_node_name}' in device tree")

        # Get the memory region from the IPC property
        if ipc_property not in ipc_node.props:
            raise ValueError(f"Property '{ipc_property}' not found in {ipc_node_name}")

        region_node = ipc_node.props[ipc_property].to_node()
        region_addr, region_size = get_memory_region_info(region_node)

        # Get WICR register values
        wicr_addr_path, wicr_addr = get_wicr_register_value(
            wicr_node, mapping_config["wicr_group"], mapping_config["addr_register"]
        )
        wicr_size_path, wicr_size = get_wicr_register_value(
            wicr_node, mapping_config["wicr_group"], mapping_config["size_register"]
        )

        # Store validation info
        validation_info = {
            "ipc_path": f"{ipc_node.path}/{ipc_property}",
            "region_node": region_node.path if region_node else None,
            "region_addr": region_addr,
            "region_size": region_size,
            "wicr_addr_path": wicr_addr_path,
            "wicr_addr": wicr_addr,
            "wicr_size_path": wicr_size_path,
            "wicr_size": wicr_size,
        }
        validations.append(validation_info)

        # Validate address match
        if (region_addr is not None and wicr_addr is not None) and (region_addr != wicr_addr):
            errors.append(
                f"IPC region address (0x{region_addr:08X}) != "
                f"WICR {mapping_config['addr_register']} (0x{wicr_addr:08X})"
            )

        # Validate size match
        if (region_size is not None and wicr_size is not None) and (region_size != wicr_size):
            errors.append(
                f"IPC region size (0x{region_size:04X}) != "
                f"WICR {mapping_config['size_register']} (0x{wicr_size:04X})"
            )

    # Report results
    status = False

    if errors:
        print("[ ERROR ] WICR and IPC memory mismatch!")
        print("\nErrors:")
        for error in errors:
            print(f"  - {error}")
    else:
        print("[ OK ] WICR and IPC memory are consistent")
        status = True

    if validations and (errors or verbose):
        print("\nValidation checks:")

        for v in validations:
            addr_region = v.get("region_addr")
            size_region = v.get("region_size")
            addr_wicr = v.get("wicr_addr")
            size_wicr = v.get("wicr_size")

            if addr_region is not None and size_region is not None:
                print(f"  {v['ipc_path']} -> {v['region_node']}")
                print(f"    - addr=0x{addr_region:08X}, size=0x{size_region:04X}")
            if addr_wicr is not None and size_wicr is not None:
                print(f"  {v['wicr_addr_path']}")
                print(f"  {v['wicr_size_path']}")
                print(f"    - addr=0x{addr_wicr:08X}, size=0x{size_wicr:04X}")
            print()

    return status


def main():
    parser = argparse.ArgumentParser(
        allow_abbrev=False, description="Validate WICR and IPC memory consistency"
    )
    parser.add_argument(
        "--dts", required=True, help="Path to compiled device tree file (zephyr.dts)"
    )
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output")
    args = parser.parse_args()

    if not Path(args.dts).exists():
        sys.exit(f"Error: DTS file not found: {args.dts}")

    try:
        success = validate_consistency(args.dts, args.verbose)
        return 0 if success else 1
    except Exception as e:
        print(f"Error during validation: {e}", file=sys.stderr)
        if args.verbose:
            traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
