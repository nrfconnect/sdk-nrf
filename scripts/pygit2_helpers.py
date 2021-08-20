# Copyright 2018 Open Source Foundries, Limited
# Copyright 2018 Foundries.io, Limited
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# Helper code used by NCS to augment the pygit2 APIs. Used by
# maintainers to help synchronize upstream open source projects and
# keep changelogs/release notes up to date.
#
# Portions were copied from:
# https://github.com/foundriesio/zephyr_tools/.

__all__ = [
    'shortlog_is_revert', 'shortlog_reverts_what', 'shortlog_has_sauce',
    'shortlog_no_sauce',

    'commit_reverts_what', 'commit_shortlog', 'commit_affects_files',

    'zephyr_commit_area',
]

from pathlib import Path
import re

def shortlog_is_revert(shortlog):
    '''Return True if and only if the shortlog starts with 'Revert '.

    :param shortlog: Git commit message shortlog.'''
    return shortlog.startswith('Revert ')

def shortlog_reverts_what(shortlog):
    '''If the shortlog is a revert, returns shortlog of what it reverted.

    :param shortlog: Git commit message shortlog

    For example, if shortlog is:

    'Revert "whoops: this turned out to be a bad idea"'

    The return value is 'whoops: this turned out to be a bad idea';
    i.e. the double quotes are also stripped.
    '''
    revert = 'Revert '
    return shortlog[len(revert) + 1:-1]

def shortlog_has_sauce(shortlog, sauce='nrf'):
    '''Check if a Git shortlog has a 'sauce tag'.

    :param shortlog: Git commit message shortlog, which might begin
                     with a "sauce tag" that looks like '[sauce <tag>] '
    :param sauce: String (or iterable of strings) indicating a source of
                  "sauce". This is organization-specific but defaults to
                  'nrf'.

    For example, sauce="xyz" and the shortlog is:

    [xyz fromlist] area: something

    Then the return value is True. If the shortlog is any of these,
    the return value is False:

    area: something
    [abc fromlist] area: something
    [WIP] area: something
    '''
    if isinstance(sauce, str):
        sauce = '[' + sauce
    else:
        sauce = tuple('[' + s for s in sauce)

    return shortlog.startswith(sauce)

def shortlog_no_sauce(shortlog, sauce='nrf'):
    '''Return a Git shortlog without a 'sauce tag'.

    :param shortlog: Git commit message shortlog, which might begin
                     with a "sauce tag" that looks like '[sauce <tag>] '
    :param sauce: String (or iterable of strings) indicating a source of
                  "sauce". This is organization-specific but defaults to
                  'nrf'.

    For example, sauce="xyz" and the shortlog is:

    "[xyz fromlist] area: something"

    Then the return value is "area: something".

    As another example with the same sauce, if shortlog is "foo: bar",
    the return value is "foo: bar".
    '''
    if isinstance(sauce, str):
        sauce = '[' + sauce
    else:
        sauce = tuple('[' + s for s in sauce)

    if shortlog.startswith(sauce):
        return shortlog[shortlog.find(']') + 1:].strip()
    else:
        return shortlog

def commit_reverts_what(commit):
    '''Look for the string "reverts commit SOME_SHA" in the commit message,
    and return SOME_SHA. Raises ValueError if the string is not found.'''
    match = re.search(r'reverts\s+commit\s+([0-9a-f]+)',
                      ' '.join(commit.message.split()))
    if not match:
        raise ValueError(commit.message)
    return match.groups()[0]

def commit_shortlog(commit):
    '''Return the first line in a commit's log message.

    :param commit: pygit2 commit object'''
    return commit.message.splitlines()[0]

def commit_affects_files(commit, files):
    '''True if and only if the commit affects one or more files.

    :param commit: pygit2 commit object
    :param files: sequence of paths relative to commit object
                  repository root
    '''
    as_paths = set(Path(f) for f in files)
    for p in commit.parents:
        diff = commit.tree.diff_to_tree(p.tree)
        for d in diff.deltas:
            if (Path(d.old_file.path) in as_paths or
                    Path(d.new_file.path) in as_paths):
                return True
    return False

# The code this was copied from has finer-grained area heuristics than
# NCS currently tracks. This kludge preserves the work that's already
# been done while making a rougher-grained breakdown.
AREAS_NCS_SUBSET = set([
    'Bluetooth',
    'Devicetree',
    'Documentation',
    'Drivers',
    'Kernel',
    'Networking',
    'Testing',
    # Keep this list sorted alphabetically.
])

def zephyr_commit_area(commit):
    '''Make a guess about what area a zephyr commit affected.

    This is entirely based on heuristics and may return complete
    nonsense, but it's correct enough for our purposes of roughly
    segmenting commits to areas so the appropriate maintainers can
    analyze them for NCS release notes.

    The return value is one of these strings: 'Kernel', 'Bluetooth',
    'Drivers', 'Networking', 'Arches/Boards', 'Other'

    :param commit: pygit2.Commit object
    '''
    area_pfx = _commit_area_prefix(commit_shortlog(commit))

    if area_pfx is None:
        return 'Other'

    for test_regex, area in _SHORTLOG_RE_TO_AREA:
        match = test_regex.fullmatch(area_pfx)
        if match:
            break
    else:
        area = 'Other'

    if area in AREAS_NCS_SUBSET:
        return area

    if area in ('Arches', 'Boards'):
        return 'Arches/Boards'

    return 'Other'

def _commit_area_prefix(commit_shortlog):
    '''Get the prefix of a pull request title which describes its area.

    This returns the "raw" prefix as it appears in the title. To
    canonicalize this to one of a known set of areas for a zephyr PR, use
    zephyr_pr_area() instead. If no prefix is present, returns None.
    '''
    # Base case for recursion.
    if not commit_shortlog:
        return None

    # 'Revert "foo"' should map to foo's area prefix.
    if shortlog_is_revert(commit_shortlog):
        commit_shortlog = shortlog_reverts_what(commit_shortlog)
        return _commit_area_prefix(commit_shortlog)

    # If there is no ':', there is no area. Otherwise, the candidate
    # area is the substring up to the first ':'.
    if ':' not in commit_shortlog:
        return None
    area, rest = [s.strip() for s in commit_shortlog.split(':', 1)]

    # subsys: foo should map to foo's area prefix, etc.
    if area in ['subsys', 'include', 'api']:
        return _commit_area_prefix(rest)

    return area

def _invert_keys_val_list(kvs):
    for k, vs in kvs:
        for v in vs:
            yield v, k

# This list maps the 'area' a commit affects to a list of
# shortlog prefixes (the content before the first ':') in the Zephyr
# commit shortlogs that belong to it.
#
# The values are lists of case-insensitive regular expressions that
# are matched against the shortlog prefix of each commit. Matches are
# done with regex.fullmatch().
#
# Keep its definition sorted alphabetically by key.
_AREA_TO_SHORTLOG_RES = [
    ('Arches', ['arch(/.*)?', 'arc(/.*)?', 'arm(/.*)?', 'esp32(/.*)?',
                'imx(/.*)?', 'native(/.*)?', 'native_posix', 'nios2(/.*)?',
                'posix(/.*)?', 'lpc(/.*)?', 'riscv(32)?(/.*)?', 'soc(/.*)?',
                'x86(_64)?(/.*)?', 'xtensa(/.*)?']),
    ('Bluetooth', ['bluetooth', 'bt']),
    ('Boards', ['boards?(/.*)?', 'mimxrt1050_evk']),
    ('Build', ['build', 'c[+][+]', 'clang(/.*)?', 'cmake', 'kconfig',
               'gen_isr_tables?', 'gen_syscall_header', 'genrest',
               'isr_tables?', 'ld', 'linker', 'menuconfig', 'size_report',
               'toolchains?']),
    ('Continuous Integration', ['ci', 'coverage', 'sanitycheck', 'gitlint']),
    ('Cryptography', ['crypto', 'mbedtls']),
    ('Debugging', ['debug']),
    ('Devicetree', ['dt', 'dts(/.*)?', 'dt-bindings',
                    'dtlib', 'edtlib', 'devicetree',
                    'extract_dts_includes?']),
    ('Documentation', ['docs?(/.*)?', 'CONTRIBUTING.rst', 'doxygen']),
    ('Drivers', ['drivers?(/.*)?',
                 'adc', 'aio', 'can', 'clock_control', 'counter', 'crc',
                 'device([.]h)?', 'display', 'dma', 'entropy', 'eth',
                 'ethernet',
                 'flash', 'flash_map', 'gpio', 'grove', 'hid', 'i2c', 'i2s',
                 'interrupt_controller', 'ipm', 'led_strip', 'led', 'netusb',
                 'pci', 'pinmux', 'pwm', 'rtc', 'sensors?(/.*)?',
                 'serial(/.*)?', 'shared_irq', 'spi', 'timer', 'uart',
                 'uart_pipe', 'usb(/.*)?', 'watchdog',
                 # Technically in subsys/ (or parts are), but treated
                 # as drivers
                 'console', 'random', 'storage']),
    ('Firmware Update', ['dfu(/.*)?', 'mgmt']),
    ('Kernel',  ['kernel(/.*)?', 'poll', 'mempool', 'spinlock', 'syscalls',
                 'work_q', 'init.h', 'userspace', 'k_queue', 'k_poll',
                 'app_memory']),
    ('Libraries', ['libc?', 'json', 'jwt', 'ring_buffer', 'lib(/.*)',
                   'misc/dlist']),
    ('Logging', ['logging', 'logger', 'log']),
    ('Maintainers', ['CODEOWNERS([.]rst)?']),
    ('Miscellaneous', ['misc', 'release', 'shell', 'printk', 'version']),
    ('Networking', ['net(/.*)?', 'openthread', 'slip', 'ieee802154']),
    ('Power Management', ['power']),
    ('Samples', ['samples?(/.*)?']),
    ('Scripts', ['scripts?(/.*)?', 'coccinelle', 'runner',
                 'gen_app_partitions(.py)?', 'gen_syscalls.py',
                 'gen_syscall_header.py', 'kconfiglib', 'west']),
    ('Storage', ['fs(/.*)?', 'disks?', 'fcb', 'settings']),
    ('Testing', ['tests?(/.*)?', 'testing', 'unittest', 'ztest', 'tracing']),
    ]

# This 'inverts' the key/value relationship in _AREA_TO_SHORTLOG_RES to
# make a list from shortlog prefix REs to areas.
_SHORTLOG_RE_TO_AREA = [(re.compile(k, flags=re.IGNORECASE), v) for k, v in
                        _invert_keys_val_list(_AREA_TO_SHORTLOG_RES)]
