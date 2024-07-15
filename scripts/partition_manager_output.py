#!/usr/bin/env python3
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause


import argparse
import yaml
from os import path
from partition_manager import PartitionError


def get_header_guard_start(filename):
    return '''/* File generated by {0}, do not modify */
#ifndef {1}_H__
#define {1}_H__'''.format(__file__, filename.split('.h')[0].upper())


def get_header_guard_end(filename):
    return '#endif /* {}_H__ */'.format(filename.split('.h')[0].upper())


DEST_HEADER = 1
DEST_KCONFIG = 2


def get_config_lines(gpm_config, greg_config, head, split, dest, current_domain=None):
    config_lines = list()
    affiliations = dict()

    def string_of_strings(mlist):
        return '"{}"'.format(' '.join([str(elem) for elem in mlist]))

    def foreach_fns(mlist):
        ulist = [elem.upper() for elem in mlist]
        return ''.join([f' fn({elem})' for elem in ulist])

    partition_id = 0

    for domain, pm_config in gpm_config.items():
        reg_config = greg_config[domain]
        def partition_has_device(p):
            return 'region' in p and 'device' in reg_config[p['region']] \
                   and reg_config[p['region']]['device']

        def add_line(text_before_split, text_after_split):
            if current_domain is None or domain == current_domain or 'LABEL' in text_before_split:
                # Don't prefix with domain for the current domain
                config_lines.append('{}PM_{}{}{}'.format(head, text_before_split, split, text_after_split))
            else:
                config_lines.append('{}PM{}_{}{}{}'.format(
                    head, '_{}'.format(domain) if domain is not None else '',
                    text_before_split, split, text_after_split))

        for name, partition in sorted(pm_config.items(),
                                      key=lambda key_value_tuple: key_value_tuple[1]['address']):

            name_upper = name.upper()
            name_lower = name.lower()
            region = reg_config[partition['region']]
            offset = partition['address'] - region['base_address']
            add_line(f'{name_upper}_OFFSET', hex(offset))
            add_line(f'{name_upper}_ADDRESS', hex(partition['address']))
            # The end address facilitates using PM values via Cmake generator expressions.
            add_line(f'{name_upper}_END_ADDRESS', hex(partition['end_address']))
            add_line(f'{name_upper}_SIZE', hex(partition['size']))
            add_line(f'{name_upper}_NAME', name)

            try:
                # To support single item string for affiliation we convert the string into a list.
                pafs = partition['affiliation']
                if isinstance(pafs, str):
                    pafs = [pafs]

                for paf in pafs:
                    ptd = affiliations.setdefault(paf, list())
                    if ptd not in affiliations[paf]:
                        affiliations[paf].append(name)

            except KeyError:
                pass

            extra_params = partition.get('extra_params')
            if extra_params:
                for epkey in extra_params.keys():
                    add_line(f'{name_upper}_EXTRA_PARAM_{epkey}', extra_params[epkey])

            if partition_has_device(partition):
                add_line(f'{name_upper}_ID', str(partition_id))
                # Used to support lowercase access. See flash_map.h.
                add_line(f'{name_lower}_ID', f'PM_{name_upper}_ID')
                # Used for IS_ENABLED access, see flash_map_pm.h
                add_line(f'{name_lower}_IS_ENABLED', "1")

                if current_domain is None or domain == current_domain:
                    add_line(f'{partition_id}_LABEL', name_upper)
                else:
                    add_line(f'{partition_id}_LABEL', f'{domain}_{name_upper}')
                partition_id += 1

            if dest is DEST_HEADER:
                if partition_has_device(partition):
                    add_line(f'{name_upper}_DEV', region['device'])
                    if region['default_driver_kconfig']:
                        add_line(f'{name_upper}_DEFAULT_DRIVER_KCONFIG', region['default_driver_kconfig'])

            elif dest is DEST_KCONFIG:
                if 'span' in partition.keys():
                    add_line(f'{name_upper}_SPAN', string_of_strings(partition['span']))

        add_line('NUM', str(partition_id))

        def find_depth(key, depth=0):
            if 'span' in pm_config[key].keys():
                return find_depth(pm_config[key]['span'][0], depth + 1)
            return depth

        flash_partition_pm_config = {k: v for k, v in pm_config.items()}
        all_by_size = list(flash_partition_pm_config.keys())
        all_by_size = sorted(all_by_size, key=find_depth)
        all_by_size = sorted(all_by_size, key=lambda key: flash_partition_pm_config[key]['size'])
        add_line('ALL_BY_SIZE', string_of_strings(all_by_size))
        for saf in affiliations:
            add_line(f'FOREACH_AFFILIATED_TO_{saf}(fn)',
                       foreach_fns(affiliations[saf]))

    return config_lines


def write_config_lines_to_file(pm_config_file_path, config_lines):
    with open(pm_config_file_path, 'w') as out_file:
        out_file.write('\n'.join(config_lines))


def write_gpm_config(gpm_config, regions_config, name, out_path):
    pm_config_file = path.basename(out_path)

    domain, image = name.split(':')
    config_lines = get_config_lines(gpm_config, regions_config, '#define ', ' ', DEST_HEADER, domain)
    image_config_lines = list.copy(config_lines)

    pm_image = image
    if any('span' in x for x in gpm_config[domain][image]):
        # Check for deprecated <name>_image images that may still linger
        # in static configurations
        deprecated = f'{image}_image'
        if gpm_config[domain].get(deprecated):
            pm_image = deprecated
            print(f"""\n
        ----------------------------------------------------------
        --- WARNING: Partition image '{image}' is a container  ---
        --- partition with a span of one or more images, but   ---
        --- has the same name of its child image. Container    ---
        --- partitions are not allowed to share the name of    ---
        --- the child image that defines it.                   ---
        ---                                                    ---
        --- A pm_static.yml file defining appears to be        ---
        --- overriding this. If possible, please rename the    ---
        --- container partition in this file and use the child ---
        --- image name for its actual image partition.         ---
        ----------------------------------------------------------\n""")
        else:
            raise PartitionError(
                f"Partition image '{image}' is a container with a span of one "
                "or more images, but has the same name of its child image. "
                "Container partitions are not allowed to share the name of the "
                " child image that defines it. Please rename the span "
                f"'{image}' in the pm.yml file of child '{image}'.")

    image_config_lines.append('#define PM_ADDRESS {}'.format(hex(gpm_config[domain][pm_image]['address'])))
    image_config_lines.append('#define PM_SIZE {}'.format(hex(gpm_config[domain][pm_image]['size'])))

    image_sram_partition = f'{image}_sram'
    image_has_custom_sram = image_sram_partition in gpm_config[domain]
    if image_has_custom_sram:
        image_config_lines.append('#define PM_SRAM_ADDRESS {}'.format(hex(gpm_config[domain][image_sram_partition]['address'])))
        image_config_lines.append('#define PM_SRAM_SIZE {}'.format(hex(gpm_config[domain][image_sram_partition]['size'])))
    else:
        image_config_lines.append('#define PM_SRAM_ADDRESS {}'.format(hex(gpm_config[domain]['sram_primary']['address'])))
        image_config_lines.append('#define PM_SRAM_SIZE {}'.format(hex(gpm_config[domain]['sram_primary']['size'])))
    image_config_lines.insert(0, get_header_guard_start(pm_config_file))

    image_config_lines.append(get_header_guard_end(pm_config_file))
    write_config_lines_to_file(out_path, image_config_lines)


def write_kconfig_file(gpm_config, regions_config, out_path):
    config_lines = get_config_lines(gpm_config, regions_config, '', '=', DEST_KCONFIG)
    write_config_lines_to_file(out_path, config_lines)


def parse_args():
    parser = argparse.ArgumentParser(
        description='''Creates files based on Partition Manager results.''',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    parser.add_argument('--input-partitions', required=True, type=str, nargs='+',
                        help='Paths to the input .yml files, one per domain.')

    parser.add_argument('--input-regions', required=True, type=str, nargs='+',
                        help='Paths to the input .yml files with region configurations, once per domain.')

    parser.add_argument('--config-file', required=False, type=str,
                        help='Path to the output .config file.')

    parser.add_argument('--images', required=False, type=str, nargs='+',
                        help='List of domain prefixed image partitions.')

    parser.add_argument('--header-files', required=False, type=str, nargs='+',
                        help='Paths to the output header files files.'
                             'These will be matched to the --images.')

    return parser.parse_args()


def main():
    args = parse_args()

    gpm_config = dict()  # GLOBAL pm_config
    greg_config = dict()  # GLOBAL pm_regions

    for partition in args.input_partitions:
        fn = path.basename(partition)
        if 'partitions_' in fn:
            domain_name = fn[fn.index('partitions_') + len('partitions_'):fn.index('.yml')]
        else:
            # Root domain does not have domain suffix in the partitions file name.
            domain_name = ''
        with open(partition, 'r') as f:
            gpm_config[domain_name] = yaml.safe_load(f)

    for region in args.input_regions:
        fn = path.basename(region)
        if 'regions_' in fn:
            domain_name = fn[fn.index('regions_') + len('regions_'):fn.index('.yml')]
        else:
            # Root domain does not have domain suffix in the regions file name.
            domain_name = ''
        with open(region, 'r') as f:
            greg_config[domain_name] = yaml.safe_load(f)

    if args.config_file:
        write_kconfig_file(gpm_config, greg_config, args.config_file)

    if args.header_files:
        for name, header_file in zip(args.images, args.header_files):
            write_gpm_config(gpm_config, greg_config, name, header_file)


if __name__ == '__main__':
    main()
