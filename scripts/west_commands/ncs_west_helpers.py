# Copyright 2018 Open Source Foundries, Limited
# Copyright 2018 Foundries.io, Limited
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# Helper code used by NCS west commands.
#
# Portions were copied from:
# https://github.com/foundriesio/zephyr_tools/.

from collections import OrderedDict
from pathlib import Path
import sys
import textwrap

try:
    import editdistance
    import pygit2
except ImportError:
    sys.exit("Can't import extra dependencies needed for NCS west extensions.\n"
             "Please install packages in nrf/scripts/requirements-extra.txt "
             "with pip3.")
from west import log

from pygit2_helpers import shortlog_is_revert, shortlog_has_sauce, \
    shortlog_no_sauce, commit_reverts_what, commit_shortlog

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

class RepoAnalyzer:
    '''Utility class for analyzing a repository, especially
    upstream/downstream differences.'''

    def __init__(self, downstream_project, upstream_project,
                 downstream_ref, upstream_ref,
                 downstream_domain='@nordicsemi.no', downstream_sauce='nrf',
                 edit_dist_threshold=3, include_mergeups=False):
        # - downstream and upstream west.manifest.Project instances
        # - revisions within them to analyze
        # - corresponding pygit2.Repository instance
        self._dp = downstream_project
        self._dr = downstream_ref
        self._up = upstream_project
        self._ur = upstream_ref
        assert self._dp.path == self._up.path
        self._repo = _load_repo(self._dp.abspath)

        # string identifying a downstream sauce tag;
        # if your sauce tags look like [xyz <tag>], use "xyz". this can
        # also be a tuple of strings to find multiple sources of sauce.
        self._downstream_sauce = _parse_sauce(downstream_sauce)
        # domain name (like "@example.com") used by downstream committers;
        # this can also be a tuple of domains.
        self._downstream_domain = downstream_domain

        # commit shortlog edit distance to use when fuzzy-matching
        # upstream and downstream patches
        self._edit_dist_threshold = edit_dist_threshold

        # whether or not to include mergeup commits as downstream outstanding
        # patches
        self._include_mergeups = include_mergeups

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
    shortlogs by edit distance to each key.'''

    #
    # Internal helpers
    #

    def _new_upstream_only_commits(self):
        '''Commits in `upstream_ref` history since merge base with
        `downstream_ref`'''
        try:
            doid = self._repo.revparse_single(self._dr + '^{commit}').oid
        except KeyError:
            raise UnknownCommitsError(self._downstream_ref)
        try:
            uoid = self._repo.revparse_single(self._ur + '^{commit}').oid
        except KeyError:
            raise UnknownCommitsError(self._upstream_ref)

        try:
            merge_base = self._repo.merge_base(doid, uoid)
        except ValueError:
            log.wrn("can't get merge base;",
                    "downstream SHA: {}, upstream SHA: {}".
                    format(doid, uoid))
            raise

        sort = pygit2.GIT_SORT_TOPOLOGICAL | pygit2.GIT_SORT_REVERSE
        walker = self._repo.walk(uoid, sort)
        walker.hide(merge_base)
        return list(walker)

    def _downstream_outstanding_commits(self):
        # Compute a list of commit objects for outstanding
        # downstream patches.

        # Convert downstream and upstream revisions to SHAs.
        dsha = self._dp.sha(self._dr)
        usha = self._up.sha(self._ur)

        # First, get a list of all downstream OOT patches. Note:
        # pygit2 doesn't seem to have any ready-made rev-list
        # equivalent, so call out to Project.git() to get the commit
        # SHAs, then wrap them with pygit2 objects.
        cp = self._dp.git('rev-list --reverse {} ^{}'.format(dsha, usha),
                          capture_stdout=True)
        if not cp.stdout.strip():
            return []
        commit_shas = cp.stdout.decode('utf-8').strip().splitlines()
        all_downstream_oot = [self._repo.revparse_single(c)
                              for c in commit_shas]

        # Now filter out reverted patches and mergeups from the
        # complete list of OOT patches.
        downstream_out = OrderedDict()
        for c in all_downstream_oot:
            sha, sl = str(c.oid), commit_shortlog(c)
            is_revert = shortlog_is_revert(sl)  # this is just a heuristic

            if len(c.parents) > 1:
                if not self._include_mergeups:
                    # Skip all the mergeup commits.
                    log.dbg('** skipped mergeup {} ("{}")'.format(sha, sl),
                            level=log.VERBOSE_VERY)
                    continue
                else:
                    is_revert = False  # a merge is never a revert

            if is_revert:
                # If a shortlog marks a revert, delete the original commit
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

                # Temporary workaround for three commits that are known
                # to specify incorrect SHAs of the commits they revert.
                if self._dp.name == 'zephyr':
                    if rsha == '54b057b603cf0e12f4128d33580523bd4cff1226':
                        rsha = 'f252fac960bf2806bf66815f292b5586c38b9f34'
                    elif rsha == 'dd18789b095584bca8e297cba79b775cea101c2b':
                        rsha = 'dabe1c9d30037b63755dd13fefd8d042043bf520'
                    elif rsha == 'd692aec3d12c73b895fc296966e031e426120e8a':
                        rsha = '3aed7234e9bcfbc09178e3917073789d5cc7ea94'

                if rsha in downstream_out:
                    log.dbg('** commit {} ("{}") was reverted in {}'.
                            format(rsha, commit_shortlog(downstream_out[rsha]),
                                   sha), level=log.VERBOSE_VERY)
                    del downstream_out[rsha]
                    continue
                elif rsha is not None:
                    # Make sure the reverted commit is in downstream history.
                    # (It might not be in all_downstream_oot if e.g.
                    # downstream reverts an upstream patch as a hotfix, and we
                    # shouldn't warn about that.)
                    is_ancestor = self._dp.git(
                        'merge-base --is-ancestor {} {}'.format(rsha, dsha),
                        capture_stdout=True).returncode == 0
                    if not is_ancestor:
                        log.wrn(('commit {} ("{}") reverts {}, '
                                 "which isn't in downstream history").
                                format(sha, sl, rsha))

            # Emit a warning if we have a non-revert patch with an
            # incorrect sauce tag. (Again, downstream might carry reverts
            # of upstream patches as hotfixes, which we shouldn't
            # warn about.)
            if (not shortlog_has_sauce(sl, self._downstream_sauce) and
                    not is_revert):
                log.wrn(f'{self._dp.name}: bad or missing sauce tag: {sha} ("{sl}")')

            downstream_out[sha] = c
            log.dbg('** added oot patch: {} ("{}")'.format(sha, sl),
                    level=log.VERBOSE_VERY)

        return list(downstream_out.values())

    def _likely_merged_commits(self):
        # Compute patches which are downstream and probably were
        # merged upstream, using the following heuristics:
        #
        # 1. downstream patches with small shortlog edit distances
        #    from upstream patches
        #
        # 2. downstream patches with shortlogs that are prefixes of
        #    upstream patches
        #
        # Heuristic #1 catches patches with typos in the shortlogs
        # that reviewers asked to be fixed, etc. E.g. upstream
        # shortlog
        #
        #    Bluetoth: do foo
        #
        # matches downstream shortlog
        #
        #    [nrf fromlist] Bluetooth: do foo
        #
        # Heuristic #2 catches situations where we had to shorten our
        # downstream shortlog to fit the "[nrf xyz]" sauce tag at the
        # beginning and still fit within CI's shortlog length
        # restrictions. E.g. upstream shortlog
        #
        #    subsys: do a thing that is very useful for everyone
        #
        # matches downstream shortlog
        #
        #    [nrf fromlist] subsys: do a thing that is very
        #
        # The return value is a map from pygit2 commit objects for
        # downstream patches, to a list of pygit2 commit objects that
        # are upstream patches which have similar shortlogs and the
        # same authors.

        likely_merged = OrderedDict()
        for dc in self.downstream_outstanding:
            sl = commit_shortlog(dc)

            def ed(upstream_commit):
                return editdistance.eval(
                    shortlog_no_sauce(sl, self._downstream_sauce),
                    commit_shortlog(upstream_commit))

            matches = [
                uc for uc in self.upstream_new if
                # Heuristic #1:
                ed(uc) < self._edit_dist_threshold or
                # Heuristic #2:
                commit_shortlog(uc).startswith(sl)
            ]

            if len(matches) != 0:
                likely_merged[dc] = matches

        return likely_merged

def _parse_sauce(sauce):
    # Returns sauce as an immutable object (by copying a sequence
    # into a tuple if sauce is not a string)

    if isinstance(sauce, str):
        return sauce
    else:
        return tuple(sauce)

def _load_repo(path):
    try:
        return pygit2.Repository(path)
    except KeyError:
        # pygit2 raises KeyError when the current path is not a Git
        # repository.
        msg = "Can't initialize Git repository at {}"
        raise InvalidRepositoryError(msg.format(path))
