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
}


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


def find_duplicate_address(regions):
    # Find duplicate addresses and return the odd area name
    address_map = {}

    for region in regions:
        address = region[1]

        if address in address_map:
            existing = address_map[address]

            if existing[0] == 'slot0_partition':
                return region[0]
            elif region[0] == 'slot0_partition':
                return existing[0]
            else:
                continue

        address_map[address] = region

    return None


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
        label = LABEL_EXCHANGE.get(name, '')
        partition_name = NAME_EXCHANGE.get(name, name)

        # Add region information: [name, address, label, size, skip, extra_labels]
        flash_regions.append([partition_name, area['address'], label, area['size'], False, ''])

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
            label = LABEL_EXCHANGE.get(name, '')
            partition_name = NAME_EXCHANGE.get(name, name)

            if area['address'] >= 0x100000000:
                area['address'] -= 0x100000000

            # Add region information: [name, address, label, size, skip, extra_labels]
            flash_regions.append([partition_name, area['address'], label, area['size'], False, ''])

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
        internal_flash_regions = [
            area for area in internal_flash_regions if area[0] != odd_area_ref[0]
        ]

    # Extend partitions for nrf70 wifi firmware
    extend_partition(internal_flash_regions)
    extend_partition(external_flash_regions)

    if parsed_cpunet:
        extend_partition(internal_flash_regions_cpunet)

    # Calculate actual entries to output
    internal_entries = len([area for area in internal_flash_regions if not area[4]])
    external_entries = len([area for area in external_flash_regions if not area[4]])

    if parsed_cpunet:
        internal_cpunet_entries = len(
            [area for area in internal_flash_regions_cpunet if not area[4]]
        )

    f = open(f'{board_target}.overlay', 'w')  # noqa: SIM115
    # Nordic boards for overlays only
    f.write('/delete-node/ &boot_partition;\n')
    f.write('/delete-node/ &slot0_partition;\n')
    f.write('/delete-node/ &slot1_partition;\n')
    f.write('/delete-node/ &storage_partition;\n\n')
    print('/delete-node/ &boot_partition;')
    print('/delete-node/ &slot0_partition;')
    print('/delete-node/ &slot1_partition;')
    print('/delete-node/ &storage_partition;\n')

    # Output internal flash regions
    if internal_entries > 0:
        f.write(f'&{nvm_device} {{\n')
        f.write('\tpartitions {\n')
        print(f'&{nvm_device} {{')
        print('\tpartitions {')

        output_count = 0
        indent = '\t\t'
        subpartition_offset = 0

        for area in internal_flash_regions:
            if area[4]:
                continue

            if output_count > 0:
                f.write('\n')
                print()

            # Handle TFM subpartitions
            if tfm_enabled and mcuboot_enabled and area[0] == 'slot0_s_partition':
                subpartition_offset = area[1]

                f.write(
                    f'{indent}slot0_partition: partition@{format(subpartition_offset, "x")} {{\n'
                )
                f.write(f'{indent}\tcompatible = "fixed-subpartitions";\n')
                f.write(f'{indent}\tlabel = "image-0";\n')
                f.write(
                    f'{indent}\treg = <0x{format(subpartition_offset, "x")} '
                    f'0x{format(tfm_slot0_combined_size, "x")}>;\n'
                )
                f.write(
                    f'{indent}\tranges = <0x0 0x{format(subpartition_offset, "x")} '
                    f'0x{format(tfm_slot0_combined_size, "x")}>;\n'
                )
                f.write(f'{indent}\t#address-cells = <1>;\n')
                f.write(f'{indent}\t#size-cells = <1>;\n\n')

                print(f'{indent}slot0_partition: partition@{format(subpartition_offset, "x")} {{')
                print(f'{indent}\tcompatible = "fixed-subpartitions";')
                print(f'{indent}\tlabel = "image-0";')
                print(
                    f'{indent}\treg = <0x{format(subpartition_offset, "x")} '
                    f'0x{format(tfm_slot0_combined_size, "x")}>;'
                )
                print(
                    f'{indent}\tranges = <0x0 0x{format(subpartition_offset, "x")} '
                    f'0x{format(tfm_slot0_combined_size, "x")}>;'
                )
                print(f'{indent}\t#address-cells = <1>;')
                print(f'{indent}\t#size-cells = <1>;\n')

                indent = '\t\t\t'

            # Write main partition
            f.write(
                f'{indent}{area[5]}{area[0]}: '
                f'partition@{format(area[1] - subpartition_offset, "x")} {{\n'
            )
            f.write(f'{indent}\tlabel = "{area[2]}";\n')
            f.write(
                f'{indent}\treg = '
                f'<0x{format(area[1] - subpartition_offset, "x")} 0x{format(area[3], "x")}>;\n'
            )
            f.write(f'{indent}}};\n')
            print(
                f'{indent}{area[5]}{area[0]}: '
                f'partition@{format(area[1] - subpartition_offset, "x")} {{'
            )
            print(f'{indent}\tlabel = "{area[2]}";')
            print(
                f'{indent}\treg = '
                f'<0x{format(area[1] - subpartition_offset, "x")} 0x{format(area[3], "x")}>;'
            )
            print(f'{indent}}};')

            # Close subpartition container
            if tfm_enabled and mcuboot_enabled and area[0] == 'slot0_ns_partition':
                indent = '\t\t'
                subpartition_offset = 0
                f.write('{indent}};\n')
                print('{indent}};')

            output_count += 1

        f.write('\t};\n')
        f.write('};\n\n')
        print('\t};')
        print('};\n')

    # Generate device tree nodes for external flash regions
    if external_entries > 0:
        f.write('&mx25r64 {\n')
        f.write('\tpartitions {\n')
        f.write('\t\tcompatible = "fixed-partitions";\n')
        f.write('\t\tranges;\n')
        f.write('\t\t#address-cells = <1>;\n')
        f.write('\t\t#size-cells = <1>;\n\n')
        print('&mx25r64 {')
        print('\tpartitions {')
        print('\t\tcompatible = "fixed-partitions";')
        print('\t\tranges;')
        print('\t\t#address-cells = <1>;')
        print('\t\t#size-cells = <1>;\n')

        output_count = 0
        indent = '\t\t'

        for area in external_flash_regions:
            if area[4]:
                continue

            if output_count > 0:
                f.write('\n')
                print()

            f.write(f'{indent}{area[5]}{area[0]}: partition@{format(area[1], "x")} {{\n')
            f.write(f'{indent}\tlabel = "{area[2]}";\n')
            f.write(f'{indent}\treg = <0x{format(area[1], "x")} 0x{format(area[3], "x")}>;\n')
            f.write(f'{indent}}};\n')
            print(f'{indent}{area[5]}{area[0]}: partition@{format(area[1], "x")} {{')
            print(f'{indent}\tlabel = "{area[2]}";')
            print(f'{indent}\treg = <0x{format(area[1], "x")} 0x{format(area[3], "x")}>;')
            print(f'{indent}}};')

            output_count += 1

        f.write('\t};\n')
        f.write('};\n\n')
        print('\t};')
        print('};\n')

    # Output CPUNET partitions
    if parsed_cpunet and internal_cpunet_entries > 0:
        f_net = open(f'{board_target}_cpunet.overlay', 'w')  # noqa: SIM115
        print('**CPUNET**:\n')

        # Nordic boards for overlays only
        f_net.write('/delete-node/ &boot_partition;\n')
        f_net.write('/delete-node/ &slot0_partition;\n')
        f_net.write('/delete-node/ &slot1_partition;\n')
        f_net.write('/delete-node/ &storage_partition;\n\n')
        print('/delete-node/ &boot_partition;')
        print('/delete-node/ &slot0_partition;')
        print('/delete-node/ &slot1_partition;')
        print('/delete-node/ &storage_partition;\n')

        f_net.write('&flash1 {\n')
        f_net.write('\tpartitions {\n')
        print('&flash1 {')
        print('\tpartitions {')

        output_count = 0
        for area in internal_flash_regions_cpunet:
            if area[4]:
                continue

            if output_count > 0:
                f_net.write('\n')
                print()

            f_net.write(f'\t\t{area[5]}{area[0]}: partition@{format(area[1], "x")} {{\n')
            f_net.write(f'\t\t\tlabel = "{area[2]}";\n')
            f_net.write(f'\t\t\treg = <0x{format(area[1], "x")} 0x{format(area[3], "x")}>;\n')
            f_net.write('\t\t};\n')
            print(f'\t\t{area[5]}{area[0]}: partition@{format(area[1], "x")} {{')
            print(f'\t\t\tlabel = "{area[2]}";')
            print(f'\t\t\treg = <0x{format(area[1], "x")} 0x{format(area[3], "x")}>;')
            print('\t\t};')

            output_count += 1

        f_net.write('\t};\n')
        f_net.write('};\n\n')
        print('\t};')
        print('};\n')


if __name__ == '__main__':
    main()
