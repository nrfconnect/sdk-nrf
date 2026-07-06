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


def get_partition_address(edt_data: Path, partition_label: str, absolute: bool = False) -> int:
    """Get partition start address from EDT.

    :param edt_data: Path to the EDT pickle file
    :param partition_label: The label of the partition node
    :param absolute: Whether address should be absolute or relative
    :return: The start address of the partition
    :raises KeyError: If the partition label is not found
    """
    node = get_edt_node(edt_data, partition_label)
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


def get_partition_size(edt_data: Path, partition_label: str) -> int:
    """Get partition size from EDT.

    :param edt_data: Path to the EDT pickle file
    :param partition_label: The label of the partition node
    :return: The size of the partition
    :raises KeyError: If the partition label is not found
    """
    node = get_edt_node(edt_data, partition_label)
    return node.regs[0].size


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
