#!/usr/bin/env python3
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
import yaml

from partition_manager import *


def test_partition_manager():
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
          'app': {'region': 'flash_primary',},
          'span_with_alignment': {'span': ['without_alignment', 'with_alignment', 'app']}}
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 1000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'EMPTY_0', 100, 100)
    expect_addr_size(td, 'with_alignment', 200, 100)
    expect_addr_size(td, 'span_with_alignment', 0, 1000)

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
          'provision': {'placement': {'before': 'end', 'align': {'start': 0x1000}}, 'size': 0x1000},
          's0_and_s1': {'span': ['s0', 's1']}}
    fix_syntactic_sugar(td)
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, sub_partitions, s, 0x100000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)
    expect_addr_size(td, 'EMPTY_0', 0x14200, 0xe00)
    expect_addr_size(td, 'EMPTY_1', 0x21200, 0xe00)
    expect_addr_size(td, 'EMPTY_2', 0xdc000, 0x1000)
    expect_addr_size(td, 's0_and_s1', 0x8000, 0xc200 + 0xc200 + 0xe00) # Check that size includes EMPTY_0
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

    # Regression test for alignment spec being deleted (NCSIDB-1032)
    td = yaml.safe_load(StringIO("""
        tfm:
            placement: {before: [app]}
            size: 0x40000
        tfm_secure:
            span: [mcuboot_pad, tfm]
        tfm_nonsecure:
            span: [app]
        tfm_storage:
            span: []
        tfm_ps:
            placement:
                before: end
                align: {start: 0x8000}
            inside: tfm_storage
            size: 0x4000
        tfm_its:
            placement:
                before: end
                align: {start: 0x8000}
            inside: tfm_storage
            size: 0x4000
        app: {}"""))

    fix_syntactic_sugar(td)
    s, sub_partitions = resolve(td, 'app')
    set_addresses_and_align(td, {}, s, 0x100000, 'app')
    set_sub_partition_address_and_size(td, sub_partitions)
    calculate_end_address(td)

    expect_addr_size(td, 'tfm', 0, None)
    expect_addr_size(td, 'tfm_secure', 0, None)
    expect_addr_size(td, 'tfm_nonsecure', 0x40000, None)
    expect_addr_size(td, 'app', 0x40000, None)
    expect_addr_size(td, 'tfm_storage', 0xF0000, None)
    expect_addr_size(td, 'tfm_its', 0xF0000, None)
    expect_addr_size(td, 'EMPTY_1', 0xF4000, None)
    expect_addr_size(td, 'tfm_ps', 0xF8000, None)
    expect_addr_size(td, 'EMPTY_0', 0xFC000, None)
