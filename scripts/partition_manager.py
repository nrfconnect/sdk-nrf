#!/usr/bin/env python3
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import yaml
from os import path
import sys
from pprint import pformat

PERMITTED_STR_KEYS = ['size', 'region']
END_TO_START = 'end_to_start'
START_TO_END = 'start_to_end'
COMPLEX = 'complex'
INVALID_ONE_OF_PROPERTIES = ['placement']

ALIGNMENT_ERROR = """Unable to fulfill alignment requirement automatically.
Please re-size the configured partition sizes to get a valid configuration.
If you are not able to get a valid configuration either re-evaluate th e
alignment requirements, or use 'static configuration' (see docs) to specify the
 partitioning. Note that aligning more than one partition which shares size
 with the dynamic partition (e.g. 'app')  is not supported."""


INVALID_ONE_OF_ERROR = """'one_of' was detected in one of the following
properties:

{}

'one of' cannot be used with these properties because its functionality is
supported in their keywords by default. Lists should be used instead.
""".format('\n'.join(f'- {x}' for x in INVALID_ONE_OF_PROPERTIES))


class PartitionError(Exception):
    pass


def remove_item_not_in_list(list_to_remove_from, list_to_check, dp):
    to_remove = [x for x in list_to_remove_from.copy() if x not in list_to_check and x != dp]
    list(map(list_to_remove_from.remove, to_remove))


def item_is_placed(d, item, after_or_before):
    assert after_or_before in ['after', 'before']
    return (('placement' in d) and
            (after_or_before in d['placement']) and
            (d['placement'][after_or_before][0] == item))


def resolve_one_of(reqs, partitions, invalid=False):
    """
    Recursively search for and handle `one_of` values in configuration keys.

    :kwarg bool invalid:    `one_of` has been marked as invalid for use in this
                            iteration of resolving, and should raise an
                            exception if found.
    """
    def empty_one_of(one_of_list):
        return "'one_of' dict did not evaluate to any partition." \
               f" Available partitions {partitions}, one_of {one_of_list}"

    for k, v in reqs.items():
        if isinstance(v, dict):
            if 'one_of' in v.keys():
                if invalid:
                    raise PartitionError(INVALID_ONE_OF_ERROR)
                if len(v.keys()) != 1:
                    raise PartitionError(
                        "'one_of' must be the only key in its dict")
                # Now fetch the first existing partition. Note that the value must be a list even if there is only
                # one entry.
                reqs[k] = [partition for partition in v['one_of'] if partition in partitions][:1]
                if len(reqs[k]) == 0:
                    raise PartitionError(empty_one_of(v['one_of']))
            else:
                # Mark as invalid in the next recursion if already invalid or
                # descending into values of invalid keys.
                resolve_one_of(v, partitions, invalid=(
                    invalid or k in INVALID_ONE_OF_PROPERTIES))
        # 'one_of' dicts can occur inside lists of partitions.
        # dicts with 'one_of' key is the only dict supported inside lists
        elif isinstance(v, list):
            # Resolve all 'one-of' dicts inside the list
            to_remove = list()
            to_add = list()
            for i in v:
                if isinstance(i, dict):
                    if invalid:
                        raise PartitionError(INVALID_ONE_OF_ERROR)
                    if 'one_of' not in i.keys():
                        raise PartitionError(
                            "Found illegal dict inside list. Only 'one_of' "
                            "dicts are allowed")
                    try:
                        to_add.append([partition for partition in i['one_of'] if partition in partitions][0])
                    except IndexError:
                        raise PartitionError(empty_one_of(i['one_of']))
                    to_remove.append(i)
            if to_add:
                reqs[k] = [i if i not in to_remove else to_add.pop(0) for i in v]


def remove_all_zero_sized_partitions(reqs, dp, system_reqs, to_delete=None):
    first = False
    if to_delete is None:
        to_delete = list()
        first = True

    for k, v in reqs.items():
        if 'size' in v and v['size'] == 0:
            to_delete.append(k)
            remove_all_zero_sized_partitions({k: v for k, v in reqs.items() if k not in to_delete},
                dp, {k: v for k, v in system_reqs.items() if k not in to_delete}, to_delete)
        if 'share_size' in v.keys():
            non_zero_partitions = [p for p in system_reqs if 'size' not in system_reqs[p] or system_reqs[p]['size'] != 0]
            actual_partitions = v['share_size'] if not isinstance(v['share_size'], dict) else v['share_size']['one_of']
            remove_item_not_in_list(actual_partitions, non_zero_partitions, dp)
            if not v['share_size'] or ('one_of' in v['share_size'] and len(v['share_size']['one_of']) == 0):
                del v['share_size']
                if 'size' not in v.keys():
                    # The partition has no size, delete it, and rerun this function with the new reqs.
                    to_delete.append(k)
                    remove_all_zero_sized_partitions({k: v for k, v in reqs.items() if k not in to_delete},
                        dp, {k: v for k, v in system_reqs.items() if k not in to_delete}, to_delete)

    if first and to_delete:
        for k in list(set(to_delete)):
            print(f"Dropping partition '{k}' since its size is 0.")
            del reqs[k]


def remove_irrelevant_requirements(reqs, system_reqs, dp):
    remove_all_zero_sized_partitions(reqs, dp, system_reqs)

    # Verify that no partitions define an empty 'placement'
    for k, v in reqs.items():
        if 'placement' in v.keys() and len(v['placement']) == 0:
            raise PartitionError(
                f"Found empty 'placement' property for partition '{k}'")

    # Exchange all occurrences of 'one_of' list, with the first existing partition in the 'one_of' list.
    # Extend the keys given as input with 'end' and 'start' as these are also valid references.
    resolve_one_of(reqs, list(reqs.keys()) + ['end', 'start'])

    # Remove dependencies to partitions which are not present
    for k, v in reqs.items():
        for before_after in ['before', 'after']:
            if 'placement' in v.keys() and before_after in v['placement'].keys():
                remove_item_not_in_list(v['placement'][before_after], [*reqs.keys(), 'start', 'end'], dp)
                if not v['placement'][before_after]:
                    del v['placement'][before_after]
        if 'span' in v.keys():
            remove_item_not_in_list(v['span'], reqs.keys(), dp)
        if 'inside' in v.keys():
            remove_item_not_in_list(v['inside'], reqs.keys(), dp)
            if not v['inside']:
                del v['inside']


def get_images_which_need_resolving(reqs, sub_partitions):
    # Get candidates which have placement specs.
    unsorted = {x for x in reqs.keys() if 'placement' in reqs[x].keys() and ('before' in reqs[x]['placement'].keys()
                                                                             or 'after' in reqs[x]['placement'].keys())}

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
    assert ab in ['after', 'before']
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


def solve_inside(reqs, sub_partitions):
    for key, value in reqs.items():
        if 'inside' in value.keys():
            sub_partitions[value['inside'][0]]['span'].append(key)


def clean_sub_partitions(reqs, sub_partitions):
    keys_to_delete = list()
    new_deletion = True

    # Remove empty partitions and partitions containing only empty partitions.
    while new_deletion:
        new_deletion = False
        for key, value in sub_partitions.items():
            if (len(value['span']) == 0) or all(x in keys_to_delete for x in value['span']):
                if key not in keys_to_delete:
                    keys_to_delete.append(key)
                    new_deletion = True

    for key in keys_to_delete:
        print(f"Dropping partition '{key}' since it is empty.")
        del sub_partitions[key]

    # "Flatten" by changing all span lists to contain the innermost partitions.
    done = False
    while not done:
        done = True
        for key, value in sub_partitions.items():
            if len(value['span']) == 0:
                raise PartitionError(f"partition {key} is empty")
            value['orig_span'] = value['span'].copy()  # Take a "backup" of the span.
            for part in (part for part in value['span'] if part in sub_partitions):
                value['span'].extend(sub_partitions[part]['span'])
                value['span'].remove(part)
                value['span'] = list(set(value['span']))  # remove duplicates
                done = False
            for part in (part for part in value['span'] if part not in sub_partitions and part not in reqs):
                value['span'].remove(part)


def convert_str_to_list(with_str):
    for k, v in with_str.items():
        if isinstance(v, dict):
            convert_str_to_list(v)
        elif isinstance(v, str) and k not in PERMITTED_STR_KEYS:
            with_str[k] = list()
            with_str[k].append(v)


def resolve_ambiguous_requirements(reqs, unsolved):
    """
    Find partitions where the requirements are identical, and therefore
    ambiguous. For all partitions with identical requirements, introduce
    requirements so that the partitions have unique placements(sorted by name).
    """

    buckets = dict()
    for partition in unsolved:
        key = str(reqs[partition]['placement'])
        if key not in buckets:
            buckets[key] = list()
        buckets[key].append(partition)

    for partitions in buckets.values():
        if len(partitions) > 1:
            # Two or more partitions share the same requirement, update the
            # requirements to ensure explicit order.
            partitions = sorted(partitions)
            for i in range(len(partitions) - 1):
                reqs[partitions[i]]['placement'] \
                    = {'before': [partitions[i + 1]]}


def resolve(reqs, dp, system_reqs = None):
    if system_reqs is None:
        system_reqs = reqs
    convert_str_to_list(reqs)
    solution = ["start", dp, "end"]

    remove_irrelevant_requirements(reqs, system_reqs, dp)
    sub_partitions = {k: v for k, v in reqs.items() if 'span' in v}
    for k, v in list(reqs.items()):
        if 'span' in v:
            del reqs[k]

    solve_inside(reqs, sub_partitions)
    clean_sub_partitions(reqs, sub_partitions)

    unsolved = get_images_which_need_resolving(reqs, sub_partitions)
    resolve_ambiguous_requirements(reqs, unsolved)

    for name, req in reqs.items():
        if (item_is_placed(req, "start", "before") or item_is_placed(req, "end", "after")):
            raise PartitionError(f'Partition "{name}" was placed before start or after end.')

    while unsolved:
        current_len = len(unsolved)
        solve_direction(reqs, sub_partitions, unsolved, solution, 'before')
        solve_direction(reqs, sub_partitions, unsolved, solution, 'after')
        if current_len == len(unsolved):
            raise PartitionError('Unable to solve the following partitions (endless loop):\n'
                                 + pformat(unsolved))

    assert(solution[0] == "start" and solution[-1] == "end"), "invalid solution wrt. start and end."
    solution.remove("start")
    solution.remove("end")

    # Validate partition spanning.
    for sub in sub_partitions:
        indices = [solution.index(part) for part in sub_partitions[sub]['span']]
        if not ((not indices)
                or (max(indices) - min(indices) + 1 == len(indices))):
            raise PartitionError(f"partition {sub}"
                                 f" ({str(sub_partitions[sub]['span'])})"
                                 f" does not span over consecutive "
                                 f"parts. Solution: {str(solution)}")
        for part in sub_partitions[sub]['span']:
            if part not in solution:
                raise PartitionError(f'Some or all parts of partition {part}'
                                     f' have not been placed.')

    return solution, sub_partitions


def shared_size(reqs, share_with, total_size, dp):
    size = sizeof(reqs, share_with, total_size, dp)
    if share_with == dp or \
            ('span' in reqs[share_with].keys() and dp in reqs[share_with]['span']):
        sharer_count = reqs[share_with]['sharers']
        size /= (sharer_count + 1)
    return int(size)


def get_size_source(reqs, sharer):
    size_source = sharer
    while 'share_size' in reqs[size_source].keys():
        # Find "original" source.
        size_source = reqs[size_source]['share_size'][0]
    return size_source


def set_shared_size(all_reqs, total_size, dp, system_reqs):
    for req in all_reqs.keys():
        if 'share_size' in all_reqs[req].keys():
            size_source = get_size_source(system_reqs, req)
            if size_source in all_reqs.keys():
                if 'sharers' not in all_reqs[size_source].keys():
                    all_reqs[size_source]['sharers'] = 0
                all_reqs[size_source]['sharers'] += 1
                all_reqs[req]['share_size'] = [size_source]

    new_sizes = dict()

    # Find partitions which share size with dynamic partition or a container partition which spans dynamic partition.
    dynamic_size_sharers = get_dependent_partitions(all_reqs, dp, system_reqs)
    static_size_sharers = [k for k, v in all_reqs.items() if 'share_size' in v.keys() and k not in dynamic_size_sharers]
    for req in static_size_sharers:
        share_with_name = system_reqs[req]['share_size'][0]
        share_with = system_reqs[share_with_name]
        if share_with['region'] == all_reqs[req]['region']:
            all_reqs[req]['size'] = shared_size(system_reqs, share_with_name, total_size, dp)
        else:
            all_reqs[req]['size'] = share_with['size']
    for req in dynamic_size_sharers:
        new_sizes[req] = shared_size(all_reqs, all_reqs[req]['share_size'][0], total_size, dp)
    # Update all sizes after-the-fact or else the calculation will be messed up.
    for key, value in new_sizes.items():
        all_reqs[key]['size'] = value


def get_dependent_partitions(all_reqs, target, system_reqs):
    return [k for k, v in all_reqs.items() if 'share_size' in v.keys()
            and (v['share_size'][0] == target
                 or ('span' in system_reqs[v['share_size'][0]].keys()
                     and target in system_reqs[v['share_size'][0]]['span']))]


def dynamic_partitions_size(reqs, total_size, dp):
    size = total_size - sum([req['size'] for name, req in reqs.items() if 'size' in req.keys() and name != dp])
    return size


def verify_layout(reqs, solution, total_size, flash_start):
    # Verify no overlap, that all flash is assigned, and that the total amount of flash
    # assigned corresponds to the total size available.
    expected_address = flash_start
    for p in solution:
        actual_address = reqs[p]['address']
        if actual_address != expected_address:
            raise PartitionError(f'Error when inspecting {p},'
                                 f' invalid address {hex(actual_address)},'
                                 f' expected {hex(expected_address)},')

        expected_address += reqs[p]['size']
    last = reqs[solution[-1]]
    if not last['address'] + last['size'] == flash_start + total_size:
        raise PartitionError('End of last partition is after last valid'
                             ' address')


def set_addresses_and_align(reqs, sub_partitions, solution, size, dp, start=0, system_reqs=None):
    all_reqs = dict(reqs, **sub_partitions)
    if system_reqs is None:
        system_reqs = all_reqs
    set_shared_size(all_reqs, size, dp, system_reqs)
    dynamic_partitions = [dp]
    dynamic_partitions += get_dependent_partitions(all_reqs, dp, system_reqs)
    reqs[dp]['size'] = dynamic_partitions_size(reqs, size, dp)
    reqs[solution[0]]['address'] = start

    if len(reqs) > 1:
        _set_addresses_and_align(reqs, sub_partitions, solution, size, start, dynamic_partitions, dp)
        verify_layout(reqs, solution, size, start)


def first_partition_has_been_aligned(first, solution):
    return 'placement' in first and 'align' in first['placement'] and 'end' in first['placement']['align'] \
           and solution[1] == 'EMPTY_0'


def _set_addresses_and_align(reqs, sub_partitions, solution, size, start, dynamic_partitions, dp):

    solution_non_empty = [part for part in solution if 'EMPTY_' not in part]

    # Check 'align' and 'align_next'.
    for part in solution_non_empty:
        if 'align' in reqs[part]:
            raise PartitionError(f"'align' config is misplaced (should be part of 'placement').")
        if 'align_next' in reqs[part]:
            raise PartitionError(f"'align_next' config is misplaced (should be part of 'placement').")

    # Resolve 'align_next'
    for i in range(len(solution_non_empty) - 1):
        part = solution_non_empty[i]
        next_part = solution_non_empty[i + 1]
        if 'placement' in reqs[part] and 'align_next' in reqs[part]['placement']:
            a_next = reqs[part]['placement'].pop('align_next')
            alignment = a_next

            reqs[next_part].setdefault('placement', {}).setdefault('align', {})

            if 'end' in reqs[next_part]['placement']['align']:
                raise PartitionError(f"Align config already present in {next_part}")

            if 'start' in reqs[next_part]['placement']['align']:
                a_start = reqs[next_part]['placement']['align']['start']
                if ((a_start % a_next) == 0) or ((a_next % a_start) == 0):
                    alignment = max(a_next, a_start)
                else:
                    raise PartitionError(f"Align config already present in {next_part}")

            reqs[next_part]['placement']['align']['start'] = alignment

    # Perform address assignment and alignment in two steps, first from start to app, then from end to app.
    for i in range(0, solution.index(dp) + 1):
        current = solution[i]

        if i != 0:
            previous = solution[i - 1]
            reqs[current]['address'] = reqs[previous]['address'] + reqs[previous]['size']

        # To avoid messing with vector table, don't store empty partition as the first.
        insert_empty_partition_before = i != 0

        # Special handling is needed when aligning the first partition
        if i == 0 and first_partition_has_been_aligned(reqs[current], solution):
            continue

        if align_if_required(i, dynamic_partitions,
                             insert_empty_partition_before, reqs, dp,
                             solution):
            _set_addresses_and_align(reqs, sub_partitions, solution, size, start, dynamic_partitions, dp)

    for i in range(len(solution) - 1, solution.index(dp), -1):
        current = solution[i]

        if i == len(solution) - 1:
            reqs[current]['address'] = (start + size) - reqs[current]['size']
        else:
            higher_partition = solution[i + 1]
            reqs[current]['address'] = reqs[higher_partition]['address'] - reqs[current]['size']

        if align_if_required(i, dynamic_partitions, False, reqs, dp, solution):
            try:
                _set_addresses_and_align(reqs, sub_partitions, solution, size,
                                         start, dynamic_partitions, dp)
            except RecursionError:
                raise PartitionError(ALIGNMENT_ERROR)


def align_if_required(i, dynamic_partitions, move_up, reqs, dp, solution):
    current = solution[i]
    if 'placement' in reqs[current] and 'align' in reqs[current]['placement']:
        required_offset = align_partition(current, reqs, move_up,
                                          dynamic_partitions, dp, solution)
        if required_offset:
            solution_index = i if move_up else i + 1
            solution.insert(solution_index, required_offset)
            return True
    return False


def align_partition(current, reqs, move_up, dynamic_partitions, dp, solution):
    required_offset = get_required_offset(align=reqs[current]['placement']['align'], start=reqs[current]['address'],
                                          size=reqs[current]['size'], move_up=move_up)
    if not required_offset:
        return None

    empty_partition_size = required_offset

    if current not in dynamic_partitions:
        if move_up:
            empty_partition_address = reqs[current]['address']
            reqs[current]['address'] += required_offset
        else:
            empty_partition_address = reqs[current]['address'] + reqs[current]['size']
            if reqs[current]['address'] != 0:  # Special handling for the first partition as it cannot be moved down
                reqs[current]['address'] -= required_offset
    elif not move_up:
        align_end = 'end' in reqs[current]['placement']['align']
        empty_partition_address, empty_partition_size = \
            get_empty_part_to_move_dyn_part(dynamic_partitions, current, reqs,
                                            required_offset, align_end,
                                            solution)
    else:
        raise PartitionError('Invalid combination, can not have dynamic'
                             ' partition in front of app with alignment')

    e = 'EMPTY_{}'.format(len([x for x in reqs.keys() if 'EMPTY' in x]))
    reqs[e] = {'address': empty_partition_address,
               'size': empty_partition_size,
               'region': reqs[dynamic_partitions[0]]['region'],
               'placement': {'before' if move_up else 'after': [current]}}

    # Adjust size of dynamic partitions to accommodate for new empty partition
    for p in dynamic_partitions:
        reqs[p]['size'] -= reqs[e]['size'] // len(dynamic_partitions)

    return e


def get_empty_part_to_move_dyn_part(dynamic_partitions, current, reqs,
                                    num_bytes_to_move_dyn_part, move_end,
                                    solution):
    """
    Find the start address and size of a new empty partition whose purpose is
    to move the partition @current @num_bytes_to_move_dyn_part bytes down
    (lower address). The empty partition will always be placed directly
    behind the last partition in the list @dynamic_partitions.

    :param dynamic_partitions: List of partitions which share size with the
    main dynamic partition.
    :param current: Partition being moved. Must be a member of the
    @dynamic_partitions list.
    :param reqs: Dict of current partition manager requirements
    :param num_bytes_to_move_dyn_part: Number of bytes to move the dynamic
    partition @current (down)
    :param move_end: Should the end or start of @current be moved
    @num_num_bytes_to_move_dyn_part bytes?
    :param solution: List representing the order of the partitions in the
    solution.
    :return: address, size of an empty partition which would result in start or
    end of @current being moved @num_bytes_to_move_dyn_part bytes down
    (lower address).
    """

    # Get the dynamic partitions in the order they appear in the flash
    # placement solution.
    dyn_solution = [x for x in solution if x in dynamic_partitions]
    first_dyn = dyn_solution[0]

    # The current partition is dynamic, and all dynamic partitions are bundled
    # together. See how many dynamic partitions are in front of the current
    # partition
    num_dyn_part_in_front = solution.index(current) - solution.index(first_dyn)

    if not num_bytes_to_move_dyn_part > 0:
        raise PartitionError('Invalid move size specified, must be > 0.')

    if move_end:
        # Add the current partition to the list of dynamic partitions in front,
        # since the end is moved, and the resize in the current partition will
        # affect the alignment as well.
        num_dyn_part_in_front += 1

    reduction_each_dynamic_part = \
        num_bytes_to_move_dyn_part // num_dyn_part_in_front

    if not reduction_each_dynamic_part % 4 == 0:
        raise PartitionError(f'The current configuration gives {first_dyn}'
                             f' a non-word-aligned size' + ALIGNMENT_ERROR)

    if not reduction_each_dynamic_part < reqs[first_dyn]['size']:
        raise PartitionError(
            f"The current configuration is invalid because it requires "
            f"{first_dyn}'s size to be less than 0." + ALIGNMENT_ERROR)

    # The size and address of the dynamic partitions will be calculated
    # automatically based on the size of the
    # non-dynamic partitions.
    last_dyn = reqs[dyn_solution[-1]]
    empty_partition_size = \
        reduction_each_dynamic_part * len(dynamic_partitions)
    empty_partition_address = \
        last_dyn['address'] + last_dyn['size'] - empty_partition_size

    return empty_partition_address, empty_partition_size


def get_required_offset(align, start, size, move_up):
    if len(align) != 1 or ('start' not in align and 'end' not in align):
        raise PartitionError(f'Invalid alignment requirement {align}')

    end = start + size
    align_start = 'start' in align

    try:
        if (align_start and start % align['start'] == 0) or (not align_start and end % align['end'] == 0):
            return 0

        if move_up:
            return align['start'] - (start % align['start']) if align_start else align['end'] - (end % align['end'])
        else:
            if align_start:
                return start % align['start']
            else:
                # Special handling is needed if start is 0 since this partition can not be moved down
                return end % align['end'] if start != 0 else align['end'] - (end % align['end'])
    except TypeError as err:
        keyword = 'start' if align_start else 'end'
        raise TypeError(f"elements in align: {{{keyword}:{align[keyword]}}} is not type of \'int\'") from err


def set_size_addr(entry, size, address):
    entry['size'] = size
    entry['address'] = address


def set_sub_partition_address_and_size(reqs, sub_partitions):
    for sp_name, sp_value in sub_partitions.items():
        size = sum([reqs[part]['size'] for part in sp_value['span']])
        if size == 0:
            raise PartitionError(
                f'No compatible parent partition found for {sp_name}')
        address = min([reqs[part]['address'] for part in sp_value['span']])

        reqs[sp_name] = sp_value
        reqs[sp_name]['span'] = reqs[sp_name]['orig_span']  # Restore "backup".
        set_size_addr(reqs[sp_name], size, address)


def sizeof(reqs, req, total_size, dp):
    if req == dp:
        size = dynamic_partitions_size(reqs, total_size, dp)
    elif 'span' not in reqs[req].keys():
        size = reqs[req]['size'] if 'size' in reqs[req].keys() else 0
    else:
        size = sum([sizeof(reqs, part, total_size, dp) for part in reqs[req]['span']])

    return size


def load_reqs(input_config):
    reqs = dict()

    for ymlpath in input_config:
        if path.exists(ymlpath):
            with open(ymlpath, 'r') as f:
                loaded_reqs = yaml.safe_load(f)
                if loaded_reqs is None:
                    continue
                for key in loaded_reqs.keys():
                    if key in reqs and loaded_reqs[key] != reqs[key]:
                        raise PartitionError(
                            f"Conflicting configuration found for '{f.name}'"
                            f" value for key '{key}' differs. val1: "
                            f"{loaded_reqs[key]} val2: {reqs[key]} ")
                reqs.update(loaded_reqs)

    return reqs


def get_dynamic_area_start_and_size(static_config, base, size, dp):
    # Remove app from this dict to simplify the case where partitions
    # before and after are removed.
    proper_partitions = [config for name, config in static_config.items()
                         if 'span' not in config.keys() and
                         name != dp]

    first_address = base
    last_address = base + size

    starts = {last_address} | {config['address']
                               for config in proper_partitions}
    ends = {first_address} | {config['address'] + config['size']
                              for config in proper_partitions}
    gaps = list(zip(sorted(ends - starts), sorted(starts - ends)))

    if len(gaps) != 1:
        raise PartitionError(
            "Incorrect amount of gaps found in static configuration. "
            "There must be exactly one gap in the static configuration to "
            "support placing the dynamic partitions (such as 'app'). "
            f"Gaps found ({len(gaps)}):" +
            " ".join([f"0x{gap[0]:x}-0x{gap[1]:x}" for gap in gaps]) +
            " The most common solution to this problem is to fill the "
            "smallest of these gaps with statically defined partition(s) until"
            " there is only one gap left. Alternatively re-order the already "
            "defined static partitions so that only one gap remains.")

    start, end = gaps[0]
    return start, end - start


def calculate_end_address(pm_config):
    for part in pm_config:
        pm_config[part]['end_address'] = pm_config[part]['address'] + pm_config[part]['size']


def get_region_config(pm_config, region_config, static_conf=None, system_reqs=None):
    if system_reqs is None:
        system_reqs = pm_config
    start = region_config['base_address']
    size = region_config['size']
    placement_strategy = region_config['placement_strategy']
    region_name = region_config['name']
    device = region_config['device']
    dp = region_config['dynamic_partition'] \
        if ('dynamic_partition' in region_config and region_config['dynamic_partition'] is not None)\
        else 'app'

    if placement_strategy in [END_TO_START, START_TO_END]:
        solve_simple_region(pm_config, start, size, placement_strategy, region_name, device,
                            static_conf, system_reqs)
    else:
        if dp != 'app':
            # All configurations use 'app' to denote the dynamic partition.
            # Replace all occurrences of 'app' in the given configuration to
            # facilitate working with it.
            replace_app_with_dynamic_partition(pm_config, dp)

            # Create a span over the dynamic partition so that 'app' can be used as a reference for the dynamic
            # partition in build system and code.
            pm_config['app'] = {'span': [dp], 'region': region_config['name']}

        pm_config[dp] = dict()
        pm_config[dp]['region'] = region_config['name']

        solve_complex_region(pm_config, start, size, placement_strategy, region_name, device, static_conf, dp, system_reqs)

    calculate_end_address(pm_config)


def solve_simple_region(pm_config, start, size, placement_strategy, region_name, device, static_conf,
                        system_reqs):
    if system_reqs is None:
        system_reqs = pm_config

    set_shared_size(pm_config, size, None, system_reqs)
    reserved = 0
    if static_conf:
        verify_static_conf_simple(size, start, placement_strategy, static_conf)
        reserved = sum([config['size'] for name, config in static_conf.items()
                        if 'region' in config.keys() and config['region'] == region_name])
        pm_config.update(static_conf)

    if placement_strategy == END_TO_START:
        address = start + size - reserved
    else:
        address = start + reserved

    # Static partitions are now added to the pm_config dict. These partitions
    # already have their 'address' set, so skip these partitions in this loop.
    for partition_name in [k for k in pm_config if 'address' not in pm_config[k]]:
        if placement_strategy == END_TO_START:
            address -= pm_config[partition_name]['size']

        pm_config[partition_name]['address'] = address

        if placement_strategy == START_TO_END:
            address += pm_config[partition_name]['size']

        if device:
            pm_config[partition_name]['device'] = device

    if not static_conf or (region_name not in static_conf):
        # Generate the region partition containing the non-reserved memory.
        # But first, verify that the user hasn't created a partition with the name of the region.
        if region_name in pm_config:
            raise PartitionError(
                f'Found partition named {region_name}, this is the name of a'
                f' region, and is a reserved name')

        pm_config[region_name] = dict()
        pm_config[region_name]['region'] = region_name

        if placement_strategy == END_TO_START:
            pm_config[region_name]['address'] = start
            pm_config[region_name]['size'] = address - start
        else:
            pm_config[region_name]['address'] = address
            pm_config[region_name]['size'] = (start + size) - address


def verify_static_conf_simple(size, start, placement_strategy, static_conf):
    # Verify the static configuration of a region with 'simple' placement.
    # Ensure that all statically defined partitions have a given address,
    # and that they are packed at the end/start of the region.
    starts = {start + size} | {c['address']
                               for c in static_conf.values() if 'size' in c}
    ends = {start} | {c['address'] + c['size']
                      for c in static_conf.values() if 'size' in c}
    gaps = list(zip(sorted(ends - starts), sorted(starts - ends)))

    # The whole region is filled, which is valid.
    if len(gaps) == 0:
        return

    if placement_strategy == START_TO_END:
        start_end_correct = gaps[0][1] == start + size
    else:
        start_end_correct = gaps[0][0] == start

    if len(gaps) != 1:
        raise PartitionError(
            "Incorrect amount of gaps found in static configuration for region"
            f" '{list(static_conf.values())[0]['region']}'. "
            "There must be exactly one gap in the static configuration to "
            "support placing the non-statically-defined partitions. "
            f"Gaps found ({len(gaps)}):" +
            " ".join([f"0x{gap[0]:x}-0x{gap[1]:x}" for gap in gaps]) +
            " The most common solution to this problem is to re-order the "
            "defined static partitions so that only one gap remains.")
    elif not start_end_correct:
        raise PartitionError(
            f"Statically defined partitions are not packed at "
            f"{'start' if placement_strategy == START_TO_END else 'end'} of "
            f"region '{list(static_conf.values())[0]['region']}'.")


def solve_complex_region(pm_config, start, size, placement_strategy, region_name, device, static_conf, dp, system_reqs):
    free_size = size

    if static_conf:
        start, free_size = \
            get_dynamic_area_start_and_size(static_conf, start, size, dp)

        # If nothing is unresolved (only dynamic partition remaining),
        # simply return the pre defined config with dynamic_partition
        if len(pm_config) == 1:
            pm_config.update(static_conf)
            pm_config[dp]['address'] = start
            pm_config[dp]['size'] = free_size
            return

    solution, sub_partitions = resolve(pm_config, dp, system_reqs)
    set_addresses_and_align(pm_config, sub_partitions, solution, free_size, dp, start=start, system_reqs=system_reqs)
    set_sub_partition_address_and_size(pm_config, sub_partitions)

    if static_conf:
        # Merge the results, take the new dynamic_partition as that has the correct size.
        pm_config.update({name: config for name, config in static_conf.items() if name != dp})


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
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)
    parser.add_argument('--input-files', required=True, type=str, nargs='+',
                        help='List of paths to input yaml files.')

    parser.add_argument('--output-partitions', required=True, type=str,
                        help='Path to output partition configuration file.')

    parser.add_argument('--output-regions', required=True, type=str,
                        help='Path to output regions configuration file.')

    parser.add_argument('--static-config', required=False, type=argparse.FileType(mode='r'),
                        help='Path static configuration.')

    parser.add_argument('--regions', required=False, type=str, nargs='*',
                        help="Space separated list of regions. For each region specified here, one must specify"
                             "--{region_name}-base-addr and --{region_name}-size. If the region is associated"
                             "with a driver, the device name must be given in --{region_name}-device (e.g. an "
                             "external flash driver. For regions with placement strategy 'complex' it is possible"
                             "to specify the --{region_name}-dynamic-partition to set the name of the dynamic partition"
                             "which occupies all non-used area.")

    main_args, region_args = parser.parse_known_args()

    # Create new instance to parse regions
    parser = argparse.ArgumentParser(allow_abbrev=False)
    for x in main_args.regions:
        # Generate arguments for each region dynamically
        parser.add_argument(f'--{x}-size', required=True, type=lambda z: int(z, 0))
        parser.add_argument(f'--{x}-base-address', required=False, type=lambda z: int(z, 0), default=0)
        parser.add_argument(f'--{x}-placement-strategy', required=False, type=str,
                            choices=[START_TO_END, END_TO_START, COMPLEX], default=START_TO_END)
        parser.add_argument(f'--{x}-device', required=False, type=str, default='')
        parser.add_argument(f'--{x}-default-driver-kconfig', required=False, type=str, default='',
                            help="Default Kconfig option that is used to turn on the driver")
        parser.add_argument(f'--{x}-dynamic-partition', required=False, type=str, help='Name of dynamic partition')

    ranges_configuration = parser.parse_args(region_args)

    return main_args, ranges_configuration


def replace_app_with_dynamic_partition(d, dynamic_partition_name):
    for k, v in d.items():
        if isinstance(v, dict):
            replace_app_with_dynamic_partition(v, dynamic_partition_name)
        elif isinstance(v, list) and 'app' in v:
            d[k] = [o if o != 'app' else dynamic_partition_name for o in v]
        elif isinstance(v, str) and v == 'app':
            v = dynamic_partition_name


def fix_syntactic_sugar(pm_config):
    for k, v in pm_config.items():
        if 'region' not in v:
            # Set SRAM primary region
            if k.endswith('_sram'):
                v['region'] = 'sram_primary'

            # Set FLASH primary region
            else:
                v['region'] = 'flash_primary'


def get_region_config_from_args(args, ranges_configuration):
    regions = {x: {k.replace(f'{x}_', ''): v
                   for k, v in vars(ranges_configuration).items() if k.startswith(x)}
               for x in [y.replace('-', '_') for y in args.regions]}  # Replace - with _ to match argparse namespace
    return regions


def solve_region(pm_config, region, region_config, static_config, regions):
    solution = dict()
    region_config['name'] = region
    partitions = {k: v for k, v in pm_config.items() if region in v['region']}
    static_partitions = {k: v for k, v in static_config.items() if region in v['region']}

    get_region_config(partitions, region_config, static_partitions, system_reqs=pm_config)

    solution.update(partitions)

    return solution


def load_static_configuration(args, pm_config):
    static_config = yaml.safe_load(args.static_config)
    fix_syntactic_sugar(static_config)

    # Delete all statically defined partitions from the pm_config dict.
    # This is done since all partitions in pm_config will be resolved.
    for statically_defined_image in static_config:
        if statically_defined_image in pm_config and statically_defined_image:
            print(f"Partition '{statically_defined_image}' is not included "
                  "in the dynamic resolving since it is statically defined.")
            del pm_config[statically_defined_image]
    return static_config

def region_sort_key(pm_config, region, used_regions):
    if region in used_regions:
        raise PartitionError("Found 'share_size' loop between the following regions\n" + pformat(used_regions))
    used_regions.append(region)
    i = 1 # Partitions without 'share_size' will have the smallest key (1)

    # Iterate over all partitions in 'region'.
    # For every partition with a share_size entry that is in another region, recursively find the sort_key of
    # that region, and add 1. Keep the biggest sort_key out of the ones found. This sort_key will correspond to
    # the longest region dependency path.
    for config in {k:v for k,v in pm_config.items() if v['region'] == region}.values():
        if 'share_size' in config:
            ssize = config['share_size']
            candidates = [ssize] if isinstance(ssize, str) else ssize if isinstance(ssize, list) else ssize['one_of']
            # For lists and 'oneof' specs, treat each option as a dependency.
            for cand in candidates:
                if cand in pm_config and config['region'] != pm_config[cand]['region']:
                    used = used_regions.copy() # For checking for loops
                    i = max(i, 1 + region_sort_key(pm_config, pm_config[cand]['region'], used))
    return i

def sort_regions(pm_config, regions):
    return {k: v for k, v in sorted(regions.items(), key = lambda r: region_sort_key(pm_config, r[0], []))}

def main():
    args, ranges_configuration = parse_args()
    pm_config = load_reqs(args.input_files)
    static_config = load_static_configuration(args, pm_config) if args.static_config else dict()
    fix_syntactic_sugar(pm_config)

    regions = get_region_config_from_args(args, ranges_configuration)

    regions = sort_regions(pm_config, regions)

    solution = dict()
    for region, region_config in regions.items():
        try:
            solution.update(solve_region(pm_config, region, region_config,
                                         static_config, regions))
        except PartitionError as e:
            print(f"Partition manager failed: {str(e)}")
            print(f"Failed to partition region {region},"
                  f" size of region: {region_config['size']}")
            print('Partition Configuration:')
            to_print = \
                {x: {a: b for a, b in y.items() if a in
                     ['size', 'placement', 'align']}
                 for x, y in {**pm_config, **static_config}.items()
                 if 'size' in y and 'region' in y and y['region'] == region}
            print(yaml.dump(to_print))
            sys.exit(1)

    write_yaml_out_file(solution, args.output_partitions)
    write_yaml_out_file(regions, args.output_regions)


def expect_addr_size(td, name, expected_address, expected_size):
    if expected_size is not None:
        assert td[name]['size'] == expected_size, \
            'Size of {} was {}, expected {}.\ntd:{}'.format(name, td[name]['size'], expected_size, pformat(td))
    if expected_address is not None:
        assert td[name]['address'] == expected_address, \
            'Address of {} was {}, expected {}.\ntd:{}'.format(name, td[name]['address'], expected_address, pformat(td))
    if expected_size is not None and expected_address is not None:
        assert td[name]['end_address'] == expected_address + expected_size, \
            'End address of {} was {}, expected {}.\ntd:{}'.format(name, td[name]['end_address'], expected_address + expected_size, pformat(td))


def expect_list(expected, actual):
    expected_list = list(sorted(expected))
    actual_list = list(sorted(actual))
    assert sorted(expected_list) == sorted(actual_list), 'Expected list {}, was {}'.format(expected_list, actual_list)


def test():
    list_one = [1, 2, 3, 4]
    items_to_check = [4]
    remove_item_not_in_list(list_one, items_to_check, 'app')
    assert list_one[0] == 4
    assert len(list_one) == 1

    test_config = {
        'first': {'address': 0,    'size': 10},
        # Gap from deleted partition.
        'app':   {'address': 20, 'size': 10},
        # Gap from deleted partition.
        'fourth': {'address': 40, 'size': 60}}
    start, size = get_dynamic_area_start_and_size(test_config, 0, 100, 'app')
    assert start == 10
    assert size == 40-10

    test_config = {
        'first':  {'address': 0,    'size': 10},
        'second': {'address': 10, 'size': 10},
        'app':    {'address': 20, 'size': 80}
        # Gap from deleted partition.
    }

    start, size = get_dynamic_area_start_and_size(test_config, 0, 100, 'app')
    assert start == 20
    assert size == 80

    test_config = {
        'app':    {'address': 0,    'size': 10},
        # Gap from deleted partition.
        'second': {'address': 40, 'size': 60}}
    start, size = get_dynamic_area_start_and_size(test_config, 0, 100, 'app')
    assert start == 0
    assert size == 40

    test_config = {
        'first': {'address': 0,    'size': 10},
        # Gap from deleted partition.
        'app':   {'address': 20, 'size': 10}}
    start, size = get_dynamic_area_start_and_size(test_config, 0, 100, 'app')
    assert start == 10
    assert size == 100 - 10

    # Verify that base address can be larger than 0 (RAM vs FLASH)
    test_config = {
        'first': {'address': 1000, 'size': 10},
        # Gap from deleted partition.
        'app': {'address': 1200, 'size': 10}}
    start, size = get_dynamic_area_start_and_size(test_config, 1000, 400,
                                                  'app')
    assert start == 1010
    assert size == 400 - 10

    # Verify that all 'end' and 'start' are valid references in 'one_of' dicts
    td = {
        'a': {'placement': {'after': ['x0', 'x1', 'start']}, 'size': 100},
        'b': {'placement': {'before': ['x0', 'x1', 'end']}, 'size': 200},
        'app': {},
    }
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'a', 0, 100)
    expect_addr_size(td, 'app', 100, 700)
    expect_addr_size(td, 'b', 800, 200)

    # Verify that 'app' spans the dynamic partition when a dynamic partition is set
    td = {'a': {'size': 100, 'region': 'flash', 'placement': {'after': 'start'}}}
    test_region = {'name': 'flash',
                   'size': 1000,
                   'base_address': 0,
                   'placement_strategy': COMPLEX,
                   'device': 'some-driver-device',
                   'dynamic_partition': 'the_dynamic_partition'}
    get_region_config(td, test_region)
    assert td['app']['span'][0] == 'the_dynamic_partition'

    # Verify that START_TO_END region configuration is correct
    td = {'b': {'size': 100, 'region': 'extflash'}}
    test_region = {'name': 'extflash',
                   'size': 1000,
                   'base_address': 2000,
                   'placement_strategy': START_TO_END,
                   'device': 'some-driver-device'}
    get_region_config(td, test_region)
    assert td['b']['size'] == 100
    assert td['extflash']['address'] == 2100
    assert td['extflash']['size'] == 900

    # Verify that SRAM configuration is correct
    td = {'b': {'size': 100, 'region': 'sram'}}
    test_region = {'name': 'sram',
                   'size': 1000,
                   'base_address': 2000,
                   'placement_strategy': END_TO_START,
                   'device': None}
    get_region_config(td, test_region)
    assert td['b']['size'] == 100
    assert td['sram']['address'] == 2000
    assert td['sram']['size'] == 900

    # Verify that sram configuration is correct
    td = {
        'b': {'size': 100, 'region': 'sram'},
        'c': {'size': 200, 'region': 'sram'},
        'd': {'size': 300, 'region': 'sram'}
    }
    test_region = {'name': 'sram',
                   'size': 1000,
                   'base_address': 2000,
                   'placement_strategy': END_TO_START,
                   'device': None}
    get_region_config(td, test_region)
    assert td['sram']['address'] == 2000
    assert td['sram']['size'] == 400
    # Can not verify the placement, as this is random
    assert td['b']['size'] == 100
    assert td['c']['size'] == 200
    assert td['d']['size'] == 300

    # Verify that SRAM configuration with given static configuration is correct
    test_region = {'name': 'sram',
                   'size': 1000,
                   'base_address': 2000,
                   'placement_strategy': END_TO_START,
                   'device': None}
    td = {
        'b': {'size': 100, 'region': 'sram'},
        'c': {'size': 200, 'region': 'sram'},
        'd': {'size': 300, 'region': 'sram'},
    }
    get_region_config(td,
                      test_region,
                      static_conf={'s1': {'size': 100,
                                          'address': (1000+2000)-100,
                                          'region': 'sram'},
                                   's2': {'size': 200,
                                          'address': (1000+2000)-100-200,
                                          'region': 'sram'}})
    assert td['sram']['address'] == 2000
    assert td['sram']['size'] == 100
    # Can not verify the placement, as this is random
    assert td['b']['size'] == 100
    assert td['c']['size'] == 200
    assert td['d']['size'] == 300

    # Verify that SRAM configuration with given static configuration fails if static SRAM partitions are not
    # packed at the end of flash, here there is a space between the two regions
    test_region = {'name': 'sram',
                   'size': 1000,
                   'base_address': 2000,
                   'placement_strategy': END_TO_START,
                   'device': None}
    failed = False
    td = {
        'a': {'placement': {'after': 'start'}, 'size': 100},
        'b': {'size': 100, 'region': 'sram'},
        'c': {'size': 200, 'region': 'sram'},
        'd': {'size': 300, 'region': 'sram'},
        'app': {}
    }
    try:
        get_region_config(td,
                          test_region,
                          static_conf={'s1': {'size': 100,
                                              'address': (1000+2000)-100,
                                              'region': 'sram'},
                                       's2': {'size': 200,
                                              'address': (1000+2000)-100-300,
                                              'region': 'sram'}})  # Note 300 not 200
    except PartitionError:
        failed = True

    assert failed

    # Verify that SRAM configuration with given static configuration fails if static SRAM partitions are not
    # packed at the end of flash, here the partitions are packed, but does not go to the end of SRAM
    failed = False
    test_region = {'name': 'sram',
                   'size': 1000,
                   'base_address': 2000,
                   'placement_strategy': END_TO_START,
                   'device': None}
    td = {
        'a': {'placement': {'after': 'start'}, 'size': 100},
        'b': {'size': 100, 'region': 'sram'},
        'c': {'size': 200, 'region': 'sram'},
        'd': {'size': 300, 'region': 'sram'},
        'app': {}
    }
    try:
        get_region_config(td,
                          test_region,
                          static_conf={'s1': {'size': 100,
                                              'address': (1000+2000-50)-100,
                                              'region': 'sram'},  # Note - 50
                                       's2': {'size': 200,
                                              'address': (1000+2000-50)-100-200,
                                              'region': 'sram'}})  # Note - 50
    except PartitionError:
        failed = True

    assert failed

    # Verify that all 'one_of' dicts are replaced with the first entry which corresponds to an existing partition
    td = {
        'a': {'placement': {'after': 'start'}, 'size': 100},
        'b': {'placement': {'after': ['x0', 'x1', 'a', 'x2']}, 'size': 200},
        'c': {'placement': {'after': 'b'}, 'share_size': {'one_of': ['x0', 'x1', 'b', 'a']}},
        'd': {'placement': {'after': 'c'}, 'share_size': {'one_of': ['a', 'b']}},  # Should take first existing
        # We can use  several 'one_of' - dicts inside lists
        's': {'span': ['a', {'one_of': ['x0', 'b', 'd']}, {'one_of': ['x2', 'c', 'a']}]},
        'app': {},
        'e': {'placement': {'after': 'app'}, 'share_size': {'one_of': ['x0', 'app']}},  # app always exists
    }
    fix_syntactic_sugar(td)
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'a', 0, 100)  # b is after a
    expect_addr_size(td, 'b', 100, 200)  # b is after a
    expect_addr_size(td, 'c', 300, 200)  # c shares size with b
    expect_addr_size(td, 'd', 500, 100)  # d shares size with a
    expect_addr_size(td, 's', 0, 500)  # s spans a, b and c
    expect_addr_size(td, 'app', 600, 200)  # s spans a, b and c
    expect_addr_size(td, 'e', 800, 200)  # s spans a, b and c

    #
    td = {
        'a': {'placement': {'after': 'start'}, 'size': 100},
        'b': {'placement': {'after': ['x0', 'x1', 'a', 'x2']}, 'size': 200},
        'c': {'placement': {'after': 'b'}, 'share_size': {'one_of': ['x0', 'x1', 'b', 'a']}},
        'd': {'placement': {'after': 'c'}, 'share_size': {'one_of': ['a', 'b']}},  # Should take first existing
        # We can use  several 'one_of' - dicts inside lists
        's': {'span': ['a', {'one_of': ['x0', 'b', 'd']}, {'one_of': ['x2', 'c', 'a']}]},
        'app': {},
        'e': {'placement': {'after': 'app'}, 'share_size': {'one_of': ['x0', 'app']}},  # app always exists
        'f': {'placement': {'before':  'end'}, 'share_size': 'p_ext'}
    }
    fix_syntactic_sugar(td)
    s_reqs = td.copy()
    s_reqs['p_ext'] = {'region': 'ext', 'size': 250}
    s, sub_partitions = resolve(td, 'app', s_reqs)
    set_addresses_and_align(td, sub_partitions, s, 1250, 'app', system_reqs = s_reqs)
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'a', 0, 100)  # b is after a
    expect_addr_size(td, 'b', 100, 200)  # b is after a
    expect_addr_size(td, 'c', 300, 200)  # c shares size with b
    expect_addr_size(td, 'd', 500, 100)  # d shares size with a
    expect_addr_size(td, 's', 0, 500)  # s spans a, b and c
    expect_addr_size(td, 'app', 600, 200)  # s spans a, b and c
    expect_addr_size(td, 'e', 800, 200)  # s spans a, b and c
    expect_addr_size(td, 'f', 1000, 250) # Shares size with 'p_ext' from a different region

    # Verify that all 'share_size' with value partition that has size 0 is compatible with 'one_of' dicts
    td = {
        'a': {'placement': {'after': 'start'}, 'size': 0},
        'b': {'placement': {'after': ['a', 'start']},
              'share_size': ['a']},
        'c': {'placement': {'after': ['a', 'b', 'start']},
              'share_size': {'one_of': ['a', 'b']}},
        'd': {'placement': {'after': ['a', 'b', 'c', 'start']},
              'share_size': {'one_of': ['a', 'b', 'c']}},
        # You get the point
        'e': {'placement': {'after': ['a', 'b', 'c', 'd', 'start']}, 'size': 100}
    }
    remove_all_zero_sized_partitions(td, 'app', td)
    assert 'a' not in td
    assert 'b' not in td
    assert 'c' not in td
    assert 'd' not in td

    # Verify that all 'share_size' with value partition that has size 0 is compatible withe 'one_of' dicts.
    td = {
        'a': {'placement': {'after': 'start'}, 'size': 0},
        'b': {'placement': {'after': ['a', 'start']},
              'share_size': ['a']},
        'c': {'placement': {'after': ['a', 'b', 'start']},
              'share_size': {'one_of': ['a', 'b']}},
        'd': {'placement': {'after': ['a', 'b', 'c', 'start']},
              'share_size': {'one_of': ['a', 'b', 'c']}},
        # Empty spans should be removed
        'e': {'span': [], 'region': 'sram'},
        # You get the point
        'f': {'placement': {'after': ['a', 'b', 'c', 'd', 'start']}, 'size': 100},
        'app': {}
    }
    # Perform the same test as above, but run it through the 'resolve' function this time.
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    calculate_end_address(td)
    assert 'a' not in td
    assert 'b' not in td
    assert 'c' not in td
    assert 'd' not in td
    assert 'e' not in td
    expect_addr_size(td, 'f', 0, 100)

    # Verify that an error is raised when no partition inside 'one_of' dicts exist as list item
    failed = False
    td = {
        'app': {},
        'a': {'placement': {'after': 'app'}, 'size': 100},
        's': {'span': ['a', {'one_of': ['x0', 'x1']}]},
    }
    try:
        resolve(td, 'app')
    except PartitionError:
        failed = True
    assert failed

    # Verify that empty placement property throws error
    td = {'spm': {'placement': {'before': ['app']}, 'size': 100, 'inside': ['mcuboot_slot0']},
          'mcuboot': {'placement': {'before': ['spm', 'app']}, 'size': 200},
          'mcuboot_slot0': {'span': ['app']},
          'invalid': {'placement': {}},
          'app': {}}
    failed = False
    try:
        s, sub_partitions = resolve(td, 'app')
    except PartitionError:
        failed = True
    assert failed

    # Verify that offset is correct when aligning partition not at address 0
    offset = get_required_offset(align={'end': 800}, start=1400, size=100, move_up=False)
    assert offset == 700

    # Verify that offset is correct when aligning partition at address 0
    offset = get_required_offset(align={'end': 800}, start=0, size=100, move_up=False)
    assert offset == 700

    # Verify that offset is correct when aligning partition at address 0
    # and end of first partition is larger than the required alignment.
    offset = get_required_offset(align={'end': 800}, start=0, size=1000, move_up=False)
    assert offset == 600

    for l in [
            lambda : get_required_offset(align={'end': ['CONFIG_VAR']}, start=0, size=1000, move_up=False),
            lambda : get_required_offset(align={'start': ['CONFIG_VAR']}, start=0, size=1000, move_up=False),
            lambda : get_required_offset(align={'start': [[2]]},start=0, size=1000, move_up=False)
            ]:
        failed = False
        try:
            l()
        except TypeError:
            failed = True
        assert failed, 'Should have received a TypeError.'
    # Verify that the first partition can be aligned, and that the inserted empty partition is placed behind it.
    td = {'first': {'placement': {'before': 'app', 'align': {'end': 800}}, 'size': 100},
          'app': {'region': 'flash_primary',}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'EMPTY_0', 100, 700)
    expect_addr_size(td, 'app', 800, 200)

    # Verify that providing a static configuration with nothing unresolved gives a valid configuration with 'app'.
    static_config = {'spm': {'address': 0, 'placement': None, 'before': ['app'], 'size': 400}}
    test_config = {'app': dict()}
    flash_region = {
        'name': 'flash_primary',
        'placement_strategy': COMPLEX,
        'size': 1000,
        'base_address': 0,
        'device': 'nordic_flash_stuff'
    }
    get_region_config(test_config, flash_region, static_config)
    assert 'app' in test_config
    assert test_config['app']['address'] == 400
    assert test_config['app']['size'] == 600
    assert 'spm' in test_config
    assert test_config['spm']['address'] == 0

    # Verify that mixing one static partition and a dynamic in a START_TO_END region configuration is allowed
    static_config = {'secondary': {'size': 200, 'address': 2000, 'region': 'extflash'}}
    td = {'b': {'size': 100, 'region': 'extflash'}}
    test_region = {'name': 'extflash',
                   'size': 1000,
                   'base_address': 2000,
                   'placement_strategy': START_TO_END,
                   'device': 'some-driver-device'}
    get_region_config(td, test_region, static_config)
    assert td['secondary']['address'] == 2000
    assert td['secondary']['size'] == 200
    assert td['b']['address'] == 2200
    assert td['extflash']['address'] == 2300
    assert td['extflash']['size'] == 700

    # Test a single partition with alignment where the address is smaller than the alignment value.
    td = {'without_alignment': {'placement': {'before': 'with_alignment'}, 'size': 100},
          'with_alignment': {'placement': {'before': 'app', 'align': {'start': 200}}, 'size': 100},
          'app': {'region': 'flash_primary',}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'EMPTY_0', 100, 100)
    expect_addr_size(td, 'with_alignment', 200, 100)

    # Test alignment after 'app'
    td = {'without_alignment': {'placement': {'after': 'app'}, 'size': 100},
          'with_alignment': {'placement': {'after': 'without_alignment', 'align': {'start': 400}}, 'size': 100},
          'app': {'region': 'flash_primary',}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'app', 0, 700)
    expect_addr_size(td, 'with_alignment', 800, 100)
    expect_addr_size(td, 'EMPTY_0', 900, 100)

    # Test two partitions with alignment where the address is smaller than the alignment value.
    td = {'without_alignment': {'placement': {'before': 'with_alignment'}, 'size': 100},
          'with_alignment': {'placement': {'before': 'with_alignment_2', 'align': {'end': 400}}, 'size': 100},
          'with_alignment_2': {'placement': {'before': 'app', 'align': {'start': 1000}}, 'size': 100},
          'app': {'region': 'flash_primary'}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 10000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'EMPTY_0', 100, 200)
    expect_addr_size(td, 'with_alignment', 300, 100)
    expect_addr_size(td, 'EMPTY_1', 400, 600)
    expect_addr_size(td, 'with_alignment_2', 1000, 100)

    # Test three partitions with alignment where the address is BIGGER than the alignment value.
    td = {'without_alignment': {'placement': {'before': 'with_alignment'}, 'size': 10000},
          'with_alignment': {'placement': {'before': 'with_alignment_2', 'align': {'end': 400}}, 'size': 100},
          'with_alignment_2': {'placement': {'before': 'app', 'align': {'start': 1000}}, 'size': 100},
          'app': {'region': 'flash_primary'}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 10000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'EMPTY_0', 10000, 300)
    expect_addr_size(td, 'with_alignment', 10300, 100)
    expect_addr_size(td, 'EMPTY_1', 10400, 600)
    expect_addr_size(td, 'with_alignment_2', 11000, 100)

    # Test 'align_next'
    # 'second' will align 'third', but 'third' already has a bigger alignment.
    # 'third' will align 'fourth' on 6000 instead of 2000.
    # 'app' will align 'fifth' on 4000.
    # 'fifth' has an 'align_next' directive that is ignored since 'fifth' is the last partition.
    td = {'first': {'placement': {'after': 'start'}, 'size': 10000},
          'second': {'placement': {'after': 'first', 'align': {'end': 2000}, 'align_next': 4000}, 'size': 1000},
          'third': {'placement': {'after':'second', 'align': {'start': 8000}, 'align_next': 6000}, 'size': 3000},
          'fourth': {'placement': {'before': 'app', 'after':'third', 'align': {'start': 2000}}, 'size': 2000},
          'app': {'region': 'flash_primary', 'placement': {'align_next': 4000}},
          'fifth': {'placement': {'after': 'app', 'align_next': 10000}, 'size': 2000}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 100000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'EMPTY_0', 10000, 1000)
    expect_addr_size(td, 'second', 11000, 1000)
    expect_addr_size(td, 'EMPTY_1', 12000, 4000)
    expect_addr_size(td, 'third', 16000, 3000)
    expect_addr_size(td, 'EMPTY_2', 19000, 5000)
    expect_addr_size(td, 'fourth', 24000, 2000)
    expect_addr_size(td, 'app', 26000, 70000)
    expect_addr_size(td, 'fifth', 96000, 2000)
    expect_addr_size(td, 'EMPTY_3', 98000, 2000)

    # Test 'align' (negative)
    # Wrong place
    td = {'first': {'placement': {'after': 'start'}, 'size': 10000},
          'second': {'placement': {'after': 'first'}, 'align': {'start': 4000}, 'size': 1000},
          'app': {'region': 'flash_primary'}}
    s, sub_partitions = resolve(td, 'app')
    try:
        set_addresses_and_align(td, sub_partitions, s, 100000, 'app')
    except PartitionError:
        pass
    else:
        assert False, "Should have raised an error."

    # Test 'align_next' (negative)
    # Wrong place
    td = {'first': {'placement': {'after': 'start'}, 'align_next': 4000, 'size': 10000},
          'second': {'placement': {'after': 'first'}, 'size': 1000},
          'app': {'region': 'flash_primary'}}
    s, sub_partitions = resolve(td, 'app')
    try:
        set_addresses_and_align(td, sub_partitions, s, 100000, 'app')
    except PartitionError:
        pass
    else:
        assert False, "Should have raised an error."

    # 'align' already exists (end)
    td = {'first': {'placement': {'after': 'start', 'align_next': 4000}, 'size': 10000},
          'second': {'placement': {'after': 'first', 'align': {'end': 2000}}, 'size': 1000},
          'app': {'region': 'flash_primary'}}
    s, sub_partitions = resolve(td, 'app')
    try:
        set_addresses_and_align(td, sub_partitions, s, 100000, 'app')
    except PartitionError:
        pass
    else:
        assert False, "Should have raised an error."

    # Test 'align_next' (negative)
    # 'align' already exists (start - not divisible)
    td = {'first': {'placement': {'after': 'start', 'align_next': 4000}, 'size': 10000},
          'second': {'placement': {'after': 'first', 'align': {'start': 3000}}, 'size': 1000},
          'app': {'region': 'flash_primary'}}
    s, sub_partitions = resolve(td, 'app')
    try:
        set_addresses_and_align(td, sub_partitions, s, 100000, 'app')
    except PartitionError:
        pass
    else:
        assert False, "Should have raised an error."


#    FLASH (0x100000):
#    +------------------------------------------+
#    | 0x0: b0 (0x8000)                         |
#    +---0x8000: s0 (0xc200)--------------------+
#    | 0x8000: s0_pad (0x200)                   |
#    +---0x8200: s0_image (0xc000)--------------+
#    | 0x8200: mcuboot (0xc000)                 |
#    | 0x14200: EMPTY_0 (0xe00)                 |
#    +---0x15000: s1 (0xc200)-------------------+
#    | 0x15000: s1_pad (0x200)                  |
#    | 0x15200: s1_image (0xc000)               |
#    | 0x21200: EMPTY_1 (0xe00)                 |
#    +---0x22000: mcuboot_primary (0x5d000)-----+
#    | 0x22000: mcuboot_pad (0x200)             |
#    +---0x22200: mcuboot_primary_app (0x5ce00)-+
#    | 0x22200: app (0x5ce00)                   |
#    | 0x7f000: mcuboot_secondary (0x5d000)     |
#    | 0xdc000: EMPTY_2 (0x1000)                |
#    | 0xdd000: mcuboot_scratch (0x1e000)       |
#    | 0xfb000: mcuboot_storage (0x4000)        |
#    | 0xff000: provision (0x1000)              |
#    +------------------------------------------+

    # Verify that alignment works with partition which shares size with app.
    td = {'b0': {'placement': {'after': 'start'}, 'size': 0x8000},
          's0': {'span': ['s0_pad', 's0_image']},
          's0_pad': {'placement': {'after': 'b0', 'align': {'start': 0x1000}}, 'share_size': 'mcuboot_pad'},
          's0_image': {'span': {'one_of': ['mcuboot', 'spm', 'app']}},
          'mcuboot': {'placement': {'before': 'mcuboot_primary', 'align_next': 0x1000}, 'size': 0xc000},
          's1': {'span': ['s1_pad', 's1_image']},
          's1_pad': {'placement': {'after': 's0'}, 'share_size': 'mcuboot_pad'},
          's1_image': {'placement': {'after': 's1_pad'}, 'share_size': 'mcuboot'},
          'mcuboot_primary': {'span': ['mcuboot_pad', 'mcuboot_primary_app']},
          'mcuboot_pad': {'placement': {'before': 'mcuboot_primary_app', 'align': {'start': 0x1000}}, 'size': 0x200},
          'mcuboot_primary_app': {'span': ['app']},
          'app': {'region': 'flash_primary'},
          'mcuboot_secondary': {'placement': {'after': 'mcuboot_primary', 'align': {'start': 0x1000}, 'align_next': 0x1000}, 'share_size': 'mcuboot_primary'},
          'mcuboot_scratch': {'placement': {'after': 'app', 'align': {'start': 0x1000}}, 'size': 0x1e000},
          'mcuboot_storage': {'placement': {'after': 'mcuboot_scratch'}, 'size': 0x4000},
          'provision': {'placement': {'before': 'end', 'align': {'start': 0x1000}}, 'size': 0x1000}}
    fix_syntactic_sugar(td)
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 0x100000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'EMPTY_0', 0x14200, 0xe00)
    expect_addr_size(td, 'EMPTY_1', 0x21200, 0xe00)
    expect_addr_size(td, 'EMPTY_2', 0xdc000, 0x1000)
    assert td['mcuboot_secondary']['size'] == td['mcuboot_primary']['size']

    # Verify that if a partition X uses 'share_size' with a non-existing partition, then partition X is given size 0,
    # and is hence not created.
    td = {'should_not_exist': {'placement': {'before': 'exists'}, 'share_size': 'does_not_exist'},
          'exists': {'placement': {'before': 'app'}, 'size': 100},
          'app': {}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    assert 'should_not_exist' not in td

    # Verify that if a partition X uses 'share_size' with a non-existing partition, but has set a default size,
    # then partition X is created with the default size.
    td = {'should_exist': {'placement': {'before': 'exists'}, 'share_size': 'does_not_exist', 'size': 200},
          'exists': {'placement': {'before': 'app'}, 'size': 100},
          'app': {}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'should_exist', 0, 200)

    td = {'spm': {'placement': {'before': ['app']}, 'size': 100},
          'mcuboot': {'placement': {'before': ['spm', 'app']}, 'size': 200},
          'app': {}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'mcuboot', 0, None)
    expect_addr_size(td, 'spm', 200, None)
    expect_addr_size(td, 'app', 300, 700)

    td = {'spm': {'placement': {'before': ['app']}, 'size': 100, 'inside': ['mcuboot_slot0']},
          'mcuboot': {'placement': {'before': ['spm', 'app']}, 'size': 200},
          'mcuboot_slot0': {'span': ['app']},
          'app': {}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
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
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
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
          'mcuboot_slot0': {'span': 'app'},
          'mcuboot_data': {'placement': {'after': ['mcuboot_slot0']}, 'size': 200},
          'mcuboot_slot1': {'share_size': ['mcuboot_slot0'], 'placement': {'after': ['mcuboot_data']}},
          'mcuboot_slot2': {'share_size': ['mcuboot_slot1'], 'placement': {'after': ['mcuboot_slot1']}},
          'app': {}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
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
    fix_syntactic_sugar(td)
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, {}, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
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
    s, _ = resolve(td, 'app')
    set_addresses_and_align(td, {}, s, 1000, 'app')
    calculate_end_address(td)
    expect_addr_size(td, 'b0', 0, None)
    expect_addr_size(td, 'mcuboot', 100, None)
    expect_addr_size(td, 'app', 300, 700)

    td = {'b0': {'placement': {'before': ['mcuboot', 'app']}, 'size': 100}, 'app': {}}
    s, _ = resolve(td, 'app')
    set_addresses_and_align(td, {}, s, 1000, 'app')
    calculate_end_address(td)
    expect_addr_size(td, 'b0', 0, None)
    expect_addr_size(td, 'app', 100, 900)

    td = {'spu': {'placement': {'before': ['app']}, 'size': 100},
          'mcuboot': {'placement': {'before': ['spu', 'app']}, 'size': 200},
          'app': {}}
    s, _ = resolve(td, 'app')
    set_addresses_and_align(td, {}, s, 1000, 'app')
    calculate_end_address(td)
    expect_addr_size(td, 'mcuboot', 0, None)
    expect_addr_size(td, 'spu', 200, None)
    expect_addr_size(td, 'app', 300, 700)

    td = {'provision': {'placement': {'before': ['end']}, 'size': 100},
          'mcuboot': {'placement': {'before': ['spu', 'app']}, 'size': 100},
          'b0': {'placement': {'before': ['mcuboot', 'app']}, 'size': 50},
          'spu': {'placement': {'before': ['app']}, 'size': 100},
          'app': {}}
    s, _ = resolve(td, 'app')
    set_addresses_and_align(td, {}, s, 1000, 'app')
    calculate_end_address(td)
    expect_addr_size(td, 'b0', 0, None)
    expect_addr_size(td, 'mcuboot', 50, None)
    expect_addr_size(td, 'spu', 150, None)
    expect_addr_size(td, 'app', 250, 650)
    expect_addr_size(td, 'provision', 900, None)

    # Test #1 for removal of empty container partitions.
    td = {'a': {'share_size': 'does_not_exist'},  # a should be removed
          'b': {'span': 'a'},  # b through d should be removed because a is removed
          'c': {'span': 'b'},
          'd': {'span': 'c'},
          'e': {'placement': {'before': ['end']}}}
    s, sub = resolve(td, 'app')
    expect_list(['e', 'app'], s)
    expect_list([], sub)

    # Test #2 for removal of empty container partitions.
    td = {'a': {'share_size': 'does_not_exist'}, # a should be removed
          'b': {'span': 'a'}, # b should not be removed, since d is placed inside it.
          'c': {'placement': {'after': ['start']}},
          'd': {'inside': ['does_not_exist', 'b'], 'placement': {'after': ['c']}}}
    s, sub = resolve(td, 'app')
    expect_list(['c', 'd', 'app'], s)
    expect_list(['b'], sub)
    expect_list(['d'], sub['b']['orig_span']) # Backup must contain edits.

    # Verify that ambiguous requirements are resolved
    td = {
        '2': {'placement': {'before': ['end']}},
        '1': {'placement': {'before': ['end']}},
        'app': {'region': 'flash_primary'}
    }
    s, _ = resolve(td, 'app')
    expect_list(['app', '1', '2'], s)

    # Verify that partitions cannot be placed after end.
    td = {
        '2': {'placement': {'before': ['end']}},
        '1': {'placement': {'after': ['end']}},
        'app': {'region': 'flash_primary'}
    }
    try:
        resolve(td, 'app')
    except PartitionError:
        failed = True

    assert failed

    # Verify that partitions cannot be placed before start.
    td = {
        '2': {'placement': {'before': ['start']}},
        '1': {'placement': {'before': ['end']}},
        'app': {'region': 'flash_primary'}
    }
    try:
        resolve(td, 'app')
    except PartitionError:
        failed = True

    assert failed

    td = {
        '6': {'placement': {'after': ['3']}},
        '4': {'placement': {'after': ['3']}},
        '5': {'placement': {'after': ['3']}},
        '2': {'placement': {'after': ['start']}},
        '3': {'placement': {'after': ['start']}},
        '1': {'placement': {'after': ['start']}},
        'app': {'region': 'flash_primary'}
    }
    s, _ = resolve(td, 'app')
    expect_list(['1', '2', '3', '4', '5', '6', 'app'], s)

    # Verify aligning dynamic partitions

    # Align end
    td = {'b0': {'address': 0, 'size': 1000},
          'app': {'address': 1000, 'size': 500},
          'share1': {'address': 1500, 'size': 500}}
    empty_partition_address, empty_partition_size = \
        get_empty_part_to_move_dyn_part(['app', 'share1'], 'share1', td, 400,
                                        move_end=True,
                                        solution=['app', 'share1'])
    assert empty_partition_address == 1600
    assert empty_partition_size == 400

    # Align start
    td = {'b0': {'address': 0, 'size': 1000},
          'app': {'address': 1000, 'size': 500},
          'share1': {'address': 1500, 'size': 500}}
    empty_partition_address, empty_partition_size = \
        get_empty_part_to_move_dyn_part(['share1', 'app'], 'share1', td, 400,
                                        move_end=False,
                                        solution=['app', 'share1'])
    assert empty_partition_address == 1200
    assert empty_partition_size == 800

    # Align start, > 2 dynamic partitions
    td = {'b0': {'address': 0, 'size': 1000},
          'app': {'address': 1000, 'size': 500},
          'share1': {'address': 1500, 'size': 500},
          'share2': {'address': 2000, 'size': 500},
          'share3': {'address': 2500, 'size': 500}}

    empty_partition_address, empty_partition_size = \
        get_empty_part_to_move_dyn_part(['app', 'share3', 'share2', 'share1'],
                                        'share3', td, 300, move_end=False,
                                        solution=['app', 'share2', 'share2',
                                                  'share3'])
    assert empty_partition_address == 2600
    assert empty_partition_size == 400

    # Align end, > 2 dynamic partitions
    td = {'b0': {'address': 0, 'size': 1000},
          'app': {'address': 1000, 'size': 500},
          'share1': {'address': 1500, 'size': 500},
          'share2': {'address': 2000, 'size': 500},
          'share3': {'address': 2500, 'size': 500}}

    empty_partition_address, empty_partition_size = \
        get_empty_part_to_move_dyn_part(['app', 'share3', 'share2', 'share1'],
                                        'share3', td, 400, move_end=True,
                                        solution=['app', 'share1', 'share2',
                                                  'share3'])
    assert empty_partition_address == 2600
    assert empty_partition_size == 400

    # Invalid alignment attempt, move size is too big. (move size is 600 and
    # size of app is 500)
    td = {'b0': {'address': 0, 'size': 1000},
          'app': {'address': 1000, 'size': 500},
          'share1': {'address': 1500, 'size': 500}}
    failed = False
    try:
        get_empty_part_to_move_dyn_part(['app', 'share1'], 'share1', td, 600,
                                        move_end=False,
                                        solution=['app', 'share1'])
    except PartitionError:
        failed = True

    assert failed

    # Invalid alignment attempt, reduction size is not word aligned.
    failed = False
    td = {'b0': {'address': 0, 'size': 1000},
          'app': {'address': 1000, 'size': 500},
          'share1': {'address': 1500, 'size': 500}}
    try:
        get_empty_part_to_move_dyn_part(['app', 'share1'], 'share1', td, 333,
                                        move_end=False,
                                        solution=['app', 'share1'])
    except PartitionError:
        failed = True
    assert failed

    td = {'first': {'region': 'region1'},
          'second': {'region': 'region1', 'share_size': 'first'},
          'third': {'region': 'region2', 'share_size': 'second'},
          'fourth': {'region': 'region3'}}
    regions = {'region1': None,
               'region2': None,
               'region3': None}
    sorted_regions = sort_regions(td, regions)
    assert list(sorted_regions.keys()) == ['region1', 'region3', 'region2']

    # dependency loop
    failed = False
    td = {'first': {'region': 'region1'},
          'second': {'region': 'region1', 'share_size': 'third'},
          'third': {'region': 'region2', 'share_size': 'second'},
          'fourth': {'region': 'region3'}}
    regions = {'region1': None,
               'region2': None,
               'region3': None}
    try:
        sorted_regions = sort_regions(td, regions)
    except PartitionError:
        failed = True
    assert failed

    td = {'first': {'region': 'region1', 'share_size': 'third'},
          'second': {'region': 'region1', 'share_size': 'fourth'},
          'third': {'region': 'region2'},
          'fourth': {'region': 'region3', 'share_size': 'fifth'},
          'fifth': {'region': 'region4'},
          'sixth': {'region': 'region4', 'share_size': 'seventh'},
          'seventh': {'region': 'region5'},
          'eighth': {'region': 'region2'}}
    regions = {'region1': None,
               'region2': None,
               'region3': None,
               'region4': None,
               'region5': None}
    sorted_regions = sort_regions(td, regions)
    assert list(sorted_regions.keys()) == ['region2', 'region5', 'region4', 'region3', 'region1']

    print('All tests passed!')


if __name__ == '__main__':
    if len(sys.argv) > 1:
        main()
    else:
        print('No input, running tests.')
        test()
