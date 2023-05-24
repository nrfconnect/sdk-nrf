# Copyright 2018 Open Source Foundries, Limited
# Copyright 2018 Foundries.io, Limited
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# Helper code used by NCS west commands.
#
# Portions were copied from:
# https://github.com/foundriesio/zephyr_tools/.

from __future__ import annotations

import collections
from dataclasses import dataclass
from pathlib import Path
import subprocess
import sys
import textwrap
from typing import Union, Iterable

try:
    import editdistance
    import pygit2  # type: ignore
except ImportError:
    sys.exit("Can't import extra dependencies needed for NCS west extensions.\n"
             "Please install packages in nrf/scripts/requirements-extra.txt "
             "with pip3.")
from west import log

from pygit2_helpers import title_is_revert, title_no_sauce, \
        commit_reverts_what, commit_title

__all__ = ['InvalidRepositoryError', 'UnknownCommitsError', 'RepoAnalyzer']

#
# Exceptions
#

class InvalidRepositoryError(RuntimeError):
    '''Git repository does not exist or cannot be analyzed.'''

class UnknownCommitsError(RuntimeError):
    '''Unknown or invalid commit ref or refs.'''

#
# Repository analysis
#

@dataclass
class Repository:
    '''Representation for a git repository on a local file system,
    and associated metadata that we want to include in an analysis.

    This is not the same thing as a pygit2.Repository.'''

    name: str
    path: str
    abspath: str
    url: str
    sha: str

def _run_git(repo: Repository, command: list[str],
             check=True) -> subprocess.CompletedProcess:
    return subprocess.run(['git'] + command,
                          capture_output=True,
                          text=True,
                          cwd=repo.abspath,
                          check=check)

class RepoAnalyzer:
    '''Utility class for analyzing a repository, especially
    upstream/downstream differences.'''

    def __init__(self,
                 downstream_repo: Repository,
                 upstream_repo: Repository,
                 downstream_domain: Union[str, tuple[str]] = '@nordicsemi.no',
                 downstream_sauce: str = 'nrf',
                 edit_dist_threshold: int = 3,
                 include_mergeups: bool = False):
        '''
        :param downstream_project: project loaded from downstream manifest
        :param upstream_project: project loaded from upstream manifest
        :param downstream_ref: downstream revision to analyze against
        :param upstream_ref: upstream revision to analyze against
        :param downstream_domain: email domains from downstream committers

        :param downstream_sauce: string or strings identifying
                                 downstream sauce tags: if your
                                 sauce tags look like [xyz <tag>], use
                                 "xyz". If they look like [xyz <tag>]
                                 or [abc <tag>], use ['xyz', 'abc']

        :param edit_dist_threshold: commit title edit distance to use when
                                    fuzzy-matching upstream and downstream
                                    patches

        :param include_mergeups: include mergeup commits as downstream
                                 outstanding patches
        '''

        self.downstream_repo: Repository = downstream_repo
        self.upstream_repo: Repository = upstream_repo
        assert Path(self.downstream_repo.path).resolve() == \
            Path(self.upstream_repo.path).resolve()
        self._pygit2_repo: pygit2.Repository = \
            _load_repo(self.downstream_repo.abspath)
        self._downstream_sauce: list[str] = _to_list(downstream_sauce)
        self._downstream_domain: list[str] = _to_list(downstream_domain)
        self._edit_dist_threshold: int = edit_dist_threshold
        self._include_mergeups: bool = include_mergeups

    #
    # Main API, which is property based.
    #
    # We use properties *upstream_new*, *downstream_outstanding*,
    # etc. so that the necessary analysis is done on demand and saved,
    # rather than wasting work doing analysis the user doesn't care
    # about.
    #

    def _upstream_new(self):
        if not hasattr(self, '__upstream_new'):
            self.__upstream_new = self._new_upstream_only_commits()
        return self.__upstream_new

    def _downstream_outstanding(self):
        if not hasattr(self, '__downstream_out'):
            self.__downstream_out = self._downstream_outstanding_commits()
        return self.__downstream_out

    def _likely_merged(self):
        if not hasattr(self, '__likely_merged'):
            self.__likely_merged = self._likely_merged_commits()
        return self.__likely_merged

    upstream_new = property(fget=_upstream_new)
    '''Commits that are new in the upstream, as a list of pygit2 objects.'''

    downstream_outstanding = property(_downstream_outstanding)
    '''A list of outstanding downstream-only (oot = "out of tree")
    patches, as pygit2 commit objects. A patch is outstanding if it is
    neither a merge commit nor a patch which was later reverted.

    Merge commits are included in the list if include_mergeupse=True
    was given to the constructor. Patches are ordered from earliest
    merged to latest merged, in a topological sorting.'''

    likely_merged = property(_likely_merged)
    '''A map representing downstream patches which look like they are
    merged upstream, and candidate upstream patches.

    The map's keys are downstream pygit2 objects. Its values are lists
    of pygit2 commit objects that are merged upstream and have similar
    titles by edit distance to each key.'''

    #
    # Internal helpers
    #

    def _new_upstream_only_commits(self) -> list[pygit2.Commit]:
        '''Commits in `upstream_ref` history since merge base with
        `downstream_ref`'''
        downstream_sha = self.downstream_repo.sha
        upstream_sha = self.upstream_repo.sha
        try:
            merge_base = self._pygit2_repo.merge_base(downstream_sha,
                                                      upstream_sha)
        except ValueError:
            log.wrn("can't get merge base; "
                    f"downstream SHA: {downstream_sha}, "
                    f"upstream SHA: {upstream_sha}")
            raise

        sort = pygit2.GIT_SORT_TOPOLOGICAL | pygit2.GIT_SORT_REVERSE
        walker = self._pygit2_repo.walk(upstream_sha, sort)
        walker.hide(merge_base)
        return list(walker)

    def _downstream_outstanding_commits(self) -> list[pygit2.Commit]:
        # Compute a list of commit objects for outstanding
        # downstream patches.

        # Convert downstream and upstream revisions to SHAs.
        downstream_sha = self.downstream_repo.sha
        upstream_sha = self.upstream_repo.sha

        # First, get a list of all downstream OOT patches. Note:
        # pygit2 doesn't seem to have any ready-made rev-list
        # equivalent, so call out to git to get the commit
        # SHAs, then wrap them with pygit2 objects.
        cp = _run_git(self.downstream_repo,
                      ['rev-list',
                       '--reverse',
                       downstream_sha,
                       f'^{upstream_sha}'])
        if not cp.stdout.strip():
            return []
        commit_shas = cp.stdout.strip().splitlines()
        all_downstream_oot: list[pygit2.Commit] = [
            self._pygit2_repo.revparse_single(c)
            for c in commit_shas]

        # Now filter out reverted patches and mergeups from the
        # complete list of OOT patches.
        downstream_out: dict[str, pygit2.Commit] = {}
        for c in all_downstream_oot:
            sha, sl = str(c.oid), commit_title(c)
            is_revert = title_is_revert(sl)  # this is just a heuristic

            if len(c.parents) > 1:
                if not self._include_mergeups:
                    # Skip all the mergeup commits.
                    log.dbg('** skipped mergeup {} ("{}")'.format(sha, sl),
                            level=log.VERBOSE_VERY)
                    continue
                else:
                    is_revert = False  # a merge is never a revert

            if is_revert:
                # If a title marks a revert, delete the original commit
                # from downstream_out, if it can be found.
                try:
                    rsha = commit_reverts_what(c)
                except ValueError:
                    # Badly formatted revert message.
                    # Treat as outstanding, but complain.
                    log.wrn(
                        'revert {} doesn\'t say "reverts commit <SHA>":\n{}'.
                        format(str(sha), textwrap.indent(c.message, '\t')))
                    rsha = None

                # Temporary workaround for any commit(s) that are known
                # to specify incorrect SHAs of the commits they revert.
                if self.downstream_repo.name == 'trusted-firmware-m':
                    if rsha == '74ebcb636cb39b498a2ac05f7587f8ee9158bba9':
                        rsha = 'af7b2f48e88bd3f260347138f87e5c4f7819b273'

                if rsha in downstream_out:
                    log.dbg('** commit {} ("{}") was reverted in {}'.
                            format(rsha, commit_title(downstream_out[rsha]),
                                   sha), level=log.VERBOSE_VERY)
                    del downstream_out[rsha]
                    continue
                elif rsha is not None:
                    # Make sure the reverted commit is in downstream history.
                    # (It might not be in all_downstream_oot if e.g.
                    # downstream reverts an upstream patch as a hotfix, and we
                    # shouldn't warn about that.)
                    is_ancestor = _run_git(
                        self.downstream_repo,
                        ['merge-base', '--is-ancestor', rsha, downstream_sha],
                        check=False).returncode == 0
                    if not is_ancestor:
                        log.wrn(('commit {} ("{}") reverts {}, '
                                 "which isn't in downstream history").
                                format(sha, sl, rsha))

            downstream_out[sha] = c
            log.dbg('** added oot patch: {} ("{}")'.format(sha, sl),
                    level=log.VERBOSE_VERY)

        return list(downstream_out.values())

    def _likely_merged_commits(self) -> dict[pygit2.Commit,
                                             list[pygit2.Commit]]:
        # Compute patches which are downstream and probably were
        # merged upstream, by looking for downstream patches with
        # small title edit distances from upstream patches.
        #
        # Using edit distance catches patches with typos in the titles
        # that reviewers asked to be fixed.
        #
        # For example, we want this upstream title:
        #
        #    Bluetoth: do foo
        #
        # to be matched with this downstream title:
        #
        #    [nrf fromlist] Bluetooth: do foo
        #
        # assuming both commits have the same author.
        #
        # The return value is a map from pygit2 commit objects for
        # downstream patches, to a list of pygit2 commit objects that
        # are upstream patches which have similar titles and the same
        # authors.

        email2title_commit = collections.defaultdict(list)
        for upstream_commit in self.upstream_new:
            email = upstream_commit.author.email
            email2title_commit[email].append(
                (commit_title(upstream_commit), upstream_commit))

        likely_merged = {}
        for downstream_commit in self.downstream_outstanding:
            downstream_email = downstream_commit.author.email

            def is_match(upstream_title):
                edit_dist = editdistance.eval(
                    title_no_sauce(commit_title(downstream_commit)),
                    upstream_title)
                return edit_dist < self._edit_dist_threshold

            matches = [
                upstream_commit for upstream_title, upstream_commit in
                email2title_commit[downstream_email] if
                is_match(upstream_title)
            ]

            if len(matches) != 0:
                likely_merged[downstream_commit] = matches

        return likely_merged

def _to_list(arg: Union[str, Iterable[str]]) -> list[str]:
    # Returns sauce as an immutable object (by copying a sequence
    # into a tuple if sauce is not a string)

    if isinstance(arg, str):
        return [arg]

    return list(arg)

def _load_repo(path: str) -> pygit2.Repository:
    try:
        return pygit2.Repository(path)
    except KeyError:
        # pygit2 raises KeyError when the current path is not a Git
        # repository.
        msg = "Can't initialize Git repository at {}"
        raise InvalidRepositoryError(msg.format(path))
