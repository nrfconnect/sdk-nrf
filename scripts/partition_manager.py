#!/usr/bin/env python3
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import argparse
import yaml
import re
from os import path
import sys
from pprint import pformat


PERMITTED_STR_KEYS = ['size']


def remove_item_not_in_list(list_to_remove_from, list_to_check):
    [list_to_remove_from.remove(x) for x in list_to_remove_from.copy() if x not in list_to_check and x is not 'app']


def item_is_placed(d, item, after_or_before):
    assert(after_or_before in ['after', 'before'])
    return after_or_before in d['placement'].keys() and \
           d['placement'][after_or_before][0] == item


def remove_irrelevant_requirements(reqs):
    # Remove dependencies to partitions which are not present
    to_delete = list()
    for k, v in reqs.items():
        for before_after in ['before', 'after']:
            if 'placement' in v.keys() and before_after in v['placement'].keys():
                remove_item_not_in_list(v['placement'][before_after], [*reqs.keys(), 'start', 'end'])
                if not v['placement'][before_after]:
                    del v['placement'][before_after]
        if 'span' in v.keys():
            if type(v['span']) == dict and 'one_of' in v['span'].keys():
                remove_item_not_in_list(v['span']['one_of'], reqs.keys())
                tmp = v['span']['one_of'].copy()
                del v['span']['one_of']
                v['span'] = list()
                v['span'].append(tmp[0])
            else:
                remove_item_not_in_list(v['span'], reqs.keys())
        if 'inside' in v.keys():
            remove_item_not_in_list(v['inside'], reqs.keys())
            if not v['inside']:
                del v['inside']
        if 'share_size' in v.keys():
            remove_item_not_in_list(v['share_size'], reqs.keys())
            if not v['share_size']:
                del v['share_size']
                if 'size' not in v.keys():
                    # The partition has no size, delete it
                    to_delete.append(k)

    for x in to_delete:
        del reqs[x]


def get_images_which_need_resolving(reqs, sub_partitions):
    # Get candidates which have placement specs.
    unsorted = {x for x in reqs.keys() if 'placement' in reqs[x].keys()
                 and ('before' in reqs[x]['placement'].keys() or 'after' in reqs[x]['placement'].keys())}

    # Sort sub_partitions by whether they are inside other sub_partitions. Innermost first.
    sorted_subs = sorted(sub_partitions.values(), key=lambda x: len(x['span']))

    # Sort candidates by whether they are part of a sub_partitions.
    # sub_partition parts come last in the result list so they are more likely
    # to end up being placed next to each other, since they are inserted last.
    result = []
    for sub in sorted_subs:
        result = [part for part in sub['span'] if part in unsorted and part not in result] + result

    # Lastly, place non-partitioned parts at the front.
    result = [part for part in unsorted if part not in result] + result

    return result


def solve_direction(reqs, sub_partitions, unsolved, solution, ab):
    assert(ab in ['after', 'before'])
    current_index = 0
    pool = solution + list(sub_partitions.keys())
    current = pool[current_index]
    while current:
        depends = [x for x in unsolved if item_is_placed(reqs[x], current, ab)]
        if depends:
            # Place based on current, or based on the first/last element in the span of current.
            if ab == 'before':
                anchor = current if current in solution else next(solved for solved in solution
                                                                  if solved in sub_partitions[current]['span'])
                solution.insert(solution.index(anchor), depends[0])
            else:
                anchor = current if current in solution else next(solved for solved in reversed(solution)
                                                                  if solved in sub_partitions[current]['span'])
                solution.insert(solution.index(anchor) + 1, depends[0])
            unsolved.remove(depends[0])
            current = depends[0]
        else:
            current_index += 1
            if current_index >= len(pool):
                break
            current = pool[current_index]


def solve_first_last(reqs, unsolved, solution):
    for fl in [('after', 'start', lambda x: solution.insert(0, x)), ('before', 'end', solution.append)]:
        first_or_last = [x for x in reqs.keys() if 'placement' in reqs[x]
                         and fl[0] in reqs[x]['placement'].keys()
                         and fl[1] in reqs[x]['placement'][fl[0]]]
        if first_or_last:
            fl[2](first_or_last[0])
            if first_or_last[0] in unsolved:
                unsolved.remove(first_or_last[0])


def extract_sub_partitions(reqs):
    sub_partitions = dict()
    keys_to_delete = list()

    for key, value in reqs.items():
        if 'inside' in value.keys():
            reqs[value['inside'][0]]['span'].append(key)
        if 'span' in value.keys():
            sub_partitions[key] = value
            sub_partitions[key]['orig_span'] = sub_partitions[key]['span'].copy() # Take a "backup" of the span.
            keys_to_delete.append(key)

    # "Flatten" by changing all span lists to contain the innermost partitions.
    done = False
    while not done:
        done = True
        for name, req in reqs.items():
            if 'span' in req.keys():
                assert len(req['span']) > 0, "partition {} is empty".format(name)
                for part in req['span']:
                    if 'span' in reqs[part].keys():
                        req['span'].extend(reqs[part]['span'])
                        req['span'].remove(part)
                        req['span'] = list(set(req['span']))  # remove duplicates
                        done = False

    for key in keys_to_delete:
        del reqs[key]

    return sub_partitions


def convert_str_to_list(with_str):
    for k, v in with_str.items():
        if type(v) is dict:
            convert_str_to_list(v)
        elif type(v) is str and k not in PERMITTED_STR_KEYS:
            with_str[k] = list()
            with_str[k].append(v)


def resolve(reqs):
    convert_str_to_list(reqs)
    solution = list(['app'])
    remove_irrelevant_requirements(reqs)
    sub_partitions = extract_sub_partitions(reqs)
    unsolved = get_images_which_need_resolving(reqs, sub_partitions)

    solve_first_last(reqs, unsolved, solution)
    while unsolved:
        solve_direction(reqs, sub_partitions, unsolved, solution, 'before')
        solve_direction(reqs, sub_partitions, unsolved, solution, 'after')

    # Validate partition spanning.
    for sub in sub_partitions.keys():
        indices = [solution.index(part) for part in sub_partitions[sub]['span']]
        assert ((not indices) or (max(indices) - min(indices) + 1 == len(indices))), \
            "partition {} ({}) does not span over consecutive parts." \
            " Solution: {}".format(sub, str(sub_partitions[sub]['span']), str(solution))
        for part in sub_partitions[sub]['span']:
            assert (part in solution), "Some or all parts of partition {} have not been placed.".format(part)

    return solution, sub_partitions


def shared_size(reqs, share_with, total_size):
    sharer_count = reqs[share_with]['sharers']
    size = sizeof(reqs, share_with, total_size)
    if share_with == 'app' or ('span' in reqs[share_with].keys() and 'app' in reqs[share_with]['span']):
        size /= (sharer_count + 1)
    return int(size)


def get_size_source(reqs, sharer):
    size_source = sharer
    while 'share_size' in reqs[size_source].keys():
        # Find "original" source.
        size_source = reqs[size_source]['share_size'][0]
    return size_source


def set_shared_size(reqs, sub_partitions, total_size):
    all_reqs = dict(reqs, **sub_partitions)
    for req in all_reqs.keys():
        if 'share_size' in all_reqs[req].keys():
            size_source = get_size_source(all_reqs, req)
            if 'sharers' not in all_reqs[size_source].keys():
                all_reqs[size_source]['sharers'] = 0
            all_reqs[size_source]['sharers'] += 1
            all_reqs[req]['share_size'] = [size_source]

    new_sizes = dict()
    dynamic_size_sharers = [k for k,v in all_reqs.items() if 'share_size' in v.keys()
                                                              and (v['share_size'][0] == 'app'
                                                                  or ('span' in all_reqs[v['share_size'][0]].keys()
                                                                      and 'app' in all_reqs[v['share_size'][0]]['span']))]
    static_size_sharers  = [k for k,v in all_reqs.items() if 'share_size' in v.keys() and k not in dynamic_size_sharers]
    for req in static_size_sharers:
        all_reqs[req]['size'] = shared_size(all_reqs, all_reqs[req]['share_size'][0], total_size)
    for req in dynamic_size_sharers:
        new_sizes[req] = shared_size(all_reqs, all_reqs[req]['share_size'][0], total_size)
    # Update all sizes after-the-fact or else the calculation will be messed up.
    for key, value in new_sizes.items():
        all_reqs[key]['size'] = value


def app_size(reqs, total_size):
    size = total_size - sum([req['size'] for name, req in reqs.items() if 'size' in req.keys() and name is not 'app'])
    return size


def set_addresses(reqs, sub_partitions, solution, size, start=0):
    set_shared_size(reqs, sub_partitions, size)

    reqs['app']['size'] = app_size(reqs, size)

    # First image starts at 0, or at start of dynamic area if static configuration is given.
    reqs[solution[0]]['address'] = start
    for i in range(1, len(solution)):
        current = solution[i]
        previous = solution[i - 1]
        reqs[current]['address'] = reqs[previous]['address'] + reqs[previous]['size']


def set_size_addr(entry, size, address):
    entry['size'] = size
    entry['address'] = address


def set_sub_partition_address_and_size(reqs, sub_partitions):
    for sp_name, sp_value in sub_partitions.items():
        size = sum([reqs[part]['size'] for part in sp_value['span']])
        if size == 0:
            raise RuntimeError("No compatible parent partition found for {}".format(sp_name))
        address = min([reqs[part]['address'] for part in sp_value['span']])

        reqs[sp_name] = sp_value
        reqs[sp_name]['span'] = reqs[sp_name]['orig_span'] # Restore "backup".
        set_size_addr(reqs[sp_name], size, address)


def sizeof(reqs, req, total_size):
    if req == 'app':
        size = app_size(reqs, total_size)
    elif 'span' not in reqs[req].keys():
        size = reqs[req]['size'] if 'size' in reqs[req].keys() else 0
    else:
        size = sum([sizeof(reqs, part, total_size) for part in reqs[req]['span']])

    return size


def load_reqs(reqs, input_config):
    for name, ymlpath in input_config.items():
        if path.exists(ymlpath):
            with open(ymlpath, 'r') as f:
                reqs.update(yaml.safe_load(f))

    reqs['app'] = dict()


def get_dynamic_area_start_and_size(static_config, flash_size):
    # Remove app from this dict to simplify the case where partitions before and after are removed.
    proper_partitions = [config for name, config in static_config.items()
                         if 'span' not in config.keys() and name != 'app']

    starts = {flash_size} | {config['address'] for config in proper_partitions}
    ends = {0} | {config['address'] + config['size'] for config in proper_partitions}
    gaps = list(zip(sorted(ends - starts), sorted(starts - ends)))

    assert len(gaps) == 1, "Incorrect amount of gaps found"

    start, end = gaps[0]
    return start, end - start


def get_pm_config(input_config, flash_size, static_config):
    to_resolve = dict()
    start = 0

    load_reqs(to_resolve, input_config)
    free_size = flash_size

    if static_config:
        start, free_size = get_dynamic_area_start_and_size(static_config, free_size)
        to_resolve = {name: config for name, config in to_resolve.items()
                      if name not in static_config.keys() or name == 'app'}
        # If nothing is unresolved (only app remaining), simply return the pre defined config
        if len(to_resolve) == 1:
            return static_config

    solution, sub_partitions = resolve(to_resolve)
    set_addresses(to_resolve, sub_partitions, solution, free_size, start)
    set_sub_partition_address_and_size(to_resolve, sub_partitions)

    if static_config:
        # Merge the results, take the new 'app' as that has the correct size.
        to_resolve.update({name: config for name, config in static_config.items() if name != 'app'})
    return to_resolve


def write_yaml_out_file(pm_config, out_path):
    def hexint_presenter(dumper, data):
        return dumper.represent_int(hex(data))
    yaml.add_representer(int, hexint_presenter)
    yamldump = yaml.dump(pm_config)
    with open(out_path, 'w') as out_file:
        out_file.write(yamldump)


def parse_args():
    parser = argparse.ArgumentParser(
        description='''Parse given 'pm.yml' partition manager configuration files to deduce the placement of partitions.

The partitions and their relative placement is defined in the 'pm.yml' files. The path to the 'pm.yml' files are used
to locate 'autoconf.h' files. These are used to find the partition sizes, as well as the total flash size.

This script generates a file for each partition - "pm_config.h".
This file contains all addresses and sizes of all partitions.

"pm_config.h" is in the same folder as the given 'pm.yml' file.''',
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("--input-names", required=True, type=str, nargs="+",
                        help="List of names of image partitions.")

    parser.add_argument("--input-files", required=True, type=str, nargs="+",
                        help="List of paths to input yaml files. "
                             "These will be matched to the --input-names.")

    parser.add_argument("--flash-size", required=True, type=int,
                        help="Flash size of chip.")

    parser.add_argument("--output", required=True, type=str,
                        help="Path to output file.")

    parser.add_argument("-s", "--static-config", required=False, type=argparse.FileType(mode='r'),
                        help="Path static configuration.")

    return parser.parse_args()


def main():
    print("Running Partition Manager...")
    if len(sys.argv) > 1:
        static_config = None
        args = parse_args()
        if args.static_config:
            print("Partition Manager using static configuration at " + args.static_config.name)
            static_config = yaml.safe_load(args.static_config)
        pm_config = get_pm_config(dict(zip(args.input_names, args.input_files)), args.flash_size, static_config)
        write_yaml_out_file(pm_config, args.output)
    else:
        print("No input, running tests.")
        test()


def expect_addr_size(td, name, expected_address, expected_size):
    if expected_size:
        assert td[name]['size'] == expected_size, \
            "Size of {} was {}, expected {}.\ntd:{}".format(name, td[name]['size'], expected_size, pformat(td))
    if expected_address:
        assert td[name]['address'] == expected_address, \
            "Address of {} was {}, expected {}.\ntd:{}".format(name, td[name]['address'], expected_address, pformat(td))


def test():
    list_one = [1, 2, 3, 4]
    items_to_check = [4]
    remove_item_not_in_list(list_one, items_to_check)
    assert list_one[0] == 4
    assert len(list_one) == 1

    test_config = {
        'first': {'address': 0,    'size': 10},
        # Gap from deleted partition.
        'app':   {'address': 20, 'size': 10},
        # Gap from deleted partition.
        'fourth': {'address': 40, 'size': 60}}
    start, size = get_dynamic_area_start_and_size(test_config, 100)
    assert(start == 10)
    assert(size == 40-10)

    test_config = {
        'first':  {'address': 0,    'size': 10},
        'second': {'address': 10, 'size': 10},
        'app':    {'address': 20, 'size': 80}
        # Gap from deleted partition.
    }

    start, size = get_dynamic_area_start_and_size(test_config, 100)
    assert(start == 20)
    assert(size == 80)

    test_config = {
        'app':    {'address': 0,    'size': 10},
        # Gap from deleted partition.
        'second': {'address': 40, 'size': 60}}
    start, size = get_dynamic_area_start_and_size(test_config, 100)
    assert(start == 0)
    assert(size == 40)

    test_config = {
        'first': {'address': 0,    'size': 10},
        # Gap from deleted partition.
        'app':   {'address': 20, 'size': 10}}
    start, size = get_dynamic_area_start_and_size(test_config, 100)
    assert (start == 10)
    assert (size == 100 - 10)

    # Verify that if a partition X uses 'share_size' with a non-existing partition, then partition X is given size 0,
    # and is hence not created.
    td = {'should_not_exist': {'placement': {'before': 'exists'}, 'share_size': 'does_not_exist'},
          'exists': {'placement': {'before': 'app'}, 'size': 100},
          'app': {}}
    s, sub_partitions = resolve(td)
    set_addresses(td, sub_partitions, s, 1000)
    set_sub_partition_address_and_size(td, sub_partitions)
    assert 'should_not_exist' not in td.keys()

    # Verify that if a partition X uses 'share_size' with a non-existing partition, but has set a default size,
    # then partition X is created with the default size.
    td = {'should_exist': {'placement': {'before': 'exists'}, 'share_size': 'does_not_exist', 'size': 200},
          'exists': {'placement': {'before': 'app'}, 'size': 100},
          'app': {}}
    s, sub_partitions = resolve(td)
    set_addresses(td, sub_partitions, s, 1000)
    set_sub_partition_address_and_size(td, sub_partitions)
    expect_addr_size(td, 'should_exist', 0, 200)

    td = {'spm': {'placement': {'before': ['app']}, 'size': 100},
          'mcuboot': {'placement': {'before': ['spm', 'app']}, 'size': 200},
          'app': {}}
    s, sub_partitions = resolve(td)
    set_addresses(td, sub_partitions, s, 1000)
    set_sub_partition_address_and_size(td, sub_partitions)
    expect_addr_size(td, 'mcuboot', 0, None)
    expect_addr_size(td, 'spm', 200, None)
    expect_addr_size(td, 'app', 300, 700)

    td = {'spm': {'placement': {'before': ['app']}, 'size': 100, 'inside': ['mcuboot_slot0']},
          'mcuboot': {'placement': {'before': ['spm', 'app']}, 'size': 200},
          'mcuboot_slot0': {'span': ['app']},
          'app': {}}
    s, sub_partitions = resolve(td)
    set_addresses(td, sub_partitions, s, 1000)
    set_sub_partition_address_and_size(td, sub_partitions)
    expect_addr_size(td, 'mcuboot', 0, None)
    expect_addr_size(td, 'spm', 200, 100)
    expect_addr_size(td, 'app', 300, 700)
    expect_addr_size(td, 'mcuboot_slot0', 200, 800)

    td = {'spm': {'placement': {'before': 'app'}, 'size': 100, 'inside': 'mcuboot_slot0'},
          'mcuboot': {'placement': {'before': 'app'}, 'size': 200},
          'mcuboot_pad': {'placement': {'after': 'mcuboot'}, 'inside': 'mcuboot_slot0', 'size': 10},
          'app_partition': {'span': ['spm', 'app'], 'inside': 'mcuboot_slot0'},
          'mcuboot_slot0': {'span': ['app', 'foo']},
          'mcuboot_data': {'placement': {'after': ['mcuboot_slot0']}, 'size': 200},
          'mcuboot_slot1': {'share_size': 'mcuboot_slot0', 'placement': {'after': 'mcuboot_data'}},
          'mcuboot_slot2': {'share_size': 'mcuboot_slot1', 'placement': {'after': 'mcuboot_slot1'}},
          'app': {}}
    s, sub_partitions = resolve(td)
    set_addresses(td, sub_partitions, s, 1000)
    set_sub_partition_address_and_size(td, sub_partitions)
    expect_addr_size(td, 'mcuboot', 0, None)
    expect_addr_size(td, 'spm', 210, None)
    expect_addr_size(td, 'mcuboot_slot0', 200, 200)
    expect_addr_size(td, 'mcuboot_slot1', 600, 200)
    expect_addr_size(td, 'mcuboot_slot2', 800, 200)
    expect_addr_size(td, 'app', 310, 90)
    expect_addr_size(td, 'mcuboot_pad', 200, 10)
    expect_addr_size(td, 'mcuboot_data', 400, 200)

    td = {'spm': {'placement': {'before': ['app']}, 'size': 100, 'inside': ['mcuboot_slot0']},
          'mcuboot': {'placement': {'before': ['app']}, 'size': 200},
          'mcuboot_pad': {'placement': {'after': ['mcuboot']}, 'inside': ['mcuboot_slot0'], 'size': 10},
          'app_partition': {'span': ['spm', 'app'], 'inside': ['mcuboot_slot0']},
          'mcuboot_slot0': {'span': {'one_of': ['app', 'mcuboot', 'foo', 'mcuboot_pad']}},
          'mcuboot_data': {'placement': {'after': ['mcuboot_slot0']}, 'size': 200},
          'mcuboot_slot1': {'share_size': ['mcuboot_slot0'], 'placement': {'after': ['mcuboot_data']}},
          'mcuboot_slot2': {'share_size': ['mcuboot_slot1'], 'placement': {'after': ['mcuboot_slot1']}},
          'app': {}}
    s, sub_partitions = resolve(td)
    set_addresses(td, sub_partitions, s, 1000)
    set_sub_partition_address_and_size(td, sub_partitions)
    expect_addr_size(td, 'mcuboot', 0, None)
    expect_addr_size(td, 'spm', 210, None)
    expect_addr_size(td, 'mcuboot_slot0', 200, 200)
    expect_addr_size(td, 'mcuboot_slot1', 600, 200)
    expect_addr_size(td, 'mcuboot_slot2', 800, 200)
    expect_addr_size(td, 'app', 310, 90)
    expect_addr_size(td, 'mcuboot_pad', 200, 10)
    expect_addr_size(td, 'mcuboot_data', 400, 200)

    td = {
        'e': {'placement': {'before': ['app']}, 'size': 100},
        'a': {'placement': {'before': ['b']}, 'size': 100},
        'd': {'placement': {'before': ['e']}, 'size': 100},
        'c': {'placement': {'before': ['d']}, 'share_size': ['z', 'a', 'g']},
        'j': {'placement': {'before': ['end']}, 'inside': ['k'], 'size': 20},
        'i': {'placement': {'before': ['j']}, 'inside': ['k'], 'size': 20},
        'h': {'placement': {'before': ['i']}, 'size': 20},
        'f': {'placement': {'after': ['app']}, 'size': 20},
        'g': {'placement': {'after': ['f']}, 'size': 20},
        'b': {'placement': {'before': ['c']}, 'size': 20},
        'k': {'span': []},
        'app': {}}
    s, sub_partitions = resolve(td)
    set_addresses(td, {}, s, 1000)
    set_sub_partition_address_and_size(td, sub_partitions)
    expect_addr_size(td, 'a', 0, None)
    expect_addr_size(td, 'b', 100, None)
    expect_addr_size(td, 'c', 120, None)
    expect_addr_size(td, 'd', 220, None)
    expect_addr_size(td, 'e', 320, None)
    expect_addr_size(td, 'app', 420, 480)
    expect_addr_size(td, 'f', 900, None)
    expect_addr_size(td, 'g', 920, None)
    expect_addr_size(td, 'h', 940, None)
    expect_addr_size(td, 'i', 960, None)
    expect_addr_size(td, 'j', 980, None)
    expect_addr_size(td, 'k', 960, 40)

    td = {'mcuboot': {'placement': {'before': ['app', 'spu']}, 'size': 200},
          'b0': {'placement': {'before': ['mcuboot', 'app']}, 'size': 100},
          'app': {}}
    s, _ = resolve(td)
    set_addresses(td, {}, s, 1000)
    expect_addr_size(td, 'b0', 0, None)
    expect_addr_size(td, 'mcuboot', 100, None)
    expect_addr_size(td, 'app', 300, 700)

    td = {'b0': {'placement': {'before': ['mcuboot', 'app']}, 'size': 100}, 'app': {}}
    s, _ = resolve(td)
    set_addresses(td, {}, s, 1000)
    expect_addr_size(td, 'b0', 0, None)
    expect_addr_size(td, 'app', 100, 900)

    td = {'spu': {'placement': {'before': ['app']}, 'size': 100},
          'mcuboot': {'placement': {'before': ['spu', 'app']}, 'size': 200},
          'app': {}}
    s, _ = resolve(td)
    set_addresses(td, {}, s, 1000)
    expect_addr_size(td, 'mcuboot', 0, None)
    expect_addr_size(td, 'spu', 200, None)
    expect_addr_size(td, 'app', 300, 700)

    td = {'provision': {'placement': {'before': ['end']}, 'size': 100},
          'mcuboot': {'placement': {'before': ['spu', 'app']}, 'size': 100},
          'b0': {'placement': {'before': ['mcuboot', 'app']}, 'size': 50},
          'spu': {'placement': {'before': ['app']}, 'size': 100},
          'app': {}}
    s, _ = resolve(td)
    set_addresses(td, {}, s, 1000)
    expect_addr_size(td, 'b0', 0, None)
    expect_addr_size(td, 'mcuboot', 50, None)
    expect_addr_size(td, 'spu', 150, None)
    expect_addr_size(td, 'app', 250, 650)
    expect_addr_size(td, 'provision', 900, None)

    print("All tests passed!")


if __name__ == "__main__":
    main()
