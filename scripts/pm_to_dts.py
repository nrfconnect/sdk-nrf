#!/usr/bin/env python3
# v0.06

import os
import sys

import yaml

# List of partitions to ignore
IGNORE_LIST = [
    'mcuboot_primary_app',
    'mcuboot_secondary_app',
    'otp',
    'pcd_sram',
    'rpmsg_nrf53_sram',
    'sram_primary',
    'mcuboot_secondary_pad',
    'b0n_container',
    'bootconf',
    'nrf70_wifi_fw_mcuboot_pad',
    'EMPTY_0',
    'EMPTY_1',
    'EMPTY_2',
    'EMPTY_3',
    'EMPTY_4',
    'EMPTY_5',
    'b0_container',
    's0',
    's1',
    'app_image',
    'tfm',
    's1_pad',
]

# Mapping for name exchanges
NAME_EXCHANGE = {
    'mcuboot': 'boot_partition',
    'app': 'slot0_partition',
    'factory_data': 'factory_data_partition',
    'settings_storage': 'storage_partition',
    'mcuboot_secondary': 'slot1_partition',
    'mcuboot_primary_1': 'slot2_partition',
    'mcuboot_secondary_1': 'slot3_partition',
    'mcuboot_primary_2': 'slot4_partition',
    'mcuboot_secondary_2': 'slot5_partition',
    'mcuboot_primary_3': 'slot6_partition',
    'mcuboot_secondary_3': 'slot7_partition',
    'mcuboot_primary_4': 'slot8_partition',
    'mcuboot_secondary_4': 'slot9_partition',
    'mcuboot_primary_5': 'slot10_partition',
    'mcuboot_secondary_5': 'slot11_partition',
    'b0n': 'b0n_partition',
    'b0': 'b0_partition',
    'provision': 'bl_storage: provision_partition',
    's0_image': 's0_partition',
    's1_image': 's1_partition',
    'tfm_secure': 'slot0_s_partition',
    'tfm_nonsecure': 'slot0_ns_partition',
    'tfm_storage': 'tfm_storage_partition',
    'tfm_its': 'tfm_its_partition',
    'tfm_otp_nv_counters': 'tfm_otp_partition',
    'tfm_ps': 'tfm_ps_partition',
}

# Mapping for label exchanges
LABEL_EXCHANGE = {
    'mcuboot': 'mcuboot',
    'app': 'image-0',
    'factory_data': 'factory-data',
    'settings_storage': 'storage',
    'mcuboot_secondary': 'image-1',
    'mcuboot_primary_1': 'image-2',
    'mcuboot_secondary_1': 'image-3',
    'mcuboot_primary_2': 'image-4',
    'mcuboot_secondary_2': 'image-5',
    'mcuboot_primary_3': 'image-6',
    'mcuboot_secondary_3': 'image-7',
    'mcuboot_primary_4': 'image-8',
    'mcuboot_secondary_4': 'image-9',
    'mcuboot_primary_5': 'image-10',
    'mcuboot_secondary_5': 'image-11',
    'b0n': 'b0n',
    'b0': 'b0',
    'provision': 'b0-provision-data',
    's0_image': 'b0-image-0',
    's1_image': 'b0-image-1',
    'tfm_secure': 'image-0-secure',
    'tfm_nonsecure': 'image-0-nonsecure',
    'tfm_storage': 'tfm-storage',
    'tfm_its': 'tfm-its',
    'tfm_otp_nv_counters': 'tfm-otp',
    'tfm_ps': 'tfm-ps',
}


def normalize_inside_parent(area):
    """Return the parent partition name from a PM `inside` field."""
    inside = area.get('inside')

    if isinstance(inside, list) and inside:
        return inside[0]
    if isinstance(inside, str):
        return inside

    return None


def normalize_before_list(area):
    """Return a list of `placement.before` partition names."""
    placement = area.get('placement')
    if not isinstance(placement, dict):
        return []

    before = placement.get('before')
    if isinstance(before, list):
        return before
    if isinstance(before, str):
        return [before]

    return []


def build_inside_relationships(flash_regions):
    """Build parent->children map from partitions using `inside`."""
    by_original_name = {area[6]: area for area in flash_regions}
    parent_children = {}

    for area in flash_regions:
        parent_name = area[7]
        if parent_name and parent_name in by_original_name:
            parent_children.setdefault(parent_name, []).append(area[6])
            # Children are emitted within their parent subpartition node.
            area[4] = True

    return parent_children


def add_topology_edge(graph, indegree, source, target):
    """Add a dependency edge if it does not already exist."""
    if target not in graph[source]:
        graph[source].add(target)
        indegree[target] += 1


def insert_available_by_address(available, candidate, by_original_name):
    """Insert an available node while preserving address order."""
    for i, current in enumerate(available):
        if by_original_name[candidate][1] < by_original_name[current][1]:
            available.insert(i, candidate)
            return

    available.append(candidate)


def release_ready_neighbors(current, graph, indegree, available, by_original_name):
    """Decrement neighbors and enqueue those that become ready."""
    for next_name in sorted(graph[current], key=lambda name: by_original_name[name][1]):
        indegree[next_name] -= 1
        if indegree[next_name] == 0:
            insert_available_by_address(available, next_name, by_original_name)


def order_subpartitions(child_names, by_original_name):
    """Order children by address and placement.before dependencies."""
    ordered_by_address = sorted(child_names, key=lambda name: by_original_name[name][1])
    child_set = set(child_names)

    graph = {name: set() for name in child_names}
    indegree = dict.fromkeys(child_names, 0)

    for child_name in child_names:
        before_list = by_original_name[child_name][8]
        for before_target in before_list:
            if before_target in child_set:
                add_topology_edge(graph, indegree, child_name, before_target)

    available = [name for name in ordered_by_address if indegree[name] == 0]
    result = []

    while available:
        current = available.pop(0)
        result.append(current)
        release_ready_neighbors(current, graph, indegree, available, by_original_name)

    # If dependency data is malformed (cycle), fall back to address order.
    if len(result) != len(child_names):
        return ordered_by_address

    return result


def format_partition_size(size):
    """Format partition size for DTS output."""
    if size % 1024 == 0:
        return f'DT_SIZE_K({size // 1024})'

    return f'0x{format(size, "x")}'


def extend_partition(all_areas):
    """Extend partition to include nrf70 wifi firmware if present"""
    entries_to_skip = 0

    for i, area in enumerate(all_areas):
        if (
            area[0][:4] == 'slot'
            and area[0][-10:] == '_partition'
            and area[0][4] > '1'
            and len(all_areas) > (i + 1)
        ):
            next_area_index = i + 1

            if all_areas[next_area_index][0] == 'nrf70_wifi_fw':
                # nrf70 firmware partition, remove and add additional label
                all_areas[next_area_index][4] = True
                all_areas[i][5] += 'nrf70_wifi_fw_partition: '
                entries_to_skip += 1

    return entries_to_skip


def flash_sort(a, b, odd_area_ref):
    """Sort function for flash regions"""
    if a[1] == b[1]:
        if (a[0] == 'slot0_partition') or (a[0] == 's0_partition'):
            odd_area_ref[0] = b[0]
        else:
            odd_area_ref[0] = a[0]

    return -1 if a[1] < b[1] else 1


def flash_sort_cpunet(a, b, odd_area_ref):
    """Sort function for cpunet flash regions"""
    if a[0] == 'slot0_partition' and b[1] == a[1]:
        odd_area_ref[0] = b[0]
    elif b[0] == 'slot0_partition' and a[1] == b[1]:
        odd_area_ref[0] = a[0]

    return -1 if a[1] < b[1] else 1


def should_keep_odd_area(odd_area, flash_regions):
    """Check whether to keep an area marked as duplicate by flash_sort."""
    if not odd_area:
        return False
    odd_original_name = odd_area[6]
    # Keep if it has a parent (inside relationship) or if it has children.
    return odd_area[7] is not None or any(a[7] == odd_original_name for a in flash_regions)


def write_and_print(f, text):
    """Write to file and print simultaneously."""
    f.write(text)
    print(text, end='')


def write_partition_node(f, indent, area, offset, subpartition_base_offset=0):
    """Write a flat (non-parent) partition node."""
    final_offset = offset - subpartition_base_offset
    node_str = f'{indent}{area[5]}{area[0]}: partition@{format(final_offset, "x")} {{\n'
    write_and_print(f, node_str)
    write_and_print(f, f'{indent}\tlabel = "{area[2]}";\n')
    reg_str = f'{indent}\treg = <0x{format(final_offset, "x")} '
    reg_str += f'0x{format(area[3], "x")}>;\n'
    write_and_print(f, reg_str)
    write_and_print(f, f'{indent}}};\n')


def write_parent_partition_node(
    f, indent, area, offset, size, by_original_name, parent_children, subpartition_base_offset=0
):
    """Write a parent partition node with fixed-subpartitions and child nodes."""
    final_offset = offset - subpartition_base_offset
    node_hdr = f'{indent}{area[5]}{area[0]}: partition@{format(final_offset, "x")} {{\n'
    write_and_print(f, node_hdr)
    write_and_print(f, f'{indent}\tcompatible = "fixed-subpartitions";\n')
    write_and_print(f, f'{indent}\tlabel = "{area[2]}";\n')
    reg_str = f'{indent}\treg = <0x{format(final_offset, "x")} '
    reg_str += f'{format_partition_size(size)}>;\n'
    write_and_print(f, reg_str)
    ranges_str = f'{indent}\tranges = <0x0 0x{format(final_offset, "x")} '
    ranges_str += f'{format_partition_size(size)}>;\n'
    write_and_print(f, ranges_str)
    write_and_print(f, f'{indent}\t#address-cells = <1>;\n')
    write_and_print(f, f'{indent}\t#size-cells = <1>;\n\n')

    child_indent = indent + '\t'
    child_names = order_subpartitions(parent_children, by_original_name)
    for child_name in child_names:
        child_area = by_original_name[child_name]
        child_offset = child_area[1] - area[1]
        child_node = (
            f'{child_indent}{child_area[5]}{child_area[0]}: '
            f'partition@{format(child_offset, "x")} {{\n'
        )
        write_and_print(f, child_node)
        write_and_print(f, f'{child_indent}\tlabel = "{child_area[2]}";\n')
        child_reg = f'{child_indent}\treg = <0x{format(child_offset, "x")} '
        child_reg += f'{format_partition_size(child_area[3])}>;\n'
        write_and_print(f, child_reg)
        write_and_print(f, f'{child_indent}}};\n\n')

    write_and_print(f, f'{indent}}};\n')


def main():
    # Check for input folder
    if len(sys.argv) < 2:
        print('Missing input folder')
        sys.exit(1)

    input_folder = sys.argv[1]

    # Validate input folder
    if not os.path.isdir(input_folder):
        print('Not a folder')
        sys.exit(1)

    partitions_file = os.path.join(input_folder, 'partitions.yml')
    if not os.path.isfile(partitions_file):
        print('Partition manager output not found')
        sys.exit(1)

    config_file = os.path.join(input_folder, 'zephyr/.config')
    if not os.path.isfile(config_file):
        print('Sysbuild build file not found')
        sys.exit(1)

    # Read build configuration to determine SOC series, board, and board family
    with open(config_file) as f:
        build_config = f.read()

    soc_marker = 'SB_CONFIG_SOC_SERIES="'
    soc_line_start = build_config.find(soc_marker)

    if soc_line_start == -1 or soc_line_start < 3:
        print('Invalid sysbuild build configuration')
        sys.exit(1)

    soc_line_start += len(soc_marker)
    soc_line_end = build_config.find('"', soc_line_start)

    if soc_line_end == -1 or soc_line_end < 3:
        print('Invalid sysbuild build configuration')
        sys.exit(1)

    soc_series = build_config[soc_line_start:soc_line_end][:5]

    # Extract board from SB_CONFIG_BOARD
    board_marker = 'SB_CONFIG_BOARD="'
    board_line_start = build_config.find(board_marker)

    if board_line_start == -1:
        print('Invalid sysbuild build configuration - board not found')
        sys.exit(1)

    board_line_start += len(board_marker)
    board_line_end = build_config.find('"', board_line_start)

    if board_line_end == -1:
        print('Invalid sysbuild build configuration - board not found')
        sys.exit(1)

    board = build_config[board_line_start:board_line_end]

    # Extract board_family from SB_CONFIG_BOARD_QUALIFIERS
    board_family_marker = 'SB_CONFIG_BOARD_QUALIFIERS="'
    board_family_line_start = build_config.find(board_family_marker)

    if board_family_line_start == -1:
        print('Invalid sysbuild build configuration - board qualifiers not found')
        sys.exit(1)

    board_family_line_start += len(board_family_marker)
    board_family_line_end = build_config.find('"', board_family_line_start)

    if board_family_line_end == -1:
        print('Invalid sysbuild build configuration - board qualifiers not found')
        sys.exit(1)

    board_family = build_config[board_family_line_start:board_family_line_end]
    board_family = board_family.replace('/', '_')

    # Target name for output files
    board_target = f'{board}_{board_family}'

    # Determine NVM device based on SOC series
    if soc_series == 'nrf54':
        nvm_device = 'cpuapp_rram'
    else:
        # Flash-based device
        nvm_device = 'flash0'

    # Load partition data
    with open(partitions_file) as f:
        data = yaml.safe_load(f)

    # Initialize variables
    mcuboot_enabled = False
    b0_enabled = False
    tfm_enabled = False
    tfm_slot0_combined_size = 0
    mcuboot_pad_size = 0
    s0_pad_size = 0
    internal_flash_regions = []
    internal_flash_regions_cpunet = []
    external_flash_regions = []
    parsed_cpunet = False

    # Process main partition YAML data
    for name, area in data.items():
        # Skip ignored partitions
        if name in IGNORE_LIST:
            continue

        # Check for special configurations
        if name in ('external_flash', 'ram_flash'):
            continue
        elif name == 'mcuboot_primary':
            mcuboot_enabled = True
            tfm_slot0_combined_size = area['size']
            continue
        elif name == 'mcuboot_pad':
            mcuboot_pad_size = area['size']
            continue
        elif name == 's0_pad':
            s0_pad_size = area['size']
            continue
        elif name == 's1_image':
            b0_enabled = True
        elif name == 'tfm_secure':
            tfm_enabled = True

        # Determine flash region type
        region = area.get('region')
        if region == 'external_flash':
            flash_regions = external_flash_regions
        elif not region or region == 'flash_primary':
            flash_regions = internal_flash_regions
        elif region in ('ram_flash', 'sram_primary'):
            continue
        else:
            print(f'Unknown region: {region}')
            continue

        # Get label and name
        label = LABEL_EXCHANGE.get(name, name.replace('_', '-'))
        partition_name = NAME_EXCHANGE.get(name, name)

        # Add region information:
        # [name, address, label, size, skip, extra_labels, original_name, inside_parent, before]
        flash_regions.append(
            [
                partition_name,
                area['address'],
                label,
                area['size'],
                False,
                '',
                name,
                normalize_inside_parent(area),
                normalize_before_list(area),
            ]
        )

    # Process CPUNET partitions if they exist
    partitions_cpunet_file = os.path.join(input_folder, 'partitions_CPUNET.yml')
    if os.path.isfile(partitions_cpunet_file):
        with open(partitions_cpunet_file) as f:
            data_cpunet = yaml.safe_load(f)

        parsed_cpunet = True

        for name, area in data_cpunet.items():
            # Skip ignored partitions
            if name in IGNORE_LIST:
                continue

            # Check for special configurations
            if name == 'mcuboot_primary':
                mcuboot_enabled = True
                continue

            # Determine flash region type
            region = area.get('region')
            if not region or region == 'flash_primary':
                flash_regions = internal_flash_regions_cpunet
            else:
                print(f'Unknown region: {region}')
                continue

            # Get label and name
            label = LABEL_EXCHANGE.get(name, name.replace('_', '-'))
            partition_name = NAME_EXCHANGE.get(name, name)

            if area['address'] >= 0x100000000:
                area['address'] -= 0x100000000

            # Add region information:
            # [name, address, label, size, skip, extra_labels, original_name, inside_parent, before]
            flash_regions.append(
                [
                    partition_name,
                    area['address'],
                    label,
                    area['size'],
                    False,
                    '',
                    name,
                    normalize_inside_parent(area),
                    normalize_before_list(area),
                ]
            )

    # Adjust for MCUboot
    if mcuboot_enabled:
        for i, area in enumerate(internal_flash_regions):
            if area[0] == 'slot0_partition':
                internal_flash_regions[i][1] -= mcuboot_pad_size
                internal_flash_regions[i][3] += mcuboot_pad_size
                break
    elif (not mcuboot_enabled) and b0_enabled:
        for i, area in enumerate(internal_flash_regions):
            if area[0] == 'slot0_partition':
                del internal_flash_regions[i]
                break

    if tfm_enabled:
        for i, area in enumerate(internal_flash_regions):
            if area[0] == 'slot0_partition' or area[0] == 'app':
                del internal_flash_regions[i]
            elif area[0] == 's0_partition' or area[0] == 's1_partition':
                internal_flash_regions[i][1] -= s0_pad_size
                internal_flash_regions[i][3] += s0_pad_size

    # Sort CPUNET regions and handle duplicates
    if parsed_cpunet:
        odd_area_ref = ['']
        from functools import cmp_to_key

        internal_flash_regions_cpunet.sort(
            key=cmp_to_key(lambda a, b: flash_sort_cpunet(a, b, odd_area_ref))
        )

        if odd_area_ref[0]:
            odd_area = next(
                (area for area in internal_flash_regions_cpunet if area[0] == odd_area_ref[0]),
                None,
            )
            if not should_keep_odd_area(odd_area, internal_flash_regions_cpunet):
                internal_flash_regions_cpunet = [
                    area for area in internal_flash_regions_cpunet if area[0] != odd_area_ref[0]
                ]

    # Sort internal and external flash regions
    odd_area_ref = ['']
    from functools import cmp_to_key

    internal_flash_regions.sort(key=cmp_to_key(lambda a, b: flash_sort(a, b, odd_area_ref)))
    external_flash_regions.sort(key=lambda x: x[1])

    # Remove potentially duplicate area
    if odd_area_ref[0]:
        odd_area = next(
            (area for area in internal_flash_regions if area[0] == odd_area_ref[0]),
            None,
        )
        if not should_keep_odd_area(odd_area, internal_flash_regions):
            internal_flash_regions = [
                area for area in internal_flash_regions if area[0] != odd_area_ref[0]
            ]

    # Extend partitions for nrf70 wifi firmware
    extend_partition(internal_flash_regions)
    extend_partition(external_flash_regions)

    if parsed_cpunet:
        extend_partition(internal_flash_regions_cpunet)

    # Build parent-child maps for inside-based subpartitions.
    internal_parent_children = build_inside_relationships(internal_flash_regions)
    external_parent_children = build_inside_relationships(external_flash_regions)

    if parsed_cpunet:
        internal_cpunet_parent_children = build_inside_relationships(internal_flash_regions_cpunet)

    # Calculate actual entries to output
    internal_entries = len([area for area in internal_flash_regions if not area[4]])
    external_entries = len([area for area in external_flash_regions if not area[4]])

    if parsed_cpunet:
        internal_cpunet_entries = len(
            [area for area in internal_flash_regions_cpunet if not area[4]]
        )

    f = open(f'{board_target}.overlay', 'w')  # noqa: SIM115

    # Output internal flash regions
    if internal_entries > 0:
        write_and_print(f, f'&{nvm_device} {{\n')
        write_and_print(f, '\tpartitions {\n')

        output_count = 0
        indent = '\t\t'
        subpartition_offset = 0
        internal_by_original_name = {area[6]: area for area in internal_flash_regions}

        for area in internal_flash_regions:
            if area[4]:
                continue

            if output_count > 0:
                write_and_print(f, '\n')

            # Handle TFM subpartitions
            if tfm_enabled and mcuboot_enabled and area[0] == 'slot0_s_partition':
                subpartition_offset = area[1]
                slot0_hdr = f'{indent}slot0_partition: partition@'
                slot0_node = f'{slot0_hdr}{format(subpartition_offset, "x")} {{\n'
                write_and_print(f, slot0_node)
                write_and_print(f, f'{indent}\tcompatible = "fixed-subpartitions";\n')
                write_and_print(f, f'{indent}\tlabel = "image-0";\n')
                slot0_reg = f'{indent}\treg = <0x{format(subpartition_offset, "x")} '
                slot0_reg += f'0x{format(tfm_slot0_combined_size, "x")}>;\n'
                write_and_print(f, slot0_reg)
                slot0_ranges = f'{indent}\tranges = <0x0 0x{format(subpartition_offset, "x")} '
                slot0_ranges += f'0x{format(tfm_slot0_combined_size, "x")}>;\n'
                write_and_print(f, slot0_ranges)
                write_and_print(f, f'{indent}\t#address-cells = <1>;\n')
                write_and_print(f, f'{indent}\t#size-cells = <1>;\n\n')
                indent = '\t\t\t'

            # Write main partition
            parent_children = internal_parent_children.get(area[6], [])
            if parent_children:
                write_parent_partition_node(
                    f,
                    indent,
                    area,
                    area[1],
                    area[3],
                    internal_by_original_name,
                    parent_children,
                    subpartition_offset,
                )
            else:
                write_partition_node(f, indent, area, area[1], subpartition_offset)

            # Close subpartition container
            if tfm_enabled and mcuboot_enabled and area[0] == 'slot0_ns_partition':
                indent = '\t\t'
                subpartition_offset = 0
                write_and_print(f, f'{indent}}};\n')

            output_count += 1

        write_and_print(f, '\t};\n')
        write_and_print(f, '};\n\n')

    # Generate device tree nodes for external flash regions
    if external_entries > 0:
        write_and_print(f, '&mx25r64 {\n')
        write_and_print(f, '\tpartitions {\n')
        write_and_print(f, '\t\tcompatible = "fixed-partitions";\n')
        write_and_print(f, '\t\tranges;\n')
        write_and_print(f, '\t\t#address-cells = <1>;\n')
        write_and_print(f, '\t\t#size-cells = <1>;\n\n')

        output_count = 0
        indent = '\t\t'
        external_by_original_name = {area[6]: area for area in external_flash_regions}

        for area in external_flash_regions:
            if area[4]:
                continue

            if output_count > 0:
                write_and_print(f, '\n')

            parent_children = external_parent_children.get(area[6], [])
            if parent_children:
                write_parent_partition_node(
                    f, indent, area, area[1], area[3], external_by_original_name, parent_children
                )
            else:
                write_partition_node(f, indent, area, area[1])

            output_count += 1

        write_and_print(f, '\t};\n')
        write_and_print(f, '};\n\n')

    # Output CPUNET partitions
    if parsed_cpunet and internal_cpunet_entries > 0:
        f_net = open(f'{board_target}_cpunet.overlay', 'w')  # noqa: SIM115
        print('**CPUNET**:\n')

        write_and_print(f_net, '&flash1 {\n')
        write_and_print(f_net, '\tpartitions {\n')

        output_count = 0
        indent = '\t\t'
        cpunet_by_original_name = {area[6]: area for area in internal_flash_regions_cpunet}
        for area in internal_flash_regions_cpunet:
            if area[4]:
                continue

            if output_count > 0:
                write_and_print(f_net, '\n')

            parent_children = internal_cpunet_parent_children.get(area[6], [])
            if parent_children:
                # Adjust write function to use f_net for CPUNET output
                final_offset = area[1]
                node_hdr = f'{indent}{area[5]}{area[0]}: partition@{format(final_offset, "x")} {{\n'
                write_and_print(f_net, node_hdr)
                write_and_print(f_net, f'{indent}\tcompatible = "fixed-subpartitions";\n')
                write_and_print(f_net, f'{indent}\tlabel = "{area[2]}";\n')
                net_reg = f'{indent}\treg = <0x{format(final_offset, "x")} '
                net_reg += f'{format_partition_size(area[3])}>;\n'
                write_and_print(f_net, net_reg)
                net_ranges = f'{indent}\tranges = <0x0 0x{format(final_offset, "x")} '
                net_ranges += f'{format_partition_size(area[3])}>;\n'
                write_and_print(f_net, net_ranges)
                write_and_print(f_net, f'{indent}\t#address-cells = <1>;\n')
                write_and_print(f_net, f'{indent}\t#size-cells = <1>;\n\n')

                child_indent = indent + '\t'
                child_names = order_subpartitions(parent_children, cpunet_by_original_name)
                for child_name in child_names:
                    child_area = cpunet_by_original_name[child_name]
                    child_offset = child_area[1] - area[1]
                    child_node = (
                        f'{child_indent}{child_area[5]}{child_area[0]}: '
                        f'partition@{format(child_offset, "x")} {{\n'
                    )
                    write_and_print(f_net, child_node)
                    write_and_print(f_net, f'{child_indent}\tlabel = "{child_area[2]}";\n')
                    net_child_reg = f'{child_indent}\treg = <0x{format(child_offset, "x")} '
                    net_child_reg += f'{format_partition_size(child_area[3])}>;\n'
                    write_and_print(f_net, net_child_reg)
                    write_and_print(f_net, f'{child_indent}}};\n\n')

                write_and_print(f_net, f'{indent}}};\n')
            else:
                write_partition_node(f_net, indent, area, area[1])

            output_count += 1

        write_and_print(f_net, '\t};\n')
        write_and_print(f_net, '};\n\n')


if __name__ == '__main__':
    main()
