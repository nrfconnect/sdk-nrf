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
