# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
import logging
import pickle
from pathlib import Path

from devicetree.edtlib import Node

logger = logging.getLogger(__name__)


def get_edt_node(edt_data: Path, node_label: str) -> Node:  # type: ignore
    """Parse the EDT pickle file and return a node by its label.

    :param edt_data: Path to the EDT pickle file
    :param node_label: The label of the node to retrieve
    :return: The node corresponding to the given label
    :raises RuntimeError: If the loaded data does not have the expected structure
    :raises KeyError: If the node label is not found in the EDT data
    """
    with open(edt_data, "rb") as file:
        data = pickle.load(file)
    try:
        return data.label2node[node_label]
    except AttributeError as e:
        raise RuntimeError("Unexpected structure in loaded EDT data") from e
    except KeyError:
        raise KeyError(f"Node label '{node_label}' not found in EDT data") from None


def get_edt_chosen_node(edt_data: Path, chosen_property: str) -> Node:  # type: ignore
    """Parse the EDT pickle file and return a node by its /chosen property name.

    Args:
        edt_data (Path): Path to the EDT pickle file.
        chosen_property (str): The /chosen node property name (e.g. 'zephyr,code-partition').

    Returns:
        devicetree.edtlib.Node: The node pointed to by the given /chosen property.

    Raises:
        RuntimeError: If the loaded data does not have the expected structure.
        KeyError: If the chosen property is not found in the EDT data.

    """
    with open(edt_data, "rb") as file:
        data = pickle.load(file)
    try:
        return data.chosen_nodes[chosen_property]
    except AttributeError as e:
        raise RuntimeError("Unexpected structure in loaded EDT data") from e
    except KeyError:
        raise KeyError(f"Chosen property '{chosen_property}' not found in EDT data") from None


def get_node_address(node: Node, absolute: bool = False) -> int:
    """Get partition start address from EDT.

    :param absolute: Whether address should be absolute or relative
    :return: The start address of the partition
    :raises KeyError: If the partition label is not found
    """
    partition_label = node.label
    address = node.regs[0].addr
    if not absolute:
        current_node = node.parent
        parent_addresses: list[int] = []
        while current_node:
            if len(current_node.regs) > 0:
                parent_addresses.append(current_node.regs[0].addr)
            current_node = current_node.parent
        if parent_addresses:
            candidate_references = [
                parent_address
                for parent_address in parent_addresses
                if 0 < parent_address <= address
            ]
            reference_address = min(candidate_references) if candidate_references else 0
            if reference_address:
                address -= reference_address
            else:
                logger.debug(
                    f"Partition {partition_label} address 0x{address:x} is already relative to "
                    f"parent addresses {[hex(parent_addr) for parent_addr in parent_addresses]}"
                )
        else:
            logger.warning(
                "No parent addresses found for partition %s - probably it is top level partition.",
                partition_label,
            )
    return address


def get_partition_address(edt_data: Path, partition_label: str, absolute: bool = False) -> int:
    """Get partition start address from EDT.

    :param edt_data: Path to the EDT pickle file
    :param partition_label: The label of the partition node
    :param absolute: Whether address should be absolute or relative
    :return: The start address of the partition
    :raises KeyError: If the partition label is not found
    """
    node = get_edt_node(edt_data, partition_label)
    return get_node_address(node, absolute)


def get_partition_size(edt_data: Path, partition_label: str) -> int:
    """Get partition size from EDT.

    :param edt_data: Path to the EDT pickle file
    :param partition_label: The label of the partition node
    :return: The size of the partition
    :raises KeyError: If the partition label is not found
    """
    node = get_edt_node(edt_data, partition_label)
    return node.regs[0].size


def get_code_partition_address(edt_data: Path, absolute: bool = True) -> str:
    """Return the absolute address of a chosen code partition as a hex string.

    :param edt_data: Path to the EDT pickle file
    :param absolute: Whether address should be absolute or relative
    :return: The start address of the partition
    :raises KeyError: If the chosen code partition is not found
    """
    node = get_edt_chosen_node(edt_data, "zephyr,code-partition")
    return str(get_node_address(node, absolute))


def get_code_slot(edt_data: Path) -> Node:
    """Return the EDT node, that contains the code partition."""
    code_partition = get_edt_chosen_node(edt_data, "zephyr,code-partition")
    code_addr = get_node_address(code_partition, True)
    code_size = code_partition.regs[0].size
    for slot in range(16):
        try:
            test_partition = get_edt_node(edt_data, f"slot{slot}_partition")
            test_partition_addr = get_node_address(test_partition, True)
            test_partition_size = test_partition.regs[0].size
            if (
                test_partition_addr <= code_addr
                and test_partition_addr + test_partition_size >= code_addr + code_size
            ):
                return test_partition
        except KeyError:
            continue
    raise KeyError("Code partition not found in EDT data")


def has_partition(edt_data: Path, partition_label: str) -> bool:
    """Check if a partition exists in EDT.

    :param edt_data: Path to the EDT pickle file
    :param partition_label: The label of the partition node
    :return: True if the partition exists, False otherwise
    """
    try:
        get_edt_node(edt_data, partition_label)
        return True
    except KeyError:
        return False
